#pragma once

#include "../string_t.hpp"

namespace gsdk
{
	class IServerNetworkable;
	class IServerUnknown;
	class CSaveRestoreData;
	enum MapLoadType_t : int;

	class CGlobalVarsBase
	{
	public:
		float realtime;
		int framecount;
		float absoluteframetime;
		float curtime;
		float frametime;
		int maxClients;
		int tickcount;
		float interval_per_tick;
		float interpolation_amount;
		int simTicksThisFrame;
		int network_protocol;
		CSaveRestoreData *pSaveData;
		bool m_bClient;
		int nTimestampNetworkingBase;
		int nTimestampRandomizeWindow;

	private:
		CGlobalVarsBase() = delete;
		CGlobalVarsBase(const CGlobalVarsBase &) = delete;
		CGlobalVarsBase &operator=(const CGlobalVarsBase &) = delete;
		CGlobalVarsBase(CGlobalVarsBase &&) = delete;
		CGlobalVarsBase &operator=(CGlobalVarsBase &&) = delete;
	};

	class CGlobalVars : public CGlobalVarsBase
	{
	public:
		string_t mapname;
		int mapversion;
		string_t startspot;
		MapLoadType_t eLoadType;
		bool bMapLoadFailed;
		bool deathmatch;
		bool coop;
		bool teamplay;
		int maxEntities;
		int serverCount;

	private:
		CGlobalVars() = delete;
		CGlobalVars(const CGlobalVars &) = delete;
		CGlobalVars &operator=(const CGlobalVars &) = delete;
		CGlobalVars(CGlobalVars &&) = delete;
		CGlobalVars &operator=(CGlobalVars &&) = delete;
	};
}
