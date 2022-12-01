#pragma once

#include <string_view>
#include "../tier1/interface.hpp"

namespace gsdk
{
	struct edict_t;
	class CCommand;
	class IPlayerInfo;

	enum PLUGIN_RESULT : int
	{
		PLUGIN_CONTINUE,
		PLUGIN_OVERRIDE,
		PLUGIN_STOP
	};

	using QueryCvarCookie_t = int;

	enum EQueryCvarValueStatus : int;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IServerPluginCallbacks
	{
	public:
		static constexpr std::string_view interface_name{"ISERVERPLUGINCALLBACKS003"};

		virtual bool Load(CreateInterfaceFn, CreateInterfaceFn);
		virtual void Unload();
		virtual void Pause();
		virtual void UnPause();
		virtual const char *GetPluginDescription();
		virtual void LevelInit(const char *);
		virtual void ServerActivate(edict_t *, int, int);
		virtual void GameFrame(bool);
		virtual void LevelShutdown();
		virtual void ClientActive(edict_t *);
		virtual void ClientDisconnect(edict_t *);
		virtual void ClientPutInServer(edict_t *, const char *);
		virtual void SetCommandClient(int);
		virtual void ClientSettingsChanged(edict_t *);
		virtual PLUGIN_RESULT ClientConnect(bool *, edict_t *, const char *, const char *, char *, int);
		virtual PLUGIN_RESULT ClientCommand(edict_t *, const CCommand &);
		virtual PLUGIN_RESULT NetworkIDValidated(const char *, const char *);
		virtual void OnQueryCvarValueFinished(QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char *);
		virtual void OnEdictAllocated(edict_t *);
		virtual void OnEdictFreed(const edict_t *);
	};
	#pragma GCC diagnostic pop
}
