#include "main.hpp"
#include <iostream>
#include "symbol_cache.hpp"
#include "vscript/vscript.hpp"
#include "gsdk/engine/vsp.hpp"
#include "gsdk/tier0/dbg.hpp"
#include <cstring>

#include "plugin.hpp"
#include "filesystem.hpp"
#include "gsdk/server/gamerules.hpp"
#include "gsdk/server/baseentity.hpp"
#include "gsdk/server/datamap.hpp"

#include <filesystem>
#include <string_view>
#include <climits>

#include "convar.hpp"
#include <utility>

#include "bindings/docs.hpp"
#include "bindings/strtables/singleton.hpp"

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
	std::unordered_map<std::string, entity_class_info> sv_ent_class_info;

	static std::unordered_map<std::string, gsdk::ScriptClassDesc_t *> sv_script_class_descs;
	static std::unordered_map<std::string, gsdk::datamap_t *> sv_datamaps;
	static std::unordered_map<std::string, gsdk::SendTable *> sv_sendtables;

	static main main_;

	main &main::instance() noexcept
	{ return main_; }

	gsdk::INetworkStringTable *m_pDownloadableFileTable;
	gsdk::INetworkStringTable *m_pModelPrecacheTable;
	gsdk::INetworkStringTable *m_pGenericPrecacheTable;
	gsdk::INetworkStringTable *m_pSoundPrecacheTable;
	gsdk::INetworkStringTable *m_pDecalPrecacheTable;

	gsdk::INetworkStringTable *g_pStringTableParticleEffectNames;
	gsdk::INetworkStringTable *g_pStringTableEffectDispatch;
	gsdk::INetworkStringTable *g_pStringTableVguiScreen;
	gsdk::INetworkStringTable *g_pStringTableMaterials;
	gsdk::INetworkStringTable *g_pStringTableInfoPanel;
	gsdk::INetworkStringTable *g_pStringTableClientSideChoreoScenes;

	main::~main() noexcept {}

	static const unsigned char *g_Script_init{nullptr};
	static const unsigned char *g_Script_vscript_server{nullptr};
	static const unsigned char *g_Script_spawn_helper{nullptr};
	static gsdk::IScriptVM **g_pScriptVM_ptr{nullptr};
	static bool(*VScriptServerInit)() {nullptr};
	static void(*VScriptServerTerm)() {nullptr};
	static bool(*VScriptServerRunScript)(const char *, gsdk::HSCRIPT, bool) {nullptr};
#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	static bool(*VScriptServerRunScriptForAllAddons)(const char *, gsdk::HSCRIPT, bool) {nullptr};
#endif
#if GSDK_ENGINE == GSDK_ENGINE_TF2
	static void(gsdk::CTFGameRules::*RegisterScriptFunctions)() {nullptr};
#endif
	static void(*PrintFunc)(HSQUIRRELVM, const SQChar *, ...) {nullptr};
	static void(*ErrorFunc)(HSQUIRRELVM, const SQChar *, ...) {nullptr};
	static void(gsdk::IScriptVM::*RegisterFunctionGuts)(gsdk::ScriptFunctionBinding_t *, gsdk::ScriptClassDesc_t *) {nullptr};
	static void(gsdk::IScriptVM::*RegisterFunction)(gsdk::ScriptFunctionBinding_t *) {nullptr};
	static bool(gsdk::IScriptVM::*RegisterClass)(gsdk::ScriptClassDesc_t *) {nullptr};
	static gsdk::HSCRIPT(gsdk::IScriptVM::*RegisterInstance)(gsdk::ScriptClassDesc_t *, void *) {nullptr};
	static bool(gsdk::IScriptVM::*SetValue_var)(gsdk::HSCRIPT, const char *, const gsdk::ScriptVariant_t &) {nullptr};
	static bool(gsdk::IScriptVM::*SetValue_str)(gsdk::HSCRIPT, const char *, const char *) {nullptr};
	static SQRESULT(*sq_setparamscheck)(HSQUIRRELVM, SQInteger, const SQChar *) {nullptr};
#if GSDK_ENGINE == GSDK_ENGINE_TF2
	static gsdk::ScriptClassDesc_t **sv_classdesc_pHead{nullptr};
#endif
	static gsdk::CUtlVector<gsdk::SendTable *> *g_SendTables{nullptr};

	static vscript::variant game_sq_versionnumber;
	static vscript::variant game_sq_version;
	static SQInteger game_sq_ver{static_cast<SQInteger>(-1)};
	static SQInteger curr_sq_ver{static_cast<SQInteger>(-1)};

	static bool in_vscript_server_init{false};
	static bool in_vscript_print{false};
	static bool in_vscript_error{false};

	static void vscript_output(const char *txt)
	{
		using namespace std::literals::string_view_literals;

		info("%s"sv, txt);
	}

	static gsdk::ScriptErrorFunc_t server_vs_error_cb{nullptr};
	static bool vscript_error_output(gsdk::ScriptErrorLevel_t lvl, const char *txt)
	{
		using namespace std::literals::string_view_literals;

		in_vscript_error = true;
		bool ret{server_vs_error_cb(lvl, txt)};
		in_vscript_error = false;

		return ret;
	}

#if GSDK_ENGINE == GSDK_ENGINE_TF2
	static gsdk::SpewOutputFunc_t old_spew{nullptr};
	static gsdk::SpewRetval_t new_spew(gsdk::SpewType_t type, const char *str)
	{
		if(in_vscript_error || in_vscript_print || in_vscript_server_init) {
			switch(type) {
				case gsdk::SPEW_LOG: {
					return gsdk::SPEW_CONTINUE;
				}
				case gsdk::SPEW_WARNING: {
					if(in_vscript_print) {
						return gsdk::SPEW_CONTINUE;
					}
				} break;
				default: break;
			}
		}

		const gsdk::Color *clr{GetSpewOutputColor()};

		if(!clr || (clr->r == 255 && clr->g == 255 && clr->b == 255)) {
			switch(type) {
				case gsdk::SPEW_MESSAGE: {
					if(in_vscript_error) {
						clr = &error_clr;
					} else {
						clr = &print_clr;
					}
				} break;
				case gsdk::SPEW_WARNING: {
					clr = &warning_clr;
				} break;
				case gsdk::SPEW_ASSERT: {
					clr = &error_clr;
				} break;
				case gsdk::SPEW_ERROR: {
					clr = &error_clr;
				} break;
				case gsdk::SPEW_LOG: {
					clr = &info_clr;
				} break;
				default: break;
			}
		}

		if(clr) {
			std::printf("\033[38;2;%hhu;%hhu;%hhum", clr->r, clr->g, clr->b);
			std::fflush(stdout);
		}

		gsdk::SpewRetval_t ret{old_spew(type, str)};

		if(clr) {
			std::fputs("\033[0m", stdout);
			std::fflush(stdout);
		}

		return ret;
	}
#endif

	static bool vscript_server_init_called{false};

	static bool in_vscript_server_term{false};
	static gsdk::IScriptVM *(gsdk::IScriptManager::*CreateVM_original)(gsdk::ScriptLanguage_t) {nullptr};
	static gsdk::IScriptVM *CreateVM_detour_callback(gsdk::IScriptManager *pthis, gsdk::ScriptLanguage_t lang) noexcept
	{
		if(in_vscript_server_init) {
			gsdk::IScriptVM *vmod_vm{main::instance().vm()};
			if(lang == vmod_vm->GetLanguage()) {
				return vmod_vm;
			} else {
				return nullptr;
			}
		}

		return (pthis->*CreateVM_original)(lang);
	}

	static void(gsdk::IScriptManager::*DestroyVM_original)(gsdk::IScriptVM *) {nullptr};
	static void DestroyVM_detour_callback(gsdk::IScriptManager *pthis, gsdk::IScriptVM *vm) noexcept
	{
		if(in_vscript_server_term) {
			if(vm == main::instance().vm()) {
				return;
			}
		}

		(pthis->*DestroyVM_original)(vm);
	}

	static gsdk::ScriptStatus_t(gsdk::IScriptVM::*Run_original)(const char *, bool) {nullptr};
	static gsdk::ScriptStatus_t Run_detour_callback(gsdk::IScriptVM *pthis, const char *script, bool wait) noexcept
	{
		if(in_vscript_server_init) {
			if(script == reinterpret_cast<const char *>(g_Script_vscript_server)) {
				return gsdk::SCRIPT_DONE;
			}
		}

		return (pthis->*Run_original)(script, wait);
	}

	static detour<decltype(VScriptServerRunScript)> VScriptServerRunScript_detour;
	static bool VScriptServerRunScript_detour_callback(const char *script, gsdk::HSCRIPT scope, bool warn) noexcept
	{
		if(!vscript_server_init_called) {
			if(std::strcmp(script, "mapspawn") == 0) {
				return true;
			}
		}

		return VScriptServerRunScript_detour(script, scope, warn);
	}

