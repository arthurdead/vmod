#pragma once

#define GSDK_LITTLE_ENDIAN

namespace gsdk
{
	class IServerNetworkable;
	class IServerUnknown;

	class CBaseEdict
	{
	public:
		int m_fStateFlags;

	#ifdef GSDK_LITTLE_ENDIAN
		short m_NetworkSerialNumber;
		short m_EdictIndex;
	#else
		short m_EdictIndex;
		short m_NetworkSerialNumber;
	#endif

		IServerNetworkable *m_pNetworkable;

		IServerUnknown *m_pUnk;
	};

	struct edict_t : public CBaseEdict
	{
		float freetime;
	};
}