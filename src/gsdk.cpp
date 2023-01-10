#include "gsdk.hpp"
#include "gsdk/server/server.hpp"

namespace vmod
{
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

	bool gsdk_launcher_library::load(const std::filesystem::path &path) noexcept
	{
		if(!gsdk_library::load(path)) {
			return false;
		}

		dedicated = iface<gsdk::IDedicatedExports>();

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

		sv_stringtables = iface<gsdk::IServerNetworkStringTableContainer>();
		if(!sv_stringtables) {
			err_str = "missing IServerNetworkStringTableContainer interface version "s;
			err_str += gsdk::IServerNetworkStringTableContainer::interface_name;
			return false;
		}

	#ifndef GSDK_NO_SYMBOLS
		if(!syms.load(path, base())) {
			err_str = syms.error_string();
			return false;
		}
	#endif

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		sv = sv_engine->GetIServer();
	#else
		#ifndef GSDK_NO_SYMBOLS
		const auto &eng_global_qual{syms.global()};

		auto sv_it{eng_global_qual.find("sv"s)};
		if(sv_it == eng_global_qual.end()) {
			error("vmod: missing 'sv' symbol\n"sv);
			return false;
		}

		sv = sv_it->second->addr<gsdk::IServer *>();
		#else
			#error
		#endif
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

	#ifndef GSDK_NO_SYMBOLS
		if(!syms.load(path, base())) {
			err_str = syms.error_string();
			return false;
		}
	#endif

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		entityfactorydict = reinterpret_cast<gsdk::CEntityFactoryDictionary *>(servertools->GetEntityFactoryDictionary());
	#else
		#ifndef GSDK_NO_SYMBOLS
		const auto &sv_global_qual{syms.global()};

		auto EntityFactoryDictionary_it{sv_global_qual.find("EntityFactoryDictionary()"s)};
		if(EntityFactoryDictionary_it == sv_global_qual.end()) {
			error("vmod: missing 'EntityFactoryDictionary' symbol\n"sv);
			return false;
		}

		entityfactorydict = reinterpret_cast<gsdk::CEntityFactoryDictionary *>(EntityFactoryDictionary_it->second->func<gsdk::IEntityFactoryDictionary *(*)()>()());
		#else
			#error
		#endif
	#endif
		if(!entityfactorydict) {
			err_str = "EntityFactoryDictionary is null"s;
			return false;
		}

		std_proxies = gamedll->GetStandardSendProxies();

		gsdk::ServerClass *temp_classes{gamedll->GetAllServerClasses()};
		while(temp_classes) {
			std::string name{temp_classes->m_pNetworkName};
			sv_classes.emplace(std::move(name), temp_classes);
			temp_classes = temp_classes->m_pNext;
		}

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
			std::uint64_t offset{symbol_cache::uncached_find_mangled_func(path, "_Z17FileSystemFactoryPKcPi"sv)};
			if(offset == 0) {
				err_str = "missing FileSystemFactory function"s;
				return false;
			}

		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wconditionally-supported"
		#endif
			gsdk::CreateInterfaceFn FileSystemFactory{reinterpret_cast<gsdk::CreateInterfaceFn>(static_cast<unsigned char *>(base()) + offset)};
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
		#else
			#error
		#endif
		}

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

	#ifndef GSDK_NO_SYMBOLS
		if(!syms.load(path, base())) {
			err_str = syms.error_string();
			return false;
		}
	#endif

		return true;
	}
}
