#include "vmod.hpp"
#include "symbol_cache.hpp"
#include "gsdk/engine/vsp.hpp"
#include <cstring>

#include "plugin.hpp"
#include "filesystem.hpp"
#include "gsdk/server/gamerules.hpp"

#include <filesystem>
#include <string_view>
#include <climits>

#include <iostream>

#include "convar.hpp"
#include "yaml.hpp"
#include <utility>

namespace vmod
{
#if __has_include("vmod_base.nut.h")
#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif
	#include "vmod_base.nut.h"
#ifdef __clang__
	#pragma clang diagnostic pop
#endif
	#define __VMOD_BASE_SCRIPT_HEADER_INCLUDED
#endif
}

namespace vmod
{
	static void(gsdk::IScriptVM::*CreateArray)(gsdk::ScriptVariant_t &);
}

namespace gsdk
{
	HSCRIPT IScriptVM::CreateArray() noexcept
	{
		ScriptVariant_t var;
		(this->*vmod::CreateArray)(var);
		return var.m_hScript;
	}
}

namespace vmod
{
	class vmod vmod;

	gsdk::IScriptVM *vm;
	gsdk::CVarDLLIdentifier_t cvar_dll_id{gsdk::INVALID_CVAR_DLL_IDENTIFIER};
	gsdk::HSCRIPT global_scope{gsdk::INVALID_HSCRIPT};
	std::filesystem::path root_dir;

	static ConCommand vmod_reload_plugins{"vmod_reload_plugins"};
	static ConCommand vmod_unload_plugins{"vmod_unload_plugins"};
	static ConCommand vmod_unload_plugin{"vmod_unload_plugin"};
	static ConCommand vmod_load_plugin{"vmod_load_plugin"};
	static ConCommand vmod_list_plugins{"vmod_list_plugins"};
	static ConCommand vmod_refresh_plugins{"vmod_refresh_plugins"};

	static void vscript_output(const char *txt)
	{
		using namespace std::literals::string_view_literals;

		info("%s"sv, txt);
	}

	static bool vscript_error_output(gsdk::ScriptErrorLevel_t lvl, const char *txt)
	{
		using namespace std::literals::string_view_literals;

		switch(lvl) {
			case gsdk::SCRIPT_LEVEL_WARNING: {
				warning("%s"sv, txt);
			} break;
			case gsdk::SCRIPT_LEVEL_ERROR: {
				error("%s"sv, txt);
			} break;
		}

		return false;
	}

	static class_desc_t<class vmod> vmod_desc{"vmod"};

	gsdk::HSCRIPT vmod::script_find_plugin(std::string_view name) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::filesystem::path path{name};
		if(!path.is_absolute()) {
			path = (plugins_dir/path);
		}
		path.replace_extension(scripts_extension);

		for(auto it{plugins.begin()}; it != plugins.end(); ++it) {
			const std::filesystem::path &pl_path{static_cast<std::filesystem::path>(*(*it))};

			if(pl_path == path) {
				return (*it)->instance();
			}
		}

