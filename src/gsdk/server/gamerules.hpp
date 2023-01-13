#pragma once

#include "../config.hpp"

namespace gsdk
{
	class CGameRules
	{
	public:
		
	};

#if GSDK_ENGINE == GSDK_ENGINE_TF2
	class CTFGameRules : public CGameRules
	{
	public:
		
	};
#elif GSDK_ENGINE == GSDK_ENGINE_PORTAL2
	class CPortalGameRules : public CGameRules
	{
	public:
		
	};

	class CPortalMPGameRules : public CGameRules
	{
	public:
		
	};
#endif
}
