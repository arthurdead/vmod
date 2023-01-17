#pragma once

#include "../gsdk/config.hpp"
#include "../vscript/vscript.hpp"
#include <filesystem>
#include <string_view>

#if GSDK_ENGINE == GSDK_ENGINE_L4D2 || GSDK_ENGINE == GSDK_ENGINE_TF2
	#define __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE override
#else
	#define __VMOD_CUSTOM_VM_L4D2_TF2_OVERRIDE
#endif

#if GSDK_ENGINE != GSDK_ENGINE_L4D2
	#define __VMOD_CUSTOM_VM_NOT_L4D2_OVERRIDE override
#else
	#define __VMOD_CUSTOM_VM_NOT_L4D2_OVERRIDE
#endif

#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	#define __VMOD_CUSTOM_VM_L4D2_OVERRIDE override
#else
	#define __VMOD_CUSTOM_VM_L4D2_OVERRIDE
#endif

#define __VMOD_SQUIRREL_OVERRIDE_SERVER_INIT_SCRIPT

#ifdef __VMOD_USING_CUSTOM_VM
	#define __VMOD_SQUIRREL_OVERRIDE_INIT_SCRIPT

	#ifdef __VMOD_USING_QUIRREL
		#ifndef __VMOD_SQUIRREL_OVERRIDE_INIT_SCRIPT
			#define __VMOD_SQUIRREL_OVERRIDE_INIT_SCRIPT
		#endif

		#ifndef __VMOD_SQUIRREL_OVERRIDE_SERVER_INIT_SCRIPT
			#define __VMOD_SQUIRREL_OVERRIDE_SERVER_INIT_SCRIPT
		#endif
	#endif
#endif

#if defined __VMOD_USING_CUSTOM_VM && !defined __VMOD_SQUIRREL_OVERRIDE_INIT_SCRIPT
	#define __VMOD_SQUIRREL_NEED_INIT_SCRIPT
#endif

namespace vmod
{
#ifdef __VMOD_SQUIRREL_NEED_INIT_SCRIPT
	extern const unsigned char *g_Script_init;
#endif

	extern bool compile_internal_script(gsdk::IScriptVM *vm, std::filesystem::path path, const unsigned char *data, gsdk::HSCRIPT &object, bool &from_file) noexcept;
}
