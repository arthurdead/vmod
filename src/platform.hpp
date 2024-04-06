#pragma once

#ifdef __x86_64__
	#define VMOD_KATTR_CDECL 
	#define VMOD_KATTR_THISCALL 
#else
	#define VMOD_KATTR_CDECL __attribute__((__cdecl__))
	#define VMOD_KATTR_THISCALL __attribute__((__thiscall__))
#endif
