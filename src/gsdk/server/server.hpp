#pragma once

#include "../tier1/interface.hpp"
#include "../mathlib/vector.hpp"

namespace gsdk
{
	class CBaseEntity;
	class IServerEntity;
	class IClientEntity;
	class IEntityFactoryDictionary;
	class CBaseTempEntity;
	class ITempEntsSystem;
	class CGlobalEntityList;
	class CBaseAnimating;
	class CTakeDamageInfo;
	class IEntityFindFilter;
	class IPlayerInfo;
	class CGlobalVars;
	enum ePrepareLevelResourcesResult : int;
	enum eCanProvideLevelResult : int;
	class IServerGCLobby;
	class CStandardSendProxies;
	class CSaveRestoreData;
	struct datamap_t;
	struct typedescription_t;
	class ServerClass;
	using QueryCvarCookie_t = int;
	enum EQueryCvarValueStatus : int;
	struct WorkshopMapDesc_t;
	class IEntityFactory;
	class IServerNetworkable;
	struct edict_t;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IPlayerInfoManager
	{
	public:
		static constexpr std::string_view interface_name{"PlayerInfoManager002"};

		virtual IPlayerInfo *GetPlayerInfo(edict_t *pEdict) = 0;
		virtual CGlobalVars *GetGlobalVars() = 0;
	};
	#pragma GCC diagnostic pop

	class IServerTools : public IBaseInterface
	{
	public:
		static constexpr std::string_view interface_name{"VSERVERTOOLS003"};

