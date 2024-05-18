#include "gsdk.hpp"
#include "gsdk/server/server.hpp"
#include "gsdk/tier0/commandline.hpp"

namespace vmod
{
	enum environment environment{environment::unknown};

	gsdk::IDedicatedExports *dedicated{nullptr};
	gsdk::IVEngineServer *sv_engine{nullptr};
	gsdk::IServer *sv{nullptr};
	gsdk::IFileSystem *filesystem{nullptr};
	gsdk::ICvar *cvar{nullptr};
	gsdk::IScriptManager *vsmgr{nullptr};
	gsdk::CGlobalVars *sv_globals{nullptr};
	gsdk::IServerGameDLL *gamedll{nullptr};
	gsdk::IServerTools *servertools{nullptr};
	gsdk::CEntityFactoryDictionary *entityfactorydict{nullptr};
	gsdk::IServerNetworkStringTableContainer *sv_stringtables{nullptr};
	std::unordered_map<std::string, gsdk::ServerClass *> sv_classes;
	gsdk::CStandardSendProxies *std_proxies{nullptr};
	gsdk::CSteam3Server &(*Steam3Server)() {nullptr};
#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	gsdk::IWorkshop *workshop{nullptr};
#endif
#ifndef GSDK_NO_SYMBOLS
	bool symbols_available{false};
#endif
	gsdk::ConVar *developer{nullptr};
#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	gsdk::CLoggingSystem *(*GetGlobalLoggingSystem)() {nullptr};
	gsdk::CEngineConsoleLoggingListener *s_EngineLoggingListener{nullptr};
	gsdk::LoggingChannelID_t LOG_General{gsdk::INVALID_LOGGING_CHANNEL_ID};
	gsdk::LoggingChannelID_t LOG_Assert{gsdk::INVALID_LOGGING_CHANNEL_ID};
	gsdk::LoggingChannelID_t LOG_Developer{gsdk::INVALID_LOGGING_CHANNEL_ID};
	gsdk::LoggingChannelID_t LOG_DeveloperConsole{gsdk::INVALID_LOGGING_CHANNEL_ID};
	gsdk::LoggingChannelID_t LOG_Console{gsdk::INVALID_LOGGING_CHANNEL_ID};
	gsdk::LoggingChannelID_t LOG_DeveloperVerbose{gsdk::INVALID_LOGGING_CHANNEL_ID};
	gsdk::LoggingChannelID_t LOG_DownloadManager{gsdk::INVALID_LOGGING_CHANNEL_ID};
	gsdk::LoggingChannelID_t LOG_SeverLog{gsdk::INVALID_LOGGING_CHANNEL_ID};
	gsdk::LoggingChannelID_t sv_LOG_VScript{gsdk::INVALID_LOGGING_CHANNEL_ID};
#endif
#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	gsdk::LoggingChannelID_t LOG_Workshop{gsdk::INVALID_LOGGING_CHANNEL_ID};
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	bool gsdk_tier0_library::load(const std::filesystem::path &path) noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		if(!library::load(path)) {
			return false;
		}

	#ifndef GSDK_NO_SYMBOLS
		if(environment == environment::dedicated_server) {
			std::uint64_t offset{symbol_cache::uncached_find_mangled_func(path, "_Z22GetGlobalLoggingSystemv"sv)};
			if(offset == 0) {
				warning("vmod: missing GetGlobalLoggingSystem func\n"sv);
			} else {
				#pragma GCC diagnostic push
				#pragma GCC diagnostic ignored "-Wconditionally-supported"
				GetGlobalLoggingSystem = reinterpret_cast<decltype(GetGlobalLoggingSystem)>(base() + offset);
				#pragma GCC diagnostic pop
			}
		} else {
	#endif
			warning("vmod: missing GetGlobalLoggingSystem func\n"sv);
	#ifndef GSDK_NO_SYMBOLS
		}
	#endif

		LOG_General = LoggingSystem_FindChannel("General");
		LOG_Assert = LoggingSystem_FindChannel("Assert");
		LOG_Developer = LoggingSystem_FindChannel("Developer");
		LOG_DeveloperConsole = LoggingSystem_FindChannel("DeveloperConsole");
		LOG_Console = LoggingSystem_FindChannel("Console");
		LOG_DeveloperVerbose = LoggingSystem_FindChannel("DeveloperVerbose");

		return true;
	}
