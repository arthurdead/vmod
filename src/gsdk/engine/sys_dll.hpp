#pragma once

#include "../tier0/dbg.hpp"
#include "../config.hpp"

namespace gsdk
{
#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class CEngineConsoleLoggingListener : public ILoggingListener
	{
	};
	#pragma GCC diagnostic pop
#endif
}
