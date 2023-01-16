#pragma once

#include "../config.hpp"

namespace gsdk
{
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class ICommandLine
	{
	public:
		virtual void CreateCmdLine(const char *) = 0;
		virtual void CreateCmdLine(int, char **) = 0;
		virtual const char *GetCmdLine() const = 0;
		virtual	const char *CheckParm(const char *, const char ** = nullptr) const = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		virtual bool HasParm(const char *) const = 0;
	#endif
		virtual void RemoveParm(const char *) = 0;
		virtual void AppendParm(const char *, const char *) = 0;
		virtual const char *ParmValue(const char *, const char * = nullptr) const = 0;
		virtual int ParmValue(const char *, int) const = 0;
		virtual float ParmValue(const char *, float) const = 0;
		virtual int ParmCount() const = 0;
		virtual int FindParm(const char *) const = 0;
		virtual const char *GetParm(int) const = 0;
		virtual void SetParm(int, const char *) =0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		virtual const char **GetParms() const = 0;
	#endif
	};
	#pragma GCC diagnostic pop
}

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
extern "C" __attribute__((__visibility__("default"))) gsdk::ICommandLine * __attribute__((__cdecl__)) CommandLine();
#else
extern "C" __attribute__((__visibility__("default"))) gsdk::ICommandLine * __attribute__((__cdecl__)) CommandLine_Tier0();

inline __attribute__((__always_inline__)) gsdk::ICommandLine *CommandLine() noexcept
{ return CommandLine_Tier0(); }
#endif
