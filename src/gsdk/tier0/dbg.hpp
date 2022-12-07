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

extern "C" void __attribute__((__visibility__("default"))) SpewOutputFunc(gsdk::SpewOutputFunc_t);
extern "C" gsdk::SpewOutputFunc_t __attribute__((__visibility__("default"))) GetSpewOutputFunc();

extern "C" void __attribute__((__visibility__("default"))) Error(const char *, ...);
extern "C" void __attribute__((__visibility__("default"))) ErrorV(const char *, va_list);

extern "C" void __attribute__((__visibility__("default"))) Msg(const char * , ...);
extern "C" void __attribute__((__visibility__("default"))) DMsg(const char *, int, const char *, ...);
extern "C" void __attribute__((__visibility__("default"))) MsgV(const char *, va_list);

extern "C" void __attribute__((__visibility__("default"))) Warning(const char *, ...);
extern "C" void __attribute__((__visibility__("default"))) DWarning(const char *, int, const char *, ...);
extern "C" void __attribute__((__visibility__("default"))) WarningV(const char *, va_list);

extern "C" void __attribute__((__visibility__("default"))) Log(const char *, ...);
extern "C" void __attribute__((__visibility__("default"))) DLog(const char *, int, const char *, ...);
extern "C" void __attribute__((__visibility__("default"))) LogV(const char *, va_list);

extern "C" void __attribute__((__visibility__("default"))) DevMsg(int,const char *, ...);

extern "C" void __attribute__((__visibility__("default"))) DevWarning(int,const char *, ...);

extern "C" void __attribute__((__visibility__("default"))) DevLog(int,const char *, ...);

extern "C" void __attribute__((__visibility__("default"))) ConColorMsg(int, const gsdk::Color &, const char *, ...);

extern "C" void __attribute__((__visibility__("default"))) ConWarning(int, const char *, ...);

extern "C" void __attribute__((__visibility__("default"))) ConMsg(int, const char *, ...);

extern "C" void __attribute__((__visibility__("default"))) ConLog(int, const char *, ...);

extern "C" void __attribute__((__visibility__("default"))) ConDColorMsg(const gsdk::Color &, const char *, ...);
extern "C" void __attribute__((__visibility__("default"))) ConDMsg(const char *, ...);
extern "C" void __attribute__((__visibility__("default"))) ConDWarning(const char *, ...);
extern "C" void __attribute__((__visibility__("default"))) ConDLog(const char *, ...);

extern "C" void __attribute__((__visibility__("default"))) NetMsg(int, const char *, ... );
extern "C" void __attribute__((__visibility__("default"))) NetWarning(int, const char *, ... );
extern "C" void __attribute__((__visibility__("default"))) NetLog(int, const char *, ... );
