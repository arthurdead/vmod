#include "../../plugin.hpp"

namespace vmod
{
	vscript::class_desc<plugin> plugin::desc{"plugin"};
	vscript::class_desc<plugin::owned_instance> plugin::owned_instance::desc{"plugin::owned_instance"};
	vscript::class_desc<plugin::callback_instance> plugin::callback_instance::desc{"plugin::callback_instance"};
	vscript::class_desc<plugin::shared_instance> plugin::shared_instance::desc{"plugin::shared_instance"};

	bool plugin::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		desc.func(&plugin::script_lookup_function, "script_lookup_function"sv, "lookup_function"sv)
		.desc("[function](name)"sv);

		desc.func(&plugin::script_lookup_value, "script_lookup_value"sv, "lookup_value"sv)
		.desc("(name)"sv);

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register plugin script class\n"sv);
			return false;
		}

		if(!owned_instance::bindings()) {
			return false;
		}

		if(!shared_instance::bindings()) {
			return false;
		}

		return true;
	}

	void plugin::unbindings() noexcept
	{
		owned_instance::unbindings();
		shared_instance::unbindings();
	}

	vscript::variant plugin::script_lookup_value(std::string_view val_name) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		vscript::variant var;
		if(vm->GetValue(*private_scope_, val_name.data(), &var)) {
			vm->SetValue(*values_table, val_name.data(), var);
		}

		return var;
	}

	vscript::func_handle_ref plugin::script_lookup_function(std::string_view func_name) noexcept
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
		} else if(func_name == "all_mods_loaded"sv) {
			return all_mods_loaded.func;
		} else if(func_name == "string_tables_created"sv) {
			return string_tables_created.func;
		} else if(func_name == "game_frame"sv) {
			return game_frame_.func;
		}

		gsdk::IScriptVM *vm{vscript::vm()};

		auto func{vm->LookupFunction(func_name.data(), *private_scope_)};
		if(!func.object || func.object == gsdk::INVALID_HSCRIPT) {
			return nullptr;
		}

		vm->SetValue(*functions_table, func_name.data(), func.object);

		return function_cache.emplace(std::move(func_name_str), std::move(func)).first->second;
	}

	bool plugin::owned_instance::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		desc.func(&owned_instance::script_delete, "script_delete"sv, "free"sv);
		desc.dtor();

		callback_instance::desc.base(desc);

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register plugin owned instance script class\n"sv);
			return false;
		}

		callback_instance::desc.func(&callback_instance::script_toggle, "script_toggle"sv, "toggle"sv);
		callback_instance::desc.func(&callback_instance::script_enable, "script_enable"sv, "enable"sv);
		callback_instance::desc.func(&callback_instance::script_disable, "script_disable"sv, "disable"sv);
		callback_instance::desc.func(&callback_instance::script_enabled, "script_enabled"sv, "enabled"sv);
		callback_instance::desc.dtor();

		if(!vm->RegisterClass(&callback_instance::desc)) {
			error("vmod: failed to register plugin callback instance script class\n"sv);
			return false;
		}

		return true;
	}

	void plugin::owned_instance::unbindings() noexcept
	{

	}

	void plugin::owned_instance::script_delete() noexcept
	{ free(); }

	bool plugin::shared_instance::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		desc.func(&shared_instance::script_delete, "script_delete"sv, "release"sv);
		desc.dtor();

		if(!vm->RegisterClass(&shared_instance::desc)) {
			error("vmod: failed to register plugin shared instance script class\n"sv);
			return false;
		}

		return true;
	}

	void plugin::shared_instance::unbindings() noexcept
	{

	}

	void plugin::shared_instance::script_delete() noexcept
	{ remove_plugin(); }
}
