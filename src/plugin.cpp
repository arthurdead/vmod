#include "plugin.hpp"
#include "main.hpp"
#include "filesystem.hpp"
#include <cctype>
#include <charconv>
#include <sys/inotify.h>
#include "bindings/docs.hpp"
#include "bindings/instance.hpp"

namespace vmod
{
	plugin *plugin::assumed_currently_running_;
	plugin *plugin::scope_assume_current::old_running;

	std::unordered_map<std::filesystem::path, plugin *> plugin::path_plugin_map;

	plugin *plugin::assumed_currently_running() noexcept
	{ return assumed_currently_running_; }

	plugin::load_status plugin::load() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(script) {
			if(running) {
				return load_status::success;
			} else {
				return load_status::error;
			}
		} else {
			if(running) {
				return load_status::error;
			}
		}

		gsdk::IScriptVM *vm{main::instance().vm()};

		{
		#ifdef __VMOD_USING_PREPROCESSOR
			std::string script_data;

			squirrel_preprocessor &pp{main::instance().preprocessor()};
			if(!pp.preprocess(script_data, path_, incs)) {
				error("vmod: plugin '%s' failed to preprocess\n"sv, path_.c_str());
				return load_status::error;
			}

			script = vm->CompileScript(script_data.c_str(), path_.c_str());
		#else
			std::unique_ptr<unsigned char[]> script_data{read_file(path_)};

			script = vm->CompileScript(reinterpret_cast<const char *>(script_data.get()), path_.c_str());
		#endif
		}

		if(!script) {
			error("vmod: plugin '%s' failed to compile\n"sv, path_.c_str());
			return load_status::error;
		} else {
			std::string base_scope_name{"__vmod_plugin_"sv};
			{
				std::hash<std::filesystem::path> path_hash{};

				char temp_buffer[25];

				char *begin{temp_buffer};
				char *end{begin + sizeof(temp_buffer)};

				std::to_chars_result tc_res{std::to_chars(begin, end, path_hash(path_))};
				tc_res.ptr[0] = '\0';

				base_scope_name += begin;
			}

			{
				std::string scope_name{base_scope_name};
				scope_name += "_private_scope__"sv;

				private_scope_ = vm->CreateScope(scope_name.c_str(), nullptr);
				if(!private_scope_) {
					error("vmod: plugin '%s' failed to create private scope\n"sv, path_.c_str());
					return load_status::error;
				}
			}

			{
				std::string scope_name{base_scope_name};
				scope_name += "_public_scope__"sv;

				public_scope_ = vm->CreateScope(scope_name.c_str(), nullptr);
				if(!public_scope_) {
					error("vmod: plugin '%s' failed to create public scope\n"sv, path_.c_str());
					return load_status::error;
				}
			}

			instance_ = vm->RegisterInstance(&desc, this);
			if(!instance_) {
				error("vmod: plugin '%s' failed to register its own instance\n"sv, path_.c_str());
				return load_status::error;
			}

			vm->SetInstanceUniqeId(*instance_, path_.c_str());

			functions_table = vm->CreateTable();
			if(!functions_table) {
				error("vmod: plugin '%s' failed to create functions table\n"sv, path_.c_str());
				return load_status::error;
			}

			if(!vm->SetValue(*public_scope_, "functions", *functions_table)) {
				error("vmod: plugin '%s' failed to set functions table value\n"sv, path_.c_str());
				return load_status::error;
			}

			values_table = vm->CreateTable();
			if(!values_table) {
				error("vmod: plugin '%s' failed to create values table\n"sv, path_.c_str());
				return load_status::error;
			}

			if(!vm->SetValue(*public_scope_, "values", *values_table)) {
				error("vmod: plugin '%s' failed to set values table value\n"sv, path_.c_str());
				return load_status::error;
			}

			if(!vm->SetValue(*public_scope_, "plugin", *instance_)) {
				error("vmod: plugin '%s' failed to set instance value\n"sv, path_.c_str());
				return load_status::error;
			}

			{
				scope_assume_current sac{this};
				if(vm->Run(*script, *private_scope_, false) == gsdk::SCRIPT_ERROR) {
					error("vmod: plugin '%s' failed to run\n"sv, path_.c_str());
					return load_status::error;
				}
			}

			{
				vscript::variant tmp_var;
				if(vm->GetValue(*private_scope_, "VMOD_DISABLED", &tmp_var) && tmp_var == true) {
					unload();
					return load_status::disabled;
				}
			}

			running = true;

			{
				vscript::variant tmp_var;
				if(vm->GetValue(*private_scope_, "VMOD_AUTO_RELOAD", &tmp_var) && tmp_var == true) {
					watch();
				} else {
					unwatch();
				}
			}

			lookup_function("map_active"sv, map_active);
			lookup_function("map_loaded"sv, map_loaded);
			lookup_function("map_unloaded"sv, map_unloaded);
			lookup_function("plugin_loaded"sv, plugin_loaded);
			lookup_function("plugin_unloaded"sv, plugin_unloaded);
			lookup_function("all_mods_loaded"sv, all_mods_loaded);
			lookup_function("string_tables_created"sv, string_tables_created);
			lookup_function("game_frame"sv, game_frame_);

			plugin_loaded();

			if(main::instance().map_is_loaded()) {
				map_loaded(gsdk::engine::STRING(sv_globals->mapname));
			}

			if(main::instance().map_is_active()) {
				map_active();
			}

			if(main::instance().string_tables_created()) {
				string_tables_created();
			}
		}

