#pragma once

#include <cstdarg>
#include "../config.hpp"
#include "threadtools.hpp"
#include "../../hacking.hpp"

union alignas(unsigned int) Color
{
	constexpr inline Color(unsigned char r_, unsigned char g_, unsigned char b_, unsigned char a_) noexcept
		: r{r_}, g{g_}, b{b_}, a{a_}
	{
	}

	constexpr inline Color(unsigned int val) noexcept
		: value{val}
	{
	}

	constexpr inline Color &operator=(unsigned int val) noexcept
	{
		value = val;
		return *this;
	}

	constexpr Color(const Color &) noexcept = default;
	constexpr Color &operator=(const Color &) noexcept = default;

	constexpr Color(Color &&) noexcept = default;
	constexpr Color &operator=(Color &&) noexcept = default;

	constexpr inline bool operator==(const Color &other) const noexcept
	{ return value == other.value; }
	constexpr inline bool operator!=(const Color &other) const noexcept
	{ return value != other.value; }

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

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
enum LoggingSeverity_t : int
{
	LS_MESSAGE = 0,
	LS_WARNING = 1,
	LS_ASSERT = 2,
	LS_ERROR = 3,
	LS_HIGHEST_SEVERITY = 4,
};
#endif

namespace gsdk
{
	using Color = ::Color;

	constexpr int MAXPRINTMSG{4096};

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
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
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	constexpr int MAX_LOGGING_MESSAGE_LENGTH{2048};

	using LoggingChannelID_t = int;

	constexpr LoggingChannelID_t INVALID_LOGGING_CHANNEL_ID{-1};

	using ::LoggingSeverity_t;

	constexpr Color UNSPECIFIED_LOGGING_COLOR{0, 0, 0, 0};

	enum LoggingChannelFlags_t : int
	{
		LCF_CONSOLE_ONLY = 0x00000001,
		LCF_DO_NOT_ECHO = 0x00000002,
	};

	struct LoggingContext_t
	{
		LoggingChannelID_t m_ChannelID;
		LoggingChannelFlags_t m_Flags;
		LoggingSeverity_t m_Severity;
		Color m_Color;
	};

	enum LoggingResponse_t : int
	{
		LR_CONTINUE,
		LR_DEBUGGER,
		LR_ABORT,
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class ILoggingListener
	{
	public:
		virtual void Log(const LoggingContext_t *, const char *) = 0;
	};

	class ILoggingResponsePolicy
	{
	public:
		virtual LoggingResponse_t OnLog(const LoggingContext_t *) = 0;
	};
	#pragma GCC diagnostic pop

	constexpr int MAX_LOGGING_IDENTIFIER_LENGTH{32};

	constexpr int MAX_LOGGING_STATE_COUNT{16};
	constexpr int MAX_LOGGING_CHANNEL_COUNT{256};
	constexpr int MAX_LOGGING_TAG_COUNT{1024};
	constexpr int MAX_LOGGING_TAG_CHARACTER_COUNT{8192};
	constexpr int MAX_LOGGING_LISTENER_COUNT{16};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class CDefaultLoggingResponsePolicy : public ILoggingResponsePolicy
	{
	private:
		LoggingResponse_t OnLog(const LoggingContext_t *) override
		{ vmod::debugtrap(); return LR_ABORT; }
	};

	class CSimpleLoggingListener : public ILoggingListener
	{
	private:
		void Log(const LoggingContext_t *, const char *) override
		{ vmod::debugtrap(); }

	public:
		bool m_bQuietPrintf;
		bool m_bQuietDebugger;
	};
	#pragma GCC diagnostic pop

	class CLoggingSystem
	{
	public:
		struct LoggingTag_t
		{
			const char *m_pTagName;
			LoggingTag_t *m_pNextTag;
		};

		struct LoggingChannel_t
		{
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
			LoggingChannelID_t m_ID;
		#endif
			LoggingChannelFlags_t m_Flags;
			LoggingSeverity_t m_MinimumSeverity;
			Color m_SpewColor;
			char m_Name[MAX_LOGGING_IDENTIFIER_LENGTH];
			LoggingTag_t *m_pFirstTag;
		};

		struct LoggingState_t
		{
			int m_nPreviousStackEntry;
			int m_nListenerCount;
			ILoggingListener *m_RegisteredListeners[MAX_LOGGING_LISTENER_COUNT];
			ILoggingResponsePolicy *m_pLoggingResponse;
		};

