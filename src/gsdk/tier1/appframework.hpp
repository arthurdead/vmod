#pragma once

#include "interface.hpp"
#include "../config.hpp"

namespace gsdk
{
	struct AppSystemInfo_t;
	enum AppSystemTier_t : int;
	enum InitReturnVal_t : int;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IAppSystem
	{
	public:
		virtual bool Connect(CreateInterfaceFn) = 0;
		virtual void Disconnect() = 0;
		virtual void *QueryInterface(const char *) = 0;
		virtual InitReturnVal_t Init() = 0;
		virtual void Shutdown() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		virtual const AppSystemInfo_t *GetDependencies() = 0;
		virtual AppSystemTier_t GetTier() = 0;
		virtual void Reconnect(CreateInterfaceFn, const char *) = 0;
	#endif
	};
	#pragma GCC diagnostic pop
}
