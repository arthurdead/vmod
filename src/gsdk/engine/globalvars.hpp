#pragma once

#include "../string_t.hpp"
#include "../config.hpp"

namespace gsdk
{
	class IServerNetworkable;
	class IServerUnknown;
	class CSaveRestoreData;
	enum MapLoadType_t : int;
	struct edict_t;

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
		engine::string_t mapname;
		int mapversion;
		engine::string_t startspot;
		MapLoadType_t eLoadType;
		bool bMapLoadFailed;
		bool deathmatch;
		bool coop;
		bool teamplay;
		int maxEntities;
		int serverCount;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		edict_t *pEdicts;
	#endif

	private:
		CGlobalVars() = delete;
		CGlobalVars(const CGlobalVars &) = delete;
		CGlobalVars &operator=(const CGlobalVars &) = delete;
		CGlobalVars(CGlobalVars &&) = delete;
		CGlobalVars &operator=(CGlobalVars &&) = delete;
	};
}