		virtual IServerEntity *GetIServerEntity(IClientEntity *) = 0;
		virtual bool SnapPlayerToPosition(const Vector &, const QAngle &, IClientEntity * = nullptr) = 0;
		virtual bool GetPlayerPosition(Vector &, QAngle &, IClientEntity * = nullptr) = 0;
		virtual bool SetPlayerFOV(int, IClientEntity * = nullptr ) = 0;
		virtual int GetPlayerFOV(IClientEntity * = nullptr ) = 0;
		virtual bool IsInNoClipMode(IClientEntity * = nullptr ) = 0;
		virtual CBaseEntity *FirstEntity() = 0;
		virtual CBaseEntity *NextEntity(CBaseEntity *) = 0;
		virtual CBaseEntity *FindEntityByHammerID(int) = 0;
		virtual bool GetKeyValue(CBaseEntity *, const char *, char *, int) = 0;
		virtual bool SetKeyValue(CBaseEntity *, const char *, const char *) = 0;
		virtual bool SetKeyValue(CBaseEntity *, const char *, float) = 0;
		virtual bool SetKeyValue(CBaseEntity *, const char *, const Vector &) = 0;
		virtual CBaseEntity *CreateEntityByName(const char *) = 0;
		virtual void DispatchSpawn(CBaseEntity *) = 0;
		virtual void ReloadParticleDefintions(const char *, const void *, int) = 0;
		virtual void AddOriginToPVS(const Vector &) = 0;
		virtual void MoveEngineViewTo(const Vector &, const QAngle &) = 0;
		virtual bool DestroyEntityByHammerId(int) = 0;
		virtual CBaseEntity *GetBaseEntityByEntIndex(int) = 0;
		virtual void RemoveEntity(CBaseEntity *) = 0;
		virtual void RemoveEntityImmediate(CBaseEntity *) = 0;
		virtual IEntityFactoryDictionary *GetEntityFactoryDictionary() = 0;
		virtual void SetMoveType(CBaseEntity *, int) = 0;
		virtual void SetMoveType(CBaseEntity *, int, int) = 0;
		virtual void ResetSequence(CBaseAnimating *, int) = 0;
		virtual void ResetSequenceInfo(CBaseAnimating *) = 0;
		virtual void ClearMultiDamage() = 0;
		virtual void ApplyMultiDamage() = 0;
		virtual void AddMultiDamage(const CTakeDamageInfo &, CBaseEntity *) = 0;
		virtual void RadiusDamage(const CTakeDamageInfo &, const Vector &, float, int, CBaseEntity *) = 0;
		virtual ITempEntsSystem *GetTempEntsSystem() = 0;
		virtual CBaseTempEntity *GetTempEntList() = 0;
		virtual CGlobalEntityList *GetEntityList() = 0;
		virtual bool IsEntityPtr(void *) = 0;
		virtual CBaseEntity *FindEntityByClassname(CBaseEntity *, const char *) = 0;
		virtual CBaseEntity *FindEntityByName(CBaseEntity *, const char *, CBaseEntity * = nullptr, CBaseEntity * = nullptr, CBaseEntity * = nullptr, IEntityFindFilter * = nullptr) = 0;
		virtual CBaseEntity *FindEntityInSphere(CBaseEntity *, const Vector &, float) = 0;
		virtual CBaseEntity *FindEntityByTarget(CBaseEntity *, const char *) = 0;
		virtual CBaseEntity *FindEntityByModel(CBaseEntity *, const char *) = 0;
		virtual CBaseEntity *FindEntityByNameNearest(const char *, const Vector &, float, CBaseEntity * = nullptr, CBaseEntity * = nullptr, CBaseEntity * = nullptr) = 0;
		virtual CBaseEntity *FindEntityByNameWithin(CBaseEntity *, const char *, const Vector &, float, CBaseEntity * = nullptr, CBaseEntity * = nullptr, CBaseEntity * = nullptr) = 0;
		virtual CBaseEntity *FindEntityByClassnameNearest(const char *, const Vector &, float) = 0;
		virtual CBaseEntity *FindEntityByClassnameWithin(CBaseEntity *, const char *, const Vector &, float) = 0;
		virtual CBaseEntity *FindEntityByClassnameWithin(CBaseEntity *, const char *, const Vector &, const Vector &) = 0;
		virtual CBaseEntity *FindEntityGeneric(CBaseEntity *, const char *, CBaseEntity * = nullptr, CBaseEntity * = nullptr, CBaseEntity * = nullptr) = 0;
		virtual CBaseEntity *FindEntityGenericWithin(CBaseEntity *, const char *, const Vector &, float, CBaseEntity * = nullptr, CBaseEntity * = nullptr, CBaseEntity * = nullptr) = 0;
		virtual CBaseEntity *FindEntityGenericNearest(const char *, const Vector &, float, CBaseEntity * = nullptr, CBaseEntity * = nullptr, CBaseEntity * = nullptr) = 0;
		virtual CBaseEntity *FindEntityNearestFacing(const Vector &, const Vector &, float) = 0;
		virtual CBaseEntity *FindEntityClassNearestFacing(const Vector &, const Vector &, float, char *) = 0;
		virtual CBaseEntity *FindEntityProcedural(const char *, CBaseEntity * = nullptr, CBaseEntity * = nullptr, CBaseEntity * = nullptr) = 0;
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IServerGameDLL
	{
	public:
		static constexpr std::string_view interface_name{"ServerGameDLL012"};

		virtual bool DLLInit(CreateInterfaceFn, CreateInterfaceFn, CreateInterfaceFn, CGlobalVars *) = 0;
		virtual bool ReplayInit(CreateInterfaceFn) = 0;
		virtual bool GameInit() = 0;
		virtual bool LevelInit(const char *, const char *, const char *, const char *, bool, bool) = 0;
		virtual void ServerActivate(edict_t *, int, int) = 0;
		virtual void GameFrame(bool) = 0;
		virtual void PreClientUpdate(bool) = 0;
		virtual void LevelShutdown() = 0;
		virtual void GameShutdown() = 0;
		virtual void DLLShutdown() = 0;
		virtual float GetTickInterval() const = 0;
		virtual ServerClass *GetAllServerClasses() = 0;
		virtual const char *GetGameDescription() = 0;
		virtual void CreateNetworkStringTables() = 0;
		virtual CSaveRestoreData *SaveInit(int) = 0;
		virtual void SaveWriteFields(CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int) = 0;
		virtual void SaveReadFields(CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int) = 0;
		virtual void SaveGlobalState(CSaveRestoreData *) = 0;
		virtual void RestoreGlobalState(CSaveRestoreData *) = 0;
		virtual void PreSave(CSaveRestoreData *) = 0;
		virtual void Save(CSaveRestoreData *) = 0;
		virtual void GetSaveComment(char *, int, float, float, bool = false) = 0;
		virtual void WriteSaveHeaders(CSaveRestoreData *) = 0;
		virtual void ReadRestoreHeaders(CSaveRestoreData *) = 0;
		virtual void Restore(CSaveRestoreData *, bool) = 0;
		virtual bool IsRestoring() = 0;
		virtual int CreateEntityTransitionList(CSaveRestoreData *, int) = 0;
		virtual void BuildAdjacentMapList() = 0;
		virtual bool GetUserMessageInfo(int, char *, int, int &) = 0;
		virtual CStandardSendProxies *GetStandardSendProxies() = 0;
		virtual void PostInit() = 0;
		virtual void Think(bool) = 0;
		virtual void PreSaveGameLoaded(const char *, bool) = 0;
		virtual bool ShouldHideServer() = 0;
		virtual void InvalidateMdlCache() = 0;
		virtual void OnQueryCvarValueFinished(QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char *) = 0;
		virtual void GameServerSteamAPIActivated() = 0;
		virtual void GameServerSteamAPIShutdown() = 0;
		virtual void SetServerHibernation(bool) = 0;
		virtual IServerGCLobby *GetServerGCLobby() = 0;
		virtual const char *GetServerBrowserMapOverride() = 0;
		virtual const char *GetServerBrowserGameData() = 0;
		virtual void Status(void (*)(const char *, ...)) = 0;
		virtual void PrepareLevelResources(char *, size_t, char *, size_t) = 0;
		virtual ePrepareLevelResourcesResult AsyncPrepareLevelResources(char *, size_t, char *, size_t, float * = nullptr) = 0;
		virtual eCanProvideLevelResult CanProvideLevel(char *, int) = 0;
		virtual bool IsManualMapChangeOkay(const char **) = 0;
		virtual bool GetWorkshopMap(unsigned int, WorkshopMapDesc_t *) = 0;
	};
	#pragma GCC diagnostic pop

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IEntityFactoryDictionary
	{
	public:
		virtual void InstallFactory(IEntityFactory *, const char *) = 0;
		virtual IServerNetworkable *Create(const char *) = 0;
		virtual void Destroy(const char *, IServerNetworkable *) = 0;
		virtual IEntityFactory *FindFactory(const char *) = 0;
		virtual const char *GetCannonicalName(const char *) = 0;
	};
	#pragma GCC diagnostic pop
}
