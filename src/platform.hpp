#pragma once

#ifdef __x86_64__
	#define VMOD_KATTR_CDECL 
	#define VMOD_KATTR_THISCALL 
#else
	#define VMOD_KATTR_CDECL __attribute__((__cdecl__))
	#define VMOD_KATTR_THISCALL __attribute__((__thiscall__))
#endif

#define ___VMOD_UNIQUE_NAME(x1,x2) x1##x2
#define __VMOD_UNIQUE_NAME(x1,x2) ___VMOD_UNIQUE_NAME(x1,x2)
#define VMOD_UNIQUE_NAME __VMOD_UNIQUE_NAME(_, __COUNTER__)