		int m_nChannelCount;
		LoggingChannel_t m_RegisteredChannels[MAX_LOGGING_CHANNEL_COUNT];
		int m_nChannelTagCount;
		LoggingTag_t m_ChannelTags[MAX_LOGGING_TAG_COUNT];
		int m_nTagNamePoolIndex;
		char m_TagNamePool[MAX_LOGGING_TAG_CHARACTER_COUNT];
		CThreadFastMutex *m_pStateMutex;
		int m_nGlobalStateIndex;
		LoggingState_t m_LoggingStates[MAX_LOGGING_STATE_COUNT];
		CDefaultLoggingResponsePolicy m_DefaultLoggingResponse;
		CSimpleLoggingListener m_DefaultLoggingListener;
	};

	using RegisterTagsFunc = void(*)();
#endif
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

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
extern "C" __attribute__((__visibility__("default"))) unsigned int LoggingSystem_GetChannelColor(gsdk::LoggingChannelID_t);
extern "C" __attribute__((__visibility__("default"))) void LoggingSystem_SetChannelColor(gsdk::LoggingChannelID_t, unsigned int);

extern "C" __attribute__((__visibility__("default"))) gsdk::LoggingChannelFlags_t LoggingSystem_GetChannelFlags(gsdk::LoggingChannelID_t);
extern "C" __attribute__((__visibility__("default"))) void LoggingSystem_SetChannelFlags(gsdk::LoggingChannelID_t, gsdk::LoggingChannelFlags_t);

extern "C" __attribute__((__visibility__("default"))) gsdk::LoggingChannelID_t LoggingSystem_GetFirstChannelID();
extern "C" __attribute__((__visibility__("default"))) gsdk::LoggingChannelID_t LoggingSystem_GetNextChannelID(gsdk::LoggingChannelID_t);
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
extern "C" __attribute__((__visibility__("default"))) const gsdk::CLoggingSystem::LoggingChannel_t *LoggingSystem_GetChannel(gsdk::LoggingChannelID_t);
extern "C" __attribute__((__visibility__("default"))) gsdk::LoggingChannelID_t LoggingSystem_FindChannel(const char *);

extern "C" __attribute__((__visibility__("default"))) bool LoggingSystem_IsChannelEnabled(gsdk::LoggingChannelID_t, gsdk::LoggingSeverity_t);

extern "C" __attribute__((__visibility__("default"))) int LoggingSystem_GetChannelCount();

extern "C" __attribute__((__visibility__("default"))) bool LoggingSystem_HasTag(gsdk::LoggingChannelID_t, const char *);

extern "C" __attribute__((__visibility__("default"))) void LoggingSystem_AddTagToCurrentChannel(const char *);

extern "C" __attribute__((__visibility__("default"))) void LoggingSystem_SetChannelSpewLevel(gsdk::LoggingChannelID_t, gsdk::LoggingSeverity_t);
extern "C" __attribute__((__visibility__("default"))) void LoggingSystem_SetChannelSpewLevelByName(const char *, gsdk::LoggingSeverity_t);
extern "C" __attribute__((__visibility__("default"))) void LoggingSystem_SetChannelSpewLevelByTag(const char *, gsdk::LoggingSeverity_t);
extern "C" __attribute__((__visibility__("default"))) void LoggingSystem_SetGlobalSpewLevel(gsdk::LoggingSeverity_t);

extern "C" __attribute__((__visibility__("default"))) gsdk::LoggingChannelID_t LoggingSystem_RegisterLoggingChannel(const char *pName, gsdk::RegisterTagsFunc, int = 0, gsdk::LoggingSeverity_t = LS_MESSAGE, gsdk::Color = gsdk::UNSPECIFIED_LOGGING_COLOR); 

extern "C" __attribute__((__visibility__("default"))) gsdk::LoggingResponse_t LoggingSystem_Log(gsdk::LoggingChannelID_t, gsdk::LoggingSeverity_t, const char *, ... );
extern "C++" __attribute__((__visibility__("default"))) gsdk::LoggingResponse_t LoggingSystem_Log(gsdk::LoggingChannelID_t, gsdk::LoggingSeverity_t, gsdk::Color, const char *, ... );

extern "C" __attribute__((__visibility__("default"))) gsdk::LoggingResponse_t LoggingSystem_LogDirect(gsdk::LoggingChannelID_t, gsdk::LoggingSeverity_t, gsdk::Color, const char *);
extern "C" __attribute__((__visibility__("default"))) gsdk::LoggingResponse_t LoggingSystem_LogAssert(const char *, ... );
#endif