#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	static detour<decltype(VScriptServerRunScriptForAllAddons)> VScriptServerRunScriptForAllAddons_detour;
	static bool VScriptServerRunScriptForAllAddons_detour_callback(const char *script, gsdk::HSCRIPT scope, bool warn) noexcept
	{
		if(!vscript_server_init_called) {
			if(std::strcmp(script, "mapspawn") == 0) {
				return true;
			}
		}

		return VScriptServerRunScriptForAllAddons_detour(script, scope, warn);
	}
#endif

	static detour<decltype(VScriptServerInit)> VScriptServerInit_detour;
	static bool VScriptServerInit_detour_callback() noexcept
	{
		in_vscript_server_init = true;
		bool ret{vscript_server_init_called ? true : VScriptServerInit_detour()};
		gsdk::IScriptVM *vm{main::instance().vm()};
		*g_pScriptVM_ptr = vm;
		gsdk::g_pScriptVM = vm;
		if(vscript_server_init_called) {
			VScriptServerRunScript_detour("mapspawn", nullptr, false);
		#if GSDK_ENGINE == GSDK_ENGINE_L4D2
			VScriptServerRunScriptForAllAddons_detour("mapspawn", nullptr, false);
		#endif
		}
		in_vscript_server_init = false;
		return ret;
	}

	static detour<decltype(VScriptServerTerm)> VScriptServerTerm_detour;
	static void VScriptServerTerm_detour_callback() noexcept
	{
		in_vscript_server_term = true;
		//VScriptServerTerm_detour();
		gsdk::IScriptVM *vm{main::instance().vm()};
		*g_pScriptVM_ptr = vm;
		gsdk::g_pScriptVM = vm;
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

	static char __vscript_errorfunc_buffer[2048];
	static detour<decltype(ErrorFunc)> ErrorFunc_detour;
	static void ErrorFunc_detour_callback(HSQUIRRELVM m_hVM, const SQChar *s, ...)
	{
		va_list varg_list;
		va_start(varg_list, s);
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wformat-nonliteral"
	#endif
		std::vsnprintf(__vscript_errorfunc_buffer, sizeof(__vscript_errorfunc_buffer), s, varg_list);
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		in_vscript_error = true;
		ErrorFunc_detour(m_hVM, "%s", __vscript_errorfunc_buffer);
		in_vscript_error = false;
		va_end(varg_list);
	}

	static gsdk::ScriptFunctionBinding_t *current_binding{nullptr};

	static detour<decltype(RegisterFunctionGuts)> RegisterFunctionGuts_detour;
	static void RegisterFunctionGuts_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptFunctionBinding_t *binding, gsdk::ScriptClassDesc_t *classdesc)
	{
		current_binding = binding;
		RegisterFunctionGuts_detour(vm, binding, classdesc);
		current_binding = nullptr;

		if(binding->m_flags & vscript::function_desc::SF_VA_FUNC) {
			constexpr std::size_t arglimit{14};
			constexpr std::size_t va_args{arglimit};

			std::size_t current_size{binding->m_desc.m_Parameters.size()};
			if(current_size < arglimit) {
				std::size_t new_size{current_size + va_args};
				if(new_size > arglimit) {
					new_size = arglimit;
				}
				for(std::size_t i{current_size}; i < new_size; ++i) {
					binding->m_desc.m_Parameters.emplace_back(gsdk::FIELD_VARIANT);
				}
			}
		}
	}

	static detour<decltype(sq_setparamscheck)> sq_setparamscheck_detour;
	static SQRESULT sq_setparamscheck_detour_callback(HSQUIRRELVM v, SQInteger nparamscheck, const SQChar *typemask)
	{
		using namespace std::literals::string_view_literals;

		std::string temp_typemask{typemask};

		if(current_binding) {
			if(current_binding->m_flags & vscript::function_desc::SF_VA_FUNC) {
				nparamscheck = -nparamscheck;
			} else if(current_binding->m_flags & vscript::function_desc::SF_OPT_FUNC) {
				nparamscheck = -(nparamscheck-1);
				temp_typemask += "|o"sv;
			}
		}

		return sq_setparamscheck_detour(v, nparamscheck, temp_typemask.c_str());
	}

	static void (gsdk::IServerGameDLL::*CreateNetworkStringTables_original)() {nullptr};
	void main::CreateNetworkStringTables_detour_callback(gsdk::IServerGameDLL *dll)
	{
		(dll->*CreateNetworkStringTables_original)();

		m_pDownloadableFileTable = sv_stringtables->FindTable(gsdk::DOWNLOADABLE_FILE_TABLENAME);
		m_pModelPrecacheTable = sv_stringtables->FindTable(gsdk::MODEL_PRECACHE_TABLENAME);
		m_pGenericPrecacheTable = sv_stringtables->FindTable(gsdk::GENERIC_PRECACHE_TABLENAME);
		m_pSoundPrecacheTable = sv_stringtables->FindTable(gsdk::SOUND_PRECACHE_TABLENAME);
		m_pDecalPrecacheTable = sv_stringtables->FindTable(gsdk::DECAL_PRECACHE_TABLENAME);

		g_pStringTableParticleEffectNames = sv_stringtables->FindTable("ParticleEffectNames");
		g_pStringTableEffectDispatch = sv_stringtables->FindTable("EffectDispatch");
		g_pStringTableVguiScreen = sv_stringtables->FindTable("VguiScreen");
		g_pStringTableMaterials = sv_stringtables->FindTable("Materials");
		g_pStringTableInfoPanel = sv_stringtables->FindTable("InfoPanel");
		g_pStringTableClientSideChoreoScenes = sv_stringtables->FindTable("Scenes");

		class main &main{instance()};

		main.are_string_tables_created = true;

		main.recreate_script_stringtables();

		for(const auto &it : main.plugins) {
			if(!*it.second) {
				continue;
			}

			it.second->string_tables_created();
		}
	}

	static void (gsdk::IServerNetworkStringTableContainer::*RemoveAllTables_original)() {nullptr};
	void main::RemoveAllTables_detour_callback(gsdk::IServerNetworkStringTableContainer *cont)
	{
		class main &main{instance()};

		main.clear_script_stringtables();

		main.are_string_tables_created = false;

		m_pDownloadableFileTable = nullptr;
		m_pModelPrecacheTable = nullptr;
		m_pGenericPrecacheTable = nullptr;
		m_pSoundPrecacheTable = nullptr;
		m_pDecalPrecacheTable = nullptr;

		g_pStringTableParticleEffectNames = nullptr;
		g_pStringTableEffectDispatch = nullptr;
		g_pStringTableVguiScreen = nullptr;
		g_pStringTableMaterials = nullptr;
		g_pStringTableInfoPanel = nullptr;
		g_pStringTableClientSideChoreoScenes = nullptr;

		(cont->*RemoveAllTables_original)();
	}

	static void (gsdk::IScriptVM::*SetErrorCallback_original)(gsdk::ScriptErrorFunc_t) {nullptr};
	static void SetErrorCallback_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptErrorFunc_t func)
	{
		if(in_vscript_server_init) {
			server_vs_error_cb = func;
			return;
		}

		(vm->*SetErrorCallback_original)(func);
	}

	static std::vector<const gsdk::ScriptFunctionBinding_t *> internal_vscript_func_bindings;
	static std::vector<const gsdk::ScriptClassDesc_t *> internal_vscript_class_bindings;
	static std::unordered_map<std::string, bindings::docs::value> internal_vscript_values;

	static std::vector<const gsdk::ScriptFunctionBinding_t *> game_vscript_func_bindings;
	static std::vector<const gsdk::ScriptClassDesc_t *> game_vscript_class_bindings;
	static std::unordered_map<std::string, bindings::docs::value> game_vscript_values;

	static void (gsdk::IScriptVM::*RegisterFunction_original)(gsdk::ScriptFunctionBinding_t *) {nullptr};
	static detour<decltype(RegisterFunction)> RegisterFunction_detour;
	static void RegisterFunction_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptFunctionBinding_t *func)
	{
		if(RegisterFunction_original) {
			(vm->*RegisterFunction_original)(func);
		} else {
			RegisterFunction_detour(vm, func);
		}

		if(!main::instance().vm()) {
			auto &vec{internal_vscript_func_bindings};
			vec.emplace_back(func);
		} else if(!vscript_server_init_called) {
			auto &vec{game_vscript_func_bindings};
			vec.emplace_back(func);
		}
	}

	static bool (gsdk::IScriptVM::*RegisterClass_original)(gsdk::ScriptClassDesc_t *) {nullptr};
	static detour<decltype(RegisterClass)> RegisterClass_detour;
	static bool RegisterClass_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptClassDesc_t *desc)
	{
		bool ret;

		if(RegisterClass_original) {
			ret = (vm->*RegisterClass_original)(desc);
		} else {
			ret = RegisterClass_detour(vm, desc);
		}

		if(!main::instance().vm()) {
			auto &vec{internal_vscript_class_bindings};
			auto it{std::find(vec.begin(), vec.end(), desc)};
			if(it == vec.end()) {
				vec.emplace_back(desc);
			}
		} else if(!vscript_server_init_called) {
			auto &vec{game_vscript_class_bindings};
			auto it{std::find(vec.begin(), vec.end(), desc)};
			if(it == vec.end()) {
				vec.emplace_back(desc);
			}
		}

		return ret;
	}

	struct registered_instance_info_t
	{
		gsdk::HSCRIPT instance{nullptr};
		gsdk::ScriptClassDesc_t *desc{nullptr};
		void *ptr{nullptr};
	};

	static registered_instance_info_t last_registered_instance;

	static gsdk::HSCRIPT (gsdk::IScriptVM::*RegisterInstance_original)(gsdk::ScriptClassDesc_t *, void *) {nullptr};
	static detour<decltype(RegisterInstance)> RegisterInstance_detour;
	static gsdk::HSCRIPT RegisterInstance_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptClassDesc_t *desc, void *ptr)
	{
		gsdk::HSCRIPT ret;

		if(RegisterInstance_original) {
			ret = (vm->*RegisterInstance_original)(desc, ptr);
		} else {
			ret = RegisterInstance_detour(vm, desc, ptr);
		}

		last_registered_instance.instance = ret;
		last_registered_instance.desc = desc;
		last_registered_instance.ptr = ptr;

		if(!main::instance().vm()) {
			auto &vec{internal_vscript_class_bindings};
			auto it{std::find(vec.begin(), vec.end(), desc)};
			if(it == vec.end()) {
				vec.emplace_back(desc);
			}
		} else if(!vscript_server_init_called) {
			auto &vec{game_vscript_class_bindings};
			auto it{std::find(vec.begin(), vec.end(), desc)};
			if(it == vec.end()) {
				vec.emplace_back(desc);
			}
		}

		return ret;
	}

	static bool (gsdk::IScriptVM::*SetValue_str_original)(gsdk::HSCRIPT, const char *, const char *) {nullptr};
	static detour<decltype(SetValue_str)> SetValue_str_detour;
	static bool SetValue_str_detour_callback(gsdk::IScriptVM *vm, gsdk::HSCRIPT scope, const char *name, const char *value)
	{
		bool ret;

		if(SetValue_str_original) {
			ret = (vm->*SetValue_str_original)(scope, name, value);
		} else {
			ret = SetValue_str_detour(vm, scope, name, value);
		}

		if(scope == nullptr) {
			if(!main::instance().vm()) {
				auto &map{internal_vscript_values};
				vscript::variant var;
				var.assign<std::string>(value);
				map.emplace(name, std::move(var));
			} else if(!vscript_server_init_called) {
				auto &map{game_vscript_values};
				vscript::variant var;
				var.assign<std::string>(value);
				map.emplace(name, std::move(var));
			}
		}

		return ret;
	}

	static bool (gsdk::IScriptVM::*SetValue_var_original)(gsdk::HSCRIPT, const char *, const gsdk::ScriptVariant_t &value) {nullptr};
	static detour<decltype(SetValue_var)> SetValue_var_detour;
	static bool SetValue_var_detour_callback(gsdk::IScriptVM *vm, gsdk::HSCRIPT scope, const char *name, const gsdk::ScriptVariant_t &value)
	{
		bool ret;

		if(SetValue_var_original) {
			ret = (vm->*SetValue_var_original)(scope, name, value);
		} else {
			ret = SetValue_var_detour(vm, scope, name, value);
		}

		if(scope == nullptr) {
			if(value.m_type == gsdk::FIELD_HSCRIPT && last_registered_instance.instance == value.m_object) {
				if(!main::instance().vm()) {
					auto &map{internal_vscript_values};
					map.emplace(name, last_registered_instance.desc);
				} else if(!vscript_server_init_called) {
					auto &map{game_vscript_values};
					map.emplace(name, last_registered_instance.desc);
				}
			} else {
				if(!main::instance().vm()) {
					auto &map{internal_vscript_values};
					map.emplace(name, value);
				} else if(!vscript_server_init_called) {
					auto &map{game_vscript_values};
					map.emplace(name, value);
				}
			}
		}

		return ret;
	}

	bool main::detours_prevm() noexcept
	{
		if(RegisterFunction) {
			RegisterFunction_detour.initialize(RegisterFunction, RegisterFunction_detour_callback);
			RegisterFunction_detour.enable();
		}

		if(RegisterClass) {
			RegisterClass_detour.initialize(RegisterClass, RegisterClass_detour_callback);
			RegisterClass_detour.enable();
		}

		if(RegisterInstance) {
			RegisterInstance_detour.initialize(RegisterInstance, RegisterInstance_detour_callback);
			RegisterInstance_detour.enable();
		}

		if(SetValue_str) {
			SetValue_str_detour.initialize(SetValue_str, SetValue_str_detour_callback);
			SetValue_str_detour.enable();
		}

		if(SetValue_var) {
			SetValue_var_detour.initialize(SetValue_var, SetValue_var_detour_callback);
			SetValue_var_detour.enable();
		}

		return true;
	}

	bool main::detours() noexcept
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

		VScriptServerRunScript_detour.initialize(VScriptServerRunScript, VScriptServerRunScript_detour_callback);
		VScriptServerRunScript_detour.enable();

	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		VScriptServerRunScriptForAllAddons_detour.initialize(VScriptServerRunScriptForAllAddons, VScriptServerRunScriptForAllAddons_detour_callback);
		VScriptServerRunScriptForAllAddons_detour.enable();
	#endif

		CreateVM_original = swap_vfunc(vsmgr, &gsdk::IScriptManager::CreateVM, CreateVM_detour_callback);
		DestroyVM_original = swap_vfunc(vsmgr, &gsdk::IScriptManager::DestroyVM, DestroyVM_detour_callback);

		Run_original = swap_vfunc(vm_, static_cast<decltype(Run_original)>(&gsdk::IScriptVM::Run), Run_detour_callback);
		SetErrorCallback_original = swap_vfunc(vm_, &gsdk::IScriptVM::SetErrorCallback, SetErrorCallback_detour_callback);

		RegisterFunction_detour.disable();
		RegisterClass_detour.disable();
		RegisterInstance_detour.disable();
		SetValue_var_detour.disable();
		SetValue_str_detour.disable();

		RegisterFunction_original = swap_vfunc(vm_, &gsdk::IScriptVM::RegisterFunction, RegisterFunction_detour_callback);
		RegisterClass_original = swap_vfunc(vm_, &gsdk::IScriptVM::RegisterClass, RegisterClass_detour_callback);
		RegisterInstance_original = swap_vfunc(vm_, &gsdk::IScriptVM::RegisterInstance_impl, RegisterInstance_detour_callback);
		SetValue_var_original = swap_vfunc(vm_, static_cast<decltype(SetValue_var_original)>(&gsdk::IScriptVM::SetValue_impl), SetValue_var_detour_callback);
		SetValue_str_original = swap_vfunc(vm_, static_cast<decltype(SetValue_str_original)>(&gsdk::IScriptVM::SetValue), SetValue_str_detour_callback);

		CreateNetworkStringTables_original = swap_vfunc(gamedll, &gsdk::IServerGameDLL::CreateNetworkStringTables, CreateNetworkStringTables_detour_callback);

		RemoveAllTables_original = swap_vfunc(sv_stringtables, &gsdk::IServerNetworkStringTableContainer::RemoveAllTables, RemoveAllTables_detour_callback);

		return true;
	}

	std::filesystem::path main::build_plugin_path(std::string_view plname) const noexcept
	{
		std::filesystem::path path{plname};
		if(!path.is_absolute()) {
			path = (plugins_dir_/path);
		}
		if(!path.has_extension()) {
			path.replace_extension(scripts_extension);
		}
		return path;
	}

	static std::filesystem::path bin_folder;

	bool main::load() noexcept
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

		std::filesystem::path exe_filename;

		{
			char exe[PATH_MAX];
			ssize_t len{readlink("/proc/self/exe", exe, sizeof(exe))};
			exe[len] = '\0';

			exe_filename = exe;
		}

		bin_folder = exe_filename.parent_path();
		bin_folder /= "bin"sv;

		exe_filename = exe_filename.filename();
		exe_filename.replace_extension();

		if(exe_filename == "portal2_linux"sv) {
			bin_folder /= "linux32"sv;
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || \
		GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(exe_filename != "hl2_linux"sv && exe_filename != "srcds_linux"sv) {
			std::cout << "\033[0;31m"sv << "vmod: unsupported exe filename: '"sv << exe_filename << "'\n"sv << "\033[0m"sv;
			return false;
		}
	#elif GSDK_ENGINE == GSDK_ENGINE_PORTAL2
		if(exe_filename != "portal2_linux"sv) {
			std::cout << "\033[0;31m"sv << "vmod: unsupported exe filename: '"sv << exe_filename << "'\n"sv << "\033[0m"sv;
			return false;
		}
	#else
		#error
	#endif

		std::string_view launcher_lib_name;
		if(exe_filename == "hl2_linux"sv ||
			exe_filename == "portal2_linux"sv) {
			launcher_lib_name = "launcher.so"sv;
		} else if(exe_filename == "srcds_linux"sv) {
			launcher_lib_name = "dedicated_srv.so"sv;
		} else {
			std::cout << "\033[0;31m"sv << "vmod: unsupported exe filename: '"sv << exe_filename << "'\n"sv << "\033[0m"sv;
			return false;
		}

		if(!launcher_lib.load(bin_folder/launcher_lib_name)) {
			std::cout << "\033[0;31m"sv << "vmod: failed to open launcher library: '"sv << launcher_lib.error_string() << "'\n"sv << "\033[0m"sv;
			return false;
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		old_spew = GetSpewOutputFunc();
		SpewOutputFunc(new_spew);
	#elif GSDK_ENGINE == GSDK_ENGINE_PORTAL2 || \
			GSDK_ENGINE == GSDK_ENGINE_L4D2
		//TODO!!!
		//LoggingSystem
	#endif

		std::string_view engine_lib_name{"engine.so"sv};
		if(dedicated) {
			engine_lib_name = "engine_srv.so"sv;
		}
		if(!engine_lib.load(bin_folder/engine_lib_name)) {
			error("vmod: failed to open engine library: '%s'\n"sv, engine_lib.error_string().c_str());
			return false;
		}

		std::string_view filesystem_lib_name{"filesystem_stdio.so"sv};
		if(sv_engine->IsDedicatedServer()) {
			filesystem_lib_name = "dedicated_srv.so"sv;
		}
		if(!filesystem_lib.load(bin_folder/filesystem_lib_name)) {
			error("vmod: failed to open filesystem library: '%s'\n"sv, filesystem_lib.error_string().c_str());
			return false;
		}

		std::string_view vstdlib_lib_name{"libvstdlib.so"sv};
		if(sv_engine->IsDedicatedServer()) {
			vstdlib_lib_name = "libvstdlib_srv.so"sv;
		}
		if(!vstdlib_lib.load(bin_folder/vstdlib_lib_name)) {
			error("vmod: failed to open vstdlib library: '%s'\n"sv, vstdlib_lib.error_string().c_str());
			return false;
		}

		cvar_dll_id_ = cvar->AllocateDLLIdentifier();

		{
			char gamedir[PATH_MAX];
			sv_engine->GetGameDir(gamedir, sizeof(gamedir));

			game_dir_ = gamedir;
		}

		root_dir_ = game_dir_;
		root_dir_ /= "addons/vmod"sv;

		{
			std::filesystem::path assets_dir{root_dir_};
			assets_dir /= "assets"sv;

			filesystem->AddSearchPath(assets_dir.c_str(), "GAME");
			filesystem->AddSearchPath(assets_dir.c_str(), "mod");
		}

		plugins_dir_ = root_dir_;
		plugins_dir_ /= "plugins"sv;

		if(!pp.initialize()) {
			return false;
		}

		base_script_path = root_dir_;
		base_script_path /= "base/vmod_base"sv;
		base_script_path.replace_extension(scripts_extension);

		std::filesystem::path server_lib_name{game_dir_};
		server_lib_name /= "bin"sv;
		if(sv_engine->IsDedicatedServer()) {
			server_lib_name /= "server_srv.so"sv;
		} else {
			server_lib_name /= "server.so"sv;
		}
		if(!server_lib.load(server_lib_name)) {
			error("vmod: failed to open server library: '%s'\n"sv, server_lib.error_string().c_str());
			return false;
		}

		const auto &eng_symbols{engine_lib.symbols()};
		const auto &eng_global_qual{eng_symbols.global()};

		const auto &sv_symbols{server_lib.symbols()};
		const auto &sv_global_qual{sv_symbols.global()};

		auto g_Script_vscript_server_it{sv_global_qual.find("g_Script_vscript_server"s)};
		if(g_Script_vscript_server_it == sv_global_qual.end()) {
			error("vmod: missing 'g_Script_vscript_server' symbol\n"sv);
			return false;
		}

		auto g_Script_spawn_helper_it{sv_global_qual.find("g_Script_spawn_helper"s)};

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

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		auto VScriptServerRunScript_it{sv_global_qual.find("VScriptRunScript(char const*, HSCRIPT__*, bool)"s)};
		if(VScriptServerRunScript_it == sv_global_qual.end()) {
			error("vmod: missing 'VScriptRunScript' symbol\n"sv);
			return false;
		}
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		auto VScriptServerRunScript_it{sv_global_qual.find("VScriptServerRunScript(char const*, HSCRIPT__*, bool)"s)};
		if(VScriptServerRunScript_it == sv_global_qual.end()) {
			error("vmod: missing 'VScriptServerRunScript' symbol\n"sv);
			return false;
		}

		auto VScriptServerRunScriptForAllAddons_it{sv_global_qual.find("VScriptServerRunScriptForAllAddons(char const*, HSCRIPT__*, bool)"s)};
		if(VScriptServerRunScriptForAllAddons_it == sv_global_qual.end()) {
			error("vmod: missing 'VScriptServerRunScriptForAllAddons' symbol\n"sv);
			return false;
		}
	#else
		#error
	#endif

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		auto CTFGameRules_it{sv_symbols.find("CTFGameRules"s)};
		if(CTFGameRules_it == sv_symbols.end()) {
			error("vmod: missing 'CTFGameRules' symbols\n"sv);
			return false;
		}

		auto RegisterScriptFunctions_it{CTFGameRules_it->second->find("RegisterScriptFunctions()"s)};
		if(RegisterScriptFunctions_it == CTFGameRules_it->second->end()) {
			error("vmod: missing 'CTFGameRules::RegisterScriptFunctions()' symbol\n"sv);
			return false;
		}
	#endif

		auto CBaseEntity_it{sv_symbols.find("CBaseEntity"s)};
		if(CBaseEntity_it == sv_symbols.end()) {
			error("vmod: missing 'CBaseEntity' symbols\n"sv);
			return false;
		}

		auto GetScriptInstance_it{CBaseEntity_it->second->find("GetScriptInstance()"s)};
		if(GetScriptInstance_it == CBaseEntity_it->second->end()) {
			error("vmod: missing 'CBaseEntity::GetScriptInstance()' symbol\n"sv);
			return false;
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		auto sv_ScriptClassDesc_t_it{sv_symbols.find("ScriptClassDesc_t"s)};
		if(sv_ScriptClassDesc_t_it == sv_symbols.end()) {
			error("vmod: missing 'ScriptClassDesc_t' symbol\n"sv);
			return false;
		}

		auto sv_GetDescList_it{sv_ScriptClassDesc_t_it->second->find("GetDescList()"s)};
		if(sv_GetDescList_it == sv_ScriptClassDesc_t_it->second->end()) {
			error("vmod: missing 'ScriptClassDesc_t::GetDescList()' symbol\n"sv);
			return false;
		}

		auto sv_pHead_it{sv_GetDescList_it->second->find("pHead"s)};
		if(sv_pHead_it == sv_GetDescList_it->second->end()) {
			error("vmod: missing 'ScriptClassDesc_t::GetDescList()::pHead' symbol\n"sv);
			return false;
		}
	#endif

		std::string_view vscript_lib_name{"vscript.so"sv};
		if(sv_engine->IsDedicatedServer()) {
			vscript_lib_name = "vscript_srv.so"sv;
		}
		if(!vscript_lib.load(bin_folder/vscript_lib_name)) {
			error("vmod: failed to open vscript library: '%s'\n"sv, vscript_lib.error_string().c_str());
			return false;
		}

		const auto &vscript_symbols{vscript_lib.symbols()};
		const auto &vscript_global_qual{vscript_symbols.global()};

		auto CSquirrelVM_it{vscript_symbols.find("CSquirrelVM"s)};
		if(CSquirrelVM_it == vscript_symbols.end()) {
			error("vmod: missing 'CSquirrelVM' symbols\n"sv);
			return false;
		}

		auto sq_getversion_it{vscript_global_qual.find("sq_getversion"s)};
		if(sq_getversion_it == vscript_global_qual.end()) {
			error("vmod: missing 'sq_getversion' symbol\n"sv);
			return false;
		}

		auto RegisterFunction_it{CSquirrelVM_it->second->find("RegisterFunction(ScriptFunctionBinding_t*)"s)};
		if(RegisterFunction_it == CSquirrelVM_it->second->end()) {
			warning("vmod: missing 'CSquirrelVM::RegisterFunction(ScriptFunctionBinding_t*)' symbol\n"sv);
		} else {
			RegisterFunction = RegisterFunction_it->second->mfp<decltype(RegisterFunction)>();
		}

		auto RegisterClass_it{CSquirrelVM_it->second->find("RegisterClass(ScriptClassDesc_t*)"s)};
		if(RegisterClass_it == CSquirrelVM_it->second->end()) {
			warning("vmod: missing 'CSquirrelVM::RegisterClass(ScriptClassDesc_t*)' symbol\n"sv);
		} else {
			RegisterClass = RegisterClass_it->second->mfp<decltype(RegisterClass)>();
		}

		auto RegisterInstance_it{CSquirrelVM_it->second->find("RegisterInstance(ScriptClassDesc_t*, void*)"s)};
		if(RegisterInstance_it == CSquirrelVM_it->second->end()) {
			warning("vmod: missing 'CSquirrelVM::RegisterInstance(ScriptClassDesc_t*, void*)' symbol\n"sv);
		} else {
			RegisterInstance = RegisterInstance_it->second->mfp<decltype(RegisterInstance)>();
		}

		auto SetValue_str_it{CSquirrelVM_it->second->find("SetValue(HSCRIPT__*, char const*, char const*)"s)};
		if(SetValue_str_it == CSquirrelVM_it->second->end()) {
			warning("vmod: missing 'CSquirrelVM::SetValue(HSCRIPT__*, char const*, char const*)' symbol\n"sv);
		} else {
			SetValue_str = SetValue_str_it->second->mfp<decltype(SetValue_str)>();
		}

		auto SetValue_var_it{CSquirrelVM_it->second->find("SetValue(HSCRIPT__*, char const*, CVariantBase<CVariantDefaultAllocator> const&)"s)};
		if(SetValue_var_it == CSquirrelVM_it->second->end()) {
			warning("vmod: missing 'CSquirrelVM::SetValue(HSCRIPT__*, char const*, CVariantBase<CVariantDefaultAllocator> const&)' symbol\n"sv);
		} else {
			SetValue_var = SetValue_var_it->second->mfp<decltype(SetValue_var)>();
		}

		if(!detours_prevm()) {
			return false;
		}

		vm_ = vsmgr->CreateVM(script_language);
		if(!vm_) {
			error("vmod: failed to create VM\n"sv);
			return false;
		}

		{
			game_sq_ver = sq_getversion_it->second->func<decltype(::sq_getversion)>()();
			curr_sq_ver = ::sq_getversion();

			if(curr_sq_ver != SQUIRREL_VERSION_NUMBER) {
				error("vmod: mismatched squirrel header '%i' vs '%i'\n"sv, curr_sq_ver, SQUIRREL_VERSION_NUMBER);
				return false;
			}

			if(game_sq_ver != curr_sq_ver) {
				error("vmod: mismatched squirrel versions '%i' vs '%i'\n"sv, game_sq_ver, curr_sq_ver);
				return false;
			}

			if(!vm_->GetValue(nullptr, "_versionnumber_", &game_sq_versionnumber)) {
				error("vmod: failed to get _versionnumber_ value\n"sv);
				return false;
			}

			if(!vm_->GetValue(nullptr, "_version_", &game_sq_version)) {
				error("vmod: failed to get _version_ value\n"sv);
				return false;
			}
		}

		auto g_Script_init_it{vscript_global_qual.find("g_Script_init"s)};

		auto sq_setparamscheck_it{vscript_global_qual.find("sq_setparamscheck"s)};
		if(sq_setparamscheck_it == vscript_global_qual.end()) {
			error("vmod: missing 'sq_setparamscheck' symbol\n"sv);
			return false;
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		auto CreateArray_it{CSquirrelVM_it->second->find("CreateArray(CVariantBase<CVariantDefaultAllocator>&)"s)};
		if(CreateArray_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::CreateArray(CVariantBase<CVariantDefaultAllocator>&)' symbol\n"sv);
			return false;
		}

		auto GetArrayCount_it{CSquirrelVM_it->second->find("GetArrayCount(HSCRIPT__*)"s)};
		if(GetArrayCount_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::GetArrayCount(HSCRIPT__*)' symbol\n"sv);
			return false;
		}

		auto IsArray_it{CSquirrelVM_it->second->find("IsArray(HSCRIPT__*)"s)};
		if(IsArray_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::IsArray(HSCRIPT__*)' symbol\n"sv);
			return false;
		}

		auto IsTable_it{CSquirrelVM_it->second->find("IsTable(HSCRIPT__*)"s)};
		if(IsTable_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::IsTable(HSCRIPT__*)' symbol\n"sv);
			return false;
		}
	#endif

		auto PrintFunc_it{CSquirrelVM_it->second->find("PrintFunc(SQVM*, char const*, ...)"s)};
		if(PrintFunc_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::PrintFunc(SQVM*, char const*, ...)' symbol\n"sv);
			return false;
		}

		auto ErrorFunc_it{CSquirrelVM_it->second->find("ErrorFunc(SQVM*, char const*, ...)"s)};
		if(ErrorFunc_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::ErrorFunc(SQVM*, char const*, ...)' symbol\n"sv);
			return false;
		}

		auto RegisterFunctionGuts_it{CSquirrelVM_it->second->find("RegisterFunctionGuts(ScriptFunctionBinding_t*, ScriptClassDesc_t*)"s)};
		if(RegisterFunctionGuts_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::RegisterFunctionGuts(ScriptFunctionBinding_t*, ScriptClassDesc_t*)' symbol\n"sv);
			return false;
		}

		auto g_SendTables_it{eng_global_qual.find("g_SendTables"s)};
		if(g_SendTables_it == eng_global_qual.end()) {
			error("vmod: missing 'g_SendTables' symbol\n"sv);
			return false;
		}

		if(g_Script_init_it == vscript_global_qual.end()) {
			warning("vmod: missing 'g_Script_init' symbol\n");
		} else {
			g_Script_init = g_Script_init_it->second->addr<const unsigned char *>();
		}

		if(g_Script_spawn_helper_it == sv_global_qual.end()) {
			warning("vmod: missing 'g_Script_spawn_helper' symbol\n");
		} else {
			g_Script_spawn_helper = g_Script_spawn_helper_it->second->addr<const unsigned char *>();
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		RegisterScriptFunctions = RegisterScriptFunctions_it->second->mfp<decltype(RegisterScriptFunctions)>();
	#endif

		VScriptServerInit = VScriptServerInit_it->second->func<decltype(VScriptServerInit)>();
		VScriptServerTerm = VScriptServerTerm_it->second->func<decltype(VScriptServerTerm)>();
		VScriptServerRunScript = VScriptServerRunScript_it->second->func<decltype(VScriptServerRunScript)>();
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		VScriptServerRunScriptForAllAddons = VScriptServerRunScriptForAllAddons_it->second->func<decltype(VScriptServerRunScriptForAllAddons)>();
	#endif
		g_Script_vscript_server = g_Script_vscript_server_it->second->addr<const unsigned char *>();
		g_pScriptVM_ptr = g_pScriptVM_it->second->addr<gsdk::IScriptVM **>();
		gsdk::g_pScriptVM = *g_pScriptVM_ptr;

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		gsdk::IScriptVM::CreateArray_ptr = CreateArray_it->second->mfp<decltype(gsdk::IScriptVM::CreateArray_ptr)>();
		gsdk::IScriptVM::GetArrayCount_ptr = GetArrayCount_it->second->mfp<decltype(gsdk::IScriptVM::GetArrayCount_ptr)>();
		gsdk::IScriptVM::IsArray_ptr = IsArray_it->second->mfp<decltype(gsdk::IScriptVM::IsArray_ptr)>();
		gsdk::IScriptVM::IsTable_ptr = IsTable_it->second->mfp<decltype(gsdk::IScriptVM::IsTable_ptr)>();
	#endif

		PrintFunc = PrintFunc_it->second->func<decltype(PrintFunc)>();
		ErrorFunc = ErrorFunc_it->second->func<decltype(ErrorFunc)>();
		RegisterFunctionGuts = RegisterFunctionGuts_it->second->mfp<decltype(RegisterFunctionGuts)>();
		sq_setparamscheck = sq_setparamscheck_it->second->func<decltype(sq_setparamscheck)>();

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		sv_classdesc_pHead = sv_pHead_it->second->addr<gsdk::ScriptClassDesc_t **>();
	#endif

		g_SendTables = g_SendTables_it->second->addr<gsdk::CUtlVector<gsdk::SendTable *> *>();

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		gsdk::ScriptClassDesc_t *tmp_desc{*sv_classdesc_pHead};
		while(tmp_desc) {
			std::string name{tmp_desc->m_pszClassname};
			sv_script_class_descs.emplace(std::move(name), tmp_desc);
			tmp_desc = tmp_desc->m_pNextDesc;
		}
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		for(const auto &it : sv_global_qual) {
			if(it.first.starts_with("ScriptClassDesc_t* GetScriptDesc<"sv)) {
				gsdk::ScriptClassDesc_t *tmp_desc{it.second->func<gsdk::ScriptClassDesc_t *(*)(generic_object_t *)>()(nullptr)};
				std::string name{tmp_desc->m_pszClassname};
				sv_script_class_descs.emplace(std::move(name), tmp_desc);
			}
		}
	#else
		#error
	#endif

		for(const auto &it : sv_classes) {
			auto info_it{sv_ent_class_info.find(it.first)};
			if(info_it == sv_ent_class_info.end()) {
				info_it = sv_ent_class_info.emplace(it.first, entity_class_info{}).first;
			}

			info_it->second.sv_class = it.second;
			info_it->second.sendtable = it.second->m_pTable;

			auto script_desc_it{sv_script_class_descs.find(it.first)};
			if(script_desc_it != sv_script_class_descs.end()) {
				info_it->second.script_desc = script_desc_it->second;
			}

			auto sv_sym_it{sv_symbols.find(it.first)};
			if(sv_sym_it == sv_symbols.end()) {
				error("vmod: missing '%s' symbols\n"sv, it.first.c_str());
				return false;
			}

			auto GetDataDescMap_it{sv_sym_it->second->find("GetDataDescMap()"s)};
			if(GetDataDescMap_it != sv_sym_it->second->end()) {
				using GetDataDescMap_t = gsdk::datamap_t *(gsdk::CBaseEntity::*)();
				GetDataDescMap_t GetDataDescMap{GetDataDescMap_it->second->mfp<GetDataDescMap_t>()};
				gsdk::datamap_t *map{(reinterpret_cast<gsdk::CBaseEntity *>(uninitialized_memory)->*GetDataDescMap)()};

				info_it->second.datamap = map;
			}
		}

		gsdk::CBaseEntity::GetScriptInstance_ptr = GetScriptInstance_it->second->mfp<decltype(gsdk::CBaseEntity::GetScriptInstance_ptr)>();

		if(!assign_entity_class_info()) {
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

	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		for(const auto &it : sv_script_class_descs) {
			if(!RegisterClass_detour_callback(vm_, it.second)) {
				error("vmod: failed to register '%s' script class\n"sv, it.first.c_str());
				return false;
			}
		}
	#endif

		if(!VScriptServerInit_detour_callback()) {
			error("vmod: VScriptServerInit failed\n"sv);
			return false;
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		(reinterpret_cast<gsdk::CTFGameRules *>(uninitialized_memory)->*RegisterScriptFunctions)();
	#endif

		vscript_server_init_called = true;

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

		if(!get_func_from_base_script(typeof_func, "__typeof__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(funcisg_func, "__get_func_sig__"sv)) {
			return false;
		}

		vmod_reload_plugins.initialize("vmod_reload_plugins"sv,
			[this](const gsdk::CCommand &) noexcept -> void {
				auto it{plugins.begin()};
				while(it != plugins.end()) {
					if(it->second->reload() == plugin::load_status::disabled) {
						it = plugins.erase(it);
						continue;
					}
					++it;
				}

				if(plugins_loaded) {
					it = plugins.begin();
					while(it != plugins.end()) {
						if(!*it->second) {
							continue;
						}

						it->second->all_plugins_loaded();
					}
				}
			}
		);

		vmod_unload_plugins.initialize("vmod_unload_plugins"sv,
			[this](const gsdk::CCommand &) noexcept -> void {
				for(const auto &it : plugins) {
					it.second->unload();
				}

				plugins.clear();

				for(const std::filesystem::path &it : added_paths) {
					filesystem->RemoveSearchPath(it.c_str(), "GAME");
					filesystem->RemoveSearchPath(it.c_str(), "mod");
				}
				added_paths.clear();

				plugins_loaded = false;
			}
		);

		vmod_unload_plugin.initialize("vmod_unload_plugin"sv,
			[this](const gsdk::CCommand &args) noexcept -> void {
				if(args.m_nArgc != 2) {
					error("vmod: usage: vmod_unload_plugin <path>\n");
					return;
				}

				std::filesystem::path path{build_plugin_path(args.m_ppArgv[1])};
				if(path.extension() != scripts_extension) {
					error("vmod: invalid extension\n");
					return;
				}

				auto it{plugins.find(path)};
				if(it != plugins.end()) {
					error("vmod: unloaded plugin '%s'\n", it->first.c_str());
					plugins.erase(it);
				} else {
					error("vmod: plugin '%s' not found\n", path.c_str());
				}
			}
		);

		vmod_load_plugin.initialize("vmod_load_plugin"sv,
			[this](const gsdk::CCommand &args) noexcept -> void {
				if(args.m_nArgc != 2) {
					error("vmod: usage: vmod_load_plugin <path>\n");
					return;
				}

				std::filesystem::path path{build_plugin_path(args.m_ppArgv[1])};
				if(path.extension() != scripts_extension) {
					error("vmod: invalid extension\n");
					return;
				}

				auto it{plugins.find(path)};
				if(it != plugins.end()) {
					switch(it->second->reload()) {
						case plugin::load_status::success: {
							success("vmod: plugin '%s' reloaded\n", it->first.c_str());
							if(plugins_loaded) {
								it->second->all_plugins_loaded();
							}
						} break;
						case plugin::load_status::disabled: {
							plugins.erase(it);
						} break;
						default: break;
					}
					return;
				}

				std::unique_ptr<plugin> pl{new plugin{path}};
				switch(pl->load()) {
					case plugin::load_status::success: {
						success("vmod: plugin '%s' loaded\n", path.c_str());
						if(plugins_loaded) {
							pl->all_plugins_loaded();
						}
					} break;
					case plugin::load_status::disabled: {
						return;
					}
					default: break;
				}

				plugins.emplace(std::move(path), std::move(pl));
			}
		);

		vmod_list_plugins.initialize("vmod_list_plugins"sv,
			[this](const gsdk::CCommand &args) noexcept -> void {
				if(args.m_nArgc != 1) {
					error("vmod: usage: vmod_list_plugins\n");
					return;
				}

				if(plugins.empty()) {
					info("vmod: no plugins loaded\n");
					return;
				}

				for(const auto &it : plugins) {
					if(*it.second) {
						success("'%s'\n", it.first.c_str());
					} else {
						error("'%s'\n", it.first.c_str());
					}
				}
			}
		);

		vmod_refresh_plugins.initialize("vmod_refresh_plugins"sv,
			[this](const gsdk::CCommand &) noexcept -> void {
				vmod_unload_plugins();

				if(std::filesystem::exists(plugins_dir_)) {
					load_plugins(plugins_dir_, load_plugins_flags::none);
				}

				for(const auto &it : plugins) {
					if(!*it.second) {
						continue;
					}

					it.second->all_plugins_loaded();
				}

				plugins_loaded = true;
			}
		);

		vmod_auto_dump_internal_scripts.initialize("vmod_auto_dump_internal_scripts"sv, false);

		vmod_dump_internal_scripts.initialize("vmod_dump_internal_scripts"sv,
			[this](const gsdk::CCommand &) noexcept -> void {
				if(g_Script_init) {
					write_file(root_dir_/"internal_scripts"sv/"init.nut"sv, g_Script_init, std::strlen(reinterpret_cast<const char *>(g_Script_init)+1));
				}

				if(g_Script_spawn_helper) {
					write_file(root_dir_/"internal_scripts"sv/"spawn_helper.nut"sv, g_Script_spawn_helper, std::strlen(reinterpret_cast<const char *>(g_Script_spawn_helper)+1));
				}

				write_file(root_dir_/"internal_scripts"sv/"vscript_server.nut"sv, g_Script_vscript_server, std::strlen(reinterpret_cast<const char *>(g_Script_vscript_server)+1));
			}
		);

		vmod_auto_dump_squirrel_ver.initialize("vmod_auto_dump_squirrel_ver"sv, false);

		vmod_dump_squirrel_ver.initialize("vmod_dump_squirrel_ver"sv,
			[](const gsdk::CCommand &) noexcept -> void {
				info("vmod: squirrel version:\n"sv);
				info("vmod:   vmod:\n"sv);
				info("vmod:    SQUIRREL_VERSION: %s\n"sv, SQUIRREL_VERSION);
				info("vmod:    SQUIRREL_VERSION_NUMBER: %i\n"sv, SQUIRREL_VERSION_NUMBER);
				info("vmod:    sq_getversion: %i\n"sv, curr_sq_ver);
				info("vmod:   game:\n"sv);
				std::string_view _version{game_sq_version.get<std::string_view>()};
				info("vmod:    _version_: %s\n"sv, _version.data());
				info("vmod:    _versionnumber_: %i\n"sv, game_sq_versionnumber.get<int>());
				info("vmod:    sq_getversion: %i\n"sv, game_sq_ver);
			}
		);

		vmod_auto_gen_docs.initialize("vmod_auto_gen_docs"sv, false);

		vmod_gen_docs.initialize("vmod_gen_docs"sv,
			[this](const gsdk::CCommand &) noexcept -> void {
				if(!can_gen_docs) {
					error("vmod: cannot gen docs yet\n"sv);
					return;
				}

				build_internal_docs();

				std::filesystem::path internal_docs{root_dir_/"docs"sv/"internal"sv};
				bindings::docs::write(internal_docs, internal_vscript_class_bindings, false);
				bindings::docs::write(internal_docs, internal_vscript_func_bindings, false);
				bindings::docs::write(internal_docs, internal_vscript_values);

				std::filesystem::path game_docs{root_dir_/"docs"sv/"game"sv};
				bindings::docs::write(game_docs, game_vscript_class_bindings, false);
				bindings::docs::write(game_docs, game_vscript_func_bindings, false);
				bindings::docs::write(game_docs, game_vscript_values);

				gsdk::HSCRIPT const_table;
				if(vm_->GetValue(nullptr, "Constants", &const_table)) {
					std::string file;

					bindings::docs::gen_date(file);

					int num{vm_->GetNumTableEntries(const_table)};
					for(int i{0}, it{0}; it != -1 && i < num; ++i) {
						vscript::variant key;
						vscript::variant value;
						it = vm_->GetKeyValue(const_table, it, &key, &value);

						std::string_view enum_name{key.get<std::string_view>()};

						if(enum_name[0] == 'E' || enum_name[0] == 'F') {
							file += "enum class "sv;
						} else {
							file += "namespace "sv;
						}
						file += enum_name;
						file += "\n{\n"sv;

						gsdk::HSCRIPT enum_table{value.get<gsdk::HSCRIPT>()};
						bindings::docs::write(file, 1, enum_table, enum_name[0] == 'F' ? bindings::docs::write_enum_how::flags : bindings::docs::write_enum_how::normal);

						file += '}';

						if(enum_name[0] == 'E' || enum_name[0] == 'F') {
							file += ';';
						}

						file += "\n\n"sv;
					}
					if(num > 0) {
						file.erase(file.end()-1, file.end());
					}

					std::filesystem::path doc_path{game_docs};
					doc_path /= "Constants"sv;
					doc_path.replace_extension(".txt"sv);

					write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
				}

				std::filesystem::path vmod_docs{root_dir_/"docs"sv/"vmod"sv};
				write_docs(vmod_docs);
			}
		);

		sv_engine->InsertServerCommand("exec vmod/load.cfg\n");
		sv_engine->ServerExecute();

		if(vmod_auto_dump_squirrel_ver.get<bool>()) {
			vmod_dump_squirrel_ver();
		}

		if(vmod_auto_dump_internal_scripts.get<bool>()) {
			vmod_dump_internal_scripts();
		}

		return true;
	}

	bool main::assign_entity_class_info() noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		{
			auto CBaseEntity_desc_it{sv_script_class_descs.find("CBaseEntity"s)};
			if(CBaseEntity_desc_it == sv_script_class_descs.end()) {
				warning("vmod: failed to find baseentity script class\n"sv);
			} else {
				gsdk::CBaseEntity::g_pScriptDesc = CBaseEntity_desc_it->second;
			}
		}

		return true;
	}

	void main::load_plugins(const std::filesystem::path &dir, load_plugins_flags flags) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::filesystem::path dir_name{dir.filename()};

		std::error_code ec;
		for(const auto &file : std::filesystem::directory_iterator{dir, ec}) {
			std::filesystem::path path{file.path()};
			std::filesystem::path name{path.filename()};

			if(name.native()[0] == '.') {
				continue;
			}

			if(file.is_directory()) {
				if(std::filesystem::exists(path/".ignored"sv)) {
					continue;
				}

				if(name == "include"sv) {
					if(dir_name == "assets"sv) {
						remark("vmod: include folder inside assets folder: '%s'\n"sv, path.c_str());
					} else if(dir_name == "docs"sv) {
						remark("vmod: include folder inside docs folder: '%s'\n"sv, path.c_str());
					}
					continue;
				} else if(name == "docs"sv) {
					if(dir_name == "assets"sv) {
						remark("vmod: docs folder inside assets folder: '%s'\n"sv, path.c_str());
					} else if(dir_name == "include"sv) {
						remark("vmod: docs folder inside include folder: '%s'\n"sv, path.c_str());
					}
					continue;
				} else if(name == "assets"sv) {
					if(!(flags & load_plugins_flags::src_folder)) {
						filesystem->AddSearchPath(path.c_str(), "GAME");
						filesystem->AddSearchPath(path.c_str(), "mod");
						added_paths.emplace_back(std::move(path));
					} else {
						remark("vmod: assets folder inside src folder: '%s'\n"sv, path.c_str());
					}
					continue;
				} else if(name == "src"sv) {
					if(!(flags & load_plugins_flags::src_folder)) {
						load_plugins(path, load_plugins_flags::src_folder|load_plugins_flags::no_recurse);
					} else {
						remark("vmod: src folder inside another src folder: '%s'\n"sv, path.c_str());
					}
					continue;
				}

				if(!(flags & load_plugins_flags::no_recurse)) {
					load_plugins(path, load_plugins_flags::none);
				}
				continue;
			} else if(!file.is_regular_file()) {
				continue;
			}

			if(name.extension() != scripts_extension) {
				continue;
			}

			std::unique_ptr<plugin> pl{new plugin{path}};
			if(pl->load() == plugin::load_status::disabled) {
				continue;
			}

			plugins.emplace(std::move(path), std::move(pl));
		}
	}

	namespace detail
	{
		static std::string to_str_buffer;
		static std::string type_of_buffer;
	}

	gsdk::ScriptVariant_t main::call_to_func(gsdk::HSCRIPT func, gsdk::HSCRIPT value) noexcept
	{
		gsdk::ScriptVariant_t ret;
		vscript::variant arg{value};

		if(main::instance().vm()->ExecuteFunction(func, &arg, 1, &ret, nullptr, true) == gsdk::SCRIPT_ERROR) {
			return {};
		}

		return ret;
	}

	std::string_view main::to_string(gsdk::HSCRIPT value) const noexcept
	{
		gsdk::ScriptVariant_t ret{call_to_func(to_string_func, value)};

		if(ret.m_type != gsdk::FIELD_CSTRING) {
			return {};
		}

		detail::to_str_buffer = ret.m_ccstr;
		return detail::to_str_buffer;
	}

	std::string_view main::type_of(gsdk::HSCRIPT value) const noexcept
	{
		gsdk::ScriptVariant_t ret{call_to_func(typeof_func, value)};

		if(ret.m_type != gsdk::FIELD_CSTRING) {
			return {};
		}

		detail::type_of_buffer = ret.m_ccstr;
		return detail::type_of_buffer;
	}

	int main::to_int(gsdk::HSCRIPT value) const noexcept
	{
		gsdk::ScriptVariant_t ret{call_to_func(to_int_func, value)};

		if(ret.m_type != gsdk::FIELD_INTEGER) {
			return 0;
		}

		return ret.m_int;
	}

	float main::to_float(gsdk::HSCRIPT value) const noexcept
	{
		gsdk::ScriptVariant_t ret{call_to_func(to_float_func, value)};

		if(ret.m_type != gsdk::FIELD_FLOAT) {
			return 0.0f;
		}

		return ret.m_float;
	}

	bool main::to_bool(gsdk::HSCRIPT value) const noexcept
	{
		gsdk::ScriptVariant_t ret{call_to_func(to_bool_func, value)};

		if(ret.m_type != gsdk::FIELD_BOOLEAN) {
			return false;
		}

		return ret.m_bool;
	}

	static gsdk::ScriptFunctionBinding_t developer_desc;
	static gsdk::ScriptFunctionBinding_t isweakref_desc;
	static gsdk::ScriptFunctionBinding_t dumpobject_desc;
	static gsdk::ScriptFunctionBinding_t getfunctionsignature_desc;
	static gsdk::ScriptFunctionBinding_t makenamespace_desc;

	static gsdk::ScriptClassDesc_t instance_desc;
	static gsdk::ScriptClassDesc_t vector_desc;
	static gsdk::ScriptClassDesc_t quaternion_desc;
	static gsdk::ScriptClassDesc_t vector4d_desc;
	static gsdk::ScriptClassDesc_t vector2d_desc;
	static gsdk::ScriptClassDesc_t qangle_desc;

	//TODO!!! automate this
	void main::build_internal_docs() noexcept
	{
		if(internal_docs_built) {
			return;
		}

		{
			gsdk::ScriptFunctionBinding_t &func{developer_desc};

			func.m_flags = 0;
			func.m_desc.m_pszDescription = nullptr;
			func.m_desc.m_pszScriptName = "developer";
			func.m_desc.m_ReturnType = gsdk::FIELD_INTEGER;

			internal_vscript_func_bindings.emplace_back(&func);
		}

		{
			gsdk::ScriptFunctionBinding_t &func{isweakref_desc};

			func.m_flags = 0;
			func.m_desc.m_pszDescription = nullptr;
			func.m_desc.m_pszScriptName = "IsWeakref";
			func.m_desc.m_ReturnType = gsdk::FIELD_BOOLEAN;

			internal_vscript_func_bindings.emplace_back(&func);
		}

		{
			gsdk::ScriptFunctionBinding_t &func{getfunctionsignature_desc};

			func.m_flags = 0;
			func.m_desc.m_pszDescription = nullptr;
			func.m_desc.m_pszScriptName = "GetFunctionSignature";
			func.m_desc.m_ReturnType = gsdk::FIELD_CSTRING;

			internal_vscript_func_bindings.emplace_back(&func);
		}

		{
			gsdk::ScriptFunctionBinding_t &func{makenamespace_desc};

			func.m_flags = 0;
			func.m_desc.m_pszDescription = nullptr;
			func.m_desc.m_pszScriptName = "MakeNamespace";
			func.m_desc.m_ReturnType = gsdk::FIELD_VOID;

			internal_vscript_func_bindings.emplace_back(&func);
		}

		{
			gsdk::ScriptClassDesc_t &tmp_desc{instance_desc};

			tmp_desc.m_pszScriptName = "instance";
			tmp_desc.m_pBaseDesc = reinterpret_cast<gsdk::ScriptClassDesc_t *>(uninitialized_memory);
			tmp_desc.m_pszDescription = nullptr;
			tmp_desc.m_pNextDesc = nullptr;
			tmp_desc.m_pfnConstruct = nullptr;
			tmp_desc.m_pfnDestruct = nullptr;

			{
				gsdk::ScriptFunctionBinding_t &func{tmp_desc.m_FunctionBindings.emplace_back()};

				func.m_flags = gsdk::SF_MEMBER_FUNC;
				func.m_desc.m_pszDescription = nullptr;
				func.m_desc.m_pszScriptName = "IsValid";
				func.m_desc.m_ReturnType = gsdk::FIELD_BOOLEAN;
			}

			internal_vscript_class_bindings.emplace_back(&tmp_desc);
		}

		{
			gsdk::ScriptClassDesc_t &tmp_desc{vector_desc};

			tmp_desc.m_pszScriptName = "Vector";
			tmp_desc.m_pBaseDesc = reinterpret_cast<gsdk::ScriptClassDesc_t *>(uninitialized_memory);
			tmp_desc.m_pszDescription = nullptr;
			tmp_desc.m_pNextDesc = nullptr;
			tmp_desc.m_pfnConstruct = nullptr;
			tmp_desc.m_pfnDestruct = nullptr;

			internal_vscript_class_bindings.emplace_back(&tmp_desc);
		}

		{
			gsdk::ScriptClassDesc_t &tmp_desc{quaternion_desc};

			tmp_desc.m_pszScriptName = "Quaternion";
			tmp_desc.m_pBaseDesc = reinterpret_cast<gsdk::ScriptClassDesc_t *>(uninitialized_memory);
			tmp_desc.m_pszDescription = nullptr;
			tmp_desc.m_pNextDesc = nullptr;
			tmp_desc.m_pfnConstruct = nullptr;
			tmp_desc.m_pfnDestruct = nullptr;

			internal_vscript_class_bindings.emplace_back(&tmp_desc);
		}

		{
			gsdk::ScriptClassDesc_t &tmp_desc{vector4d_desc};

			tmp_desc.m_pszScriptName = "Vector4D";
			tmp_desc.m_pBaseDesc = reinterpret_cast<gsdk::ScriptClassDesc_t *>(uninitialized_memory);
			tmp_desc.m_pszDescription = nullptr;
			tmp_desc.m_pNextDesc = nullptr;
			tmp_desc.m_pfnConstruct = nullptr;
			tmp_desc.m_pfnDestruct = nullptr;

			internal_vscript_class_bindings.emplace_back(&tmp_desc);
		}

		{
			gsdk::ScriptClassDesc_t &tmp_desc{vector2d_desc};

			tmp_desc.m_pszScriptName = "Vector2D";
			tmp_desc.m_pBaseDesc = reinterpret_cast<gsdk::ScriptClassDesc_t *>(uninitialized_memory);
			tmp_desc.m_pszDescription = nullptr;
			tmp_desc.m_pNextDesc = nullptr;
			tmp_desc.m_pfnConstruct = nullptr;
			tmp_desc.m_pfnDestruct = nullptr;

			internal_vscript_class_bindings.emplace_back(&tmp_desc);
		}

		{
			gsdk::ScriptClassDesc_t &tmp_desc{qangle_desc};

			tmp_desc.m_pszScriptName = "QAngle";
			tmp_desc.m_pBaseDesc = reinterpret_cast<gsdk::ScriptClassDesc_t *>(uninitialized_memory);
			tmp_desc.m_pszDescription = nullptr;
			tmp_desc.m_pNextDesc = nullptr;
			tmp_desc.m_pfnConstruct = nullptr;
			tmp_desc.m_pfnDestruct = nullptr;

			internal_vscript_class_bindings.emplace_back(&tmp_desc);
		}

		internal_docs_built = true;
	}

	bool main::load_late() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(!bindings()) {
			return false;
		}

		can_gen_docs = true;

		sv_engine->InsertServerCommand("exec vmod/load_late.cfg\n");
		sv_engine->ServerExecute();

		if(vmod_auto_gen_docs.get<bool>()) {
			vmod_gen_docs();
		}

		vmod_refresh_plugins();

		return true;
	}

	void main::map_loaded(std::string_view mapname) noexcept
	{
		is_map_loaded = true;

		for(const auto &it : plugins) {
			if(!*it.second) {
				continue;
			}

			it.second->map_loaded(mapname);
		}
	}

	void main::map_active() noexcept
	{
		is_map_active = true;

		for(const auto &it : plugins) {
			if(!*it.second) {
				continue;
			}

			it.second->map_active();
		}
	}

	void main::map_unloaded() noexcept
	{
		if(is_map_loaded) {
			for(const auto &it : plugins) {
				if(!*it.second) {
					continue;
				}

				it.second->map_unloaded();
			}
		}

		is_map_loaded = false;
		is_map_active = false;
	}

	void main::game_frame(bool simulating) noexcept
	{
	#if 0
		vm_->Frame(sv_globals->frametime);
	#endif

		for(const auto &it : plugins) {
			it.second->game_frame(simulating);
		}
	}

	void main::unload() noexcept
	{
		vmod_unload_plugins();

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

			if(typeof_func && typeof_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(typeof_func);
			}

			if(funcisg_func && funcisg_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(funcisg_func);
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

			unbindings();

			vsmgr->DestroyVM(vm_);

			if(*g_pScriptVM_ptr == vm_) {
				*g_pScriptVM_ptr = nullptr;
			}

			if(gsdk::g_pScriptVM == vm_) {
				gsdk::g_pScriptVM = nullptr;
			}
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

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		if(old_spew) {
			SpewOutputFunc(old_spew);
		}
	#endif
	}
}

namespace vmod
{
	class vsp final : public gsdk::IServerPluginCallbacks
	{
	public:
		virtual inline ~vsp() noexcept
		{
			if(!unloaded) {
				main::instance().unload();
			}
		}

		static vsp &instance() noexcept;

	private:
		const char *GetPluginDescription() override;
		bool Load(gsdk::CreateInterfaceFn, gsdk::CreateInterfaceFn) override;
		void Unload() override;
		void GameFrame(bool simulating) override;
		void ServerActivate([[maybe_unused]] gsdk::edict_t *edicts, [[maybe_unused]] int num_edicts, [[maybe_unused]] int max_clients) override;
		void LevelInit(const char *name) override;
		void LevelShutdown() override;

		bool load_return;
		bool unloaded;
	};

	const char *vsp::GetPluginDescription()
	{ return "vmod"; }

	bool vsp::Load(gsdk::CreateInterfaceFn, gsdk::CreateInterfaceFn)
	{
		load_return = main::instance().load();

		if(!load_return) {
			return false;
		}

		if(!main::instance().load_late()) {
			return false;
		}

		return true;
	}

	void vsp::Unload()
	{
		main::instance().unload();
		unloaded = true;
	}

	void vsp::GameFrame(bool simulating)
	{ main::instance().game_frame(simulating); }

	void vsp::ServerActivate([[maybe_unused]] gsdk::edict_t *edicts, [[maybe_unused]] int num_edicts, [[maybe_unused]] int max_clients)
	{ main::instance().map_active(); }

	void vsp::LevelInit(const char *name)
	{ main::instance().map_loaded(name); }

	void vsp::LevelShutdown()
	{ main::instance().map_unloaded(); }

	static class vsp vsp;

	class vsp &vsp::instance() noexcept
	{ return ::vmod::vsp; }
}

extern "C" __attribute__((__visibility__("default"))) void * __attribute__((__cdecl__)) CreateInterface(const char *name, int *status)
{
	using namespace gsdk;

	std::string_view interface_name{IServerPluginCallbacks::interface_name};
	if(std::strncmp(name, interface_name.data(), interface_name.length()) == 0) {
		if(status) {
			*status = IFACE_OK;
		}
		return &vmod::vsp::instance();
	} else {
		if(status) {
			*status = IFACE_FAILED;
		}
		return nullptr;
	}
}
