#pragma once

#include <string_view>
#include "../mathlib/vector.hpp"
#include "../config.hpp"
#include "../tier1/utlstring.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wredundant-tags"
#pragma GCC diagnostic ignored "-Wconditionally-supported"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <steam/steam_gameserver.h>
#pragma GCC diagnostic pop

namespace gsdk
{
#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
	constexpr int ABSOLUTE_PLAYER_LIMIT{255};
#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	constexpr int ABSOLUTE_PLAYER_LIMIT{64};
#else
	#error
#endif

	using QueryCvarCookie_t = int;
	enum eFindMapResult : int;
	class INetChannelInfo;
	struct edict_t;
	struct bf_write;
	class IRecipientFilter;
	class SendTable;
	struct client_textmessage_t;
	struct con_nprint_s;
	class ISpatialPartition;
	class ICollideable;
	class IScratchPad3D;
	class CCheckTransmitInfo;
	class CSharedEdictChangeInfo;
	struct IChangeInfoAccessor;
	class IAchievementMgr;
	class IServer;
	enum soundlevel_t : int;
	class ServerClass;
	class VPlane;
	struct PVSInfo_t;
	struct player_info_t;
	class CGamestatsData;
	class KeyValues;
	struct bbox_t;
	template <int S>
	class CBitVec;
	class ISPSharedMemory;
	template <typename T>
	class CUtlVector;
	struct model_t;
	enum CrossPlayPlatform_t : int;

	constexpr int MAX_EDICT_BITS{11};
	constexpr int MAX_EDICTS{1 << MAX_EDICT_BITS};

	constexpr int NUM_ENT_ENTRY_BITS{MAX_EDICT_BITS + 2};
	constexpr int NUM_ENT_ENTRIES{1 << NUM_ENT_ENTRY_BITS};

	constexpr int NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS{10};
	constexpr int NUM_NETWORKED_EHANDLE_BITS{MAX_EDICT_BITS + NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS};
	constexpr int INVALID_NETWORKED_EHANDLE_VALUE{(1 << NUM_NETWORKED_EHANDLE_BITS) - 1};

