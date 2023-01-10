#pragma once

#pragma push_macro("__VMOD_COMPILING_GSDK")
#define __VMOD_COMPILING_GSDK
#include "../vscript/vscript.hpp"
#undef __VMOD_COMPILING_GSDK
#pragma pop_macro("__VMOD_COMPILING_GSDK")

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
		static std::size_t vtable_size;
		static std::size_t GetServerClass_vindex;

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
		virtual engine::string_t GetModelName() const = 0;
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
		virtual ServerClass *GetServerClass() = 0;
		virtual void YouForgotToImplementOrDeclareServerClass() = 0;
		virtual datamap_t *GetDataDescMap() = 0;
		virtual ScriptClassDesc_t *GetScriptDesc() = 0;

		static std::size_t UpdateOnRemove_vindex;
		static std::size_t GetDataDescMap_vindex;
		static std::size_t GetServerClass_vindex;

		static ScriptClassDesc_t *g_pScriptDesc;
		static HSCRIPT (CBaseEntity::*GetScriptInstance_ptr)();

		HSCRIPT GetScriptInstance() noexcept;
		static CBaseEntity *from_instance(HSCRIPT instance) noexcept;
	};

	template <typename T>
	class CHandle;

	class CBaseHandle
	{
	public:
		CBaseHandle() noexcept = default;
		CBaseHandle(const CBaseHandle &) noexcept = default;
		CBaseHandle &operator=(const CBaseHandle &) noexcept = default;

		inline CBaseHandle(CBaseHandle &&other) noexcept
		{ operator=(std::move(other)); }
		inline CBaseHandle &operator=(CBaseHandle &&other) noexcept
		{
			m_Index = other.m_Index;
			other.m_Index = INVALID_EHANDLE_INDEX;
			return *this;
		}

		bool operator==(const CBaseHandle &) const noexcept = default;
		bool operator!=(const CBaseHandle &) const noexcept = default;

		template <typename T>
		inline operator const CHandle<T> &() const noexcept
		{ return *static_cast<const CHandle<T> *>(this); }
		template <typename T>
		inline operator CHandle<T> &() noexcept
		{ return *static_cast<CHandle<T> *>(this); }

		unsigned long m_Index{INVALID_EHANDLE_INDEX};
	};

	template <typename T>
	class alignas(CBaseHandle) CHandle : public CBaseHandle
	{
	public:
		using CBaseHandle::CBaseHandle;
		using CBaseHandle::operator=;

		bool operator==(const CHandle &) const noexcept = default;
		bool operator!=(const CHandle &) const noexcept = default;

		template <typename U>
		inline bool operator==(const CHandle<U> &other) const noexcept
		{
			static_assert(std::is_base_of_v<U, T>);
			return CBaseHandle::operator==(static_cast<const CBaseHandle &>(other));
		}

		template <typename U>
		inline bool operator!=(const CHandle<U> &other) const noexcept
		{
			static_assert(std::is_base_of_v<U, T>);
			return CBaseHandle::operator!=(static_cast<const CBaseHandle &>(other));
		}
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

namespace std
{
	template <>
	struct hash<gsdk::CBaseHandle> : public hash<unsigned long>
	{
		inline size_t operator()(const gsdk::CBaseHandle &other) const noexcept
		{ return hash<unsigned long>::operator()(other.m_Index); }
	};

	template <typename T>
	struct hash<gsdk::CHandle<T>> : public hash<gsdk::CBaseHandle>
	{
		inline size_t operator()(const gsdk::CHandle<T> &other) const noexcept
		{ return hash<gsdk::CBaseHandle>::operator()(static_cast<const gsdk::CBaseHandle &>(other)); }
	};
}