		return load_status::success;
	}

	void plugin::watch() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(inotify_fd != -1) {
			return;
		}

		inotify_fd = inotify_init1(IN_NONBLOCK);
		if(inotify_fd != -1) {
			int watch_fd{inotify_add_watch(inotify_fd, path_.c_str(), IN_MODIFY)};
			if(watch_fd != -1) {
				watch_fds.emplace_back(watch_fd);
			} else {
				warning("vmod: could not watch file '%s'\n"sv, path_.c_str());
			}

			for(const std::filesystem::path &inc : incs) {
				watch_fd = inotify_add_watch(inotify_fd, inc.c_str(), IN_MODIFY);
				if(watch_fd != -1) {
					watch_fds.emplace_back(watch_fd);
				} else {
					warning("vmod: could not watch file '%s'\n"sv, inc.c_str());
				}
			}
		}
	}

	void plugin::unwatch() noexcept
	{
		if(inotify_fd == -1) {
			return;
		}

		for(int it : watch_fds) {
			if(it == -1) {
				continue;
			}

			inotify_rm_watch(inotify_fd, it);
		}
		watch_fds.clear();

		close(inotify_fd);
		inotify_fd = -1;
	}

	plugin::plugin(std::filesystem::path &&path__) noexcept
		: path_{std::move(path__)}
	{
		path_plugin_map.emplace(path_, this);
	}
	plugin::plugin(const std::filesystem::path &path__) noexcept
		: path_{path__}
	{
		path_plugin_map.emplace(path_, this);
	}

	plugin::~plugin() noexcept
	{
		auto it{path_plugin_map.find(path_)};
		if(it == path_plugin_map.end()) {
			path_plugin_map.erase(it);
		}

		unload();
	}

	plugin::callback_instance::callback_instance(callable *caller_, vscript::func_handle_wrapper &&callback_, bool post_) noexcept
		: callback{std::move(callback_)}, post{post_}, caller{caller_}, enabled{true}
	{
		auto &callbacks{post ? caller->callbacks_post : caller->callbacks_pre};

		callbacks.emplace(callback, this);

		if(callbacks.size() == 1) {
			caller->on_wake();
		}
	}

	plugin::callback_instance::~callback_instance() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(callback) {
			if(caller && !caller->clearing_callbacks) {
				auto &callbacks{post ? caller->callbacks_post : caller->callbacks_pre};
				auto it{callbacks.find(callback)};
				if(it != callbacks.end()) {
					callbacks.erase(it);
				}
				if(caller->empty()) {
					caller->on_sleep();
				}
			}
			callback.free();
		}
	}

	void plugin::callback_instance::callable_destroyed() noexcept
	{ delete this; }

	void plugin::callable::on_sleep() noexcept {}
	void plugin::callable::on_wake() noexcept {}

	plugin::callable::~callable() noexcept
	{
		clearing_callbacks = true;

		if(!callbacks_pre.empty()) {
			for(auto &it : callbacks_pre) {
				it.second->callable_destroyed();
			}
			callbacks_pre.clear();
		}

		if(!callbacks_post.empty()) {
			for(auto &it : callbacks_post) {
				it.second->callable_destroyed();
			}
			callbacks_post.clear();
		}

		clearing_callbacks = false;

	#ifdef __VMOD_PASS_EXTRA_INFO_TO_CALLABLES
		if(argsblock) {
			std::free(argsblock);
		}
	#endif
	}

	plugin::callable::return_value plugin::callable::call(callbacks_t &callbacks, gsdk::ScriptVariant_t *args, std::size_t num_args, bool post, bool copyback) noexcept
	{
		if(callbacks.empty()) {
			return return_value::call_orig;
		}

		gsdk::IScriptVM *vm{main::instance().vm()};

	#ifdef __VMOD_PASS_EXTRA_INFO_TO_CALLABLES
		constexpr const std::size_t extra_args{1};

		const std::size_t argsmem{sizeof(gsdk::ScriptVariant_t) * num_args};
		if(argsmem > argsblock_size) {
			argsblock_size = (argsmem + (sizeof(gsdk::ScriptVariant_t) * extra_args));
			std::realloc(static_cast<void *>(argsblock), argsblock_size);
		}

		std::memcpy(static_cast<void *>(argsblock), args, argsmem);

		new (argsblock + argsmem + (sizeof(gsdk::ScriptVariant_t) * 1)) vscript::variant{post};

		num_args += extra_args;

		args = argsblock;
	#endif

		unsigned char execflags{gsdk::SCRIPT_EXEC_WAIT};
		if(copyback) {
			execflags |= gsdk::SCRIPT_EXEC_COPYBACK;
		}

		return_value retval{return_value::call_orig};

		for(auto &it : callbacks) {
			if(!it.second->enabled) {
				continue;
			}

			vscript::scope_handle_ref pl_scope{it.second->owner_scope()};

			vscript::variant ret_var;
			if(vm->ExecuteFunction(*it.first, args, static_cast<int>(num_args), &ret_var, *pl_scope, static_cast<gsdk::ScriptExecuteFlags_t>(execflags)) == gsdk::SCRIPT_ERROR) {
				continue;
			}

			return_flags retflags{ret_var.get<return_flags>()};

			if(retflags & return_flags::error) {
				continue;
			}

			if(retflags & return_flags::changed) {
				retval |= return_value::changed;
			}

			if(retflags & return_flags::handled) {
				retval &= ~return_value::call_orig;
			}

			if(retflags & return_flags::halt) {
				break;
			}
		}

		return retval;
	}

	plugin::owned_instance::~owned_instance() noexcept
	{
		if(owner_plugin && !owner_plugin->clearing_instances) {
			std::vector<owned_instance *> &instances{owner_plugin->owned_instances};

			auto it{std::find(instances.begin(), instances.end(), this)};
			if(it != instances.end()) {
				instances.erase(it);
			}
		}
	}

	void plugin::owned_instance::plugin_unloaded() noexcept
	{
		if(owner_plugin && !owner_plugin->clearing_instances) {
			std::vector<owned_instance *> &instances{owner_plugin->owned_instances};

			auto it{std::find(instances.begin(), instances.end(), this)};
			if(it != instances.end()) {
				instances.erase(it);
			}
		}

		instance_.free();

		owner_plugin = nullptr;

		free();
	}

	plugin::shared_instance::~shared_instance() noexcept
	{
		for(auto pl : owner_plugins) {
			if(!pl->clearing_instances) {
				auto it{std::find(pl->shared_instances.begin(), pl->shared_instances.end(), this)};
				if(it != pl->shared_instances.end()) {
					pl->shared_instances.erase(it);
				}
			}
		}
	}

	void plugin::shared_instance::plugin_unloaded(plugin *pl) noexcept
	{
		auto it{std::find(owner_plugins.begin(), owner_plugins.end(), pl)};
		if(it != owner_plugins.end()) {
			owner_plugins.erase(it);
		}

		if(owner_plugins.empty()) {
			delete this;
		}
	}

	void plugin::shared_instance::remove_plugin() noexcept
	{
		plugin *currpl{assumed_currently_running()};
		if(currpl) {
			if(!currpl->clearing_instances) {
				auto it{std::find(currpl->shared_instances.begin(), currpl->shared_instances.end(), this)};
				if(it != currpl->shared_instances.end()) {
					currpl->shared_instances.erase(it);
				}
			}

			plugin_unloaded(currpl);
		}
	}

	bool plugin::owned_instance::register_class(gsdk::ScriptClassDesc_t *target_desc) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(target_desc != &desc) {
			if(target_desc->m_pBaseDesc != &desc) {
				if(!target_desc->m_pBaseDesc) {
					target_desc->m_pBaseDesc = &desc;
				} else {
				#if 0
					bool found{false};

					gsdk::ScriptClassDesc_t *base_desc{target_desc->m_pBaseDesc};
					while(base_desc) {
						if(base_desc == &desc) {
							found = true;
							break;
						}

						base_desc = base_desc->m_pBaseDesc;
					}

					if(!found) {
						error("vmod: owned instance class '%s' doenst have owned_instance as base\n", target_desc->m_pszClassname);
						return false;
					}
				#endif
				}
			}

			if(!target_desc->m_pfnDestruct) {
				target_desc->m_pfnDestruct = 
					[](void *ptr) noexcept -> void {
						delete static_cast<owned_instance *>(ptr);
					}
				;
			}

			return vm->RegisterClass(target_desc);
		} else {
			error("vmod: tried to register owned instance desc using its helper\n");
			return false;
		}
	}

	bool plugin::owned_instance::register_instance(gsdk::ScriptClassDesc_t *target_desc, void *pthis) noexcept
	{
		if(!bindings::instance_base::register_instance(target_desc, pthis)) {
			return false;
		}

		set_plugin();

		return true;
	}

	void plugin::owned_instance::set_plugin() noexcept
	{
		owner_plugin = assumed_currently_running();

		if(owner_plugin) {
			owner_plugin->owned_instances.emplace_back(this);
		}
	}

	bool plugin::shared_instance::register_instance(gsdk::ScriptClassDesc_t *target_desc, void *pthis) noexcept
	{
		if(!bindings::instance_base::register_instance(target_desc, pthis)) {
			return false;
		}

		add_plugin();

		return true;
	}

	void plugin::shared_instance::add_plugin() noexcept
	{
		plugin *currpl{assumed_currently_running()};
		if(currpl) {
			auto it{std::find(owner_plugins.begin(), owner_plugins.end(), currpl)};
			if(it == owner_plugins.end()) {
				owner_plugins.emplace_back(currpl);
			}
		}
	}

	plugin::load_status plugin::reload() noexcept
	{
		unload();
		return load();
	}

	void plugin::unload() noexcept
	{
		unwatch();

		plugin_unloaded();

		if(!owned_instances.empty()) {
			clearing_instances = true;
			for(owned_instance *it : owned_instances) {
				it->plugin_unloaded();
			}
			owned_instances.clear();
			clearing_instances = false;
		}

		if(!shared_instances.empty()) {
			clearing_instances = true;
			for(shared_instance *it : shared_instances) {
				it->plugin_unloaded(this);
			}
			shared_instances.clear();
			clearing_instances = false;
		}

		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!function_cache.empty()) {
			for(auto &it : function_cache) {
				if(vm->ValueExists(*functions_table, it.first.c_str())) {
					vm->ClearValue(*functions_table, it.first.c_str());
				}

				it.second.free();
			}
			function_cache.clear();
		}

		map_active.free();
		map_loaded.free();
		map_unloaded.free();
		plugin_loaded.free();
		plugin_unloaded.free();
		all_mods_loaded.free();

		functions_table.free();

		values_table.free();

		if(public_scope_) {
			if(vm->ValueExists(*public_scope_, "functions")) {
				vm->ClearValue(*public_scope_, "functions");
			}

			if(vm->ValueExists(*public_scope_, "values")) {
				vm->ClearValue(*public_scope_, "values");
			}

			if(vm->ValueExists(*public_scope_, "plugin")) {
				vm->ClearValue(*public_scope_, "plugin");
			}
		}

		instance_.free();

		script.free();

		running = false;

		if(!incs.empty()) {
			incs.clear();
		}

		private_scope_.free();

		public_scope_.free();
	}

	bool plugin::lookup_function(std::string_view func_name, function &func) noexcept
	{
		auto func_obj{main::instance().vm()->LookupFunction(func_name.data(), *private_scope_)};
		if(!func_obj.object || func_obj.object == gsdk::INVALID_HSCRIPT) {
			func.owner = nullptr;
			func.scope = nullptr;
			func.func.free();
			return false;
		}

		func.owner = this;
		func.scope = private_scope_;
		func.func = std::move(func_obj);
		return true;
	}

	bool plugin::read_function(vscript::func_handle_ref func_hndl, function &func_obj) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!func_hndl) {
			func_obj.owner = nullptr;
			func_obj.scope = nullptr;
			func_obj.func.free();
			return false;
		}

		switch(vm->GetLanguage()) {
		case gsdk::ScriptLanguage_t::SL_SQUIRREL: {
			#ifdef __clang__
			#pragma clang diagnostic push
			#pragma clang diagnostic ignored "-Wcast-align"
			#else
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wcast-align"
			#endif
			HSQOBJECT &sq_obj{*reinterpret_cast<HSQOBJECT *>(*func_hndl)};
			#ifdef __clang__
			#pragma clang diagnostic pop
			#else
			#pragma GCC diagnostic pop
			#endif

			if(!sq_isclosure(sq_obj)) {
				func_obj.owner = nullptr;
				func_obj.scope = nullptr;
				func_obj.func.free();
				return false;
			}

			SQClosure *closure{sq_obj._unVal.pClosure};
			SQFunctionProto *proto{closure->_function};

			if(sq_isstring(proto->_sourcename)) {
				std::filesystem::path srcfile{proto->_sourcename._unVal.pString->_val};

				auto it{path_plugin_map.find(srcfile)};
				if(it != path_plugin_map.end()) {
					func_obj.owner = it->second;
					func_obj.scope = it->second->private_scope_;
				} else {
					func_obj.owner = nullptr;
					func_obj.scope = nullptr;
				}
			} else {
				func_obj.owner = nullptr;
				func_obj.scope = nullptr;
			}

			func_obj.func = vm->ReferenceFunction(*func_hndl);

			return static_cast<bool>(func_obj.func);
		}
		default: return false;
		}
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(gsdk::ScriptVariant_t &ret, const gsdk::ScriptVariant_t *args, std::size_t num_args) noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return main::instance().vm()->ExecuteFunction(*func, args, static_cast<int>(num_args), &ret, scope ? *scope : nullptr, true);
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(gsdk::ScriptVariant_t &ret) noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return main::instance().vm()->ExecuteFunction(*func, nullptr, 0, &ret, scope ? *scope : nullptr, true);
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(const gsdk::ScriptVariant_t *args, std::size_t num_args) noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return main::instance().vm()->ExecuteFunction(*func, args, static_cast<int>(num_args), nullptr, scope ? *scope : nullptr, true);
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal() noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return main::instance().vm()->ExecuteFunction(*func, nullptr, 0, nullptr, scope ? *scope : nullptr, true);
	}

	void plugin::function::free() noexcept
	{
		func.free();
	}

	plugin::function::~function() noexcept
	{
		func.free();
	}

	void plugin::game_frame(bool simulating) noexcept
	{
		if(inotify_fd != -1) {
			unsigned char event_bytes[sizeof(inotify_event)+NAME_MAX+1]{};
			ssize_t evbytes{read(inotify_fd, event_bytes, sizeof(event_bytes))};
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wcast-align"
			auto event{reinterpret_cast<inotify_event *>(event_bytes)};
			#pragma GCC diagnostic pop
			if((evbytes > 0) && (static_cast<size_t>(evbytes) >= sizeof(inotify_event)) && (event->mask & IN_MODIFY)) {
				if(reload() != load_status::success) {
					return;
				}
			}
		}

		if(!running || !script) {
			return;
		}

		game_frame_(simulating);
	}
}
