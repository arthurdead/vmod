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
#endif
}
