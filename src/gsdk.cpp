#include "gsdk.hpp"
#include "gsdk/server/server.hpp"
#include <dlfcn.h>

namespace vmod
{
	gsdk::IDedicatedExports *dedicated;
	gsdk::IVEngineServer *sv_engine;
	gsdk::IFileSystem *filesystem;
	gsdk::ICvar *cvar;
	gsdk::IScriptManager *vsmgr;
	gsdk::CGlobalVars *sv_globals;
	gsdk::IServerGameDLL *gamedll;
	gsdk::IServerTools *servertools;
	gsdk::CEntityFactoryDictionary *entityfactorydict;
	gsdk::IServerNetworkStringTableContainer *sv_stringtables;
	std::unordered_map<std::string, gsdk::ServerClass *> sv_classes;

	bool gsdk_library::load(const std::filesystem::path &path) noexcept
	{
		using namespace std::literals::string_literals;

		dl = dlopen(path.c_str(), RTLD_LAZY|RTLD_LOCAL|RTLD_NODELETE|RTLD_NOLOAD);
		if(!dl) {
			const char *err{dlerror()};
			if(err) {
				err_str = err;
			}
			return false;
		} else {
			iface_fac = reinterpret_cast<gsdk::CreateInterfaceFn>(dlsym(dl, "CreateInterface"));
			if(!iface_fac) {
				err_str = "missing CreateInterface export"s;
				return false;
			}

			Dl_info base_info;
			if(dladdr(reinterpret_cast<const void *>(iface_fac), &base_info) == 0) {
				err_str = "failed to get base address"s;
				return false;
			}

			base_addr = base_info.dli_fbase;
		}

		return true;
	}

	void *gsdk_library::find_iface(std::string_view name) noexcept
	{
		int status{gsdk::IFACE_OK};
		void *iface{iface_fac(name.data(), &status)};
		if(!iface || status != gsdk::IFACE_OK) {
			return nullptr;
		}
		return iface;
	}

	void *gsdk_library::find_addr(std::string_view name) noexcept
	{
		return dlsym(dl, name.data());
	}

	void gsdk_library::unload() noexcept
	{
		if(dl) {
			dlclose(dl);
			dl = nullptr;
		}
	}

	gsdk_library::~gsdk_library() noexcept
	{
		if(dl) {
			dlclose(dl);
		}
	}

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

		return true;
	}

	bool gsdk_server_library::load(const std::filesystem::path &path) noexcept
	{
		using namespace std::literals::string_literals;

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

		entityfactorydict = reinterpret_cast<gsdk::CEntityFactoryDictionary *>(servertools->GetEntityFactoryDictionary());
		if(!entityfactorydict) {
			err_str = "EntityFactoryDictionary is null"s;
			return false;
		}

		if(!syms.load(path, base())) {
			err_str = syms.error_string();
			return false;
		}

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

		if(!gsdk_library::load(path)) {
			return false;
		}

		filesystem = iface<gsdk::IFileSystem>();
		if(!filesystem) {
			err_str = "missing IFileSystem interface version "s;
			err_str += gsdk::IFileSystem::interface_name;
			return false;
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

		if(!syms.load(path, base())) {
			err_str = syms.error_string();
			return false;
		}

		return true;
	}
}
