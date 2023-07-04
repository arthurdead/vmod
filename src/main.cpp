#include "main.hpp"
#include <cstddef>
#include <functional>
#include <iostream>
#include "vscript/vscript.hpp"
#include "gsdk/engine/vsp.hpp"
#include "gsdk/tier0/dbg.hpp"
#include <cstring>
#include <dlfcn.h>

#ifdef __VMOD_USING_CUSTOM_VM
	#include "vm/squirrel/vm.hpp"

	#ifdef __VMOD_ENABLE_SOURCEPAWN
		#include "vm/sourcepawn/vm.hpp"
	#endif

	#ifdef __VMOD_ENABLE_V8
		#include "vm/v8/vm.hpp"
	#endif
#endif

#include "plugin.hpp"
#include "filesystem.hpp"
#include "gsdk/server/gamerules.hpp"
#include "gsdk/server/baseentity.hpp"
#include "gsdk/server/datamap.hpp"

#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <climits>

#include "convar.hpp"
#include <utility>

#include "bindings/docs.hpp"
#include "bindings/strtables/singleton.hpp"
#include "bindings/ent/bindings.hpp"

namespace vmod
{
	namespace detail
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
		#define __VMOD_SQUIRREL_BASE_SCRIPT_HEADER_INCLUDED
	#endif
	}

#ifdef __VMOD_SQUIRREL_BASE_SCRIPT_HEADER_INCLUDED
	static std::string __squirrel_vmod_base_script{reinterpret_cast<const char *>(detail::__squirrel_vmod_base_script_data), sizeof(detail::__squirrel_vmod_base_script_data)};
#endif
}

namespace vmod
{
	namespace detail
	{
	#if __has_include("vmod_server_init.nut.h")
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
	#endif
		#include "vmod_server_init.nut.h"
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		#define __VMOD_SQUIRREL_SERVER_SCRIPT_HEADER_INCLUDED
	#endif
	}

#ifdef __VMOD_SQUIRREL_SERVER_SCRIPT_HEADER_INCLUDED
	static std::string __squirrel_vmod_server_init_script{reinterpret_cast<const char *>(detail::__squirrel_vmod_server_init_script_data), sizeof(detail::__squirrel_vmod_server_init_script_data)};
#endif
}

namespace vmod
{
	std::unordered_map<std::string, entity_class_info> sv_ent_class_info;

	static std::unordered_map<std::string, gsdk::ScriptClassDesc_t *> sv_script_class_descs;
	static std::unordered_map<std::string, gsdk::datamap_t *> sv_datamaps;
	static std::unordered_map<std::string, gsdk::SendTable *> sv_sendtables;
	static std::unordered_map<std::string, gsdk::IEntityFactory *> sv_ent_factories;

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

#ifndef __VMOD_SQUIRREL_NEED_INIT_SCRIPT
	static
#endif
	const unsigned char *g_Script_init{nullptr};

	static const unsigned char *g_Script_vscript_server{nullptr};
	static const unsigned char *g_Script_spawn_helper{nullptr};
	static const unsigned char *szAddCode{nullptr};

	static gsdk::IScriptVM **g_pScriptVM_ptr{nullptr};
	static bool(*VScriptServerInit)() {nullptr};
	static void(*VScriptServerTerm)() {nullptr};
	static bool(*VScriptServerRunScript)(const char *, gsdk::HSCRIPT, bool) {nullptr};
#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	static bool(*VScriptServerRunScriptForAllAddons)(const char *, gsdk::HSCRIPT, bool) {nullptr};
#endif
#if GSDK_ENGINE == GSDK_ENGINE_TF2
	static void(gsdk::CTFGameRules::*RegisterScriptFunctions)() {nullptr};
#elif GSDK_ENGINE == GSDK_ENGINE_PORTAL2
	static void(gsdk::CPortalGameRules::*RegisterScriptFunctionsSP)() {nullptr};
	static void(gsdk::CPortalMPGameRules::*RegisterScriptFunctionsMP)() {nullptr};
#endif
	static void(*PrintFunc)(HSQUIRRELVM, const SQChar *, ...) {nullptr};
	static void(*ErrorFunc)(HSQUIRRELVM, const SQChar *, ...) {nullptr};
#ifdef __VMOD_USING_CUSTOM_VM
	static gsdk::IScriptVM *(*ScriptCreateSquirrelVM)() {nullptr};
	static void (*ScriptDestroySquirrelVM)(gsdk::IScriptVM *) {nullptr};
#endif
#ifndef __VMOD_USING_CUSTOM_VM
	static void(gsdk::IScriptVM::*RegisterFunctionGuts)(gsdk::ScriptFunctionBinding_t *, gsdk::ScriptClassDesc_t *) {nullptr};
	static SQRESULT(*sq_setparamscheck)(HSQUIRRELVM, SQInteger, const SQChar *) {nullptr};
#endif
	static void(gsdk::IScriptVM::*RegisterFunction)(gsdk::ScriptFunctionBinding_t *) {nullptr};
	static bool(gsdk::IScriptVM::*RegisterClass)(gsdk::ScriptClassDesc_t *) {nullptr};
	static gsdk::HSCRIPT(gsdk::IScriptVM::*RegisterInstance)(gsdk::ScriptClassDesc_t *, void *) {nullptr};
	static bool(gsdk::IScriptVM::*SetValue_var)(gsdk::HSCRIPT, const char *, const gsdk::ScriptVariant_t &) {nullptr};
	static bool(gsdk::IScriptVM::*SetValue_str)(gsdk::HSCRIPT, const char *, const char *) {nullptr};
#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
	static gsdk::ScriptClassDesc_t **sv_classdesc_pHead{nullptr};
#endif
	static gsdk::CUtlVector<gsdk::SendTable *> *g_SendTables{nullptr};

#if GSDK_ENGINE != GSDK_ENGINE_TF2
	static vscript::function_desc register_scripthook_listener_desc;
#endif

	static vscript::variant game_sq_versionnumber;
	static vscript::variant game_sq_version;
	static SQInteger game_sq_ver{-1};
#ifndef __VMOD_USING_QUIRREL
	static SQInteger curr_sq_ver{-1};
#endif

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

		bool ret{false};
		if(server_vs_error_cb) {
			ret = server_vs_error_cb(lvl, txt);
		} else {
			switch(lvl) {
				case gsdk::SCRIPT_LEVEL_ERROR: {
					error("%s"sv, txt);
				} break;
				case gsdk::SCRIPT_LEVEL_WARNING: {
					warning("%s"sv, txt);
				} break;
			#ifndef __clang__
				default: break;
			#endif
			}
			ret = false;
		}

		return ret;
	}

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
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

	bool main::dump_scripts() const noexcept
	{
		return vmod_auto_dump_internal_scripts.get<bool>();
	}

	static gsdk::HSCRIPT(gsdk::IScriptVM::*CompileScript_original)(const char *, const char *) {nullptr};
	static gsdk::HSCRIPT CompileScript_detour_callback(gsdk::IScriptVM *pthis, const char *script, const char *name) noexcept
	{
		using namespace std::literals::string_view_literals;

		main &main{main::instance()};

		if(main.dump_scripts()) {
			std::filesystem::path dump_path{main.root_dir()};
			dump_path /= "dumps/compiled_scripts"sv;

			if(name && name[0] != '\0') {
				dump_path /= "named"sv;

				std::error_code ec;
				std::filesystem::create_directories(dump_path, ec);

				dump_path /= name;
			} else {
				dump_path /= "unnamed"sv;

				std::error_code ec;
				std::filesystem::create_directories(dump_path, ec);

				std::size_t hash{std::hash<std::string_view>{}(script)};

				std::string tmp;
				tmp.resize(12);

				char *begin{tmp.data()};
				char *end{begin + tmp.size()};

				std::to_chars_result tc_res{std::to_chars(begin, end, hash)};
				tc_res.ptr[0] = '\0';

				dump_path /= begin;
			}

			write_file(dump_path, reinterpret_cast<const unsigned char *>(script), std::strlen(script));
		}

		return (pthis->*CompileScript_original)(script, name);
	}

	static gsdk::ScriptStatus_t(gsdk::IScriptVM::*Run_original)(const char *, bool) {nullptr};
	static gsdk::ScriptStatus_t Run_detour_callback(gsdk::IScriptVM *pthis, const char *script, bool wait) noexcept
	{
		using namespace std::literals::string_view_literals;

		if(in_vscript_server_init) {
			if(script == reinterpret_cast<const char *>(g_Script_vscript_server)) {
				return gsdk::SCRIPT_DONE;
			}
		}

		main &main{main::instance()};

		if(main.dump_scripts()) {
			std::filesystem::path dump_path{main.root_dir()};
			dump_path /= "dumps/compiled_scripts/unnamed"sv;

			std::error_code ec;
			std::filesystem::create_directories(dump_path, ec);

			std::size_t hash{std::hash<std::string_view>{}(script)};

			std::string tmp;
			tmp.resize(12);

			char *begin{tmp.data()};
			char *end{begin + tmp.size()};

			std::to_chars_result tc_res{std::to_chars(begin, end, hash)};
			tc_res.ptr[0] = '\0';

			dump_path /= begin;

			write_file(dump_path, reinterpret_cast<const unsigned char *>(script), std::strlen(script));
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
		#if GSDK_ENGINE == GSDK_ENGINE_L4D2
			else if(std::strcmp(script, "response_testbed") == 0) {
				return true;
			}
		#endif
		}

		return VScriptServerRunScriptForAllAddons_detour(script, scope, warn);
	}
