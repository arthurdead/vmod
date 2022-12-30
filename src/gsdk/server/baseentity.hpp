#pragma once

#include "../vscript/vscript.hpp"
#include "../string_t.hpp"
#include "../engine/dt_send.hpp"
#include "../engine/sv_engine.hpp"
#include "../tier1/utldict.hpp"
#include "../engine/stringtable.hpp"

namespace gsdk
{
	class ICollideable;
	class IServerNetworkable;
	class CBaseEntity;
	class CBaseHandle;
	class CBaseNetworkable;
	class IHandleEntity;
	struct edict_t;
	class ServerClass;
	struct PVSInfo_t;
	class SendTable;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IServerNetworkable
	{
	public:
		virtual IHandleEntity *GetEntityHandle() = 0;
		virtual ServerClass *GetServerClass() = 0;
		virtual edict_t *GetEdict() const = 0;
		virtual const char *GetClassName() const = 0;
		virtual void Release() = 0;
		virtual int AreaNum() const = 0;
		virtual CBaseNetworkable *GetBaseNetworkable() = 0;
		virtual CBaseEntity *GetBaseEntity() = 0;
		virtual PVSInfo_t *GetPVSInfo() = 0;
	};
	#pragma GCC diagnostic pop

	class IHandleEntity
	{
	public:
		virtual ~IHandleEntity() = 0;
		virtual void SetRefEHandle(const CBaseHandle &) = 0;
		virtual const CBaseHandle &GetRefEHandle() const = 0;
	};

	class IServerUnknown : public IHandleEntity
	{
	public:
		virtual ICollideable *GetCollideable() = 0;
		virtual IServerNetworkable *GetNetworkable() = 0;
		virtual CBaseEntity *GetBaseEntity() = 0;
	};

	class IServerEntity : public IServerUnknown
	{
	public:
		virtual int GetModelIndex() const = 0;
		virtual string_t GetModelName() const = 0;
		virtual void SetModelIndex(int) = 0;
	};

	class ServerClass
	{
	public:
		const char *m_pNetworkName{nullptr};
		SendTable *m_pTable{nullptr};
		ServerClass *m_pNext{nullptr};
		int m_ClassID{-1};
		int m_InstanceBaselineIndex{INVALID_STRING_INDEX};

	private:
		ServerClass() = delete;
		ServerClass(const ServerClass &) = delete;
		ServerClass &operator=(const ServerClass &) = delete;
		ServerClass(ServerClass &&) = delete;
		ServerClass &operator=(ServerClass &&) = delete;
	};

	class CBaseEntity : public IServerEntity
	{
	public:
		static ScriptClassDesc_t *g_pScriptDesc;
		static HSCRIPT (CBaseEntity::*GetScriptInstance_ptr)();

		HSCRIPT GetScriptInstance() noexcept;
		static CBaseEntity *from_instance(HSCRIPT instance) noexcept;
	};

	class CBaseHandle
	{
	public:
		CBaseHandle() noexcept = default;

		unsigned long m_Index{INVALID_EHANDLE_INDEX};

	private:
		CBaseHandle(const CBaseHandle &) = delete;
		CBaseHandle &operator=(const CBaseHandle &) = delete;
		CBaseHandle(CBaseHandle &&) = delete;
		CBaseHandle &operator=(CBaseHandle &&) = delete;
	};

	template <typename T>
	class CHandle : public CBaseHandle
	{
	};

	using EHANDLE = CHandle<CBaseEntity>;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IEntityFactory
	{
	public:
		virtual IServerNetworkable *Create(const char *) = 0;
		virtual void Destroy(IServerNetworkable *net);
		virtual size_t GetEntitySize() = 0;
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

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class CEntityFactoryDictionary : public IEntityFactoryDictionary
	{
	public:
		CUtlDict<IEntityFactory *, unsigned short> m_Factories;
	};
	#pragma GCC diagnostic pop
}