		return nullptr;
	}

	bool vmod::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		vmod_desc.func(&vmod::script_find_plugin, "find_plugin"sv);

		instance = vm->RegisterInstance(&vmod_desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to register vmod script instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(instance, "vmod");

		if(!vm->SetValue(global_scope, "vmod", script_variant_t{instance})) {
			error("vmod: failed to set vmod script instance value\n"sv);
			return false;
		}

		if(!vm->SetValue(global_scope, "VMOD_GAME_DIR", game_dir.c_str())) {
			error("vmod: failed to set game dir value\n"sv);
			return false;
		}

		if(!plugin::bindings()) {
			return false;
		}

		if(!yaml::bindings()) {
			return false;
		}

		return true;
	}

	void vmod::unbindings() noexcept
	{
		plugin::unbindings();

		yaml::unbindings();

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance);
		}

		if(vm->ValueExists(global_scope, "vmod")) {
			vm->ClearValue(global_scope, "vmod");
		}
	}

	static const unsigned char *g_Script_init;

	static const unsigned char *g_Script_vscript_server;
	static gsdk::IScriptVM **g_pScriptVM;
	static bool(*VScriptServerInit)();
	static void(*VScriptServerTerm)();
	static bool(*VScriptRunScript)(const char *, gsdk::HSCRIPT, bool);
	static void(gsdk::CTFGameRules::*RegisterScriptFunctions)();

	bool vmod::load() noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		gsdk::ScriptLanguage_t script_language{gsdk::SL_SQUIRREL};

		switch(script_language) {
			case gsdk::SL_NONE: break;
			case gsdk::SL_GAMEMONKEY: scripts_extension = ".gm"sv; break;
			case gsdk::SL_SQUIRREL: scripts_extension = ".nut"sv; break;
			case gsdk::SL_LUA: scripts_extension = ".lua"sv; break;
			case gsdk::SL_PYTHON: scripts_extension = ".py"sv; break;
		}

		if(!symbol_cache::initialize()) {
			std::cout << "\033[0;31m"sv << "vmod: failed to initialize symbol cache\n"sv << "\033[0m"sv;
			return false;
		}

		std::string_view launcher_lib_name{"bin/dedicated_srv.so"sv};
		if(!launcher_lib.load(launcher_lib_name)) {
			std::cout << "\033[0;31m"sv << "vmod: failed to open launcher library: '"sv << launcher_lib.error_string() << "'\n"sv << "\033[0m"sv;
			return false;
		}

		std::string_view engine_lib_name{"bin/engine.so"sv};
		if(dedicated) {
			engine_lib_name = "bin/engine_srv.so"sv;
		}
		if(!engine_lib.load(engine_lib_name)) {
			std::cout << "\033[0;31m"sv << "vmod: failed to open engine library: '"sv << engine_lib.error_string() << "'\n"sv << "\033[0m"sv;
			return false;
		}

		{
			char gamedir[PATH_MAX];
			sv_engine->GetGameDir(gamedir, sizeof(gamedir));

			game_dir = gamedir;
		}

		root_dir = game_dir;
		root_dir /= "addons/vmod"sv;

		plugins_dir = root_dir;
		plugins_dir /= "plugins"sv;

		base_script_path = root_dir;
		base_script_path /= "base/vmod_base"sv;
		base_script_path.replace_extension(scripts_extension);

		std::filesystem::path server_lib_name{game_dir};
		if(sv_engine->IsDedicatedServer()) {
			server_lib_name /= "bin/server_srv.so";
		} else {
			server_lib_name /= "bin/server.so";
		}
		if(!server_lib.load(server_lib_name)) {
			error("vmod: failed to open server library: '%s'\n"sv, server_lib.error_string().c_str());
			return false;
		}

		const auto &sv_symbols{server_lib.symbols()};
		const auto &sv_global_qual{sv_symbols.global()};

		auto g_Script_vscript_server_it{sv_global_qual.find("g_Script_vscript_server"s)};
		if(g_Script_vscript_server_it == sv_global_qual.end()) {
			error("vmod: missing 'g_Script_vscript_server' symbol\n"sv);
			return false;
		}

		auto g_pScriptVM_it{sv_global_qual.find("g_pScriptVM"s)};
		if(g_pScriptVM_it == sv_global_qual.end()) {
			error("vmod: missing 'g_pScriptVM' symbol\n"sv);
			return false;
		}

		auto VScriptServerInit_it{sv_global_qual.find("VScriptServerInit()"s)};
		if(VScriptServerInit_it == sv_global_qual.end()) {
			error("vmod: missing 'VScriptServerInit' symbol\n"sv);
			return false;
		}

		auto VScriptServerTerm_it{sv_global_qual.find("VScriptServerTerm()"s)};
		if(VScriptServerTerm_it == sv_global_qual.end()) {
			error("vmod: missing 'VScriptServerTerm' symbol\n"sv);
			return false;
		}

		auto VScriptRunScript_it{sv_global_qual.find("VScriptRunScript(char const*, HSCRIPT__*, bool)"s)};
		if(VScriptRunScript_it == sv_global_qual.end()) {
			error("vmod: missing 'VScriptRunScript' symbol\n"sv);
			return false;
		}

		auto CTFGameRules_it{sv_symbols.find("CTFGameRules"s)};
		if(CTFGameRules_it == sv_symbols.end()) {
			error("vmod: missing 'CTFGameRules' symbols\n"sv);
			return false;
		}

		auto RegisterScriptFunctions_it{CTFGameRules_it->second.find("RegisterScriptFunctions()"s)};
		if(RegisterScriptFunctions_it == CTFGameRules_it->second.end()) {
			error("vmod: missing 'CTFGameRules::RegisterScriptFunctions()' symbol\n"sv);
			return false;
		}

		std::string_view vstdlib_lib_name{"bin/libvstdlib.so"sv};
		if(sv_engine->IsDedicatedServer()) {
			vstdlib_lib_name = "bin/libvstdlib_srv.so"sv;
		}
		if(!vstdlib_lib.load(vstdlib_lib_name)) {
			error("vmod: failed to open vstdlib library: %s\n"sv, vstdlib_lib.error_string().c_str());
			return false;
		}

		std::string_view vscript_lib_name{"bin/vscript.so"sv};
		if(sv_engine->IsDedicatedServer()) {
			vscript_lib_name = "bin/vscript_srv.so"sv;
		}
		if(!vscript_lib.load(vscript_lib_name)) {
			error("vmod: failed to open vscript library: '%s'\n"sv, vscript_lib.error_string().c_str());
			return false;
		}

		const auto &vscript_symbols{vscript_lib.symbols()};
		const auto &vscript_global_qual{vscript_symbols.global()};

		auto g_Script_init_it{vscript_global_qual.find("g_Script_init"s)};
		if(g_Script_init_it == vscript_global_qual.end()) {
			error("vmod: missing 'g_Script_init' symbol\n"sv);
			return false;
		}

		auto CSquirrelVM_it{vscript_symbols.find("CSquirrelVM"s)};
		if(CSquirrelVM_it == vscript_symbols.end()) {
			error("vmod: missing 'CSquirrelVM' symbols\n"sv);
			return false;
		}

		auto CreateArray_it{CSquirrelVM_it->second.find("CreateArray(CVariantBase<CVariantDefaultAllocator>&)"s)};
		if(CreateArray_it == CSquirrelVM_it->second.end()) {
			error("vmod: missing 'CSquirrelVM::CreateArray(CVariantBase<CVariantDefaultAllocator>&)' symbol\n"sv);
			return false;
		}

		g_Script_init = g_Script_init_it->second.addr<const unsigned char *>();

		RegisterScriptFunctions = RegisterScriptFunctions_it->second.mfp<void, gsdk::CTFGameRules>();
		VScriptServerInit = VScriptServerInit_it->second.func<bool>();
		VScriptServerTerm = VScriptServerTerm_it->second.func<void>();
		VScriptRunScript = VScriptRunScript_it->second.func<bool, const char *, gsdk::HSCRIPT, bool>();
		g_Script_vscript_server = g_Script_vscript_server_it->second.addr<const unsigned char *>();
		g_pScriptVM = g_pScriptVM_it->second.addr<gsdk::IScriptVM **>();

		CreateArray = CreateArray_it->second.mfp<void, gsdk::IScriptVM, gsdk::ScriptVariant_t &>();

		write_file(root_dir/"internal_scripts"sv/"init.nut"sv, g_Script_init, std::strlen(reinterpret_cast<const char *>(g_Script_init)+1));
		write_file(root_dir/"internal_scripts"sv/"vscript_server.nut"sv, g_Script_vscript_server, std::strlen(reinterpret_cast<const char *>(g_Script_vscript_server)+1));

		vm = vsmgr->CreateVM(script_language);
		if(!vm) {
			error("vmod: failed to create VM\n"sv);
			return false;
		}

		if(!detours()) {
			return false;
		}

		vm->SetOutputCallback(vscript_output);
		vm->SetErrorCallback(vscript_error_output);

		vmod_reload_plugins = [this](const gsdk::CCommand &) noexcept -> void {
			for(const auto &pl : plugins) {
				pl->reload();
			}

			if(plugins_loaded) {
				for(const auto &pl : plugins) {
					if(!*pl) {
						continue;
					}

					pl->all_plugins_loaded();
				}
			}
		};

		vmod_unload_plugins = [this](const gsdk::CCommand &) noexcept -> void {
			for(const auto &pl : plugins) {
				pl->unload();
			}

			plugins.clear();

			plugins_loaded = false;
		};

		vmod_unload_plugin = [this](const gsdk::CCommand &args) noexcept -> void {
			if(args.m_nArgc != 2) {
				error("vmod: usage: vmod_unload_plugin <path>\n");
				return;
			}

			std::filesystem::path path{args.m_ppArgv[1]};
			if(!path.is_absolute()) {
				path = (plugins_dir/path);
			}
			path.replace_extension(scripts_extension);

			for(auto it{plugins.begin()}; it != plugins.end(); ++it) {
				const std::filesystem::path &pl_path{static_cast<std::filesystem::path>(*(*it))};

				if(pl_path == path) {
					plugins.erase(it);
					error("vmod: unloaded plugin '%s'\n", path.c_str());
					return;
				}
			}

			error("vmod: plugin '%s' not found\n", path.c_str());
		};

		vmod_load_plugin = [this](const gsdk::CCommand &args) noexcept -> void {
			if(args.m_nArgc != 2) {
				error("vmod: usage: vmod_load_plugin <path>\n");
				return;
			}

			std::filesystem::path path{args.m_ppArgv[1]};
			if(!path.is_absolute()) {
				path = (plugins_dir/path);
			}
			path.replace_extension(scripts_extension);

			for(const auto &pl : plugins) {
				const std::filesystem::path &pl_path{static_cast<std::filesystem::path>(*pl)};

				if(pl_path == path) {
					if(pl->reload()) {
						success("vmod: plugin '%s' reloaded\n", path.c_str());
						if(plugins_loaded) {
							pl->all_plugins_loaded();
						}
					}
					return;
				}
			}

			plugin &pl{*plugins.emplace_back(new plugin{std::move(path)})};
			if(pl.load()) {
				success("vmod: plugin '%s' loaded\n", static_cast<std::filesystem::path>(pl).c_str());
				if(plugins_loaded) {
					pl.all_plugins_loaded();
				}
			}
		};

		vmod_list_plugins = [this](const gsdk::CCommand &args) noexcept -> void {
			if(args.m_nArgc != 1) {
				error("vmod: usage: vmod_list_plugins\n");
				return;
			}

			if(plugins.empty()) {
				info("vmod: no plugins loaded\n");
				return;
			}

			for(const auto &pl : plugins) {
				if(*pl) {
					success("'%s'\n", static_cast<std::filesystem::path>(*pl).c_str());
				} else {
					error("'%s'\n", static_cast<std::filesystem::path>(*pl).c_str());
				}
			}
		};

		vmod_refresh_plugins = [this](const gsdk::CCommand &) noexcept -> void {
			plugins.clear();

			for(const auto &file : std::filesystem::directory_iterator{plugins_dir}) {
				if(!file.is_regular_file()) {
					continue;
				}

				std::filesystem::path path{file.path()};
				if(path.extension() != scripts_extension) {
					continue;
				}

				plugins.emplace_back(new plugin{std::move(path)})->load();
			}

			for(const auto &pl : plugins) {
				if(!*pl) {
					continue;
				}

				pl->all_plugins_loaded();
			}

			plugins_loaded = true;
		};

		cvar_dll_id = cvar->AllocateDLLIdentifier();

		vmod_reload_plugins.initialize();
		vmod_unload_plugins.initialize();
		vmod_unload_plugin.initialize();
		vmod_load_plugin.initialize();
		vmod_list_plugins.initialize();
		vmod_refresh_plugins.initialize();

		return true;
	}

	static bool vscript_server_init_called;

	static bool in_vscript_server_init;
	static bool in_vscript_server_term;
	static gsdk::IScriptVM *(gsdk::IScriptManager::*CreateVM_original)(gsdk::ScriptLanguage_t);
	static gsdk::IScriptVM *CreateVM_detour_callback(gsdk::IScriptManager *pthis, gsdk::ScriptLanguage_t lang) noexcept
	{
		if(in_vscript_server_init) {
			if(lang == vm->GetLanguage()) {
				return vm;
			} else {
				return nullptr;
			}
		}

		return (pthis->*CreateVM_original)(lang);
	}

	static void(gsdk::IScriptManager::*DestroyVM_original)(gsdk::IScriptVM *);
	static void DestroyVM_detour_callback(gsdk::IScriptManager *pthis, gsdk::IScriptVM *vm_) noexcept
	{
		if(in_vscript_server_term) {
			if(vm_ == vm) {
				return;
			}
		}

		(pthis->*DestroyVM_original)(vm_);
	}

	static gsdk::ScriptStatus_t(gsdk::IScriptVM::*Run_original)(const char *, bool);
	static gsdk::ScriptStatus_t Run_detour_callback(gsdk::IScriptVM *pthis, const char *script, bool wait) noexcept
	{
		if(in_vscript_server_init) {
			if(script == reinterpret_cast<const char *>(g_Script_vscript_server)) {
				return gsdk::SCRIPT_DONE;
			}
		}

		return (pthis->*Run_original)(script, wait);
	}

	static detour VScriptRunScript_detour;
	static bool VScriptRunScript_detour_callback(const char *script, gsdk::HSCRIPT scope, bool warn) noexcept
	{
		if(!vscript_server_init_called) {
			if(strcmp(script, "mapspawn") == 0) {
				return true;
			}
		}

		return VScriptRunScript_detour.call<bool>(script, scope, warn);
	}

	static detour VScriptServerInit_detour;
	static bool VScriptServerInit_detour_callback() noexcept
	{
		in_vscript_server_init = true;
		bool ret{vscript_server_init_called ? true : VScriptServerInit_detour.call<bool>()};
		*g_pScriptVM = vm;
		if(vscript_server_init_called) {
			VScriptRunScript_detour.call<bool>("mapspawn", nullptr, false);
		}
		in_vscript_server_init = false;
		return ret;
	}

	static detour VScriptServerTerm_detour;
	static void VScriptServerTerm_detour_callback() noexcept
	{
		in_vscript_server_term = true;
		//VScriptServerTerm_detour.call<void>();
		*g_pScriptVM = vm;
		in_vscript_server_term = false;
	}

	bool vmod::detours() noexcept
	{
		VScriptServerInit_detour.initialize(VScriptServerInit, VScriptServerInit_detour_callback);
		VScriptServerInit_detour.enable();

		VScriptServerTerm_detour.initialize(VScriptServerTerm, VScriptServerTerm_detour_callback);
		VScriptServerTerm_detour.enable();

		VScriptRunScript_detour.initialize(VScriptRunScript, VScriptRunScript_detour_callback);
		VScriptRunScript_detour.enable();

		CreateVM_original = swap_vfunc(vsmgr, &gsdk::IScriptManager::CreateVM, CreateVM_detour_callback);
		DestroyVM_original = swap_vfunc(vsmgr, &gsdk::IScriptManager::DestroyVM, DestroyVM_detour_callback);

		Run_original = swap_vfunc(vm, static_cast<gsdk::ScriptStatus_t(gsdk::IScriptVM::*)(const char *, bool)>(&gsdk::IScriptVM::Run), Run_detour_callback);

		return true;
	}

	std::string_view vmod::to_string(gsdk::HSCRIPT value) noexcept
	{
		script_variant_t ret;
		script_variant_t arg{value};

		if(vm->ExecuteFunction(to_string_func, &arg, 1, &ret, global_scope, true) == gsdk::SCRIPT_ERROR) {
			return {};
		}

		if(ret.m_type != gsdk::FIELD_CSTRING) {
			return {};
		}

		return ret.m_pszString;
	}

	bool vmod::load_late() noexcept
	{
		using namespace std::literals::string_view_literals;

		global_scope = vm->CreateScope("vmod_scope", nullptr);
		if(!global_scope || global_scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create global scope\n"sv);
			return false;
		}

		if(!VScriptServerInit_detour_callback()) {
			error("vmod: VScriptServerInit failed\n"sv);
			return false;
		}

		(reinterpret_cast<gsdk::CTFGameRules *>(0xbebebebe)->*RegisterScriptFunctions)();

		vscript_server_init_called = true;

		if(vm->GetLanguage() == gsdk::SL_SQUIRREL) {
			server_init_script = vm->CompileScript(reinterpret_cast<const char *>(g_Script_vscript_server), "vscript_server.nut");
			if(!server_init_script || server_init_script == gsdk::INVALID_HSCRIPT) {
				error("vmod: failed to compile server init script\n"sv);
				return false;
			}
		} else {
			error("vmod: server init script not supported on this language\n"sv);
			return false;
		}

		if(vm->Run(server_init_script, nullptr, true) == gsdk::SCRIPT_ERROR) {
			error("vmod: failed to run server init script\n"sv);
			return false;
		}

		std::string base_script_name{"vmod_base"sv};
		base_script_name += scripts_extension;

		if(std::filesystem::exists(base_script_path)) {
			{
				std::unique_ptr<unsigned char[]> script_data{read_file(base_script_path)};

				base_script = vm->CompileScript(reinterpret_cast<const char *>(script_data.get()), base_script_path.c_str());
				if(!base_script || base_script == gsdk::INVALID_HSCRIPT) {
				#ifndef __VMOD_BASE_SCRIPT_HEADER_INCLUDED
					error("vmod: failed to compile base script '%s'\n"sv, base_script_path.c_str());
					return false;
				#else
					base_script = vm->CompileScript(reinterpret_cast<const char *>(__vmod_base_script), base_script_name.c_str());
					if(!base_script || base_script == gsdk::INVALID_HSCRIPT) {
						error("vmod: failed to compile base script\n"sv);
						return false;
					}
				#endif
				}
			}
		} else {
		#ifndef __VMOD_BASE_SCRIPT_HEADER_INCLUDED
			error("vmod: missing base script '%s'\n"sv, base_script_path.c_str());
			return false;
		#else
			base_script = vm->CompileScript(reinterpret_cast<const char *>(__vmod_base_script), base_script_name.c_str());
			if(!base_script || base_script == gsdk::INVALID_HSCRIPT) {
				error("vmod: failed to compile base script\n"sv);
				return false;
			}
		#endif
		}

		if(vm->Run(base_script, global_scope, true) == gsdk::SCRIPT_ERROR) {
			error("vmod: failed to run base script '%s'\n"sv, base_script_path.c_str());
			return false;
		}

		to_string_func = vm->LookupFunction("__vmod_tostring", global_scope);
		if(!to_string_func || to_string_func == gsdk::INVALID_HSCRIPT) {
			error("vmod: base script '%s' missing '__vmod_tostring' function\n"sv, base_script_path.c_str());
			return false;
		}

		if(!bindings()) {
			return false;
		}

		vmod_refresh_plugins();

		return true;
	}

	void vmod::map_loaded(std::string_view name) noexcept
	{
		is_map_loaded = true;

		for(const auto &pl : plugins) {
			if(!*pl) {
				continue;
			}

			pl->map_loaded(name);
		}
	}

	void vmod::map_active() noexcept
	{
		for(const auto &pl : plugins) {
			if(!*pl) {
				continue;
			}

			pl->map_active();
		}
	}

	void vmod::map_unloaded() noexcept
	{
		if(is_map_loaded) {
			for(const auto &pl : plugins) {
				if(!*pl) {
					continue;
				}

				pl->map_unloaded();
			}
		}

		is_map_loaded = false;
	}

	void vmod::game_frame([[maybe_unused]] bool) noexcept
	{
	#if 0
		vm->Frame(sv_globals->frametime);
	#endif

		for(const auto &pl : plugins) {
			if(!*pl) {
				continue;
			}

			pl->game_frame();
		}
	}

	void vmod::unload() noexcept
	{
		vmod_unload_plugins();

		unbindings();

		vmod_reload_plugins.unregister();
		vmod_unload_plugins.unregister();
		vmod_unload_plugin.unregister();
		vmod_load_plugin.unregister();
		vmod_list_plugins.unregister();
		vmod_refresh_plugins.unregister();

		if(cvar_dll_id != gsdk::INVALID_CVAR_DLL_IDENTIFIER) {
			cvar->UnregisterConCommands(cvar_dll_id);
		}

		if(vm) {
			if(to_string_func && to_string_func != gsdk::INVALID_HSCRIPT) {
				vm->ReleaseFunction(to_string_func);
			}

			if(base_script && base_script != gsdk::INVALID_HSCRIPT) {
				vm->ReleaseScript(base_script);
			}

			if(server_init_script && server_init_script != gsdk::INVALID_HSCRIPT) {
				vm->ReleaseScript(server_init_script);
			}

			if(global_scope && global_scope != gsdk::INVALID_HSCRIPT) {
				vm->ReleaseScope(global_scope);
			}

			vsmgr->DestroyVM(vm);

			if(*g_pScriptVM == vm) {
				*g_pScriptVM = nullptr;
			}
		}
	}
}

