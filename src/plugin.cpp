#include "plugin.hpp"
#include "vmod.hpp"
#include "filesystem.hpp"
#include <cctype>
#include <sys/inotify.h>

namespace vmod
{
	static class_desc_t<plugin> plugin_desc{"plugin"};

	script_variant_t plugin::script_lookup_value(std::string_view val_name) noexcept
	{
		script_variant_t var;
		if(!vmod.vm()->GetValue(scope_, val_name.data(), &var)) {
			null_variant(var);
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
		}

		gsdk::HSCRIPT func{vmod.vm()->LookupFunction(func_name.data(), scope_)};
		if(!func || func == gsdk::INVALID_HSCRIPT) {
			return nullptr;
		}

		function_cache.emplace(std::move(func_name_str), func);

		return func;
	}

	bool plugin::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		plugin_desc.func(&plugin::script_lookup_function, "script_lookup_function"sv, "lookup_function"sv);
		plugin_desc.func(&plugin::script_lookup_value, "script_lookup_value"sv, "lookup_value"sv);

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
		watch_d{-1},
		instance_{gsdk::INVALID_HSCRIPT},
		script{gsdk::INVALID_HSCRIPT},
		scope_{gsdk::INVALID_HSCRIPT}
	{
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

		scope_ = other.scope_;
		other.scope_ = gsdk::INVALID_HSCRIPT;

		instance_ = other.instance_;
		other.instance_ = gsdk::INVALID_HSCRIPT;

		inotify_fd = other.inotify_fd;
		other.inotify_fd = -1;

		watch_d = other.watch_d;
		other.watch_d = -1;

		map_active = std::move(other.map_active);
		map_loaded = std::move(other.map_loaded);
		map_unloaded = std::move(other.map_unloaded);
		plugin_loaded = std::move(other.plugin_loaded);
		plugin_unloaded = std::move(other.plugin_unloaded);
		all_plugins_loaded = std::move(other.all_plugins_loaded);

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
			std::unique_ptr<unsigned char[]> script_data{read_file(path)};

			script = vm->CompileScript(reinterpret_cast<const char *>(script_data.get()), path.c_str());
		}

		if(script == gsdk::INVALID_HSCRIPT) {
			error("vmod: plugin '%s' failed to compile\n"sv, path.c_str());
			return false;
		} else {
			std::string scope_name{"__vmod_plugin_"sv};
			scope_name += name;
			scope_name += "_scope__"sv;

			scope_ = vm->CreateScope(scope_name.c_str(), nullptr);
			if(!scope_ || scope_ == gsdk::INVALID_HSCRIPT) {
				error("vmod: plugin '%s' failed to create scope\n"sv, path.c_str());
				return false;
			}

			instance_ = vm->RegisterInstance(&plugin_desc, this);
			if(!instance_ || instance_ == gsdk::INVALID_HSCRIPT) {
				error("vmod: plugin '%s' failed to register its own instance\n"sv, path.c_str());
				return false;
			}

			vm->SetInstanceUniqeId(instance_, path.c_str());

			if(!vm->SetValue(vmod.plugins_table(), name.c_str(), instance_)) {
				error("vmod: plugin '%s' failed to set instance value in plugins table\n"sv, path.c_str());
				return false;
			}

			if(vm->Run(script, scope_, false) == gsdk::SCRIPT_ERROR) {
				error("vmod: plugin '%s' failed to run\n"sv, path.c_str());
				return false;
			}

			map_active = lookup_function("map_active"sv);
			map_loaded = lookup_function("map_loaded"sv);
			map_unloaded = lookup_function("map_unloaded"sv);
			plugin_loaded = lookup_function("plugin_loaded"sv);
			plugin_unloaded = lookup_function("plugin_unloaded"sv);
			all_plugins_loaded = lookup_function("all_plugins_loaded"sv);

			plugin_loaded();

			if(vmod.map_is_loaded()) {
				map_loaded(gsdk::STRING(sv_globals->mapname));
			}

			{
				script_variant_t temp_var;
				if(vm->GetValue(scope_, "VMOD_AUTO_RELOAD", &temp_var) && temp_var == true) {
					watch();
				} else {
					unwatch();
				}
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
			watch_d = inotify_add_watch(inotify_fd, path.c_str(), IN_MODIFY);
		}
	}