#endif

	static detour<decltype(VScriptServerInit)> VScriptServerInit_detour;
	static bool VScriptServerInit_detour_callback() noexcept
	{
		in_vscript_server_init = true;
		gsdk::IScriptVM *vm{main::instance().vm()};
		*g_pScriptVM_ptr = vm;
		gsdk::g_pScriptVM = vm;
		bool ret{vscript_server_init_called ? true : VScriptServerInit_detour()};
		if(!vscript_server_init_called) {
			*g_pScriptVM_ptr = vm;
			gsdk::g_pScriptVM = vm;
		}
		if(vscript_server_init_called) {
			VScriptServerRunScript_detour("mapspawn", nullptr, false);
		#if GSDK_ENGINE == GSDK_ENGINE_L4D2
			VScriptServerRunScriptForAllAddons_detour("mapspawn", nullptr, false);
			VScriptServerRunScriptForAllAddons_detour("response_testbed", nullptr, false);
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
	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		vm->CollectGarbage(nullptr, false);
	#endif
	#if GSDK_ENGINE != GSDK_ENGINE_L4D2
		vm->RemoveOrphanInstances();
	#endif
		*g_pScriptVM_ptr = vm;
		gsdk::g_pScriptVM = vm;
		in_vscript_server_term = false;
	}

	static char __vscript_printfunc_buffer[gsdk::MAXPRINTMSG];
	static detour<decltype(PrintFunc)> PrintFunc_detour;
	static __attribute__((__format__(__printf__, 2, 3))) void PrintFunc_detour_callback(HSQUIRRELVM m_hVM, const SQChar *s, ...)
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

	static char __vscript_errorfunc_buffer[gsdk::MAXPRINTMSG];
	static detour<decltype(ErrorFunc)> ErrorFunc_detour;
	static __attribute__((__format__(__printf__, 2, 3))) void ErrorFunc_detour_callback(HSQUIRRELVM m_hVM, const SQChar *s, ...)
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

#ifndef __VMOD_USING_CUSTOM_VM
	static gsdk::ScriptFunctionBinding_t *current_binding{nullptr};

	static detour<decltype(RegisterFunctionGuts)> RegisterFunctionGuts_detour;
	static void RegisterFunctionGuts_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptFunctionBinding_t *binding, gsdk::ScriptClassDesc_t *classdesc)
	{
		current_binding = binding;
		RegisterFunctionGuts_detour(vm, binding, classdesc);
		current_binding = nullptr;

		if(binding->m_flags & gsdk::SF_VA_FUNC) {
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
			if(current_binding->m_flags & gsdk::SF_VA_FUNC) {
				nparamscheck = -nparamscheck;
			} else if(current_binding->m_flags & gsdk::SF_OPT_FUNC) {
				nparamscheck = -(nparamscheck-1);
				temp_typemask += "|o"sv;
			}
		}

		return sq_setparamscheck_detour(v, nparamscheck, temp_typemask.c_str());
	}
#endif

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

#ifdef __VMOD_USING_CUSTOM_VM
	static detour<decltype(ScriptCreateSquirrelVM)> ScriptCreateSquirrelVM_detour;
	static gsdk::IScriptVM *ScriptCreateSquirrelVM_detour_callback()
	{
		return new vm::squirrel;
	}

	static detour<decltype(ScriptDestroySquirrelVM)> ScriptDestroySquirrelVM_detour;
	static void ScriptDestroySquirrelVM_detour_callback(gsdk::IScriptVM *vm)
	{
		delete static_cast<vm::squirrel *>(vm);
	}
#endif

#if GSDK_ENGINE != GSDK_ENGINE_TF2
	static void RegisterScriptHookListener(std::string_view name) noexcept
	{
		//TODO!!!!!
	}
