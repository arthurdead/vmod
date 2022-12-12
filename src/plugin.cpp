#include "plugin.hpp"
#include "vmod.hpp"
#include "filesystem.hpp"
#include <cctype>
#include <charconv>
#include <sys/inotify.h>

namespace vmod
{
	class_desc_t<plugin> plugin_desc{"__vmod_plugin_class"};

	plugin *plugin::assumed_currently_running;
	plugin *plugin::scope_assume_current::old_running;

	script_variant_t plugin::script_lookup_value(std::string_view val_name) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		script_variant_t var;
		if(!vm->GetValue(private_scope_, val_name.data(), &var)) {
			null_variant(var);
		} else {
			vm->SetValue(values_table, val_name.data(), var);
		}

		return var;
	}

	gsdk::HSCRIPT plugin::script_lookup_function(std::string_view func_name) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string func_name_str{func_name};

		auto it{function_cache.find(func_name_str)};
		if(it != function_cache.end()) {
			return it->second;
		}

		if(func_name == "map_active"sv) {
			return map_active.func;
		} else if(func_name == "map_loaded"sv) {
			return map_loaded.func;
		} else if(func_name == "map_unloaded"sv) {
			return map_unloaded.func;
		} else if(func_name == "plugin_loaded"sv) {
			return plugin_loaded.func;
		} else if(func_name == "plugin_unloaded"sv) {
			return plugin_unloaded.func;
		} else if(func_name == "all_plugins_loaded"sv) {
			return all_plugins_loaded.func;
		} else if(func_name == "string_tables_created"sv) {
			return string_tables_created.func;
		} else if(func_name == "game_frame"sv) {
			return game_frame_.func;
		}

		gsdk::IScriptVM *vm{vmod.vm()};

		gsdk::HSCRIPT func{vm->LookupFunction(func_name.data(), private_scope_)};
		if(!func || func == gsdk::INVALID_HSCRIPT) {
			return nullptr;
		}

		vm->SetValue(functions_table, func_name.data(), func);

		function_cache.emplace(std::move(func_name_str), func);

		return func;
	}

	bool plugin::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		plugin_desc.func(&plugin::script_lookup_function, "script_lookup_function"sv, "lookup_function"sv);
		plugin_desc.func(&plugin::script_lookup_value, "script_lookup_value"sv, "lookup_value"sv);
		plugin_desc.doc_class_name("plugin"sv);

		if(!vmod.vm()->RegisterClass(&plugin_desc)) {
			error("vmod: failed to register plugin script class\n"sv);
			return false;
		}

		return true;
	}

	void plugin::unbindings() noexcept
	{

	}

	plugin::plugin(std::filesystem::path &&path_) noexcept
		: path{std::move(path_)},
		inotify_fd{-1},
		instance_{gsdk::INVALID_HSCRIPT},
		script{gsdk::INVALID_HSCRIPT},
		private_scope_{gsdk::INVALID_HSCRIPT},
		public_scope_{gsdk::INVALID_HSCRIPT},
		functions_table{gsdk::INVALID_HSCRIPT},
		values_table{gsdk::INVALID_HSCRIPT}
	{
		//TODO!!!! handle nested path
		std::filesystem::path filename{path.filename()};
		filename.replace_extension();

		name = filename;

		for(char &c : name) {
			if(std::isalnum(c)) {
				continue;
			}

			c = '_';
		}

		if(std::isdigit(name[0])) {
			name.insert(name.begin(), '_');
		}
	}

	plugin &plugin::operator=(plugin &&other) noexcept
	{
		path = std::move(other.path);

		script = other.script;
		other.script = gsdk::INVALID_HSCRIPT;

		private_scope_ = other.private_scope_;
		other.private_scope_ = gsdk::INVALID_HSCRIPT;

		public_scope_ = other.public_scope_;
		other.public_scope_ = gsdk::INVALID_HSCRIPT;

		functions_table = other.functions_table;
		other.functions_table = gsdk::INVALID_HSCRIPT;

		values_table = other.values_table;
		other.values_table = gsdk::INVALID_HSCRIPT;

		instance_ = other.instance_;
		other.instance_ = gsdk::INVALID_HSCRIPT;

		inotify_fd = other.inotify_fd;
		other.inotify_fd = -1;

		watch_ds = std::move(other.watch_ds);

		map_active = std::move(other.map_active);
		map_loaded = std::move(other.map_loaded);
		map_unloaded = std::move(other.map_unloaded);
		plugin_loaded = std::move(other.plugin_loaded);
		plugin_unloaded = std::move(other.plugin_unloaded);
		all_plugins_loaded = std::move(other.all_plugins_loaded);
		string_tables_created = std::move(other.string_tables_created);
		game_frame_ = std::move(other.game_frame_);

		return *this;
	}

	bool plugin::load() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(script != gsdk::INVALID_HSCRIPT) {
			return true;
		}

		gsdk::IScriptVM *vm{vmod.vm()};

		{
			std::size_t size;
			std::unique_ptr<unsigned char[]> script_data{read_file(path, size)};

			std::string new_script_data{reinterpret_cast<const char *>(script_data.get()), size};

			squirrel_preprocessor &pp{vmod.preprocessor()};
			if(!pp.preprocess(new_script_data, path, incs)) {
				error("vmod: plugin '%s' failed to preprocess\n"sv, path.c_str());
				return false;
			}

			script = vm->CompileScript(reinterpret_cast<const char *>(new_script_data.c_str()), path.c_str());
		}

		if(script == gsdk::INVALID_HSCRIPT) {
			error("vmod: plugin '%s' failed to compile\n"sv, path.c_str());
			return false;
		} else {
			std::string base_scope_name{"__vmod_plugin_"sv};
			{
				std::hash<std::filesystem::path> path_hash{};

				char temp_buffer[16];
				std::to_chars_result tc_res{std::to_chars(temp_buffer, temp_buffer + sizeof(temp_buffer), path_hash(path))};
				tc_res.ptr[0] = '\0';

				base_scope_name += temp_buffer;
			}

			{
				std::string scope_name{base_scope_name};
				scope_name += "_private_scope__"sv;

				private_scope_ = vm->CreateScope(scope_name.c_str(), nullptr);
				if(!private_scope_ || private_scope_ == gsdk::INVALID_HSCRIPT) {
					error("vmod: plugin '%s' failed to create private scope\n"sv, path.c_str());
					return false;
				}
			}

			{
				std::string scope_name{base_scope_name};
				scope_name += "_public_scope__"sv;

				public_scope_ = vm->CreateScope(scope_name.c_str(), nullptr);
				if(!public_scope_ || public_scope_ == gsdk::INVALID_HSCRIPT) {
					error("vmod: plugin '%s' failed to create public scope\n"sv, path.c_str());
					return false;
				}
			}

			instance_ = vm->RegisterInstance(&plugin_desc, this);
			if(!instance_ || instance_ == gsdk::INVALID_HSCRIPT) {
				error("vmod: plugin '%s' failed to register its own instance\n"sv, path.c_str());
				return false;
			}

			vm->SetInstanceUniqeId(instance_, path.c_str());

			functions_table = vm->CreateTable();
			if(!functions_table || functions_table == gsdk::INVALID_HSCRIPT) {
				error("vmod: plugin '%s' failed to create functions table\n"sv, path.c_str());
				return false;
			}

			if(!vm->SetValue(public_scope_, "functions", functions_table)) {
				error("vmod: plugin '%s' failed to set functions table value\n"sv, path.c_str());
				return false;
			}

			values_table = vm->CreateTable();
			if(!values_table || values_table == gsdk::INVALID_HSCRIPT) {
				error("vmod: plugin '%s' failed to create values table\n"sv, path.c_str());
				return false;
			}

			if(!vm->SetValue(public_scope_, "values", values_table)) {
				error("vmod: plugin '%s' failed to set values table value\n"sv, path.c_str());
				return false;
			}

			if(!vm->SetValue(public_scope_, "plugin", instance_)) {
				error("vmod: plugin '%s' failed to set instance value\n"sv, path.c_str());
				return false;
			}

			if(!vm->SetValue(vmod.plugins_table(), name.c_str(), public_scope_)) {
				error("vmod: plugin '%s' failed to set value in plugins table\n"sv, path.c_str());
				return false;
			}

			{
				scope_assume_current sac{this};
				if(vm->Run(script, private_scope_, false) == gsdk::SCRIPT_ERROR) {
					error("vmod: plugin '%s' failed to run\n"sv, path.c_str());
					return false;
				}
			}

			{
				script_variant_t temp_var;
				if(vm->GetValue(private_scope_, "VMOD_AUTO_RELOAD", &temp_var) && temp_var == true) {
					watch();
				} else {
					unwatch();
				}
			}

			map_active = lookup_function("map_active"sv);
			map_loaded = lookup_function("map_loaded"sv);
			map_unloaded = lookup_function("map_unloaded"sv);
			plugin_loaded = lookup_function("plugin_loaded"sv);
			plugin_unloaded = lookup_function("plugin_unloaded"sv);
			all_plugins_loaded = lookup_function("all_plugins_loaded"sv);
			string_tables_created = lookup_function("string_tables_created"sv);
			game_frame_ = lookup_function("game_frame"sv);

			plugin_loaded();

			if(vmod.map_is_loaded()) {
				map_loaded(gsdk::STRING(sv_globals->mapname));
			}

			if(vmod.map_is_active()) {
				map_active();
			}

			if(vmod.string_tables_created()) {
				string_tables_created();
			}
		}

		return true;
	}

	void plugin::watch() noexcept
	{
		if(inotify_fd != -1) {
			return;
		}

		inotify_fd = inotify_init1(IN_NONBLOCK);
		if(inotify_fd != -1) {
			watch_ds.emplace_back(inotify_add_watch(inotify_fd, path.c_str(), IN_MODIFY));

			for(const std::filesystem::path &inc : incs) {
				watch_ds.emplace_back(inotify_add_watch(inotify_fd, inc.c_str(), IN_MODIFY));
			}
		}
	}

	void plugin::unwatch() noexcept
	{
		if(inotify_fd != -1) {
			for(int it : watch_ds) {
				inotify_rm_watch(inotify_fd, it);
			}
			watch_ds.clear();
			close(inotify_fd);
			inotify_fd = -1;
		}
	}

	plugin::~plugin() noexcept
	{
		unload();
		unwatch();
	}

	plugin::owned_instance::~owned_instance() noexcept
	{
		if(!owner) {
			return;
		}

		if(!owner->deleting_instances) {
			std::vector<owned_instance *> &instances{owner->owned_instances};

			auto it{std::find(instances.begin(), instances.end(), this)};
			if(it != instances.end()) {
				instances.erase(it);
			}
		}
	}

	void plugin::owned_instance::plugin_unloaded() noexcept
	{ delete this; }

	void plugin::owned_instance::set_plugin() noexcept
	{
		owner = assumed_currently_running;

		if(owner) {
			owner->owned_instances.emplace_back(this);
		}
	}

	void plugin::unload() noexcept
	{
		plugin_unloaded();

		if(!owned_instances.empty()) {
			deleting_instances = true;
			for(owned_instance *it : owned_instances) {
				it->plugin_unloaded();
			}
			owned_instances.clear();
			deleting_instances = false;
		}

		gsdk::IScriptVM *vm{vmod.vm()};

		if(!function_cache.empty()) {
			for(const auto &it : function_cache) {
				vm->ReleaseFunction(it.second);

				if(vm->ValueExists(functions_table, it.first.c_str())) {
					vm->ClearValue(functions_table, it.first.c_str());
				}
			}
			function_cache.clear();
		}

		map_active.unload();
		map_loaded.unload();
		map_unloaded.unload();
		plugin_loaded.unload();
		plugin_unloaded.unload();
		all_plugins_loaded.unload();

		if(functions_table && functions_table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(functions_table);
			functions_table = gsdk::INVALID_HSCRIPT;
		}

		if(values_table && values_table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(values_table);
			values_table = gsdk::INVALID_HSCRIPT;
		}

		if(vm->ValueExists(public_scope_, "functions")) {
			vm->ClearValue(public_scope_, "functions");
		}

		if(vm->ValueExists(public_scope_, "values")) {
			vm->ClearValue(public_scope_, "values");
		}

		if(vm->ValueExists(public_scope_, "plugin")) {
			vm->ClearValue(public_scope_, "plugin");
		}

		if(instance_ && instance_ != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance_);
			instance_ = gsdk::INVALID_HSCRIPT;
		}

		if(script && script != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScript(script);
			script = gsdk::INVALID_HSCRIPT;
		}

		if(!incs.empty()) {
			incs.clear();
		}

		if(private_scope_ && private_scope_ != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScope(private_scope_);
			private_scope_ = gsdk::INVALID_HSCRIPT;
		}

		if(public_scope_ && public_scope_ != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScope(public_scope_);
			public_scope_ = gsdk::INVALID_HSCRIPT;
		}

		gsdk::HSCRIPT plugins_table{vmod.plugins_table()};
		if(vm->ValueExists(plugins_table, name.c_str())) {
			vm->ClearValue(plugins_table, name.c_str());
		}
	}

	plugin::function::function(plugin &pl, std::string_view func_name) noexcept
	{
		if(pl) {
			func = vmod.vm()->LookupFunction(func_name.data(), pl.private_scope_);
			if(!func || func == gsdk::INVALID_HSCRIPT) {
				scope = gsdk::INVALID_HSCRIPT;
				func = gsdk::INVALID_HSCRIPT;
			} else {
				scope = pl.private_scope_;
			}
		} else {
			scope = gsdk::INVALID_HSCRIPT;
			func = gsdk::INVALID_HSCRIPT;
		}
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(script_variant_t &ret, const std::vector<script_variant_t> &args) noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return vmod.vm()->ExecuteFunction(func, static_cast<const gsdk::ScriptVariant_t *>(args.data()), static_cast<int>(args.size()), static_cast<gsdk::ScriptVariant_t *>(&ret), scope, true);
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(script_variant_t &ret) noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return vmod.vm()->ExecuteFunction(func, nullptr, 0, static_cast<gsdk::ScriptVariant_t *>(&ret), scope, true);
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(const std::vector<script_variant_t> &args) noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return vmod.vm()->ExecuteFunction(func, static_cast<const gsdk::ScriptVariant_t *>(args.data()), static_cast<int>(args.size()), nullptr, scope, true);
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal() noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return vmod.vm()->ExecuteFunction(func, nullptr, 0, nullptr, scope, true);
	}

	void plugin::function::unload() noexcept
	{
		if(func && func != gsdk::INVALID_HSCRIPT) {
			vmod.vm()->ReleaseFunction(func);
			func = gsdk::INVALID_HSCRIPT;
		}

		if(scope && scope != gsdk::INVALID_HSCRIPT) {
			scope = gsdk::INVALID_HSCRIPT;
		}
	}

	plugin::function::~function() noexcept
	{
		if(func && func != gsdk::INVALID_HSCRIPT) {
			vmod.vm()->ReleaseFunction(func);
		}
	}

	void plugin::game_frame(bool simulating) noexcept
	{
		if(inotify_fd != -1) {
			inotify_event event;
			if(read(inotify_fd, &event, sizeof(inotify_event)) == sizeof(inotify_event)) {
				reload();
				return;
			}
		}

		game_frame_(simulating);
	}
}
