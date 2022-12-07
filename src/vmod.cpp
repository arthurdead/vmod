#include "vmod.hpp"
#include "symbol_cache.hpp"
#include "gsdk/engine/vsp.hpp"
#include "gsdk/tier0/dbg.hpp"
#include <cstring>

#include "plugin.hpp"
#include "filesystem.hpp"
#include "gsdk/server/gamerules.hpp"

#include <filesystem>
#include <string_view>
#include <climits>

#include <iostream>

#include "convar.hpp"
#include <utility>

#include "gsdk/tier1/utlstring.hpp"
#include "yaml.hpp"
#include "ffi.hpp"

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
	static int(gsdk::IScriptVM::*GetArrayCount)(gsdk::HSCRIPT);
}

namespace gsdk
{
	int IScriptVM::GetArrayCount(HSCRIPT array)
	{
		return (this->*vmod::GetArrayCount)(array);
	}

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
				return false;
			}
			case gsdk::SCRIPT_LEVEL_ERROR: {
				error("%s"sv, txt);
				return false;
			}
		}
	}

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

	static class_desc_t<class vmod> vmod_desc{"__vmod_singleton_class"};

	bool vmod::Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value)
	{
		return vm_->GetValue(instance, name.c_str(), &value);
	}

	static class server_symbols_singleton final {
		friend class vmod;

		static gsdk::HSCRIPT script_lookup_shared(symbol_cache::const_iterator it) noexcept;
		static gsdk::HSCRIPT script_lookup_shared(symbol_cache::qualification_info::const_iterator it) noexcept;

		struct script_qual_it_t final
		{
			inline ~script_qual_it_t() noexcept
			{
				if(this->instance && this->instance != gsdk::INVALID_HSCRIPT) {
					vmod.vm()->RemoveInstance(this->instance);
				}
			}

			gsdk::HSCRIPT script_lookup(std::string_view name) const noexcept
			{
				using namespace std::literals::string_view_literals;

				std::string name_tmp{name};

				auto tmp_it{it_->second.find(name_tmp)};
				if(tmp_it == it_->second.end()) {
					return nullptr;
				}

				return script_lookup_shared(tmp_it);
			}

			inline std::string_view script_name() const noexcept
			{ return it_->first; }

			inline void script_delete() noexcept
			{ delete this; }

			symbol_cache::const_iterator it_;
			gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		};

		struct script_name_it_t final
		{
			inline ~script_name_it_t() noexcept
			{
				if(this->instance && this->instance != gsdk::INVALID_HSCRIPT) {
					vmod.vm()->RemoveInstance(this->instance);
				}
			}

			gsdk::HSCRIPT script_lookup(std::string_view name) const noexcept
			{
				using namespace std::literals::string_view_literals;

				std::string name_tmp{name};

				auto tmp_it{it_->second.find(name_tmp)};
				if(tmp_it == it_->second.end()) {
					return nullptr;
				}

				return script_lookup_shared(tmp_it);
			}

			inline std::string_view script_name() const noexcept
			{ return it_->first; }
			inline void *script_addr() const noexcept
			{ return it_->second.addr<void *>(); }
			inline generic_func_t script_func() const noexcept
			{ return it_->second.func<generic_func_t>(); }
			inline generic_mfp_t script_mfp() const noexcept
			{ return it_->second.mfp<generic_mfp_t>(); }
			inline std::size_t script_size() const noexcept
			{ return it_->second.size(); }

			inline void script_delete() noexcept
			{ delete this; }

			symbol_cache::qualification_info::const_iterator it_;
			gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		};

		static inline class_desc_t<script_qual_it_t> qual_it_desc{"qual_it"};
		static inline class_desc_t<script_name_it_t> name_it_desc{"name_it"};

		gsdk::HSCRIPT script_lookup(std::string_view name) const noexcept
		{
			using namespace std::literals::string_view_literals;

			std::string name_tmp{name};

			const symbol_cache &syms{vmod.server_lib.symbols()};

			auto it{syms.find(name_tmp)};
			if(it == syms.end()) {
				return nullptr;
			}

			return script_lookup_shared(it);
		}

		gsdk::HSCRIPT script_lookup_global(std::string_view name) const noexcept
		{
			using namespace std::literals::string_view_literals;

			std::string name_tmp{name};

			const symbol_cache::qualification_info &syms{vmod.server_lib.symbols().global()};

			auto it{syms.find(name_tmp)};
			if(it == syms.end()) {
				return nullptr;
			}

			return script_lookup_shared(it);
		}

		static bool bindings() noexcept;

		static inline void unbindings() noexcept
		{
			if(instance && instance != gsdk::INVALID_HSCRIPT) {
				vmod.vm()->RemoveInstance(instance);
			}
		}

		static inline gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
	} server_symbols_singleton;

	static class_desc_t<class server_symbols_singleton> server_symbols_desc{"server_symbols"};

	gsdk::HSCRIPT server_symbols_singleton::script_lookup_shared(symbol_cache::const_iterator it) noexcept
	{
		script_qual_it_t *script_it{new script_qual_it_t};
		script_it->it_ = it;

		gsdk::IScriptVM *vm{vmod.vm()};

		script_it->instance = vm->RegisterInstance(&qual_it_desc, script_it);
		if(!script_it->instance || script_it->instance == gsdk::INVALID_HSCRIPT) {
			delete script_it;
			vm->RaiseException("vmod: failed to create symbol qualification iterator instance");
			return nullptr;
		}

		//vm->SetInstanceUniqeId

		return script_it->instance;
	}

	gsdk::HSCRIPT server_symbols_singleton::script_lookup_shared(symbol_cache::qualification_info::const_iterator it) noexcept
	{
		script_name_it_t *script_it{new script_name_it_t};
		script_it->it_ = it;

		gsdk::IScriptVM *vm{vmod.vm()};

		script_it->instance = vm->RegisterInstance(&name_it_desc, script_it);
		if(!script_it->instance || script_it->instance == gsdk::INVALID_HSCRIPT) {
			delete script_it;
			vm->RaiseException("vmod: failed to create symbol name iterator instance");
			return nullptr;
		}

		//vm->SetInstanceUniqeId

		return script_it->instance;
	}

	bool server_symbols_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		server_symbols_desc.func(&server_symbols_singleton::script_lookup, "__script_lookup"sv, "lookup"sv);
		server_symbols_desc.func(&server_symbols_singleton::script_lookup_global, "__script_lookup_global"sv, "lookup_global"sv);

		if(!vm->RegisterClass(&server_symbols_desc)) {
			error("vmod: failed to register server symbols script class\n"sv);
			return false;
		}

		instance = vm->RegisterInstance(&server_symbols_desc, &::vmod::server_symbols_singleton);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create server symbols instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(instance, "__vmod_server_symbols_singleton");

		if(!vm->SetValue(vmod.symbols_table(), "server", instance)) {
			error("vmod: failed to set server symbols table value\n"sv);
			return false;
		}

		qual_it_desc.func(&script_qual_it_t::script_name, "__script_name"sv, "get_name"sv);
		qual_it_desc.func(&script_qual_it_t::script_lookup, "__script_lookup"sv, "lookup"sv);
		qual_it_desc.func(&script_qual_it_t::script_delete, "__script_delete"sv, "free"sv);
		qual_it_desc.dtor();

		if(!vm->RegisterClass(&qual_it_desc)) {
			error("vmod: failed to register symbol qualification iterator script class\n"sv);
			return false;
		}

		name_it_desc.func(&script_name_it_t::script_name, "__script_name"sv, "get_name"sv);
		name_it_desc.func(&script_name_it_t::script_addr, "__script_addr"sv, "get_addr"sv);
		name_it_desc.func(&script_name_it_t::script_func, "__script_func"sv, "get_func"sv);
		name_it_desc.func(&script_name_it_t::script_mfp, "__script_mfp"sv, "get_mfp"sv);
		name_it_desc.func(&script_name_it_t::script_size, "__script_size"sv, "get_size"sv);
		name_it_desc.func(&script_name_it_t::script_lookup, "__script_lookup"sv, "lookup"sv);
		name_it_desc.func(&script_name_it_t::script_delete, "__script_delete"sv, "free"sv);
		name_it_desc.dtor();

		if(!vm->RegisterClass(&name_it_desc)) {
			error("vmod: failed to register symbol name iterator script class\n"sv);
			return false;
		}

		return true;
	}

	bool vmod::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		vmod_desc.func(&vmod::script_find_plugin, "__script_find_plugin"sv, "find_plugin"sv);

		if(!vm_->RegisterClass(&vmod_desc)) {
			error("vmod: failed to register vmod script class\n"sv);
			return false;
		}

		instance = vm_->RegisterInstance(&vmod_desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create vmod instance\n"sv);
			return false;
		}

		vm_->SetInstanceUniqeId(instance, "__vmod_singleton");

		plugins_table_ = vm_->CreateTable();
		if(!plugins_table_ || plugins_table_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create plugins table\n"sv);
			return false;
		}

		if(!vm_->SetValue(scope_, "plugins", plugins_table_)) {
			error("vmod: failed to set plugins table value\n"sv);
			return false;
		}

		symbols_table_ = vm_->CreateTable();
		if(!symbols_table_ || symbols_table_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create symbols table\n"sv);
			return false;
		}

		if(!vm_->SetValue(scope_, "symbols", symbols_table_)) {
			error("vmod: failed to set symbols table value\n"sv);
			return false;
		}

		if(!server_symbols_singleton::bindings()) {
			return false;
		}

		get_impl = vm_->MakeSquirrelMetamethod_Get(nullptr, "vmod", this, false);
		if(!get_impl) {
			error("vmod: failed to create vmod _get metamethod\n"sv);
			return false;
		}

		if(!vm_->SetValue(scope_, "game_dir", game_dir.c_str())) {
			error("vmod: failed to set game dir value\n"sv);
			return false;
		}

		if(!plugin::bindings()) {
			return false;
		}

		if(!yaml::bindings()) {
			return false;
		}

		if(!ffi_bindings()) {
			return false;
		}

		return true;
	}

	bool vmod::binding_mods() noexcept
	{
		return true;
	}

	void vmod::unbindings() noexcept
	{
		plugin::unbindings();

		yaml::unbindings();

		server_symbols_singleton::unbindings();

		ffi_unbindings();

		if(plugins_table_ && plugins_table_ != gsdk::INVALID_HSCRIPT) {
			vm_->ReleaseTable(plugins_table_);
		}

		if(vm_->ValueExists(scope_, "plugins")) {
			vm_->ClearValue(scope_, "plugins");
		}

		if(symbols_table_ && symbols_table_ != gsdk::INVALID_HSCRIPT) {
			vm_->ReleaseTable(symbols_table_);
		}

		if(vm_->ValueExists(scope_, "symbols")) {
			vm_->ClearValue(scope_, "symbols");
		}

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm_->RemoveInstance(instance);
		}

		if(get_impl) {
			vm_->DestroySquirrelMetamethod_Get(get_impl);
		}

		if(vm_->ValueExists(scope_, "game_dir")) {
			vm_->ClearValue(scope_, "game_dir");
		}
	}

	static const unsigned char *g_Script_init;

	static const unsigned char *g_Script_vscript_server;
	static gsdk::IScriptVM **g_pScriptVM;
	static bool(*VScriptServerInit)();
	static void(*VScriptServerTerm)();
	static bool(*VScriptRunScript)(const char *, gsdk::HSCRIPT, bool);
	static void(gsdk::CTFGameRules::*RegisterScriptFunctions)();
	static void(*PrintFunc)(HSQUIRRELVM, const SQChar *, ...);
	static void(*ErrorFunc)(HSQUIRRELVM, const SQChar *, ...);
	static void(gsdk::IScriptVM::*RegisterFunctionGuts)(gsdk::ScriptFunctionBinding_t *, gsdk::ScriptClassDesc_t *);
	static SQRESULT(*sq_setparamscheck)(HSQUIRRELVM, SQInteger, const SQChar *);
	static gsdk::ScriptClassDesc_t **sv_classdesc_pHead;

	static bool in_vscript_server_init;
	static bool in_vscript_print;

	static gsdk::SpewOutputFunc_t old_spew;
	static gsdk::SpewRetval_t new_spew(gsdk::SpewType_t type, const char *str)
	{
		switch(type) {
			case gsdk::SPEW_LOG: {
				if(in_vscript_server_init || in_vscript_print) {
					return gsdk::SPEW_CONTINUE;
				}
			} break;
			case gsdk::SPEW_WARNING: {
				if(in_vscript_print) {
					return gsdk::SPEW_CONTINUE;
				}
			} break;
			case gsdk::SPEW_MESSAGE: {
				if(in_vscript_print) {
					return gsdk::SPEW_CONTINUE;
				}
			} break;
			default: break;
		}

		return old_spew(type, str);
	}

	static bool vscript_server_init_called;

	static bool in_vscript_server_term;
	static gsdk::IScriptVM *(gsdk::IScriptManager::*CreateVM_original)(gsdk::ScriptLanguage_t);
	static gsdk::IScriptVM *CreateVM_detour_callback(gsdk::IScriptManager *pthis, gsdk::ScriptLanguage_t lang) noexcept
	{
		if(in_vscript_server_init) {
			gsdk::IScriptVM *vmod_vm{vmod.vm()};
			if(lang == vmod_vm->GetLanguage()) {
				return vmod_vm;
			} else {
				return nullptr;
			}
		}

		return (pthis->*CreateVM_original)(lang);
	}

	static void(gsdk::IScriptManager::*DestroyVM_original)(gsdk::IScriptVM *);
	static void DestroyVM_detour_callback(gsdk::IScriptManager *pthis, gsdk::IScriptVM *vm) noexcept
	{
		if(in_vscript_server_term) {
			if(vm == vmod.vm()) {
				return;
			}
		}

		(pthis->*DestroyVM_original)(vm);
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

	static detour<decltype(VScriptRunScript)> VScriptRunScript_detour;
	static bool VScriptRunScript_detour_callback(const char *script, gsdk::HSCRIPT scope, bool warn) noexcept
	{
		if(!vscript_server_init_called) {
			if(strcmp(script, "mapspawn") == 0) {
				return true;
			}
		}

		return VScriptRunScript_detour(script, scope, warn);
	}

	static detour<decltype(VScriptServerInit)> VScriptServerInit_detour;
	static bool VScriptServerInit_detour_callback() noexcept
	{
		in_vscript_server_init = true;
		bool ret{vscript_server_init_called ? true : VScriptServerInit_detour()};
		*g_pScriptVM = vmod.vm();
		if(vscript_server_init_called) {
			VScriptRunScript_detour("mapspawn", nullptr, false);
		}
		in_vscript_server_init = false;
		return ret;
	}

	static detour<decltype(VScriptServerTerm)> VScriptServerTerm_detour;
	static void VScriptServerTerm_detour_callback() noexcept
	{
		in_vscript_server_term = true;
		//VScriptServerTerm_detour();
		*g_pScriptVM = vmod.vm();
		in_vscript_server_term = false;
	}

	static char __vscript_printfunc_buffer[2048];
	static detour<decltype(PrintFunc)> PrintFunc_detour;
	static void PrintFunc_detour_callback(HSQUIRRELVM m_hVM, const SQChar *s, ...)
	{
		va_list varg_list;
		va_start(varg_list, s);
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wformat-nonliteral"
	#endif
		std::vsnprintf(__vscript_printfunc_buffer, sizeof(__vscript_printfunc_buffer), s, varg_list);
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		in_vscript_print = true;
		PrintFunc_detour(m_hVM, "%s", __vscript_printfunc_buffer);
		in_vscript_print = false;
		va_end(varg_list);
	}

	static detour<decltype(ErrorFunc)> ErrorFunc_detour;
	static void ErrorFunc_detour_callback(HSQUIRRELVM m_hVM, const SQChar *s, ...)
	{
		va_list varg_list;
		va_start(varg_list, s);
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wformat-nonliteral"
	#endif
		std::vsnprintf(__vscript_printfunc_buffer, sizeof(__vscript_printfunc_buffer), s, varg_list);
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		in_vscript_print = true;
		ErrorFunc_detour(m_hVM, "%s", __vscript_printfunc_buffer);
		in_vscript_print = false;
		va_end(varg_list);
	}

	static gsdk::ScriptFunctionBinding_t *current_binding;

	static detour<decltype(RegisterFunctionGuts)> RegisterFunctionGuts_detour;
	static void RegisterFunctionGuts_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptFunctionBinding_t *binding, gsdk::ScriptClassDesc_t *classdesc)
	{
		current_binding = binding;

		RegisterFunctionGuts_detour(vm, binding, classdesc);

		if(binding->m_flags & func_desc_t::SF_VA_FUNC) {
			for(std::size_t i{0}; i < 14; ++i) {
				binding->m_desc.m_Parameters.emplace_back(gsdk::FIELD_VARIANT);
			}
		}

		current_binding = nullptr;
	}

	static detour<decltype(sq_setparamscheck)> sq_setparamscheck_detour;
	static SQRESULT sq_setparamscheck_detour_callback(HSQUIRRELVM v, SQInteger nparamscheck, const SQChar *typemask)
	{
		if(current_binding && (current_binding->m_flags & func_desc_t::SF_VA_FUNC)) {
			nparamscheck = -nparamscheck;
		}

		return sq_setparamscheck_detour(v, nparamscheck, typemask);
	}

	static bool (gsdk::IScriptVM::*RaiseException_original)(const char *);
	static bool RaiseException_detour_callback(gsdk::IScriptVM *vm, const char *str)
	{
		error("%s\n", str);
		return (vm->*RaiseException_original)(str);
	}

	bool vmod::detours() noexcept
	{
		RegisterFunctionGuts_detour.initialize(RegisterFunctionGuts, RegisterFunctionGuts_detour_callback);
		RegisterFunctionGuts_detour.enable();

		sq_setparamscheck_detour.initialize(sq_setparamscheck, sq_setparamscheck_detour_callback);
		sq_setparamscheck_detour.enable();

		PrintFunc_detour.initialize(PrintFunc, PrintFunc_detour_callback);
		PrintFunc_detour.enable();

		ErrorFunc_detour.initialize(ErrorFunc, ErrorFunc_detour_callback);
		ErrorFunc_detour.enable();

		VScriptServerInit_detour.initialize(VScriptServerInit, VScriptServerInit_detour_callback);
		VScriptServerInit_detour.enable();

		VScriptServerTerm_detour.initialize(VScriptServerTerm, VScriptServerTerm_detour_callback);
		VScriptServerTerm_detour.enable();

		VScriptRunScript_detour.initialize(VScriptRunScript, VScriptRunScript_detour_callback);
		VScriptRunScript_detour.enable();

		CreateVM_original = swap_vfunc(vsmgr, &gsdk::IScriptManager::CreateVM, CreateVM_detour_callback);
		DestroyVM_original = swap_vfunc(vsmgr, &gsdk::IScriptManager::DestroyVM, DestroyVM_detour_callback);

		Run_original = swap_vfunc(vm_, static_cast<decltype(Run_original)>(&gsdk::IScriptVM::Run), Run_detour_callback);

		RaiseException_original = swap_vfunc(vm_, &gsdk::IScriptVM::RaiseException, RaiseException_detour_callback);

		return true;
	}

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

		auto sv_ScriptClassDesc_t_it{sv_symbols.find("ScriptClassDesc_t"s)};
		if(sv_ScriptClassDesc_t_it == sv_symbols.end()) {
			error("vmod: missing 'ScriptClassDesc_t' symbol\n"sv);
			return false;
		}

		auto sv_GetDescList_it{sv_ScriptClassDesc_t_it->second.find("GetDescList()"s)};
		if(sv_GetDescList_it == sv_ScriptClassDesc_t_it->second.end()) {
			error("vmod: missing 'ScriptClassDesc_t::GetDescList()' symbol\n"sv);
			return false;
		}

		auto sv_pHead_it{sv_GetDescList_it->second.find("pHead"s)};
		if(sv_pHead_it == sv_GetDescList_it->second.end()) {
			error("vmod: missing 'ScriptClassDesc_t::GetDescList()::pHead' symbol\n"sv);
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

		auto sq_setparamscheck_it{vscript_global_qual.find("sq_setparamscheck"s)};
		if(sq_setparamscheck_it == vscript_global_qual.end()) {
			error("vmod: missing 'sq_setparamscheck' symbol\n"sv);
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

		auto GetArrayCount_it{CSquirrelVM_it->second.find("GetArrayCount(HSCRIPT__*)"s)};
		if(GetArrayCount_it == CSquirrelVM_it->second.end()) {
			error("vmod: missing 'CSquirrelVM::GetArrayCount(HSCRIPT__*)' symbol\n"sv);
			return false;
		}

		auto PrintFunc_it{CSquirrelVM_it->second.find("PrintFunc(SQVM*, char const*, ...)"s)};
		if(PrintFunc_it == CSquirrelVM_it->second.end()) {
			error("vmod: missing 'CSquirrelVM::PrintFunc(SQVM*, char const*, ...)' symbol\n"sv);
			return false;
		}

		auto ErrorFunc_it{CSquirrelVM_it->second.find("ErrorFunc(SQVM*, char const*, ...)"s)};
		if(ErrorFunc_it == CSquirrelVM_it->second.end()) {
			error("vmod: missing 'CSquirrelVM::ErrorFunc(SQVM*, char const*, ...)' symbol\n"sv);
			return false;
		}

		auto RegisterFunctionGuts_it{CSquirrelVM_it->second.find("RegisterFunctionGuts(ScriptFunctionBinding_t*, ScriptClassDesc_t*)"s)};
		if(RegisterFunctionGuts_it == CSquirrelVM_it->second.end()) {
			error("vmod: missing 'CSquirrelVM::RegisterFunctionGuts(ScriptFunctionBinding_t*, ScriptClassDesc_t*)' symbol\n"sv);
			return false;
		}

		g_Script_init = g_Script_init_it->second.addr<const unsigned char *>();

		RegisterScriptFunctions = RegisterScriptFunctions_it->second.mfp<decltype(RegisterScriptFunctions)>();
		VScriptServerInit = VScriptServerInit_it->second.func<decltype(VScriptServerInit)>();
		VScriptServerTerm = VScriptServerTerm_it->second.func<decltype(VScriptServerTerm)>();
		VScriptRunScript = VScriptRunScript_it->second.func<decltype(VScriptRunScript)>();
		g_Script_vscript_server = g_Script_vscript_server_it->second.addr<const unsigned char *>();
		g_pScriptVM = g_pScriptVM_it->second.addr<gsdk::IScriptVM **>();

		CreateArray = CreateArray_it->second.mfp<decltype(CreateArray)>();
		GetArrayCount = GetArrayCount_it->second.mfp<decltype(GetArrayCount)>();

		PrintFunc = PrintFunc_it->second.func<decltype(PrintFunc)>();
		ErrorFunc = ErrorFunc_it->second.func<decltype(ErrorFunc)>();
		RegisterFunctionGuts = RegisterFunctionGuts_it->second.mfp<decltype(RegisterFunctionGuts)>();
		sq_setparamscheck = sq_setparamscheck_it->second.func<decltype(sq_setparamscheck)>();

		sv_classdesc_pHead = sv_pHead_it->second.addr<gsdk::ScriptClassDesc_t **>();

		write_file(root_dir/"internal_scripts"sv/"init.nut"sv, g_Script_init, std::strlen(reinterpret_cast<const char *>(g_Script_init)+1));
		write_file(root_dir/"internal_scripts"sv/"vscript_server.nut"sv, g_Script_vscript_server, std::strlen(reinterpret_cast<const char *>(g_Script_vscript_server)+1));

		vm_ = vsmgr->CreateVM(script_language);
		if(!vm_) {
			error("vmod: failed to create VM\n"sv);
			return false;
		}

		vm_->SetOutputCallback(vscript_output);
		vm_->SetErrorCallback(vscript_error_output);

		if(!detours()) {
			return false;
		}

		if(!binding_mods()) {
			return false;
		}

		scope_ = vm_->CreateScope("vmod", nullptr);
		if(!scope_ || scope_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create vmod scope\n"sv);
			return false;
		}

		cvar_dll_id_ = cvar->AllocateDLLIdentifier();

		vmod_reload_plugins.initialize("vmod_reload_plugins"sv, [this](const gsdk::CCommand &) noexcept -> void {
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
		});

		vmod_unload_plugins.initialize("vmod_unload_plugins"sv, [this](const gsdk::CCommand &) noexcept -> void {
			for(const auto &pl : plugins) {
				pl->unload();
			}

			plugins.clear();

			plugins_loaded = false;
		});

		vmod_unload_plugin.initialize("vmod_unload_plugin"sv, [this](const gsdk::CCommand &args) noexcept -> void {
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
		});

		vmod_load_plugin.initialize("vmod_load_plugin"sv, [this](const gsdk::CCommand &args) noexcept -> void {
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
		});

		vmod_list_plugins.initialize("vmod_list_plugins"sv, [this](const gsdk::CCommand &args) noexcept -> void {
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
		});

		vmod_refresh_plugins.initialize("vmod_refresh_plugins"sv, [this](const gsdk::CCommand &) noexcept -> void {
			plugins.clear();

			for(const auto &file : std::filesystem::directory_iterator{plugins_dir}) {
				if(!file.is_regular_file()) {
					continue;
				}

				std::filesystem::path path{file.path()};
				if(path.extension() != scripts_extension) {
					continue;
				}

				plugin *pl{new plugin{std::move(path)}};
				pl->load();

				plugins.emplace_back(pl);
			}

			for(const auto &pl : plugins) {
				if(!*pl) {
					continue;
				}

				pl->all_plugins_loaded();
			}

			plugins_loaded = true;
		});

		old_spew = GetSpewOutputFunc();
		SpewOutputFunc(new_spew);

		if(!VScriptServerInit_detour_callback()) {
			error("vmod: VScriptServerInit failed\n"sv);
			return false;
		}

		(reinterpret_cast<gsdk::CTFGameRules *>(0xbebebebe)->*RegisterScriptFunctions)();

		vm_->SetOutputCallback(vscript_output);
		vm_->SetErrorCallback(vscript_error_output);

		vscript_server_init_called = true;

		if(vm_->GetLanguage() == gsdk::SL_SQUIRREL) {
			server_init_script = vm_->CompileScript(reinterpret_cast<const char *>(g_Script_vscript_server), "vscript_server.nut");
			if(!server_init_script || server_init_script == gsdk::INVALID_HSCRIPT) {
				error("vmod: failed to compile server init script\n"sv);
				return false;
			}
		} else {
			error("vmod: server init script not supported on this language\n"sv);
			return false;
		}

		if(vm_->Run(server_init_script, nullptr, true) == gsdk::SCRIPT_ERROR) {
			error("vmod: failed to run server init script\n"sv);
			return false;
		}

		std::string base_script_name{"vmod_base"sv};
		base_script_name += scripts_extension;

		if(std::filesystem::exists(base_script_path)) {
			{
				std::unique_ptr<unsigned char[]> script_data{read_file(base_script_path)};

				base_script = vm_->CompileScript(reinterpret_cast<const char *>(script_data.get()), base_script_path.c_str());
				if(!base_script || base_script == gsdk::INVALID_HSCRIPT) {
				#ifndef __VMOD_BASE_SCRIPT_HEADER_INCLUDED
					error("vmod: failed to compile base script '%s'\n"sv, base_script_path.c_str());
					return false;
				#else
					base_script = vm_->CompileScript(reinterpret_cast<const char *>(__vmod_base_script), base_script_name.c_str());
					if(!base_script || base_script == gsdk::INVALID_HSCRIPT) {
						error("vmod: failed to compile base script\n"sv);
						return false;
					}
				#endif
				} else {
					base_script_from_file = true;
				}
			}
		} else {
		#ifndef __VMOD_BASE_SCRIPT_HEADER_INCLUDED
			error("vmod: missing base script '%s'\n"sv, base_script_path.c_str());
			return false;
		#else
			base_script = vm_->CompileScript(reinterpret_cast<const char *>(__vmod_base_script), base_script_name.c_str());
			if(!base_script || base_script == gsdk::INVALID_HSCRIPT) {
				error("vmod: failed to compile base script\n"sv);
				return false;
			}
		#endif
		}

		base_script_scope = vm_->CreateScope("__vmod_base_script_scope__", nullptr);
		if(!base_script_scope || base_script_scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create base script scope\n"sv);
			return false;
		}

		if(vm_->Run(base_script, base_script_scope, true) == gsdk::SCRIPT_ERROR) {
			if(base_script_from_file) {
				error("vmod: failed to run base script '%s'\n"sv, base_script_path.c_str());
			} else {
				error("vmod: failed to run base script\n"sv);
			}
			return false;
		}

		auto get_func_from_base_script{[this](gsdk::HSCRIPT &func, std::string_view name) noexcept -> bool {
			func = vm_->LookupFunction(name.data(), base_script_scope);
			if(!func || func == gsdk::INVALID_HSCRIPT) {
				if(base_script_from_file) {
					error("vmod: base script '%s' missing '%s' function\n"sv, base_script_path.c_str(), name.data());
				} else {
					error("vmod: base script missing '%s' function\n"sv, name.data());
				}
				return false;
			}
			return true;
		}};

		if(!get_func_from_base_script(to_string_func, "__to_string__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(to_int_func, "__to_int__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(to_float_func, "__to_float__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(to_bool_func, "__to_bool__"sv)) {
			return false;
		}

		return true;
	}

	static script_variant_t call_to_func(gsdk::HSCRIPT func, gsdk::HSCRIPT value) noexcept
	{
		script_variant_t ret;
		script_variant_t arg{value};

		if(vmod.vm()->ExecuteFunction(func, &arg, 1, &ret, nullptr, true) == gsdk::SCRIPT_ERROR) {
			null_variant(ret);
			return ret;
		}

		return ret;
	}

	std::string_view vmod::to_string(gsdk::HSCRIPT value) const noexcept
	{
		script_variant_t ret{call_to_func(to_string_func, value)};

		if(ret.m_type != gsdk::FIELD_CSTRING) {
			return {};
		}

		return ret.m_pszString;
	}

	int vmod::to_int(gsdk::HSCRIPT value) const noexcept
	{
		script_variant_t ret{call_to_func(to_int_func, value)};

		if(ret.m_type != gsdk::FIELD_INTEGER) {
			return 0;
		}

		return ret.m_int;
	}

	float vmod::to_float(gsdk::HSCRIPT value) const noexcept
	{
		script_variant_t ret{call_to_func(to_float_func, value)};

		if(ret.m_type != gsdk::FIELD_FLOAT) {
			return 0.0f;
		}

		return ret.m_float;
	}

	bool vmod::to_bool(gsdk::HSCRIPT value) const noexcept
	{
		script_variant_t ret{call_to_func(to_bool_func, value)};

		if(ret.m_type != gsdk::FIELD_BOOLEAN) {
			return false;
		}

		return ret.m_bool;
	}

	bool vmod::load_late() noexcept
	{
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
		vm_->Frame(sv_globals->frametime);
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
		if(old_spew) {
			SpewOutputFunc(old_spew);
		}

		vmod_unload_plugins();

		if(vm_) {
			unbindings();
		}

		vmod_reload_plugins.unregister();
		vmod_unload_plugins.unregister();
		vmod_unload_plugin.unregister();
		vmod_load_plugin.unregister();
		vmod_list_plugins.unregister();
		vmod_refresh_plugins.unregister();

		if(cvar_dll_id_ != gsdk::INVALID_CVAR_DLL_IDENTIFIER) {
			cvar->UnregisterConCommands(cvar_dll_id_);
		}

		if(vm_) {
			if(to_string_func && to_string_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(to_string_func);
			}

			if(to_int_func && to_int_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(to_int_func);
			}

			if(to_float_func && to_float_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(to_float_func);
			}

			if(to_bool_func && to_bool_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(to_bool_func);
			}

			if(base_script && base_script != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseScript(base_script);
			}

			if(base_script_scope && base_script_scope != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseScope(base_script_scope);
			}

			if(server_init_script && server_init_script != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseScript(server_init_script);
			}

			if(scope_ && scope_ != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseScope(scope_);
			}

			vsmgr->DestroyVM(vm_);

			if(*g_pScriptVM == vm_) {
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