	void plugin::unwatch() noexcept
	{
		if(inotify_fd != -1) {
			if(watch_d != -1) {
				inotify_rm_watch(inotify_fd, watch_d);
				watch_d = -1;
			}

			close(inotify_fd);
			inotify_fd = -1;
		}
	}

	plugin::~plugin() noexcept
	{
		unload();
		unwatch();
	}

	void plugin::unload() noexcept
	{
		plugin_unloaded();

		gsdk::IScriptVM *vm{vmod.vm()};

		if(instance_ && instance_ != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance_);
			instance_ = gsdk::INVALID_HSCRIPT;
		}

		if(!function_cache.empty()) {
			for(const auto &it : function_cache) {
				vm->ReleaseFunction(it.second);
			}
			function_cache.clear();
		}

		map_active.unload();
		map_loaded.unload();
		map_unloaded.unload();
		plugin_loaded.unload();
		plugin_unloaded.unload();
		all_plugins_loaded.unload();

		if(script && script != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScript(script);
			script = gsdk::INVALID_HSCRIPT;
		}

		if(scope_ && scope_ != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScope(scope_);
			scope_ = gsdk::INVALID_HSCRIPT;
		}

		gsdk::HSCRIPT plugins_table{vmod.plugins_table()};
		if(vm->ValueExists(plugins_table, name.c_str())) {
			vm->ClearValue(plugins_table, name.c_str());
		}
	}

	plugin::function::function(plugin &pl, std::string_view func_name) noexcept
	{
		if(pl) {
			func = vmod.vm()->LookupFunction(func_name.data(), pl.scope_);
			if(!func || func == gsdk::INVALID_HSCRIPT) {
				scope = gsdk::INVALID_HSCRIPT;
				func = gsdk::INVALID_HSCRIPT;
			} else {
				scope = pl.scope_;
			}
		} else {
			scope = gsdk::INVALID_HSCRIPT;
			func = gsdk::INVALID_HSCRIPT;
		}
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(script_variant_t &ret, const std::vector<script_variant_t> &args) noexcept
	{ return valid() ? vmod.vm()->ExecuteFunction(func, static_cast<const gsdk::ScriptVariant_t *>(args.data()), static_cast<int>(args.size()), static_cast<gsdk::ScriptVariant_t *>(&ret), scope, true) : gsdk::SCRIPT_ERROR; }
	gsdk::ScriptStatus_t plugin::function::execute_internal(script_variant_t &ret) noexcept
	{ return valid() ? vmod.vm()->ExecuteFunction(func, nullptr, 0, static_cast<gsdk::ScriptVariant_t *>(&ret), scope, true) : gsdk::SCRIPT_ERROR; }
	gsdk::ScriptStatus_t plugin::function::execute_internal(const std::vector<script_variant_t> &args) noexcept
	{ return valid() ? vmod.vm()->ExecuteFunction(func, static_cast<const gsdk::ScriptVariant_t *>(args.data()), static_cast<int>(args.size()), nullptr, scope, true) : gsdk::SCRIPT_ERROR; }
	gsdk::ScriptStatus_t plugin::function::execute_internal() noexcept
	{ return valid() ? vmod.vm()->ExecuteFunction(func, nullptr, 0, nullptr, scope, true) : gsdk::SCRIPT_ERROR; }

	void plugin::function::unload() noexcept
	{
		if(func && func != gsdk::INVALID_HSCRIPT) {
			vmod.vm()->ReleaseFunction(func);
			func = gsdk::INVALID_HSCRIPT;
			scope = gsdk::INVALID_HSCRIPT;
		}
	}

	plugin::function::~function() noexcept
	{
		if(func && func != gsdk::INVALID_HSCRIPT) {
			vmod.vm()->ReleaseFunction(func);
		}
	}

	void plugin::game_frame() noexcept
	{
		if(inotify_fd != -1) {
			inotify_event event;
			if(read(inotify_fd, &event, sizeof(inotify_event)) == sizeof(inotify_event)) {
				reload();
			}
		}
	}
}