namespace vmod
{
#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
	#pragma clang diagnostic ignored "-Wweak-vtables"
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
	class vsp final : public gsdk::IServerPluginCallbacks
	{
	public:
		inline vsp() noexcept
		{
			load_return = vmod.load();
		}

		inline ~vsp() noexcept
		{
			if(!unloaded) {
				vmod.unload();
			}
		}

	private:
		const char *GetPluginDescription() override
		{ return "vmod"; }

		bool Load(gsdk::CreateInterfaceFn, gsdk::CreateInterfaceFn) override
		{
			if(!load_return) {
				return false;
			}

			if(!vmod.load_late()) {
				return false;
			}

			return true;
		}

		void Unload() override
		{
			vmod.unload();
			unloaded = true;
		}

		void GameFrame(bool simulating) override
		{ vmod.game_frame(simulating); }

		void ServerActivate([[maybe_unused]] gsdk::edict_t *edicts, [[maybe_unused]] int num_edicts, [[maybe_unused]] int max_clients) override
		{ vmod.map_active(); }

		void LevelInit(const char *name) override
		{ vmod.map_loaded(name); }

		void LevelShutdown() override
		{ vmod.map_unloaded(); }

		bool load_return;
		bool unloaded;
	};
#ifdef __clang__
	#pragma clang diagnostic pop
#else
	#pragma GCC diagnostic pop
#endif

	static vsp vsp;
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif
extern "C" __attribute__((__visibility__("default"))) void * __attribute__((__cdecl__)) CreateInterface(const char *name, int *status)
{
	using namespace gsdk;

	if(std::strncmp(name, IServerPluginCallbacks::interface_name.data(), IServerPluginCallbacks::interface_name.length()) == 0) {
		if(status) {
			*status = IFACE_OK;
		}
		return static_cast<IServerPluginCallbacks *>(&vmod::vsp);
	} else {
		if(status) {
			*status = IFACE_FAILED;
		}
		return nullptr;
	}
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif
