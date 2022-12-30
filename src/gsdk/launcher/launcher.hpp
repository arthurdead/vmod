#pragma once

#include "../tier1/interface.hpp"
#include "../config.hpp"

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
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual bool IsGuiDedicatedServer() = 0;
	#endif
	};
	#pragma GCC diagnostic pop
}
