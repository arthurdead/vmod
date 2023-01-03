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
	plugin *plugin::assumed_currently_running;
	plugin *plugin::scope_assume_current::old_running;

	plugin::plugin(std::filesystem::path &&path_) noexcept
		: path{std::move(path_)}
	{
	}

	plugin::load_status plugin::load() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(script && script != gsdk::INVALID_HSCRIPT) {
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
			std::string script_data;

			squirrel_preprocessor &pp{main::instance().preprocessor()};
			if(!pp.preprocess(script_data, path, incs)) {
				error("vmod: plugin '%s' failed to preprocess\n"sv, path.c_str());
				return load_status::error;
			}

			script = vm->CompileScript(script_data.c_str(), path.c_str());
		}

		if(script == gsdk::INVALID_HSCRIPT) {
			error("vmod: plugin '%s' failed to compile\n"sv, path.c_str());
			return load_status::error;
		} else {
			std::string base_scope_name{"__vmod_plugin_"sv};
			{
				std::hash<std::filesystem::path> path_hash{};

				char temp_buffer[16];

				char *begin{temp_buffer};
				char *end{begin + sizeof(temp_buffer)};

				std::to_chars_result tc_res{std::to_chars(begin, end, path_hash(path))};
				tc_res.ptr[0] = '\0';

				base_scope_name += begin;
			}

			{
				std::string scope_name{base_scope_name};
				scope_name += "_private_scope__"sv;

				private_scope_ = vm->CreateScope(scope_name.c_str(), nullptr);
				if(!private_scope_ || private_scope_ == gsdk::INVALID_HSCRIPT) {
					error("vmod: plugin '%s' failed to create private scope\n"sv, path.c_str());
					return load_status::error;
				}
			}

			{
				std::string scope_name{base_scope_name};
				scope_name += "_public_scope__"sv;

				public_scope_ = vm->CreateScope(scope_name.c_str(), nullptr);
				if(!public_scope_ || public_scope_ == gsdk::INVALID_HSCRIPT) {
					error("vmod: plugin '%s' failed to create public scope\n"sv, path.c_str());
					return load_status::error;
				}
			}

			instance_ = vm->RegisterInstance(&desc, this);
			if(!instance_ || instance_ == gsdk::INVALID_HSCRIPT) {
				error("vmod: plugin '%s' failed to register its own instance\n"sv, path.c_str());
				return load_status::error;
			}

			vm->SetInstanceUniqeId(instance_, path.c_str());

			functions_table = vm->CreateTable();
			if(!functions_table || functions_table == gsdk::INVALID_HSCRIPT) {
				error("vmod: plugin '%s' failed to create functions table\n"sv, path.c_str());
				return load_status::error;
			}

			if(!vm->SetValue(public_scope_, "functions", functions_table)) {
				error("vmod: plugin '%s' failed to set functions table value\n"sv, path.c_str());
				return load_status::error;
			}

			values_table = vm->CreateTable();
			if(!values_table || values_table == gsdk::INVALID_HSCRIPT) {
				error("vmod: plugin '%s' failed to create values table\n"sv, path.c_str());
				return load_status::error;
			}

			if(!vm->SetValue(public_scope_, "values", values_table)) {
				error("vmod: plugin '%s' failed to set values table value\n"sv, path.c_str());
				return load_status::error;
			}

			if(!vm->SetValue(public_scope_, "plugin", instance_)) {
				error("vmod: plugin '%s' failed to set instance value\n"sv, path.c_str());
				return load_status::error;
			}

			{
				scope_assume_current sac{this};
				if(vm->Run(script, private_scope_, false) == gsdk::SCRIPT_ERROR) {
					error("vmod: plugin '%s' failed to run\n"sv, path.c_str());
					return load_status::error;
				}
			}

			{
				vscript::variant tmp_var;
				if(vm->GetValue(private_scope_, "VMOD_DISABLED", &tmp_var) && tmp_var == true) {
					unwatch();
					unload();
					return load_status::disabled;
				}
			}

			running = true;

			{
				vscript::variant tmp_var;
				if(vm->GetValue(private_scope_, "VMOD_AUTO_RELOAD", &tmp_var) && tmp_var == true) {
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
			lookup_function("all_plugins_loaded"sv, all_plugins_loaded);
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
			int watch_fd{inotify_add_watch(inotify_fd, path.c_str(), IN_MODIFY)};
			if(watch_fd != -1) {
				watch_fds.emplace_back(watch_fd);
			} else {
				warning("vmod: could not watch file '%s'\n"sv, path.c_str());
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
		if(inotify_fd != -1) {
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
	}

	plugin::~plugin() noexcept
	{
		unload();
		unwatch();
	}

	plugin::callback_instance::callback_instance(callable *caller_, gsdk::HSCRIPT callback_, bool post_) noexcept
		: post{post_}, caller{caller_}
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		callback = vm->ReferenceObject(callback_);

		auto &callbacks{post ? caller->callbacks_post : caller->callbacks_pre};

		callbacks.emplace(callback, this);
	}

	plugin::callback_instance::~callback_instance() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(callback && callback != gsdk::INVALID_HSCRIPT) {
			if(caller && !caller->clearing_callbacks) {
				auto &callbacks{post ? caller->callbacks_post : caller->callbacks_pre};
				auto it{callbacks.find(callback)};
				if(it != callbacks.end()) {
					callbacks.erase(it);
				}
				if(caller->empty()) {
					caller->on_empty();
				}
			}
			vm->ReleaseFunction(callback);
		}
	}

	void plugin::callback_instance::callable_destroyed() noexcept
	{ delete this; }

	void plugin::callable::on_empty() noexcept {}

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
	}

	bool plugin::callable::call(callbacks_t &callbacks, const gsdk::ScriptVariant_t *args, std::size_t num_args) noexcept
	{
		if(callbacks.empty()) {
			return true;
		}

		gsdk::IScriptVM *vm{main::instance().vm()};

		bool call_orig{true};

		for(auto &it : callbacks) {
			gsdk::HSCRIPT pl_scope{it.second->owner_scope()};

			vscript::variant ret;
			if(vm->ExecuteFunction(it.first, args, static_cast<int>(num_args), &ret, pl_scope, true) == gsdk::SCRIPT_ERROR) {
				continue;
			}

			return_flags flags{ret.get<return_flags>()};

			if(flags & return_flags::handled) {
				call_orig = false;
			}

			if(flags & return_flags::halt) {
				break;
			}
		}

		return call_orig;
	}

	plugin::owned_instance::~owned_instance() noexcept
	{
		if(owner_ && !owner_->clearing_instances) {
			std::vector<owned_instance *> &instances{owner_->owned_instances};

			auto it{std::find(instances.begin(), instances.end(), this)};
			if(it != instances.end()) {
				instances.erase(it);
			}
		}
	}

	void plugin::owned_instance::plugin_unloaded() noexcept
	{ delete this; }

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
		owner_ = assumed_currently_running;

		if(owner_) {
			owner_->owned_instances.emplace_back(this);
		}
	}

	void plugin::unload() noexcept
	{
		plugin_unloaded();

		if(!owned_instances.empty()) {
			clearing_instances = true;
			for(owned_instance *it : owned_instances) {
				it->plugin_unloaded();
			}
			owned_instances.clear();
			clearing_instances = false;
		}

		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!function_cache.empty()) {
			for(const auto &it : function_cache) {
				if(vm->ValueExists(functions_table, it.first.c_str())) {
					vm->ClearValue(functions_table, it.first.c_str());
				}

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

		running = false;

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
	}

	void plugin::lookup_function(std::string_view func_name, function &func) noexcept
	{
		gsdk::HSCRIPT func_obj{main::instance().vm()->LookupFunction(func_name.data(), private_scope_)};
		if(!func_obj || func_obj == gsdk::INVALID_HSCRIPT) {
			return;
		}

		func.owner = this;
		func.scope = private_scope_;
		func.func = func_obj;
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(gsdk::ScriptVariant_t &ret, const gsdk::ScriptVariant_t *args, std::size_t num_args) noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return main::instance().vm()->ExecuteFunction(func, args, static_cast<int>(num_args), &ret, scope, true);
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(gsdk::ScriptVariant_t &ret) noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return main::instance().vm()->ExecuteFunction(func, nullptr, 0, &ret, scope, true);
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(const gsdk::ScriptVariant_t *args, std::size_t num_args) noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return main::instance().vm()->ExecuteFunction(func, args, static_cast<int>(num_args), nullptr, scope, true);
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal() noexcept
	{
		if(!valid()) {
			return gsdk::SCRIPT_ERROR;
		}

		scope_assume_current sac{owner};
		return main::instance().vm()->ExecuteFunction(func, nullptr, 0, nullptr, scope, true);
	}

	void plugin::function::unload() noexcept
	{
		if(func && func != gsdk::INVALID_HSCRIPT) {
			main::instance().vm()->ReleaseFunction(func);
			func = gsdk::INVALID_HSCRIPT;
		}
	}

	plugin::function::~function() noexcept
	{
		if(func && func != gsdk::INVALID_HSCRIPT) {
			main::instance().vm()->ReleaseFunction(func);
		}
	}

	void plugin::game_frame(bool simulating) noexcept
	{
		if(inotify_fd != -1) {
			inotify_event event;
			if(read(inotify_fd, &event, sizeof(inotify_event)) == sizeof(inotify_event)) {
				if(reload() != load_status::success) {
					return;
				}
			}
		}

		if(!running || script == gsdk::INVALID_HSCRIPT) {
			return;
		}

		game_frame_(simulating);
	}
}
