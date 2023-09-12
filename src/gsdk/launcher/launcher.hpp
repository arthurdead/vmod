#pragma once

#include "../tier1/interface.hpp"
#include "../tier1/appframework.hpp"
#include "../config.hpp"
#include "../tier0/dbg.hpp"
#include <string_view>

namespace gsdk
{
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IDedicatedExports : public IAppSystem
	{
	public:
		static constexpr std::string_view interface_name{"VENGINE_DEDICATEDEXPORTS_API_VERSION003"};

		virtual void Sys_Printf(const char *) = 0;
		virtual void RunServer() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual bool IsGuiDedicatedServer() = 0;
	#endif
	};
	#pragma GCC diagnostic pop
}