#endif

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

	#ifdef __VMOD_USING_CUSTOM_VM
		if(!ScriptCreateSquirrelVM) {
			error("vmod: missing 'ScriptCreateSquirrelVM' address\n");
			return false;
		}
		ScriptCreateSquirrelVM_detour.initialize(ScriptCreateSquirrelVM, ScriptCreateSquirrelVM_detour_callback);
		ScriptCreateSquirrelVM_detour.enable();

		if(!ScriptDestroySquirrelVM) {
			error("vmod: missing 'ScriptDestroySquirrelVM' address\n");
			return false;
		}
		ScriptDestroySquirrelVM_detour.initialize(ScriptDestroySquirrelVM, ScriptDestroySquirrelVM_detour_callback);
		ScriptDestroySquirrelVM_detour.enable();
	#endif

		return true;
	}

	bool main::detours() noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(RegisterFunctionGuts) {
			RegisterFunctionGuts_detour.initialize(RegisterFunctionGuts, RegisterFunctionGuts_detour_callback);
			RegisterFunctionGuts_detour.enable();
		}

		if(sq_setparamscheck) {
			sq_setparamscheck_detour.initialize(sq_setparamscheck, sq_setparamscheck_detour_callback);
			sq_setparamscheck_detour.enable();
		}
	#endif

		if(PrintFunc) {
		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
		#endif
			PrintFunc_detour.initialize(PrintFunc, PrintFunc_detour_callback);
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif
			PrintFunc_detour.enable();
		}

		if(ErrorFunc) {
		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
		#endif
			ErrorFunc_detour.initialize(ErrorFunc, ErrorFunc_detour_callback);
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif
			ErrorFunc_detour.enable();
		}

		if(!VScriptServerInit) {
			error("vmod: missing VScriptServerInit address\n");
			return false;
		}
		VScriptServerInit_detour.initialize(VScriptServerInit, VScriptServerInit_detour_callback);
		VScriptServerInit_detour.enable();

		if(!VScriptServerTerm) {
			error("vmod: missing VScriptServerTerm address\n");
			return false;
		}
		VScriptServerTerm_detour.initialize(VScriptServerTerm, VScriptServerTerm_detour_callback);
		VScriptServerTerm_detour.enable();

		if(!VScriptServerRunScript) {
			error("vmod: missing VScriptServerRunScript address\n");
			return false;
		}
		VScriptServerRunScript_detour.initialize(VScriptServerRunScript, VScriptServerRunScript_detour_callback);
		VScriptServerRunScript_detour.enable();

	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(!VScriptServerRunScriptForAllAddons) {
			error("vmod: missing VScriptServerRunScriptForAllAddons address\n");
			return false;
		}
		VScriptServerRunScriptForAllAddons_detour.initialize(VScriptServerRunScriptForAllAddons, VScriptServerRunScriptForAllAddons_detour_callback);
		VScriptServerRunScriptForAllAddons_detour.enable();
	#endif

		generic_vtable_t vsmgr_vtable{vtable_from_object(vsmgr)};

		CreateVM_original = swap_vfunc(vsmgr_vtable, &gsdk::IScriptManager::CreateVM, CreateVM_detour_callback);
		DestroyVM_original = swap_vfunc(vsmgr_vtable, &gsdk::IScriptManager::DestroyVM, DestroyVM_detour_callback);

		generic_vtable_t vm_vtable{vtable_from_object(vm_)};

		CompileScript_original = swap_vfunc(vm_vtable, &gsdk::IScriptVM::CompileScript, CompileScript_detour_callback);
		Run_original = swap_vfunc(vm_vtable, static_cast<decltype(Run_original)>(&gsdk::IScriptVM::Run), Run_detour_callback);
		SetErrorCallback_original = swap_vfunc(vm_vtable, &gsdk::IScriptVM::SetErrorCallback, SetErrorCallback_detour_callback);

		RegisterFunction_detour.disable();
		RegisterClass_detour.disable();
		RegisterInstance_detour.disable();
		SetValue_var_detour.disable();
		SetValue_str_detour.disable();

		RegisterFunction_original = swap_vfunc(vm_vtable, &gsdk::IScriptVM::RegisterFunction, RegisterFunction_detour_callback);
		RegisterClass_original = swap_vfunc(vm_vtable, &gsdk::IScriptVM::RegisterClass, RegisterClass_detour_callback);
		RegisterInstance_original = swap_vfunc(vm_vtable, &gsdk::IScriptVM::RegisterInstance_impl, RegisterInstance_detour_callback);
		SetValue_var_original = swap_vfunc(vm_vtable, static_cast<decltype(SetValue_var_original)>(&gsdk::IScriptVM::SetValue_impl), SetValue_var_detour_callback);
		SetValue_str_original = swap_vfunc(vm_vtable, static_cast<decltype(SetValue_str_original)>(&gsdk::IScriptVM::SetValue), SetValue_str_detour_callback);

		generic_vtable_t gamedll_vtable{vtable_from_object(gamedll)};

		CreateNetworkStringTables_original = swap_vfunc(gamedll_vtable, &gsdk::IServerGameDLL::CreateNetworkStringTables, CreateNetworkStringTables_detour_callback);

		generic_vtable_t sv_stringtables_vtable{vtable_from_object(sv_stringtables)};

		RemoveAllTables_original = swap_vfunc(sv_stringtables_vtable, &gsdk::IServerNetworkStringTableContainer::RemoveAllTables, RemoveAllTables_detour_callback);

		if(!bindings::ent::detours()) {
			return false;
		}

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

		std::filesystem::path exe_filename;

		{
			char exe[PATH_MAX];
			ssize_t len{readlink("/proc/self/exe", exe, sizeof(exe))};
			exe[len] = '\0';

			exe_filename = exe;
		}

		{
			Dl_info info;
		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wconditionally-supported"
		#endif
			dladdr(reinterpret_cast<void *>(::CreateInterface), &info);
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif

			std::filesystem::path lib_path{info.dli_fname};

			std::filesystem::path bin_dir{lib_path.parent_path()};
			std::filesystem::path vmod_dir{bin_dir.parent_path()};
			std::filesystem::path addons_dir{vmod_dir.parent_path()};

			game_dir_ = addons_dir.parent_path();

			root_dir_ = vmod_dir;
		}

		if(!symbol_cache::initialize()) {
			std::cout << "\033[0;31m"sv << "vmod: failed to initialize symbol cache\n"sv << "\033[0m"sv;
			return false;
		}

		bin_folder = exe_filename.parent_path();
		bin_folder /= "bin"sv;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		bin_folder /= "linux32"sv;
	#endif

		exe_filename = exe_filename.filename();
		exe_filename.replace_extension();

	#if GSDK_ENGINE == GSDK_ENGINE_PORTAL2
		if(exe_filename != "portal2_linux"sv) {
			std::cout << "\033[0;31m"sv << "vmod: unsupported exe filename: '"sv << exe_filename << "'\n"sv << "\033[0m"sv;
			return false;
		}
	#else
		if(exe_filename != "hl2_linux"sv && exe_filename != "srcds_linux"sv) {
			std::cout << "\033[0;31m"sv << "vmod: unsupported exe filename: '"sv << exe_filename << "'\n"sv << "\033[0m"sv;
			return false;
		}
	#endif

		std::string_view launcher_lib_name;
		if(exe_filename == "srcds_linux"sv) {
			launcher_lib_name = "dedicated_srv.so"sv;
		} else {
			launcher_lib_name = "launcher.so"sv;
		}

		if(!launcher_lib.load(bin_folder/launcher_lib_name)) {
			std::cout << "\033[0;31m"sv << "vmod: failed to open launcher library: '"sv << launcher_lib.error_string() << "'\n"sv << "\033[0m"sv;
			return false;
		}

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		SpewActivate("*", 0);
		SpewActivate("console", 0);
		SpewActivate("developer", 0);
		old_spew = GetSpewOutputFunc();
		SpewOutputFunc(new_spew);
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		//TODO!!!
		//LoggingSystem
	#else
		#error
	#endif

		std::string_view engine_lib_name{"engine.so"sv};
		if(dedicated) {
			engine_lib_name = "engine_srv.so"sv;
		}
		if(!engine_lib.load(bin_folder/engine_lib_name)) {
			error("vmod: failed to open engine library: '%s'\n"sv, engine_lib.error_string().c_str());
			return false;
		}

		{
			char gamedir[PATH_MAX];
			sv_engine->GetGameDir(gamedir, sizeof(gamedir));

			game_dir_ = gamedir;
		}

		root_dir_ = game_dir_;
		root_dir_ /= "addons/vmod"sv;

		//TODO!!!
	#if 0
		{
			int appid{sv_engine->GetAppID()};
		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			if(appid != 440 && appid != 232250) {
				error("vmod: unsupported appid: %i for tf2\n"sv, appid);
				return false;
			}
		#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
			if(appid != 550 && appid != 222860) {
				error("vmod: unsupported appid: %i for l4d2\n"sv, appid);
				return false;
			}
		#else
			#error
		#endif
		}
	#endif

		{
			const auto &eng_symbols{engine_lib.symbols()};
			const auto &eng_global_qual{eng_symbols.global()};

			auto g_SendTables_it{eng_global_qual.find("g_SendTables"s)};
			if(g_SendTables_it == eng_global_qual.end()) {
				warning("vmod: missing 'g_SendTables' symbol\n"sv);
			}

			if(g_SendTables_it != eng_global_qual.end()) {
				g_SendTables = g_SendTables_it->second->addr<gsdk::CUtlVector<gsdk::SendTable *> *>();
			}
		}

		std::string_view vstdlib_lib_name{"libvstdlib.so"sv};
		if(sv_engine->IsDedicatedServer()) {
			vstdlib_lib_name = "libvstdlib_srv.so"sv;
		}
		if(!vstdlib_lib.load(bin_folder/vstdlib_lib_name)) {
			error("vmod: failed to open vstdlib library: '%s'\n"sv, vstdlib_lib.error_string().c_str());
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

		std::string_view vscript_lib_name{"vscript.so"sv};
		if(sv_engine->IsDedicatedServer()) {
			vscript_lib_name = "vscript_srv.so"sv;
		}
		if(!vscript_lib.load(bin_folder/vscript_lib_name)) {
			error("vmod: failed to open vscript library: '%s'\n"sv, vscript_lib.error_string().c_str());
			return false;
		}

		gsdk::ScriptLanguage_t script_language{gsdk::SL_SQUIRREL};

		std::filesystem::path base_scripts_dir{root_dir_};
		base_scripts_dir /= "base"sv;

		switch(script_language) {
			case gsdk::SL_NONE: break;
			case gsdk::SL_GAMEMONKEY: {
				scripts_extension = ".gm"sv;
				base_scripts_dir /= "gamemonkey"sv;
			} break;
			case gsdk::SL_SQUIRREL: {
				scripts_extension = ".nut"sv;
				base_scripts_dir /= "squirrel"sv;
			} break;
			case gsdk::SL_LUA: {
				scripts_extension = ".lua"sv;
				base_scripts_dir /= "lua"sv;
			} break;
			case gsdk::SL_PYTHON: {
				scripts_extension = ".py"sv;
				base_scripts_dir /= "python"sv;
			} break;
		#if defined __VMOD_USING_CUSTOM_VM
			#ifdef __VMOD_ENABLE_SOURCEPAWN
			case gsdk::SL_SOURCEPAWN: {
				scripts_extension = ".sp"sv;
				base_scripts_dir /= "sourcepawn"sv;
			} break;
			#endif
			#ifdef __VMOD_ENABLE_V8
			case gsdk::SL_V8: {
				scripts_extension = ".js"sv;
				base_scripts_dir /= "javascript"sv;
			} break;
			#endif
		#endif
		#ifndef __clang__
			default: break;
		#endif
		}

		{
			const auto &vscript_symbols{vscript_lib.symbols()};
			const auto &vscript_global_qual{vscript_symbols.global()};

			auto g_Script_init_it{vscript_global_qual.find("g_Script_init"s)};
			if(g_Script_init_it == vscript_global_qual.end()) {
			#ifndef __VMOD_SQUIRREL_NEED_INIT_SCRIPT
				warning("vmod: missing 'g_Script_init' symbol\n");
			#else
				error("vmod: missing 'g_Script_init' symbol\n");
				return false;
			#endif
			}

		#ifdef __VMOD_USING_CUSTOM_VM
			auto ScriptCreateSquirrelVM_it{vscript_global_qual.find("ScriptCreateSquirrelVM()"s)};
			if(ScriptCreateSquirrelVM_it == vscript_global_qual.end()) {
				error("vmod: missing 'ScriptCreateSquirrelVM()' symbol\n");
				return false;
			}

			auto ScriptDestroySquirrelVM_it{vscript_global_qual.find("ScriptDestroySquirrelVM(IScriptVM*)"s)};
			if(ScriptDestroySquirrelVM_it == vscript_global_qual.end()) {
				error("vmod: missing 'ScriptDestroySquirrelVM(IScriptVM*)' symbol\n");
				return false;
			}
		#endif

			auto sq_getversion_it{vscript_global_qual.find("sq_getversion"s)};
			if(sq_getversion_it == vscript_global_qual.end()) {
				warning("vmod: missing 'sq_getversion' symbol\n"sv);
			}

		#ifndef __VMOD_USING_CUSTOM_VM
			auto sq_setparamscheck_it{vscript_global_qual.find("sq_setparamscheck"s)};
			if(sq_setparamscheck_it == vscript_global_qual.end()) {
				warning("vmod: missing 'sq_setparamscheck' symbol\n"sv);
			}
		#endif

		#ifndef __VMOD_USING_CUSTOM_VM
			auto CSquirrelVM_it{vscript_symbols.find("CSquirrelVM"s)};
			if(CSquirrelVM_it == vscript_symbols.end()) {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2
				error("vmod: missing 'CSquirrelVM' symbols\n"sv);
				return false;
			#else
				warning("vmod: missing 'CSquirrelVM' symbols\n"sv);
			#endif
			}

			if(CSquirrelVM_it != vscript_symbols.end()) {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2
				auto squirrel_CreateArray_it{CSquirrelVM_it->second->find("CreateArray(CVariantBase<CVariantDefaultAllocator>&)"s)};
				if(squirrel_CreateArray_it == CSquirrelVM_it->second->end()) {
					error("vmod: missing 'CSquirrelVM::CreateArray(CVariantBase<CVariantDefaultAllocator>&)' symbol\n"sv);
					return false;
				}

				auto squirrel_GetArrayCount_it{CSquirrelVM_it->second->find("GetArrayCount(HSCRIPT__*)"s)};
				if(squirrel_GetArrayCount_it == CSquirrelVM_it->second->end()) {
					error("vmod: missing 'CSquirrelVM::GetArrayCount(HSCRIPT__*)' symbol\n"sv);
					return false;
				}

				auto squirrel_IsArray_it{CSquirrelVM_it->second->find("IsArray(HSCRIPT__*)"s)};
				if(squirrel_IsArray_it == CSquirrelVM_it->second->end()) {
					error("vmod: missing 'CSquirrelVM::IsArray(HSCRIPT__*)' symbol\n"sv);
					return false;
				}

				auto squirrel_IsTable_it{CSquirrelVM_it->second->find("IsTable(HSCRIPT__*)"s)};
				if(squirrel_IsTable_it == CSquirrelVM_it->second->end()) {
					error("vmod: missing 'CSquirrelVM::IsTable(HSCRIPT__*)' symbol\n"sv);
					return false;
				}
			#endif

				auto PrintFunc_it{CSquirrelVM_it->second->find("PrintFunc(SQVM*, char const*, ...)"s)};
				if(PrintFunc_it == CSquirrelVM_it->second->end()) {
					warning("vmod: missing 'CSquirrelVM::PrintFunc(SQVM*, char const*, ...)' symbol\n"sv);
				}

				auto ErrorFunc_it{CSquirrelVM_it->second->find("ErrorFunc(SQVM*, char const*, ...)"s)};
				if(ErrorFunc_it == CSquirrelVM_it->second->end()) {
					warning("vmod: missing 'CSquirrelVM::ErrorFunc(SQVM*, char const*, ...)' symbol\n"sv);
				}

				auto RegisterFunctionGuts_it{CSquirrelVM_it->second->find("RegisterFunctionGuts(ScriptFunctionBinding_t*, ScriptClassDesc_t*)"s)};
				if(RegisterFunctionGuts_it == CSquirrelVM_it->second->end()) {
					warning("vmod: missing 'CSquirrelVM::RegisterFunctionGuts(ScriptFunctionBinding_t*, ScriptClassDesc_t*)' symbol\n"sv);
				}

				auto RegisterFunction_it{CSquirrelVM_it->second->find("RegisterFunction(ScriptFunctionBinding_t*)"s)};
				if(RegisterFunction_it == CSquirrelVM_it->second->end()) {
					warning("vmod: missing 'CSquirrelVM::RegisterFunction(ScriptFunctionBinding_t*)' symbol\n"sv);
				}

				auto RegisterClass_it{CSquirrelVM_it->second->find("RegisterClass(ScriptClassDesc_t*)"s)};
				if(RegisterClass_it == CSquirrelVM_it->second->end()) {
					warning("vmod: missing 'CSquirrelVM::RegisterClass(ScriptClassDesc_t*)' symbol\n"sv);
				}

				auto RegisterInstance_it{CSquirrelVM_it->second->find("RegisterInstance(ScriptClassDesc_t*, void*)"s)};
				if(RegisterInstance_it == CSquirrelVM_it->second->end()) {
					warning("vmod: missing 'CSquirrelVM::RegisterInstance(ScriptClassDesc_t*, void*)' symbol\n"sv);
				}

				auto SetValue_str_it{CSquirrelVM_it->second->find("SetValue(HSCRIPT__*, char const*, char const*)"s)};
				if(SetValue_str_it == CSquirrelVM_it->second->end()) {
					warning("vmod: missing 'CSquirrelVM::SetValue(HSCRIPT__*, char const*, char const*)' symbol\n"sv);
				}

				auto SetValue_var_it{CSquirrelVM_it->second->find("SetValue(HSCRIPT__*, char const*, CVariantBase<CVariantDefaultAllocator> const&)"s)};
				if(SetValue_var_it == CSquirrelVM_it->second->end()) {
					warning("vmod: missing 'CSquirrelVM::SetValue(HSCRIPT__*, char const*, CVariantBase<CVariantDefaultAllocator> const&)' symbol\n"sv);
				}

				if(RegisterFunction_it != CSquirrelVM_it->second->end()) {
					RegisterFunction = RegisterFunction_it->second->mfp<decltype(RegisterFunction)>();
				}

				if(RegisterClass_it != CSquirrelVM_it->second->end()) {
					RegisterClass = RegisterClass_it->second->mfp<decltype(RegisterClass)>();
				}

				if(RegisterInstance_it != CSquirrelVM_it->second->end()) {
					RegisterInstance = RegisterInstance_it->second->mfp<decltype(RegisterInstance)>();
				}

				if(SetValue_str_it != CSquirrelVM_it->second->end()) {
					SetValue_str = SetValue_str_it->second->mfp<decltype(SetValue_str)>();
				}

				if(SetValue_var_it != CSquirrelVM_it->second->end()) {
					SetValue_var = SetValue_var_it->second->mfp<decltype(SetValue_var)>();
				}

				if(PrintFunc_it != CSquirrelVM_it->second->end()) {
					PrintFunc = PrintFunc_it->second->func<decltype(PrintFunc)>();
				}

				if(ErrorFunc_it != CSquirrelVM_it->second->end()) {
					ErrorFunc = ErrorFunc_it->second->func<decltype(ErrorFunc)>();
				}

				if(RegisterFunctionGuts_it != CSquirrelVM_it->second->end()) {
					RegisterFunctionGuts = RegisterFunctionGuts_it->second->mfp<decltype(RegisterFunctionGuts)>();
				}

			#if GSDK_ENGINE == GSDK_ENGINE_TF2
				gsdk::IScriptVM::squirrel_CreateArray_ptr = squirrel_CreateArray_it->second->mfp<decltype(gsdk::IScriptVM::squirrel_CreateArray_ptr)>();
				gsdk::IScriptVM::squirrel_GetArrayCount_ptr = squirrel_GetArrayCount_it->second->mfp<decltype(gsdk::IScriptVM::squirrel_GetArrayCount_ptr)>();
				gsdk::IScriptVM::squirrel_IsArray_ptr = squirrel_IsArray_it->second->mfp<decltype(gsdk::IScriptVM::squirrel_IsArray_ptr)>();
				gsdk::IScriptVM::squirrel_IsTable_ptr = squirrel_IsTable_it->second->mfp<decltype(gsdk::IScriptVM::squirrel_IsTable_ptr)>();
			#endif
			}
		#else
			RegisterFunction = reinterpret_cast<decltype(RegisterFunction)>(&vm::squirrel::RegisterFunction_nonvirtual);
			RegisterClass = reinterpret_cast<decltype(RegisterClass)>(&vm::squirrel::RegisterClass_nonvirtual);
			RegisterInstance = reinterpret_cast<decltype(RegisterInstance)>(&vm::squirrel::RegisterInstance_impl_nonvirtual);
			SetValue_str = reinterpret_cast<decltype(SetValue_str)>(static_cast<bool(vm::squirrel::*)(gsdk::HSCRIPT, const char *, const char *)>(&vm::squirrel::SetValue_nonvirtual));
			SetValue_var = reinterpret_cast<decltype(SetValue_var)>(static_cast<bool(vm::squirrel::*)(gsdk::HSCRIPT, const char *, const gsdk::ScriptVariant_t &)>(&vm::squirrel::SetValue_impl_nonvirtual));
			PrintFunc = vm::squirrel::print_func;
			ErrorFunc = vm::squirrel::error_func;

			#if GSDK_ENGINE == GSDK_ENGINE_TF2
			gsdk::IScriptVM::squirrel_CreateArray_ptr = reinterpret_cast<decltype(gsdk::IScriptVM::squirrel_CreateArray_ptr)>(&vm::squirrel::CreateArray_impl_nonvirtual);
			gsdk::IScriptVM::squirrel_GetArrayCount_ptr = reinterpret_cast<decltype(gsdk::IScriptVM::squirrel_GetArrayCount_ptr)>(&vm::squirrel::GetArrayCount_nonvirtual);
			gsdk::IScriptVM::squirrel_IsArray_ptr = reinterpret_cast<decltype(gsdk::IScriptVM::squirrel_IsArray_ptr)>(&vm::squirrel::IsArray_nonvirtual);
			gsdk::IScriptVM::squirrel_IsTable_ptr = reinterpret_cast<decltype(gsdk::IScriptVM::squirrel_IsTable_ptr)>(&vm::squirrel::IsTable_nonvirtual);
			#endif
		#endif

			if(sq_getversion_it != vscript_global_qual.end()) {
				game_sq_ver = sq_getversion_it->second->func<decltype(::sq_getversion)>()();
			}

		#ifndef __VMOD_USING_CUSTOM_VM
			if(sq_setparamscheck_it != vscript_global_qual.end()) {
				sq_setparamscheck = sq_setparamscheck_it->second->func<decltype(sq_setparamscheck)>();
			}
		#endif

		#ifndef __VMOD_SQUIRREL_NEED_INIT_SCRIPT
			if(g_Script_init_it != vscript_global_qual.end())
		#endif
			{
				g_Script_init = g_Script_init_it->second->addr<const unsigned char *>();
			}

		#ifdef __VMOD_USING_CUSTOM_VM
			ScriptCreateSquirrelVM = ScriptCreateSquirrelVM_it->second->func<decltype(ScriptCreateSquirrelVM)>();
			ScriptDestroySquirrelVM = ScriptDestroySquirrelVM_it->second->func<decltype(ScriptDestroySquirrelVM)>();
		#endif
		}

		std::filesystem::path server_lib_name{game_dir_};
		server_lib_name /= "bin"sv;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		server_lib_name /= "linux32"sv;
	#endif
		if(sv_engine->IsDedicatedServer()) {
			server_lib_name /= "server_srv.so"sv;
		} else {
			server_lib_name /= "server.so"sv;
		}
		if(!server_lib.load(server_lib_name)) {
			error("vmod: failed to open server library: '%s'\n"sv, server_lib.error_string().c_str());
			return false;
		}

		{
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
				error("vmod: missing 'VScriptServerInit()' symbol\n"sv);
				return false;
			}

			auto VScriptServerTerm_it{sv_global_qual.find("VScriptServerTerm()"s)};
			if(VScriptServerTerm_it == sv_global_qual.end()) {
				error("vmod: missing 'VScriptServerTerm()' symbol\n"sv);
				return false;
			}

		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
			auto VScriptServerRunScript_it{sv_global_qual.find("VScriptRunScript(char const*, HSCRIPT__*, bool)"s)};
			if(VScriptServerRunScript_it == sv_global_qual.end()) {
				error("vmod: missing 'VScriptRunScript(char const*, HSCRIPT__*, bool)' symbol\n"sv);
				return false;
			}
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, <=, GSDK_ENGINE_BRANCH_2010_V0)
			auto VScriptServerRunScript_it{sv_global_qual.find("VScriptServerRunScript(char const*, HSCRIPT__*, bool)"s)};
			if(VScriptServerRunScript_it == sv_global_qual.end()) {
				error("vmod: missing 'VScriptServerRunScript(char const*, HSCRIPT__*, bool)' symbol\n"sv);
				return false;
			}
		#else
			#error
		#endif

		#if GSDK_ENGINE == GSDK_ENGINE_L4D2
			auto VScriptServerRunScriptForAllAddons_it{sv_global_qual.find("VScriptServerRunScriptForAllAddons(char const*, HSCRIPT__*, bool)"s)};
			if(VScriptServerRunScriptForAllAddons_it == sv_global_qual.end()) {
				error("vmod: missing 'VScriptServerRunScriptForAllAddons(char const*, HSCRIPT__*, bool)' symbol\n"sv);
				return false;
			}
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
		#elif GSDK_ENGINE == GSDK_ENGINE_PORTAL2
			auto CPortalGameRules_it{sv_symbols.find("CPortalGameRules"s)};
			if(CPortalGameRules_it == sv_symbols.end()) {
				error("vmod: missing 'CPortalGameRules' symbols\n"sv);
				return false;
			}

			auto RegisterScriptFunctionsSP_it{CPortalGameRules_it->second->find("RegisterScriptFunctions()"s)};
			if(RegisterScriptFunctionsSP_it == CPortalGameRules_it->second->end()) {
				error("vmod: missing 'CPortalGameRules::RegisterScriptFunctions()' symbol\n"sv);
				return false;
			}

			auto CPortalMPGameRules_it{sv_symbols.find("CPortalMPGameRules"s)};
			if(CPortalMPGameRules_it == sv_symbols.end()) {
				error("vmod: missing 'CPortalMPGameRules' symbols\n"sv);
				return false;
			}

			auto RegisterScriptFunctionsMP_it{CPortalMPGameRules_it->second->find("RegisterScriptFunctions()"s)};
			if(RegisterScriptFunctionsMP_it == CPortalMPGameRules_it->second->end()) {
				error("vmod: missing 'CPortalMPGameRules::RegisterScriptFunctions()' symbol\n"sv);
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

			auto UpdateOnRemove_it{CBaseEntity_it->second->find("UpdateOnRemove()"s)};
			if(UpdateOnRemove_it == CBaseEntity_it->second->end()) {
				error("vmod: missing 'CBaseEntity::UpdateOnRemove' symbol\n"sv);
				return false;
			}

		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
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

			std::size_t net_vtable_size{sv_symbols.vtable_size("CServerNetworkProperty"s)};
			if(net_vtable_size == static_cast<std::size_t>(-1)) {
				error("vmod: missing 'CServerNetworkProperty' vtable symbol\n"sv);
				return false;
			}

			for(const auto &it : sv_classes) {
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

					std::string dataname{map->dataClassName};
					sv_datamaps.emplace(std::move(dataname), map);
				}
			}

		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, <=, GSDK_ENGINE_BRANCH_2010_V0)
			for(const auto &it : sv_global_qual) {
				if(it.first.starts_with("ScriptClassDesc_t* GetScriptDesc<"sv)) {
					gsdk::ScriptClassDesc_t *tmp_desc{it.second->func<gsdk::ScriptClassDesc_t *(*)(generic_object_t *)>()(nullptr)};
					std::string descname{tmp_desc->m_pszClassname};
					sv_script_class_descs.emplace(std::move(descname), tmp_desc);
				}
			}
		#endif

			auto CLogicScript_it{sv_symbols.find("CLogicScript"s)};
			if(CLogicScript_it == sv_symbols.end()) {
				warning("vmod: missing 'CLogicScript' symbols\n"sv);
			} else {
				auto RunVScripts_it{CLogicScript_it->second->find("RunVScripts()"s)};
				if(RunVScripts_it == CLogicScript_it->second->end()) {
					warning("vmod: missing 'CLogicScript::RunVScripts()' symbol\n"sv);
				} else {
					auto szAddCode_it{RunVScripts_it->second->find("szAddCode"s)};
					if(szAddCode_it != RunVScripts_it->second->end()) {
						szAddCode = szAddCode_it->second->addr<const unsigned char *>();
					} else {
						warning("vmod: missing 'CLogicScript::RunVScripts()::szAddCode' symbol\n"sv);
					}
				}
			}

		#define __VMOD_GET_SV_SENDPROXY_FUNC(name) \
			auto name##_it{sv_global_qual.find(#name"(SendProp const*, void const*, void const*, DVariant*, int, int)"s)}; \
			if(name##_it == sv_global_qual.end()) { \
				warning("vmod: missing '%s' symbol\n"sv, #name); \
			} else { \
				gsdk::name = name##_it->second->func<gsdk::SendVarProxyFn>(); \
			}

			__VMOD_GET_SV_SENDPROXY_FUNC(SendProxy_StringT_To_String)
			__VMOD_GET_SV_SENDPROXY_FUNC(SendProxy_Color32ToInt)
			__VMOD_GET_SV_SENDPROXY_FUNC(SendProxy_EHandleToInt)
			__VMOD_GET_SV_SENDPROXY_FUNC(SendProxy_IntAddOne)
			__VMOD_GET_SV_SENDPROXY_FUNC(SendProxy_ShortAddOne)
			__VMOD_GET_SV_SENDPROXY_FUNC(SendProxy_OnlyToTeam)
			__VMOD_GET_SV_SENDPROXY_FUNC(SendProxy_PredictableIdToInt)

			auto g_Script_spawn_helper_it{sv_global_qual.find("g_Script_spawn_helper"s)};
			if(g_Script_spawn_helper_it == sv_global_qual.end()) {
				warning("vmod: missing 'g_Script_spawn_helper' symbol\n");
			}

			if(g_Script_spawn_helper_it != sv_global_qual.end()) {
				g_Script_spawn_helper = g_Script_spawn_helper_it->second->addr<const unsigned char *>();
			}

		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			RegisterScriptFunctions = RegisterScriptFunctions_it->second->mfp<decltype(RegisterScriptFunctions)>();
		#elif GSDK_ENGINE == GSDK_ENGINE_PORTAL2
			RegisterScriptFunctionsSP = RegisterScriptFunctionsSP_it->second->mfp<decltype(RegisterScriptFunctionsSP)>();
			RegisterScriptFunctionsMP = RegisterScriptFunctionsMP_it->second->mfp<decltype(RegisterScriptFunctionsMP)>();
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

		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
			sv_classdesc_pHead = sv_pHead_it->second->addr<gsdk::ScriptClassDesc_t **>();
		#endif

			gsdk::IServerNetworkable::vtable_size = net_vtable_size;

			gsdk::CBaseEntity::UpdateOnRemove_vindex = UpdateOnRemove_it->second->virtual_index();
			gsdk::CBaseEntity::GetScriptInstance_ptr = GetScriptInstance_it->second->mfp<decltype(gsdk::CBaseEntity::GetScriptInstance_ptr)>();
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		if(!sv_classdesc_pHead) {
			error("vmod: missing 'ScriptClassDesc_t::GetDescList()::pHead' address\n");
			return false;
		}
		gsdk::ScriptClassDesc_t *tmp_desc{*sv_classdesc_pHead};
		while(tmp_desc) {
			std::string descname{tmp_desc->m_pszClassname};
			sv_script_class_descs.emplace(std::move(descname), tmp_desc);
			tmp_desc = tmp_desc->m_pNextDesc;
		}
	#endif

		if(!detours_prevm()) {
			return false;
		}

		plugins_dir_ = root_dir_;
		plugins_dir_ /= "plugins"sv;

		cvar_dll_id_ = cvar->AllocateDLLIdentifier();

	#ifdef __VMOD_USING_PREPROCESSOR
		if(!pp.initialize()) {
			return false;
		}
	#endif

		vm_ = reinterpret_cast<IScriptVM *>(vsmgr->CreateVM(script_language));
		if(!vm_) {
			error("vmod: failed to create VM\n"sv);
			return false;
		}

		{
		#if !defined __VMOD_USING_QUIRREL && (defined SQUIRREL_VERSION_NUMBER && SQUIRREL_VERSION_NUMBER >= 303)
			curr_sq_ver = ::sq_getversion();
		#endif

		#ifndef __VMOD_USING_CUSTOM_VM
			if(curr_sq_ver != -1) {
				if(curr_sq_ver != SQUIRREL_VERSION_NUMBER) {
					warning("vmod: mismatched squirrel header '%i' vs '%i'\n"sv, curr_sq_ver, SQUIRREL_VERSION_NUMBER);
				}
			}

			if(game_sq_ver != -1 && curr_sq_ver != -1) {
				if(game_sq_ver != curr_sq_ver) {
					warning("vmod: mismatched squirrel versions '%i' vs '%i'\n"sv, game_sq_ver, curr_sq_ver);
				}
			}

			if(!vm_->GetValue(nullptr, "_versionnumber_", &game_sq_versionnumber)) {
				warning("vmod: failed to get _versionnumber_ value\n"sv);
			} else {
				SQInteger _versionnumber_{game_sq_versionnumber.get<SQInteger>()};
				if(curr_sq_ver != -1 && _versionnumber_ != curr_sq_ver) {
					warning("vmod: mismatched squirrel versions '%i' vs '%i'\n"sv, _versionnumber_, curr_sq_ver);
				}
			}

			if(!vm_->GetValue(nullptr, "_version_", &game_sq_version)) {
				warning("vmod: failed to get _version_ value\n"sv);
			} else {
				std::string_view _version_{game_sq_version.get<std::string_view>()};
				if(_version_ != SQUIRREL_VERSION) {
					warning("vmod: mismatched squirrel versions '%s' vs '%s'\n"sv, _version_.data(), SQUIRREL_VERSION);
				}
			}
		#endif
		}

		if(entityfactorydict) {
			for(const auto &it : entityfactorydict->m_Factories) {
				sv_ent_factories.emplace(it.first, it.second);
			}
		}

		for(const auto &it : sv_classes) {
			auto info_it{sv_ent_class_info.find(it.first)};
			if(info_it == sv_ent_class_info.end()) {
				info_it = sv_ent_class_info.emplace(it.first, entity_class_info{}).first;
			}

			info_it->second.sv_class = it.second;
			info_it->second.sendtable = it.second->m_pTable;

			std::string tablename{it.second->m_pTable->m_pNetTableName};
			sv_sendtables.emplace(std::move(tablename), it.second->m_pTable);

			auto script_desc_it{sv_script_class_descs.find(it.first)};
			if(script_desc_it != sv_script_class_descs.end()) {
				info_it->second.script_desc = script_desc_it->second;
			}

			auto datamap_it{sv_datamaps.find(it.first)};
			if(datamap_it != sv_datamaps.end()) {
				info_it->second.datamap = datamap_it->second;
			}
		}

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

		std::filesystem::path base_script_path{base_scripts_dir};
		base_script_path /= "vmod_base"sv;
		base_script_path.replace_extension(scripts_extension);

		bool base_script_from_file{false};

		if(!compile_internal_script(vm_, base_script_path,
	#ifdef __VMOD_SQUIRREL_BASE_SCRIPT_HEADER_INCLUDED
		(script_language == gsdk::SL_SQUIRREL) ? reinterpret_cast<const unsigned char *>(__squirrel_vmod_base_script.c_str()) : nullptr
	#else
		nullptr
	#endif
		, base_script, base_script_from_file)) {
			return false;
		}

		base_script_scope = vm_->CreateScope("__vmod_base_script_scope__", nullptr);
		if(!base_script_scope || base_script_scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create base script scope\n"sv);
			return false;
		}

		if(vm_->Run(base_script, base_script_scope, true) == gsdk::SCRIPT_ERROR) {
			if(base_script_from_file) {
				error("vmod: failed to run base script from file '%s'\n"sv, base_script_path.c_str());
			} else {
				error("vmod: failed to run embedded base script\n"sv);
			}
			return false;
		}

	#ifdef __VMOD_SQUIRREL_OVERRIDE_SERVER_INIT_SCRIPT
		std::filesystem::path server_init_script_path{base_scripts_dir};
		server_init_script_path /= "vmod_server_init"sv;
		server_init_script_path.replace_extension(scripts_extension);

		bool server_init_script_from_file{false};

		if(!compile_internal_script(vm_, server_init_script_path,
		#ifdef __VMOD_SQUIRREL_SERVER_SCRIPT_HEADER_INCLUDED
		(script_language == gsdk::SL_SQUIRREL) ? reinterpret_cast<const unsigned char *>(__squirrel_vmod_server_init_script.c_str()) : nullptr
		#else
		nullptr
		#endif
		, server_init_script, server_init_script_from_file)) {
			return false;
		}
	#else
		if(script_language == gsdk::SL_SQUIRREL) {
			server_init_script = vm_->CompileScript(reinterpret_cast<const char *>(g_Script_vscript_server), "vscript_server.nut");
			if(!server_init_script || server_init_script == gsdk::INVALID_HSCRIPT) {
				error("vmod: failed to compile server init script\n"sv);
				return false;
			}
		} else {
			error("vmod: server init script not supported on this language\n"sv);
			return false;
		}
	#endif

		if(vm_->Run(server_init_script, nullptr, true) == gsdk::SCRIPT_ERROR) {
			error("vmod: failed to run server init script\n"sv);
			return false;
		}

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, <=, GSDK_ENGINE_BRANCH_2010_V0)
		for(const auto &it : sv_script_class_descs) {
			if(!RegisterClass_detour_callback(vm_, it.second)) {
				error("vmod: failed to register '%s' script class\n"sv, it.first.c_str());
				return false;
			}
		}
	#endif

	#if GSDK_ENGINE != GSDK_ENGINE_TF2
		register_scripthook_listener_desc.initialize(RegisterScriptHookListener, "RegisterScriptHookListener"sv, "RegisterScriptHookListener"sv);

		vm_->RegisterFunction(&register_scripthook_listener_desc);
	#endif

		if(!VScriptServerInit_detour_callback()) {
			error("vmod: VScriptServerInit failed\n"sv);
			return false;
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		if(!RegisterScriptFunctions) {
			error("vmod: missing 'CTFGameRules::RegisterScriptFunctions' address\n");
			return false;
		}
		(reinterpret_cast<gsdk::CTFGameRules *>(uninitialized_memory)->*RegisterScriptFunctions)();
	#elif GSDK_ENGINE == GSDK_ENGINE_PORTAL2
		if(!RegisterScriptFunctionsSP) {
			error("vmod: missing 'CPortalGameRules::RegisterScriptFunctions' address\n");
			return false;
		}
		(reinterpret_cast<gsdk::CPortalGameRules *>(uninitialized_memory)->*RegisterScriptFunctionsSP)();
		if(!RegisterScriptFunctionsMP) {
			error("vmod: missing 'CPortalMPGameRules::RegisterScriptFunctions' address\n");
			return false;
		}
		(reinterpret_cast<gsdk::CPortalMPGameRules *>(uninitialized_memory)->*RegisterScriptFunctionsMP)();
	#endif

		vscript_server_init_called = true;

		auto get_func_from_base_script{
			[this,base_script_from_file,&base_script_path = std::as_const(base_script_path)]
			(gsdk::HSCRIPT &func, std::string_view funcname) noexcept -> bool {
				func = vm_->LookupFunction(funcname.data(), base_script_scope);
				if(!func || func == gsdk::INVALID_HSCRIPT) {
					if(base_script_from_file) {
						error("vmod: base script '%s' missing '%s' function\n"sv, base_script_path.c_str(), funcname.data());
					} else {
						error("vmod: base script missing '%s' function\n"sv, funcname.data());
					}
					return false;
				}
				return true;
			}
		};

		if(!get_func_from_base_script(to_string_func, "__vmod_to_string__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(to_int_func, "__vmod_to_int__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(to_float_func, "__vmod_to_float__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(to_bool_func, "__vmod_to_bool__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(typeof_func, "__vmod_typeof__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(funcisg_func, "__vmod_get_func_sig__"sv)) {
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

				if(filesystem) {
					for(const std::filesystem::path &it : added_paths) {
						filesystem->RemoveSearchPath(it.c_str(), "GAME");
						filesystem->RemoveSearchPath(it.c_str(), "mod");
					}
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

		vmod_auto_dump_squirrel_ver.initialize("vmod_auto_dump_squirrel_ver"sv, false);

		vmod_dump_squirrel_ver.initialize("vmod_dump_squirrel_ver"sv,
			[](const gsdk::CCommand &) noexcept -> void {
				info("vmod: squirrel version:\n"sv);
				info("vmod:   vmod:\n"sv);
			#ifndef __VMOD_USING_QUIRREL
				info("vmod:    SQUIRREL_VERSION: %s\n"sv, SQUIRREL_VERSION);
				info("vmod:    SQUIRREL_VERSION_NUMBER: %i\n"sv, SQUIRREL_VERSION_NUMBER);
				if(curr_sq_ver != -1) {
					info("vmod:    sq_getversion: %i\n"sv, curr_sq_ver);
				}
			#else
				info("vmod:    SQUIRREL_VERSION: %s\n"sv, SQUIRREL_VERSION);
				info("vmod:    SQUIRREL_VERSION_NUMBER: (%i, %i, %i)\n"sv, SQUIRREL_VERSION_NUMBER_MAJOR, SQUIRREL_VERSION_NUMBER_MINOR, SQUIRREL_VERSION_NUMBER_PATCH);
			#endif
				info("vmod:   game:\n"sv);
				if(game_sq_version.valid()) {
					std::string_view _version{game_sq_version.get<std::string_view>()};
					info("vmod:    _version_: %s\n"sv, _version.data());
				}
				if(game_sq_versionnumber.valid()) {
					info("vmod:    _versionnumber_: %i\n"sv, game_sq_versionnumber.get<int>());
				}
				if(game_sq_ver != -1) {
					info("vmod:    sq_getversion: %i\n"sv, game_sq_ver);
				}
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

				std::error_code ec;
				std::filesystem::create_directories(internal_docs, ec);

				bindings::docs::write(internal_docs, internal_vscript_class_bindings, false);
				bindings::docs::write(internal_docs, internal_vscript_func_bindings, false);
				bindings::docs::write(internal_docs, internal_vscript_values);

				std::filesystem::path game_docs{root_dir_/"docs"sv/"game"sv};

				std::filesystem::create_directories(game_docs, ec);

				bindings::docs::write(game_docs, game_vscript_class_bindings, false);
				bindings::docs::write(game_docs, game_vscript_func_bindings, false);
				bindings::docs::write(game_docs, game_vscript_values);

				gsdk::ScriptVariant_t const_table;
				if(vm_->GetValue(nullptr, "Constants", &const_table)) {
					std::string file;

					bindings::docs::gen_date(file);

					int num{vm_->GetNumTableEntries(const_table.m_object)};
					for(int i{0}, it{0}; it != -1 && i < num; ++i) {
						vscript::variant key;
						vscript::variant value;
						it = vm_->GetKeyValue(const_table.m_object, it, &key, &value);

						std::string_view enum_name{key.get<std::string_view>()};

						if(enum_name[0] == 'E' || enum_name[0] == 'F') {
							file += "enum class "sv;
						} else {
							file += "namespace "sv;
						}
						file += enum_name;
						file += "\n{\n"sv;

						gsdk::HSCRIPT enum_table{value.get<gsdk::HSCRIPT>()};
						if(enum_table && enum_table != gsdk::INVALID_HSCRIPT) {
							bindings::docs::write(file, 1, enum_table, enum_name[0] == 'F' ? bindings::docs::write_enum_how::flags : bindings::docs::write_enum_how::normal);
						}

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

				std::filesystem::create_directories(vmod_docs, ec);

				write_docs(vmod_docs);
			}
		);

		vmod_auto_dump_internal_scripts.initialize("vmod_auto_dump_internal_scripts"sv, false);

		vmod_dump_internal_scripts.initialize("vmod_dump_internal_scripts"sv,
			[this](const gsdk::CCommand &) noexcept -> void {
				std::filesystem::path dump_dir{root_dir_/"dumps"sv/"internal_scripts"sv};

				std::error_code ec;
				std::filesystem::create_directories(dump_dir, ec);

			#ifndef __VMOD_SQUIRREL_NEED_INIT_SCRIPT
				if(g_Script_init)
			#endif
				{
					write_file(dump_dir/"init.nut"sv, g_Script_init, std::strlen(reinterpret_cast<const char *>(g_Script_init)+1));
				}

				if(szAddCode) {
					write_file(dump_dir/"szAddCode.nut"sv, szAddCode, std::strlen(reinterpret_cast<const char *>(szAddCode)+1));
				}

				if(g_Script_spawn_helper) {
					write_file(dump_dir/"spawn_helper.nut"sv, g_Script_spawn_helper, std::strlen(reinterpret_cast<const char *>(g_Script_spawn_helper)+1));
				}

				write_file(dump_dir/"vscript_server.nut"sv, g_Script_vscript_server, std::strlen(reinterpret_cast<const char *>(g_Script_vscript_server)+1));
			}
		);

		vmod_auto_dump_netprops.initialize("vmod_auto_dump_netprops"sv, false);

		vmod_dump_netprops.initialize("vmod_dump_netprops"sv,
			[this](const gsdk::CCommand &) noexcept -> void {
				std::filesystem::path dump_dir{root_dir_/"dumps"sv/"netprops"sv};

				std::error_code ec;
				std::filesystem::create_directories(dump_dir, ec);

				for(const auto &it : sv_sendtables) {
					std::string file;

					bindings::docs::gen_date(file);

					std::size_t num_props{static_cast<std::size_t>(it.second->m_nProps)};
					for(std::size_t i{0}; i < num_props; ++i) {
						gsdk::SendProp &prop{it.second->m_pProps[i]};
						file += prop.m_pVarName;
						file += '\n';
					}

					std::filesystem::path dump_path{dump_dir};
					dump_path /= it.first;
					dump_path.replace_extension(".txt"sv);

					write_file(dump_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
				}
			}
		);

		vmod_auto_dump_datamaps.initialize("vmod_auto_dump_datamaps"sv, false);

		auto dump_datamaps_impl{
			[this](bool only_keyvalues) noexcept -> void {
				std::filesystem::path dump_dir{root_dir_/"dumps"sv};

				if(only_keyvalues) {
					dump_dir /= "keyvalues"sv;
				} else {
					dump_dir /= "datamaps"sv;
				}

				std::error_code ec;
				std::filesystem::create_directories(dump_dir, ec);

				for(const auto &it : sv_datamaps) {
					std::string file;

					bindings::docs::gen_date(file);

					std::function<void(gsdk::datamap_t &, std::string_view, std::size_t)> write_table{
						[&file,&write_table,only_keyvalues](gsdk::datamap_t &targettable, std::string_view tablename, std::size_t depth) noexcept -> void {
							bindings::docs::ident(file, depth);
							file += "class "sv;
							file += tablename;

							if(targettable.baseMap) {
								file += " : "sv;
								file += targettable.baseMap->dataClassName;
							}

							file += '\n';
							bindings::docs::ident(file, depth);
							file += "{\n"sv;

							std::size_t num_props{static_cast<std::size_t>(targettable.dataNumFields)};
							for(std::size_t i{0}; i < num_props; ++i) {
								gsdk::typedescription_t &prop{targettable.dataDesc[i]};

								if(num_props == 1 && i == 0 && prop == gsdk::typedescription_t::empty) {
									break;
								}

								std::function<void(gsdk::typedescription_t &, std::string_view, std::size_t)> write_prop{
									[&file,&write_table](gsdk::typedescription_t &targetprop, std::string_view propname, std::size_t funcdepth) noexcept -> void {
										if(targetprop.td) {
											write_table(*targetprop.td, targetprop.td->dataClassName ? targetprop.td->dataClassName : "<<unknown>>"sv, funcdepth);
										}

										bindings::docs::ident(file, funcdepth);

										gsdk::fieldtype_t type{targetprop.fieldType};

										std::size_t size{static_cast<std::size_t>(-1)};
										if(targetprop.fieldSizeInBytes > 0 && targetprop.fieldSize > 0) {
											size = static_cast<std::size_t>(targetprop.fieldSizeInBytes / targetprop.fieldSize);
										}

										if(targetprop.fieldType == gsdk::FIELD_EMBEDDED) {
											if(targetprop.td && targetprop.td->dataClassName) {
												file += targetprop.td->dataClassName;
											} else {
												file += "<<unknown>>"sv;
											}
											file += ' ';
										} else {
											switch(type) {
												case gsdk::FIELD_VOID: {
													if(targetprop.flags & gsdk::FTYPEDESC_FUNCTIONTABLE ||
														targetprop.flags & gsdk::FTYPEDESC_INPUT) {
														type = gsdk::FIELD_FUNCTION;
													}
												} break;
												case gsdk::FIELD_EMBEDDED: {
													if(targetprop.flags & gsdk::FTYPEDESC_PTR) {
														type = gsdk::FIELD_CLASSPTR;
													}
												} break;
												case gsdk::FIELD_CUSTOM: {
													if(targetprop.flags & gsdk::FTYPEDESC_OUTPUT) {
														type = gsdk::FIELD_FUNCTION;
													}
												} break;
												default: {
													if(targetprop.flags & gsdk::FTYPEDESC_INPUT) {
														type = gsdk::FIELD_FUNCTION;
													}
												} break;
											}

											file += bindings::docs::type_name(type, size, targetprop.flags);
											if(*(file.end()-1) != '*') {
												file += ' ';
											}
										}

										file += propname;

										if(targetprop.fieldSize > 1) {
											file += '[';

											std::string num_str;
											num_str.resize(6 + 6);

											char *begin{num_str.data()};
											char *end{begin + num_str.size()};

											std::to_chars_result tc_res{std::to_chars(begin, end, targetprop.fieldSize)};
											tc_res.ptr[0] = '\0';

											file += begin;
											file += ']';
										}

										if(targetprop.flags & gsdk::FTYPEDESC_INPUT) {
											file += '(';
											file += bindings::docs::type_name(targetprop.fieldType, size, targetprop.flags);
											file += ')';
										}

										file += ';';

										if(targetprop.flags != 0) {
											std::string flags{" ["s};
											//TODO!!!
											if(flags.length() > 2) {
												flags.pop_back();
												flags += ']';
												file += std::move(flags);
											}
										}

										file += '\n';
									}
								};

								if(prop.flags & gsdk::FTYPEDESC_INPUT ||
									prop.flags & gsdk::FTYPEDESC_OUTPUT) {
									if(only_keyvalues) {
										write_prop(prop, prop.externalName ? prop.externalName : "<<unknown>>"sv, depth+1);
									}
								} else if(prop.flags & gsdk::FTYPEDESC_FUNCTIONTABLE) {
									if(!only_keyvalues) {
										std::string funcname;
										if(prop.fieldName) {
											funcname = prop.fieldName;
											funcname.erase(0, tablename.length());
										} else {
											funcname = "<<unknown>>"s;
										}
										write_prop(prop, funcname, depth+1);
									}
								} else if(prop.flags & gsdk::FTYPEDESC_KEY) {
									if(only_keyvalues) {
										write_prop(prop, prop.externalName ? prop.externalName : "<<unknown>>"sv, depth+1);
									} else {
										write_prop(prop, prop.fieldName ? prop.fieldName : "<<unknown>>"sv, depth+1);
									}
								} else if(!only_keyvalues) {
									write_prop(prop, prop.fieldName ? prop.fieldName : "<<unknown>>"sv, depth+1);
								}
							}

							bindings::docs::ident(file, depth);
							file += "};\n"sv;
						}
					};

					write_table(*it.second, it.second->dataClassName ? it.second->dataClassName : "<<unknown>>"sv, 0);

					std::filesystem::path dump_path{dump_dir};
					dump_path /= it.first;
					dump_path.replace_extension(".txt"sv);

					write_file(dump_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
				}
			}
		};

		vmod_dump_datamaps.initialize("vmod_dump_datamaps"sv,
			[dump_datamaps_impl](const gsdk::CCommand &) -> void {
				dump_datamaps_impl(false);
			}
		);

		vmod_auto_dump_keyvalues.initialize("vmod_auto_dump_keyvalues"sv, false);

		vmod_dump_keyvalues.initialize("vmod_dump_keyvalues"sv,
			[dump_datamaps_impl](const gsdk::CCommand &) -> void {
				dump_datamaps_impl(true);
			}
		);

		vmod_auto_dump_entity_classes.initialize("vmod_auto_dump_entity_classes"sv, false);

		vmod_dump_entity_classes.initialize("vmod_dump_entity_classes"sv,
			[this](const gsdk::CCommand &) noexcept -> void {
				std::filesystem::path dump_dir{root_dir_/"dumps"sv/"classes"sv};

				std::error_code ec;
				std::filesystem::create_directories(dump_dir, ec);

				std::string file;

				bindings::docs::gen_date(file);

				for(const auto &it : sv_ent_factories) {
					file += it.first;
					file += '\n';
				}

				std::filesystem::path dump_path{dump_dir};
				dump_path /= "classes"sv;
				dump_path.replace_extension(".txt"sv);

				write_file(dump_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
			}
		);

	#ifndef GSDK_NO_SYMBOLS
		vmod_auto_dump_entity_vtables.initialize("vmod_auto_dump_entity_vtables"sv, false);

		vmod_dump_entity_vtables.initialize("vmod_dump_entity_vtables"sv,
			[this](const gsdk::CCommand &) noexcept -> void {
				if(!symbols_available) {
					error("vmod: no symbols available\n"sv);
					return;
				}

				const auto &sv_symbols{server_lib.symbols()};

				std::filesystem::path dump_dir{root_dir_/"dumps"sv/"vtables"sv};

				std::error_code ec;
				std::filesystem::create_directories(dump_dir, ec);

				for(const auto &it : sv_classes) {
					auto sv_sym_it{sv_symbols.find(it.first)};
					if(sv_sym_it == sv_symbols.end()) {
						error("vmod: missing '%s' symbols\n"sv, it.first.c_str());
						continue;
					}

					const symbol_cache::class_info *class_info{dynamic_cast<const symbol_cache::class_info *>(sv_sym_it->second.get())};
					const symbol_cache::class_info::vtable_info &vtable_info{class_info->vtable()};

					std::string file;

					bindings::docs::gen_date(file);

					file += "class "sv;
					file += it.first;

					std::size_t start{0};

					auto data_it{sv_datamaps.find(it.first)};
					if(data_it != sv_datamaps.end()) {
						if(data_it->second->baseMap) {
							std::string basename{data_it->second->baseMap->dataClassName};

							file += " : "sv;
							file += basename;

							auto base_sym_it{sv_symbols.find(basename)};
							if(base_sym_it != sv_symbols.end()) {
								const symbol_cache::class_info *base_class_info{dynamic_cast<const symbol_cache::class_info *>(base_sym_it->second.get())};
								const symbol_cache::class_info::vtable_info &base_vtable_info{base_class_info->vtable()};

								start = base_vtable_info.size();
							}
						}
					}

					file += "\n{\n"sv;

					for(std::size_t i{start}; i < vtable_info.size(); ++i) {
						const auto &func_it{vtable_info[i]};
						bindings::docs::ident(file, 1);

						if(func_it.qual != sv_symbols.end()) {
							if(func_it.qual->first != it.first) {
								file += func_it.qual->first;
								file += "::"sv;
							}
						} else {
							file += "<<unknown>>::"sv;
						}

						if(func_it.func != class_info->end()) {
							file += func_it.func->first;
							file += ";\n"sv;
						} else {
							file += "<<unknown>>\n"sv;
						}
					}

					file += "};"sv;

					std::filesystem::path dump_path{dump_dir};
					dump_path /= it.first;
					dump_path.replace_extension(".txt"sv);

					write_file(dump_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
				}
			}
		);

		vmod_auto_dump_entity_funcs.initialize("vmod_auto_dump_entity_funcs"sv, false);

		vmod_dump_entity_funcs.initialize("vmod_dump_entity_funcs"sv,
			[this](const gsdk::CCommand &) noexcept -> void {
				if(!symbols_available) {
					error("vmod: no symbols available\n"sv);
					return;
				}

				const auto &sv_symbols{server_lib.symbols()};

				std::filesystem::path dump_dir{root_dir_/"dumps"sv/"funcs"sv};

				std::error_code ec;
				std::filesystem::create_directories(dump_dir, ec);

				for(const auto &it : sv_classes) {
					auto sv_sym_it{sv_symbols.find(it.first)};
					if(sv_sym_it == sv_symbols.end()) {
						error("vmod: missing '%s' symbols\n"sv, it.first.c_str());
						continue;
					}

					const symbol_cache::class_info *class_info{dynamic_cast<const symbol_cache::class_info *>(sv_sym_it->second.get())};

					std::string file;

					bindings::docs::gen_date(file);

					file += "class "sv;
					file += it.first;

					auto data_it{sv_datamaps.find(it.first)};
					if(data_it != sv_datamaps.end()) {
						if(data_it->second->baseMap) {
							std::string basename{data_it->second->baseMap->dataClassName};

							file += " : "sv;
							file += std::move(basename);
						}
					}

					file += "\n{\n"sv;

					for(const auto &func_it : *class_info) {
						bindings::docs::ident(file, 1);
						if(func_it.second->virtual_index() != static_cast<std::size_t>(-1)) {
							file += "virtual "sv;
						}
						file += func_it.first;
						file += ";\n"sv;
					}

					file += "};"sv;

					std::filesystem::path dump_path{dump_dir};
					dump_path /= it.first;
					dump_path.replace_extension(".txt"sv);

					write_file(dump_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
				}
			}
		);
	#endif

		if(filesystem) {
			std::filesystem::path assets_dir{root_dir_};
			assets_dir /= "assets"sv;

			filesystem->AddSearchPath(assets_dir.c_str(), "GAME");
			filesystem->AddSearchPath(assets_dir.c_str(), "mod");

			std::filesystem::path vscript_override_dir{root_dir_};
			vscript_override_dir /= "script_overrides"sv;

			filesystem->AddSearchPath(vscript_override_dir.c_str(), "GAME");
		}

		sv_engine->InsertServerCommand("exec vmod/load.cfg\n");
		sv_engine->ServerExecute();

		return true;
	}

	bool main::assign_entity_class_info() noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		{
			auto CBaseEntity_desc_it{sv_script_class_descs.find("CBaseEntity"s)};
			if(CBaseEntity_desc_it == sv_script_class_descs.end()) {
				warning("vmod: failed to find CBaseEntity script class\n"sv);
			}

			if(CBaseEntity_desc_it != sv_script_class_descs.end()) {
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
			std::filesystem::path filename{path.filename()};

			if(filename.native()[0] == '.') {
				continue;
			}

			if(file.is_directory()) {
				if(std::filesystem::exists(path/".ignored"sv)) {
					continue;
				}

				if(filename == "include"sv) {
					if(dir_name == "assets"sv) {
						remark("vmod: include folder inside assets folder: '%s'\n"sv, path.c_str());
					} else if(dir_name == "docs"sv) {
						remark("vmod: include folder inside docs folder: '%s'\n"sv, path.c_str());
					}
					continue;
				} else if(filename == "docs"sv) {
					if(dir_name == "assets"sv) {
						remark("vmod: docs folder inside assets folder: '%s'\n"sv, path.c_str());
					} else if(dir_name == "include"sv) {
						remark("vmod: docs folder inside include folder: '%s'\n"sv, path.c_str());
					}
					continue;
				} else if(filename == "assets"sv) {
					if(!(flags & load_plugins_flags::src_folder)) {
						if(filesystem) {
							filesystem->AddSearchPath(path.c_str(), "GAME");
							filesystem->AddSearchPath(path.c_str(), "mod");
						}
						added_paths.emplace_back(std::move(path));
					} else {
						remark("vmod: assets folder inside src folder: '%s'\n"sv, path.c_str());
					}
					continue;
				} else if(filename == "src"sv) {
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

			if(filename.extension() != scripts_extension) {
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

	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2 || defined __VMOD_USING_CUSTOM_VM
		{
			gsdk::ScriptFunctionBinding_t &func{isweakref_desc};

			func.m_flags = 0;
			func.m_desc.m_pszDescription = nullptr;
			func.m_desc.m_pszScriptName = "IsWeakref";
			func.m_desc.m_ReturnType = gsdk::FIELD_BOOLEAN;

			func.m_desc.m_Parameters.emplace_back(gsdk::FIELD_HSCRIPT);
			func.m_desc.m_Parameters.emplace_back(gsdk::FIELD_CSTRING);

			internal_vscript_func_bindings.emplace_back(&func);
		}
	#endif

		{
			gsdk::ScriptFunctionBinding_t &func{getfunctionsignature_desc};

			func.m_flags = 0;
			func.m_desc.m_pszDescription = nullptr;
			func.m_desc.m_pszScriptName = "GetFunctionSignature";
			func.m_desc.m_ReturnType = gsdk::FIELD_CSTRING;

			func.m_desc.m_Parameters.emplace_back(gsdk::FIELD_HSCRIPT);
			func.m_desc.m_Parameters.emplace_back(gsdk::FIELD_CSTRING);

			internal_vscript_func_bindings.emplace_back(&func);
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2 || defined __VMOD_USING_CUSTOM_VM
		{
			gsdk::ScriptFunctionBinding_t &func{dumpobject_desc};

			func.m_flags = 0;
			func.m_desc.m_pszDescription = nullptr;
			func.m_desc.m_pszScriptName = "DumpObject";
			func.m_desc.m_ReturnType = gsdk::FIELD_VOID;

			func.m_desc.m_Parameters.emplace_back(gsdk::FIELD_VARIANT);

			internal_vscript_func_bindings.emplace_back(&func);
		}
	#endif

	#if (GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2) && !defined __VMOD_USING_CUSTOM_VM
		{
			gsdk::ScriptFunctionBinding_t &func{makenamespace_desc};

			func.m_flags = 0;
			func.m_desc.m_pszDescription = nullptr;
			func.m_desc.m_pszScriptName = "MakeNamespace";
			func.m_desc.m_ReturnType = gsdk::FIELD_VOID;

			func.m_desc.m_Parameters.emplace_back(gsdk::FIELD_HSCRIPT);

			internal_vscript_func_bindings.emplace_back(&func);
		}
	#endif

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

	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2 || __VMOD_USING_CUSTOM_VM
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
	#endif

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

		if(vmod_auto_dump_squirrel_ver.get<bool>()) {
			vmod_dump_squirrel_ver();
		}

		if(vmod_auto_dump_internal_scripts.get<bool>()) {
			vmod_dump_internal_scripts();
		}

	#ifndef GSDK_NO_SYMBOLS
		if(vmod_auto_dump_entity_vtables.get<bool>()) {
			vmod_dump_entity_vtables();
		}

		if(vmod_auto_dump_entity_funcs.get<bool>()) {
			vmod_dump_entity_funcs();
		}
	#endif

		if(vmod_auto_gen_docs.get<bool>()) {
			vmod_gen_docs();
		}

		vmod_refresh_plugins();

		if(vmod_auto_dump_netprops.get<bool>()) {
			vmod_dump_netprops();
		}

		if(vmod_auto_dump_datamaps.get<bool>()) {
			vmod_dump_datamaps();
		}

		if(vmod_auto_dump_keyvalues.get<bool>()) {
			vmod_dump_keyvalues();
		}

		if(vmod_auto_dump_entity_classes.get<bool>()) {
			vmod_dump_entity_classes();
		}

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

	#ifdef __VMOD_USING_PREPROCESSOR
		pp.shutdown();
	#endif

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

			if(g_pScriptVM_ptr && *g_pScriptVM_ptr == vm_) {
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

		vmod_dump_internal_scripts.unregister();
		vmod_auto_dump_internal_scripts.unregister();

		vmod_dump_squirrel_ver.unregister();
		vmod_auto_dump_squirrel_ver.unregister();

		vmod_dump_netprops.unregister();
		vmod_auto_dump_netprops.unregister();

		vmod_dump_datamaps.unregister();
		vmod_auto_dump_datamaps.unregister();

		vmod_dump_keyvalues.unregister();
		vmod_auto_dump_keyvalues.unregister();

		vmod_dump_entity_classes.unregister();
		vmod_auto_dump_entity_classes.unregister();

	#ifndef GSDK_NO_SYMBOLS
		vmod_dump_entity_vtables.unregister();
		vmod_auto_dump_entity_vtables.unregister();

		vmod_dump_entity_funcs.unregister();
		vmod_auto_dump_entity_funcs.unregister();
	#endif

		vmod_gen_docs.unregister();
		vmod_auto_gen_docs.unregister();

		if(cvar_dll_id_ != gsdk::INVALID_CVAR_DLL_IDENTIFIER) {
			cvar->UnregisterConCommands(cvar_dll_id_);
		}

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		if(old_spew) {
			SpewOutputFunc(old_spew);
		}
	#endif
	}
}

namespace vmod
{
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
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
	#pragma GCC diagnostic pop

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

	static vsp vsp;

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
