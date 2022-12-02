#include "plugin.hpp"
#include "vmod.hpp"
#include "filesystem.hpp"

namespace vmod
{
	plugin::plugin(std::filesystem::path &&path_) noexcept
		: path{std::move(path_)}
	{
		load();
	}

	plugin &plugin::operator=(plugin &&other) noexcept
	{
		path = std::move(other.path);
		script = other.script;
		scope = other.scope;
		other.script = gsdk::INVALID_HSCRIPT;
		other.scope = gsdk::INVALID_HSCRIPT;
		map_active = std::move(other.map_active);
		map_loaded = std::move(other.map_loaded);
		map_unloaded = std::move(other.map_unloaded);
		plugin_loaded = std::move(other.plugin_loaded);
		plugin_unloaded = std::move(other.plugin_unloaded);
		all_plugins_loaded = std::move(other.all_plugins_loaded);
		return *this;
	}

	bool plugin::reload() noexcept
	{
		unload();
		return load();
	}

	bool plugin::load() noexcept
	{
		using namespace std::literals::string_view_literals;

		{
			std::unique_ptr<unsigned char[]> script_data{read_file(path)};

			script = vm->CompileScript(reinterpret_cast<const char *>(script_data.get()), path.c_str());
		}

		if(script == gsdk::INVALID_HSCRIPT) {
			error("vmod: plugin '%s' failed to compile\n"sv, path.c_str());
			return false;
		} else {
			scope = vm->CreateScope(path.c_str(), global_scope);
			if(!scope || scope == gsdk::INVALID_HSCRIPT) {
				error("vmod: plugin '%s' failed to create scope\n"sv, path.c_str());
				return false;
			}

			if(vm->Run(script, scope, false) == gsdk::SCRIPT_ERROR) {
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
		}

		return true;
	}

	void plugin::unload() noexcept
	{
		if(script != gsdk::INVALID_HSCRIPT) {
			plugin_unloaded();

			map_loaded.unload();
			map_unloaded.unload();
			plugin_unloaded.unload();

			vm->ReleaseScript(script);
			script = gsdk::INVALID_HSCRIPT;
		}

		if(scope != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScope(scope);
			scope = gsdk::INVALID_HSCRIPT;
		}
	}

	plugin::function::function(plugin &pl, std::string_view name) noexcept
	{
		if(pl) {
			func = vm->LookupFunction(name.data(), pl.scope);
			if(!func || func == gsdk::INVALID_HSCRIPT) {
				scope = gsdk::INVALID_HSCRIPT;
				func = gsdk::INVALID_HSCRIPT;
			} else {
				scope = pl.scope;
			}
		} else {
			scope = gsdk::INVALID_HSCRIPT;
			func = gsdk::INVALID_HSCRIPT;
		}
	}

	gsdk::ScriptStatus_t plugin::function::execute_internal(script_variant_t &ret, const std::vector<script_variant_t> &args) noexcept
	{ return valid() ? vm->ExecuteFunction(func, static_cast<const gsdk::ScriptVariant_t *>(args.data()), static_cast<int>(args.size()), static_cast<gsdk::ScriptVariant_t *>(&ret), scope, true) : gsdk::SCRIPT_ERROR; }
	gsdk::ScriptStatus_t plugin::function::execute_internal(script_variant_t &ret) noexcept
	{ return valid() ? vm->ExecuteFunction(func, nullptr, 0, static_cast<gsdk::ScriptVariant_t *>(&ret), scope, true) : gsdk::SCRIPT_ERROR; }
	gsdk::ScriptStatus_t plugin::function::execute_internal(const std::vector<script_variant_t> &args) noexcept
	{ return valid() ? vm->ExecuteFunction(func, static_cast<const gsdk::ScriptVariant_t *>(args.data()), static_cast<int>(args.size()), nullptr, scope, true) : gsdk::SCRIPT_ERROR; }
	gsdk::ScriptStatus_t plugin::function::execute_internal() noexcept
	{ return valid() ? vm->ExecuteFunction(func, nullptr, 0, nullptr, scope, true) : gsdk::SCRIPT_ERROR; }

	void plugin::function::unload() noexcept
	{
		if(func != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseFunction(func);
			func = gsdk::INVALID_HSCRIPT;
		}
	}

	plugin::function::~function() noexcept
	{
		if(func != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseFunction(func);
		}
	}
}
