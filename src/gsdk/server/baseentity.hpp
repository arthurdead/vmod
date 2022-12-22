#pragma once

#include "../vscript/vscript.hpp"
#include "../string_t.hpp"
#include "../engine/dt_send.hpp"
#include "../engine/sv_engine.hpp"

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

	extern IScriptVM *g_pScriptVM;

	class ServerClass
	{
	public:
		const char *m_pNetworkName;
		SendTable *m_pTable;
		ServerClass *m_pNext;
		int m_ClassID;
		int m_InstanceBaselineIndex;
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
		unsigned long m_Index{INVALID_EHANDLE_INDEX};
	};

	template <typename T>
	class CHandle : public CBaseHandle
	{
	};

	using EHANDLE = CHandle<CBaseEntity>;
}
