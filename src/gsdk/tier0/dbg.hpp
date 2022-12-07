#pragma once

#include <cstdarg>

namespace gsdk
{
	struct Color;

	enum SpewType_t : int
	{
		SPEW_MESSAGE,
		SPEW_WARNING,
		SPEW_ASSERT,
		SPEW_ERROR,
		SPEW_LOG,
		SPEW_TYPE_COUNT
	};

	enum SpewRetval_t : int
	{
		SPEW_DEBUGGER,
		SPEW_CONTINUE,
		SPEW_ABORT
	};

	using SpewOutputFunc_t = SpewRetval_t(*)(SpewType_t, const char *);
}

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) SpewOutputFunc(gsdk::SpewOutputFunc_t);
extern "C" __attribute__((__visibility__("default"))) gsdk::SpewOutputFunc_t __attribute__((__cdecl__)) GetSpewOutputFunc();

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) Error(const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ErrorV(const char *, va_list);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) Msg(const char * , ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DMsg(const char *, int, const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) MsgV(const char *, va_list);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) Warning(const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DWarning(const char *, int, const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) WarningV(const char *, va_list);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) Log(const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DLog(const char *, int, const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) LogV(const char *, va_list);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DevMsg(int,const char *, ...);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DevWarning(int,const char *, ...);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DevLog(int,const char *, ...);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConColorMsg(int, const gsdk::Color &, const char *, ...);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConWarning(int, const char *, ...);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConMsg(int, const char *, ...);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConLog(int, const char *, ...);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConDColorMsg(const gsdk::Color &, const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConDMsg(const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConDWarning(const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConDLog(const char *, ...);

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) NetMsg(int, const char *, ... );
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) NetWarning(int, const char *, ... );
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) NetLog(int, const char *, ... );
