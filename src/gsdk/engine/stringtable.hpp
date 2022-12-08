#pragma once

#include <string_view>

namespace gsdk
{
	static constexpr auto DOWNLOADABLE_FILE_TABLENAME{"downloadables"};
	static constexpr auto MODEL_PRECACHE_TABLENAME{"modelprecache"};
	static constexpr auto GENERIC_PRECACHE_TABLENAME{"genericprecache"};
	static constexpr auto SOUND_PRECACHE_TABLENAME{"soundprecache"};
	static constexpr auto DECAL_PRECACHE_TABLENAME{"decalprecache"};

	class INetworkStringTable;

	using TABLEID = int;
	using pfnStringChanged = void(*)(void *, INetworkStringTable *, int, const char *, const void *);

	class INetworkStringTable
	{
	public:
		virtual ~INetworkStringTable() = 0;
		virtual const char *GetTableName() const = 0;
		virtual TABLEID GetTableId() const = 0;
		virtual int GetNumStrings() const = 0;
		virtual int GetMaxStrings() const = 0;
		virtual int GetEntryBits() const = 0;
		virtual void SetTick(int) = 0;
		virtual bool ChangedSinceTick(int) const = 0;
		virtual int AddString(bool, const char *, int = -1, const void * = nullptr) = 0; 
		virtual const char *GetString(int) = 0;
		virtual void SetStringUserData(int, int, const void *) = 0;
		virtual const void *GetStringUserData(int, int *) = 0;
		virtual int FindStringIndex(const char *) = 0;
		virtual void SetStringChangedCallback(void *, pfnStringChanged) = 0;
	};

	class INetworkStringTableContainer
	{
	public:
		virtual ~INetworkStringTableContainer() = 0;
		virtual INetworkStringTable *CreateStringTable(const char *, int, int = 0, int = 0) = 0;
		virtual void RemoveAllTables() = 0;
		virtual INetworkStringTable *FindTable(const char *) const = 0;
		virtual INetworkStringTable *GetTable(TABLEID) const = 0;
		virtual int GetNumTables() const = 0;
		virtual INetworkStringTable *CreateStringTableEx(const char *, int, int = 0, int = 0, bool = false) = 0;
		virtual void SetAllowClientSideAddString(INetworkStringTable *, bool) = 0;
	};

	class IServerNetworkStringTableContainer : public INetworkStringTableContainer
	{
	public:
		static constexpr std::string_view interface_name{"VEngineServerStringTable001"};
	};

	class IClientNetworkStringTableContainer : public INetworkStringTableContainer
	{
	public:
		static constexpr std::string_view interface_name{"VEngineClientStringTable001"};
	};
}
