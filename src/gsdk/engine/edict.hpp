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

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		#ifdef GSDK_LITTLE_ENDIAN
		short m_NetworkSerialNumber;
		short m_EdictIndex;
		#else
		short m_EdictIndex;
		short m_NetworkSerialNumber;
		#endif
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		int m_NetworkSerialNumber;
	#else
		#error
	#endif

		IServerNetworkable *m_pNetworkable;

		IServerUnknown *m_pUnk;
	};

	struct edict_t : public CBaseEdict
	{
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		float freetime;
	#endif
	};
}