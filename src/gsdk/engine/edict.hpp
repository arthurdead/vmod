#pragma once

#include "../config.hpp"

#define GSDK_LITTLE_ENDIAN

namespace gsdk
{
	class IServerNetworkable;
	class IServerUnknown;

	class CBaseEdict
	{
	public:
		int m_fStateFlags;

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		#ifdef GSDK_LITTLE_ENDIAN
		short m_NetworkSerialNumber;
		short m_EdictIndex;
		#else
		short m_EdictIndex;
		short m_NetworkSerialNumber;
		#endif
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		int m_NetworkSerialNumber;
	#else
		#error
	#endif

		IServerNetworkable *m_pNetworkable;

		IServerUnknown *m_pUnk;
	};

	struct edict_t : public CBaseEdict
	{
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		float freetime;
	#endif
	};
}