	constexpr unsigned long INVALID_EHANDLE_INDEX{0xFFFFFFFF};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IVEngineServer
	{
	public:
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		static constexpr std::string_view interface_name{"VEngineServer023"};
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		static constexpr std::string_view interface_name{"VEngineServer022"};
	#else
		#error
	#endif

		virtual void ChangeLevel(const char *, const char *) = 0;
		virtual int IsMapValid(const char *) = 0;
		virtual bool IsDedicatedServer() = 0;
		virtual int IsInEditMode() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual KeyValues *GetLaunchOptions() = 0;
	#endif
		virtual int PrecacheModel(const char *, bool = false ) = 0;
		virtual int PrecacheSentenceFile(const char *, bool = false ) = 0;
		virtual int PrecacheDecal(const char *, bool = false ) = 0;
		virtual int PrecacheGeneric( const char *, bool = false ) = 0;
		virtual bool IsModelPrecached(const char *) const = 0;
		virtual bool IsDecalPrecached(const char *) const = 0;
		virtual bool IsGenericPrecached(const char *) const = 0;
		virtual int GetClusterForOrigin(const Vector &) = 0;
		virtual int GetPVSForCluster(int, int, unsigned char *) = 0;
		virtual bool CheckOriginInPVS(const Vector &, const unsigned char *, int) = 0;
		virtual bool CheckBoxInPVS(const Vector &, const Vector &, const unsigned char *, int) = 0;
		virtual int GetPlayerUserId(const edict_t *) = 0; 
		virtual const char *GetPlayerNetworkIDString(const edict_t *) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual bool IsUserIDInUse(int) = 0;
		virtual int GetLoadingProgressForUserID(int) = 0;
	#endif
		virtual int GetEntityCount() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual int IndexOfEdict(const edict_t *) = 0;
		virtual edict_t *PEntityOfEntIndex(int) = 0;
	#endif
		virtual INetChannelInfo *GetPlayerNetInfo(int) = 0;
		virtual edict_t *CreateEdict(int = -1) = 0;
		virtual void RemoveEdict(edict_t *) = 0;
		virtual void *PvAllocEntPrivateData(long cb) = 0;
		virtual void FreeEntPrivateData(void *) = 0;
		virtual void *SaveAllocMemory(size_t, size_t) = 0;
		virtual void SaveFreeMemory(void *) = 0;
		virtual void EmitAmbientSound(int, const Vector &, const char *, float, soundlevel_t, int, int, float = 0.0f) = 0;
		virtual void FadeClientVolume(const edict_t *, float, float, float, float) = 0;
		virtual int SentenceGroupPick(int, char *, int) = 0;
		virtual int SentenceGroupPickSequential(int, char *, int, int, int) = 0;
		virtual int SentenceIndexFromName(const char *) = 0;
		virtual const char *SentenceNameFromIndex(int) = 0;
		virtual int SentenceGroupIndexFromName(const char *) = 0;
		virtual const char *SentenceGroupNameFromIndex(int) = 0;
		virtual float SentenceLength(int) = 0;
		virtual void ServerCommand(const char *) = 0;
		virtual void ServerExecute() = 0;
		virtual void ClientCommand(edict_t *, const char *, ...) = 0;
		virtual void LightStyle(int, const char *) = 0;
		virtual void StaticDecal(const Vector &, int, int, int, bool) = 0;
		virtual void Message_DetermineMulticastRecipients(bool, const Vector &, CBitVec<ABSOLUTE_PLAYER_LIMIT> &) = 0;
		virtual bf_write *EntityMessageBegin(int, ServerClass *, bool) = 0;
		virtual bf_write *UserMessageBegin(IRecipientFilter *, int) = 0;
		virtual void MessageEnd() = 0;
		virtual void ClientPrintf(edict_t *, const char *) = 0;
		virtual void Con_NPrintf(int, const char *, ...) = 0;
		virtual void Con_NXPrintf(const con_nprint_s *info, const char *, ...) = 0;
		virtual void SetView(const edict_t *, const edict_t *) = 0;
	#if !(GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1))
		virtual float Time() = 0;
	#endif
		virtual void CrosshairAngle(const edict_t *, float, float) = 0;
		virtual void GetGameDir(char *, int) = 0;
		virtual int CompareFileTime(const char *, const char *, int *) = 0;
		virtual bool LockNetworkStringTables(bool) = 0;
		virtual edict_t *CreateFakeClient(const char *) = 0;
		virtual const char *GetClientConVarValue(int, const char *) = 0;
		virtual const char *ParseFile(const char *, char *, int) = 0;
		virtual bool CopyFile(const char *, const char *) = 0;
		virtual void ResetPVS(unsigned char *, int) = 0;
		virtual void AddOriginToPVS(const Vector &) = 0;
		virtual void SetAreaPortalState(int, int) = 0;
		virtual void PlaybackTempEntity(IRecipientFilter &filter, float, const void *, const SendTable *, int) = 0;
		virtual int CheckHeadnodeVisible(int, const unsigned char *, int) = 0;
		virtual int CheckAreasConnected(int, int) = 0;
		virtual int GetArea(const Vector &) = 0;
		virtual void GetAreaBits(int, unsigned char *, int) = 0;
		virtual bool GetAreaPortalPlane(const Vector &, int, VPlane *) = 0;
		virtual bool LoadGameState(const char *, bool) = 0;
		virtual void LoadAdjacentEnts(const char *, const char *) = 0;
		virtual void ClearSaveDir() = 0;
		virtual const char *GetMapEntitiesString() = 0;
		virtual client_textmessage_t *TextMessageGet(const char *) = 0;
		virtual void LogPrint(const char *) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual bool IsLogEnabled() = 0;
	#endif
		virtual void BuildEntityClusterList(edict_t *, PVSInfo_t *) = 0;
		virtual void SolidMoved(edict_t *, ICollideable *, const Vector *, bool) = 0;
		virtual void TriggerMoved(edict_t *, bool) = 0;
		virtual ISpatialPartition *CreateSpatialPartition(const Vector &, const Vector &) = 0;
		virtual void DestroySpatialPartition(ISpatialPartition *) = 0;
		virtual void DrawMapToScratchPad(IScratchPad3D *, unsigned long) = 0;
		virtual const CBitVec<MAX_EDICTS> *GetEntityTransmitBitsForClient(int) = 0;
		virtual bool IsPaused() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual float GetTimescale() const = 0;
	#endif
		virtual void ForceExactFile(const char *) = 0;
		virtual void ForceModelBounds(const char *, const Vector &, const Vector &) = 0;
		virtual void ClearSaveDirAfterClientLoad() = 0;
		virtual void SetFakeClientConVarValue(edict_t *, const char *, const char *) = 0;
		virtual void ForceSimpleMaterial(const char *) = 0;
		virtual int IsInCommentaryMode() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual bool IsLevelMainMenuBackground() = 0;
	#endif
		virtual void SetAreaPortalStates(const int *, const int *, int) = 0;
		virtual void NotifyEdictFlagsChange(int) = 0;
		virtual const CCheckTransmitInfo *GetPrevCheckTransmitInfo(edict_t *) = 0;
		virtual CSharedEdictChangeInfo *GetSharedEdictChangeInfo() = 0;
		virtual void AllowImmediateEdictReuse() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual bool IsInternalBuild() = 0;
	#endif
		virtual IChangeInfoAccessor *GetChangeAccessor(const edict_t *) = 0;
		virtual const char *GetMostRecentlyLoadedFileName() = 0;
		virtual const char *GetSaveFileName() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void WriteSavegameScreenshot(const char *) = 0;
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual void MultiplayerEndGame() = 0;
		virtual void ChangeTeam(const char *) = 0;
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual int GetLightForPointListenServerOnly(const Vector &, bool, Vector *) = 0;
		virtual int TraceLightingListenServerOnly(const Vector &, const Vector &, Vector *, Vector *) = 0;
	#endif
		virtual void CleanUpEntityClusterList(PVSInfo_t *) = 0;
		virtual void SetAchievementMgr(IAchievementMgr *) =0;
		virtual IAchievementMgr *GetAchievementMgr() = 0;
		virtual int GetAppID() = 0;
		virtual bool IsLowViolence() = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual bool IsAnyClientLowViolence() = 0;
	#endif
		virtual QueryCvarCookie_t StartQueryCvarValue(edict_t *, const char *) = 0;
		virtual void InsertServerCommand(const char *) = 0;
		virtual bool GetPlayerInfo(int, player_info_t *) = 0;
		virtual bool IsClientFullyAuthenticated(edict_t *) = 0;
		virtual void SetDedicatedServerBenchmarkMode(bool) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual bool IsSplitScreenPlayer(int) = 0;
		virtual edict_t *GetSplitScreenPlayerAttachToEdict(int) = 0;
		virtual int	GetNumSplitScreenUsersAttachedToEdict(int) = 0;
		virtual edict_t *GetSplitScreenPlayerForEdict(int, int) = 0;
		virtual bool IsOverrideLoadGameEntsOn() = 0;
		virtual void ForceFlushEntity(int) = 0;
		virtual ISPSharedMemory *GetSinglePlayerSharedMemorySpace(const char *, int = MAX_EDICTS) = 0;
		virtual void *AllocLevelStaticData(size_t) = 0;
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual void SetGamestatsData(CGamestatsData *) = 0;
		virtual CGamestatsData *GetGamestatsData() = 0;
		virtual const CSteamID *GetClientSteamID(edict_t *) = 0;
		virtual const CSteamID *GetGameServerSteamID() = 0;
		virtual void ClientCommandKeyValues(edict_t *, KeyValues *) = 0;
		virtual const CSteamID *GetClientSteamIDByPlayerIndex(int) = 0;
	#endif
		virtual int GetClusterCount() = 0;
		virtual int GetAllClusterBounds(bbox_t *, int) = 0;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual bool IsCreatingReslist() = 0;
		virtual bool IsCreatingXboxReslist() = 0;
		virtual bool IsDedicatedServerForXbox() = 0;
		virtual void Pause(bool, bool = false ) = 0;
		virtual void SetTimescale(float) = 0;
		virtual void SetGamestatsData(CGamestatsData *) = 0;
		virtual CGamestatsData *GetGamestatsData() = 0;
		virtual const CSteamID *GetClientSteamID(edict_t *) = 0;
		virtual void HostValidateSession() = 0;
		virtual void RefreshScreenIfNecessary() = 0;
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, <=, GSDK_ENGINE_BRANCH_2010_V0)
		virtual void *AllocLevelStaticDataName(unsigned, const char *) = 0;
		#endif
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		virtual bool HasPaintmap() = 0;
		virtual bool SpherePaintSurface(const model_t *, const Vector &, unsigned char, float, float) = 0;
		virtual void SphereTracePaintSurface(const model_t *, const Vector &, const Vector &, float, CUtlVector<unsigned char> &) = 0;
		virtual void RemoveAllPaint() = 0;
		virtual void PaintAllSurfaces(unsigned char) = 0;
		virtual void RemovePaint(const model_t *) = 0;
		#endif
		virtual void ClientCommandKeyValues(edict_t *, KeyValues *) = 0;
		virtual unsigned long long GetClientXUID(edict_t *) = 0;
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		virtual edict_t *CreateFakeClientEx(const char *, bool = true) = 0;
		virtual int GetServerVersion() const = 0;
		virtual float GetServerTime() const = 0;
		virtual IServer *GetIServer() = 0;
		virtual bool IsPlayerNameLocked(const edict_t *) = 0;
		virtual bool CanPlayerChangeName(const edict_t *) = 0;
		virtual eFindMapResult FindMap(char *, int) = 0;
		virtual void SetPausedForced(bool, float = -1.0f) = 0;
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		virtual bool IsActiveApp() = 0;
		virtual void SetNoClipEnabled(bool) = 0;
		virtual void GetPaintmapDataRLE(CUtlVector<unsigned int> &) = 0;
		virtual void LoadPaintmapDataRLE(const CUtlVector<unsigned int> &) = 0;
		virtual void SendPaintmapDataToClient(edict_t *) = 0;
		virtual float GetLatencyForChoreoSounds() = 0;
		virtual CrossPlayPlatform_t GetClientCrossPlayPlatform(int) = 0;
	#endif
	};
	#pragma GCC diagnostic pop

	class CSteam3Server : public CSteamGameServerAPIContext
	{
	};
}