#endif

	bool gsdk_launcher_library::load(const std::filesystem::path &path) noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		if(!gsdk_library::load(path)) {
			return false;
		}

		dedicated = iface<gsdk::IDedicatedExports>();

	#ifndef GSDK_NO_SYMBOLS
		if(CommandLine()->FindParm("-vmod_disable_syms") == 0) {
			symbols_available = (dedicated != nullptr);
		}
	#endif

		return true;
	}

	bool gsdk_engine_library::load(const std::filesystem::path &path) noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		if(!gsdk_library::load(path)) {
			return false;
		}

		sv_engine = iface<gsdk::IVEngineServer>();
		if(!sv_engine) {
			err_str = "missing IVEngineServer interface version "s;
			err_str += gsdk::IVEngineServer::interface_name;
			return false;
		}

	#ifndef GSDK_NO_SYMBOLS
		if(CommandLine()->FindParm("-vmod_disable_syms") == 0) {
			symbols_available = sv_engine->IsDedicatedServer();
		}
	#endif

		sv_stringtables = iface<gsdk::IServerNetworkStringTableContainer>();
		if(!sv_stringtables) {
			err_str = "missing IServerNetworkStringTableContainer interface version "s;
			err_str += gsdk::IServerNetworkStringTableContainer::interface_name;
			return false;
		}

		if(!syms.load(path, base())) {
			err_str = syms.error_string();
			return false;
		}

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		sv = sv_engine->GetIServer();
	#else
		const auto &eng_global_qual{syms.global()};

		auto sv_it{eng_global_qual.find("sv"s)};
		if(sv_it == eng_global_qual.end()) {
			warning("vmod: missing 'sv' symbol\n"sv);
		}

		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		auto loglistener_it{eng_global_qual.find("g_EngineLoggingListener"s)};
		if(loglistener_it == eng_global_qual.end()) {
			warning("vmod: missing 's_EngineLoggingListener' symbol\n"sv);
		}
		#endif

		if(sv_it != eng_global_qual.end()) {
			sv = sv_it->second->addr<gsdk::IServer *>();
		}

		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		if(loglistener_it != eng_global_qual.end()) {
			s_EngineLoggingListener = loglistener_it->second->addr<decltype(s_EngineLoggingListener)>();
		}
		#endif
	#endif

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		LOG_DownloadManager = LoggingSystem_FindChannel("DownloadManager");
		LOG_SeverLog = LoggingSystem_FindChannel("SeverLog");
	#endif

	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		LOG_Workshop = LoggingSystem_FindChannel("Workshop");
	#endif

	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		workshop = iface<gsdk::IWorkshop>();
		if(!workshop) {
			warning("missing IWorkshop interface version %s\n"sv, gsdk::IWorkshop::interface_name);
		}
	#endif

		return true;
	}

	bool gsdk_server_library::load(const std::filesystem::path &path) noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		if(!gsdk_library::load(path)) {
			return false;
		}

		gsdk::IPlayerInfoManager *plrinfomgr{iface<gsdk::IPlayerInfoManager>()};
		if(!plrinfomgr) {
			err_str = "missing IPlayerInfoManager interface version "s;
			err_str += gsdk::IPlayerInfoManager::interface_name;
			return false;
		}

		sv_globals = plrinfomgr->GetGlobalVars();

		gamedll = iface<gsdk::IServerGameDLL>();
		if(!gamedll) {
			err_str = "missing IServerGameDLL interface version "s;
			err_str += gsdk::IServerGameDLL::interface_name;
			return false;
		}

		servertools = iface<gsdk::IServerTools>();
		if(!servertools) {
			err_str = "missing IServerTools interface version "s;
			err_str += gsdk::IServerTools::interface_name;
			return false;
		}

		if(!syms.load(path, base())) {
			err_str = syms.error_string();
			return false;
		}

		const auto &sv_global_qual{syms.global()};

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		entityfactorydict = reinterpret_cast<gsdk::CEntityFactoryDictionary *>(servertools->GetEntityFactoryDictionary());
	#else
		auto EntityFactoryDictionary_it{sv_global_qual.find("EntityFactoryDictionary()"s)};
		if(EntityFactoryDictionary_it == sv_global_qual.end()) {
			warning("vmod: missing 'EntityFactoryDictionary' symbol\n"sv);
		}

		if(EntityFactoryDictionary_it != sv_global_qual.end()) {
			gsdk::IEntityFactoryDictionary *(*EntityFactoryDictionary)(){EntityFactoryDictionary_it->second->func<decltype(EntityFactoryDictionary)>()};

			entityfactorydict = reinterpret_cast<gsdk::CEntityFactoryDictionary *>(EntityFactoryDictionary());
		}
	#endif

		auto Steam3Server_it{sv_global_qual.find("Steam3Server()"s)};
		if(Steam3Server_it == sv_global_qual.end()) {
			warning("vmod: missing 'Steam3Server' symbol\n"sv);
		}

		if(Steam3Server_it != sv_global_qual.end()) {
			Steam3Server = Steam3Server_it->second->func<decltype(Steam3Server)>();
		}

		std_proxies = gamedll->GetStandardSendProxies();

		gsdk::ServerClass *temp_classes{gamedll->GetAllServerClasses()};
		while(temp_classes) {
			std::string name{temp_classes->m_pNetworkName};
			sv_classes.emplace(std::move(name), temp_classes);
			temp_classes = temp_classes->m_pNext;
		}

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		sv_LOG_VScript = LoggingSystem_FindChannel("VScript");
	#endif

		return true;
	}

	bool gsdk_filesystem_library::load(const std::filesystem::path &path) noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		if(!gsdk_library::load(path)) {
			return false;
		}

		bool is_ded{iface<gsdk::IDedicatedExports>() != nullptr};
		if(!is_ded) {
			filesystem = iface<gsdk::IFileSystem>();
			if(!filesystem) {
				err_str = "missing IFileSystem interface version "s;
				err_str += gsdk::IFileSystem::interface_name;
				return false;
			}
		} else {
		#ifndef GSDK_NO_SYMBOLS
			if(symbols_available) {
				std::uint64_t offset{symbol_cache::uncached_find_mangled_func(path, "_Z17FileSystemFactoryPKcPi"sv)};
				if(offset == 0) {
					warning("vmod: missing FileSystemFactory function\n"sv);
				} else {
				#ifndef __clang__
					#pragma GCC diagnostic push
					#pragma GCC diagnostic ignored "-Wconditionally-supported"
				#endif
					gsdk::CreateInterfaceFn FileSystemFactory{reinterpret_cast<gsdk::CreateInterfaceFn>(base() + offset)};
				#ifndef __clang__
					#pragma GCC diagnostic pop
				#endif

					int status{gsdk::IFACE_OK};
					filesystem = static_cast<gsdk::IFileSystem *>(FileSystemFactory(gsdk::IFileSystem::interface_name.data(), &status));
					if(!filesystem || status != gsdk::IFACE_OK) {
						err_str = "missing IFileSystem interface version "s;
						err_str += gsdk::IFileSystem::interface_name;
						return false;
					}
				}
			} else {
		#endif
				warning("vmod: missing FileSystemFactory function\n"sv);
		#ifndef GSDK_NO_SYMBOLS
			}
		#endif
		}

	#ifndef GSDK_NO_SYMBOLS
		if(symbols_available) {
			std::uint64_t offset{symbol_cache::uncached_find_mangled_func(path, "_ZN15CBaseFileSystem10AddVPKFileEPKcS1_15SearchPathAdd_t"sv)};
			if(offset != 0) {
				mfp_internal_t<void, gsdk::IFileSystem, const char *, const char *, gsdk::SearchPathAdd_t> internal{reinterpret_cast<uintptr_t>(base() + offset)};
				gsdk::IFileSystem::AddVPKFile_ptr = internal.func;
			}

			offset = symbol_cache::uncached_find_mangled_func(path, "_ZN15CBaseFileSystem13RemoveVPKFileEPKcS1_"sv);
			if(offset != 0) {
				mfp_internal_t<bool, gsdk::IFileSystem, const char *, const char *> internal{reinterpret_cast<uintptr_t>(base() + offset)};
				gsdk::IFileSystem::RemoveVPKFile_ptr = internal.func;
			}
		}
	#endif

		return true;
	}

	bool gsdk_vstdlib_library::load(const std::filesystem::path &path) noexcept
	{
		using namespace std::literals::string_literals;

		if(!gsdk_library::load(path)) {
			return false;
		}

		cvar = iface<gsdk::ICvar>();
		if(!cvar) {
			err_str = "missing ICvar interface version "s;
			err_str += gsdk::ICvar::interface_name;
			return false;
		}

		developer = cvar->FindVar("developer");

		return true;
	}

	bool gsdk_vscript_library::load(const std::filesystem::path &path) noexcept
	{
		using namespace std::literals::string_literals;

		if(!gsdk_library::load(path)) {
			return false;
		}

		vsmgr = iface<gsdk::IScriptManager>();
		if(!vsmgr) {
			err_str = "missing IScriptManager interface version "s;
			err_str += gsdk::IScriptManager::interface_name;
			return false;
		}

		if(!syms.load(path, base())) {
			err_str = syms.error_string();
			return false;
		}

		return true;
	}
}
