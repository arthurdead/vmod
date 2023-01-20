#pragma once

#include <cstdarg>
#include "../config.hpp"

union alignas(unsigned int) Color
{
	constexpr Color(unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_) noexcept
		: r{r_}, g{g_}, b{b_}, a{a_}
	{
	}

	struct {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	};
	unsigned int value;
};

static_assert(sizeof(Color) == sizeof(unsigned int));
static_assert(alignof(Color) == alignof(unsigned int));

namespace gsdk
{
	using Color = ::Color;

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

	constexpr int MAXPRINTMSG{4096};
	constexpr int MAX_LOGGING_MESSAGE_LENGTH{2048};

	using LoggingChannelID_t = int;

	constexpr LoggingChannelID_t INVALID_LOGGING_CHANNEL_ID{-1};

	constexpr Color UNSPECIFIED_LOGGING_COLOR{0, 0, 0, 0};
}

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) const char * __attribute__((__cdecl__)) GetSpewOutputGroup();
extern "C" __attribute__((__visibility__("default"))) int __attribute__((__cdecl__)) GetSpewOutputLevel();
extern "C" __attribute__((__visibility__("default"))) const gsdk::Color * __attribute__((__cdecl__)) GetSpewOutputColor();

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) SpewOutputFunc(gsdk::SpewOutputFunc_t);
extern "C" __attribute__((__visibility__("default"))) gsdk::SpewOutputFunc_t __attribute__((__cdecl__)) GetSpewOutputFunc();

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) SpewActivate(const char *, int);

extern "C" __attribute__((__visibility__("default"))) gsdk::SpewRetval_t __attribute__((__cdecl__)) ColorSpewMessage(gsdk::SpewType_t, const gsdk::Color *, const char *, ...);
#endif

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) Error(const char *, ...);
#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ErrorV(const char *, va_list);
#endif

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) Msg(const char * , ...);
#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DMsg(const char *, int, const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) MsgV(const char *, va_list);
#endif

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) Warning(const char *, ...);
#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DWarning(const char *, int, const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) WarningV(const char *, va_list);
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) Log(const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DLog(const char *, int, const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) LogV(const char *, va_list);
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DevLog(const char *, ...);
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DevMsg(int, const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DevWarning(int, const char *, ...);
#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DevMsg(const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) DevWarning(const char *, ...);
#else
	#error
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConColorMsg(int, const gsdk::Color &, const char *, ...);
#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
extern __attribute__((__visibility__("default"))) void ConColorMsg(const gsdk::Color &, const char *, ...);
#else
	#error
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConWarning(int, const char *, ...);
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConMsg(int, const char *, ...);
#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
extern __attribute__((__visibility__("default"))) void ConMsg(const char *, ...);
#else
	#error
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConLog(int, const char *, ...);
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConDColorMsg(const gsdk::Color &, const char *, ...);
#endif

extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConDMsg(const char *, ...);

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConDWarning(const char *, ...);
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) ConDLog(const char *, ...);
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) NetMsg(int, const char *, ... );
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) NetWarning(int, const char *, ... );
extern "C" __attribute__((__visibility__("default"))) void __attribute__((__cdecl__)) NetLog(int, const char *, ... );
#endif
