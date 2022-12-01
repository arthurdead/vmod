#include "vsp.hpp"

namespace gsdk
{
	bool IServerPluginCallbacks::Load(CreateInterfaceFn, CreateInterfaceFn) { return false; }
	void IServerPluginCallbacks::Unload() {}
	void IServerPluginCallbacks::Pause() {}
	void IServerPluginCallbacks::UnPause() {}
	const char *IServerPluginCallbacks::GetPluginDescription() { return ""; }
	void IServerPluginCallbacks::LevelInit(const char *) {}
	void IServerPluginCallbacks::ServerActivate(edict_t *, int, int) {}
	void IServerPluginCallbacks::GameFrame(bool) {}
	void IServerPluginCallbacks::LevelShutdown() {}
	void IServerPluginCallbacks::ClientActive(edict_t *) {}
	void IServerPluginCallbacks::ClientDisconnect(edict_t *) {}
	void IServerPluginCallbacks::ClientPutInServer(edict_t *, const char *) {}
	void IServerPluginCallbacks::SetCommandClient(int) {}
	void IServerPluginCallbacks::ClientSettingsChanged(edict_t *) {}
	PLUGIN_RESULT IServerPluginCallbacks::ClientConnect(bool *, edict_t *, const char *, const char *, char *, int) { return PLUGIN_CONTINUE; }
	PLUGIN_RESULT IServerPluginCallbacks::ClientCommand(edict_t *, const CCommand &) { return PLUGIN_CONTINUE; }
	PLUGIN_RESULT IServerPluginCallbacks::NetworkIDValidated(const char *, const char *) { return PLUGIN_CONTINUE; }
	void IServerPluginCallbacks::OnQueryCvarValueFinished(QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char *) {}
	void IServerPluginCallbacks::OnEdictAllocated(edict_t *) {}
	void IServerPluginCallbacks::OnEdictFreed(const edict_t *) {}
}
