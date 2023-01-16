#pragma once

#include "vscript.hpp"

#ifdef __VMOD_USING_CUSTOM_VM
	#define __VMOD_OVERRIDE_INIT_SCRIPT
	#define __VMOD_OVERRIDE_SERVER_INIT_SCRIPT

	#ifdef __VMOD_USING_QUIRREL
		#ifndef __VMOD_OVERRIDE_SERVER_INIT_SCRIPT
			#define __VMOD_OVERRIDE_SERVER_INIT_SCRIPT
		#endif

		#ifndef __VMOD_OVERRIDE_INIT_SCRIPT
			#define __VMOD_OVERRIDE_INIT_SCRIPT
		#endif
	#endif
#endif

namespace vmod
{
#ifndef __VMOD_OVERRIDE_INIT_SCRIPT
	extern const unsigned char *g_Script_init;
#endif
}
