#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include "gsdk_library.hpp"
#include "gsdk/launcher/launcher.hpp"
#include "gsdk/engine/sv_engine.hpp"
#include "gsdk/engine/globalvars.hpp"
#include "gsdk/engine/stringtable.hpp"
#include "gsdk/engine/sys_dll.hpp"
#include "gsdk/engine/workshop.hpp"
#include "gsdk/server/server.hpp"
#include "gsdk/filesystem/filesystem.hpp"
#include "gsdk/vstdlib/convar.hpp"
#include "vscript/vscript.hpp"
#include "gsdk/tier0/dbg.hpp"
#include "gsdk/server/baseentity.hpp"
#include "symbol_cache.hpp"

namespace vmod
{
	extern gsdk::IDedicatedExports *dedicated;
	extern gsdk::IVEngineServer *sv_engine;
	extern gsdk::IServer *sv;
	extern gsdk::IFileSystem *filesystem;
	extern gsdk::ICvar *cvar;
	extern gsdk::IScriptManager *vsmgr;
	extern gsdk::CGlobalVars *sv_globals;
	extern gsdk::IServerGameDLL *gamedll;
	extern gsdk::IServerTools *servertools;
	extern gsdk::CEntityFactoryDictionary *entityfactorydict;
	extern gsdk::IServerNetworkStringTableContainer *sv_stringtables;
	extern std::unordered_map<std::string, gsdk::ServerClass *> sv_classes;
	extern gsdk::CStandardSendProxies *std_proxies;
	extern gsdk::CSteam3Server &(*Steam3Server)();
#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	extern gsdk::IWorkshop *workshop;
#endif
#ifndef GSDK_NO_SYMBOLS
	extern bool symbols_available;
#endif
	extern bool srcds_executable;
	extern gsdk::ConVar *developer;
#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	extern gsdk::CLoggingSystem *(*GetGlobalLoggingSystem)();
	extern gsdk::CEngineConsoleLoggingListener *s_EngineLoggingListener;
	extern gsdk::LoggingChannelID_t LOG_General;
	extern gsdk::LoggingChannelID_t LOG_Assert;
	extern gsdk::LoggingChannelID_t LOG_Developer;
	extern gsdk::LoggingChannelID_t LOG_DeveloperConsole;
	extern gsdk::LoggingChannelID_t LOG_Console;
	extern gsdk::LoggingChannelID_t LOG_DeveloperVerbose;
	extern gsdk::LoggingChannelID_t LOG_DownloadManager;
	extern gsdk::LoggingChannelID_t LOG_SeverLog;
	extern gsdk::LoggingChannelID_t sv_LOG_VScript;
#endif
#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	extern gsdk::LoggingChannelID_t LOG_Workshop;
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	extern gsdk::LoggingChannelID_t LOG_vmod;
	extern gsdk::LoggingChannelID_t LOG_vmod_vm;
	extern gsdk::LoggingChannelID_t LOG_vmod_vscript;
	extern gsdk::LoggingChannelID_t LOG_vmod_workshop;
#endif

	constexpr gsdk::Color print_clr{255, 255, 255, 255};
	constexpr gsdk::Color success_clr{0, 255, 0, 255};
	constexpr gsdk::Color info_clr{0, 150, 150, 255};
	constexpr gsdk::Color remark_clr{150, 0, 255, 255};
	constexpr gsdk::Color warning_clr{255, 255, 0, 255};
	constexpr gsdk::Color error_clr{255, 0, 0, 255};

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
	#define __VMOD_PRINT(chan, clr, sve) \
		ConColorMsg(0, clr, fmt.data(), std::forward<Args>(args)...);
#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	#define __VMOD_PRINT(chan, clr, sve) \
		switch(LoggingSystem_Log(chan, sve, clr, fmt.data(), std::forward<Args>(args)...)) { \
		default: \
		case gsdk::LR_CONTINUE: break; \
		case gsdk::LR_DEBUGGER: debugtrap(); break; \
		case gsdk::LR_ABORT: std::abort(); break; \
		}
#else
	#error
#endif

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	template <typename ...Args>
	inline void print(gsdk::LoggingChannelID_t chan, std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(chan, print_clr, LS_MESSAGE) }
	template <typename ...Args>
	inline void success(gsdk::LoggingChannelID_t chan, std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(chan, success_clr, LS_MESSAGE) }
	template <typename ...Args>
	inline void info(gsdk::LoggingChannelID_t chan, std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(chan, info_clr, LS_MESSAGE) }
	template <typename ...Args>
	inline void remark(gsdk::LoggingChannelID_t chan, std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(chan, remark_clr, LS_MESSAGE) }
	template <typename ...Args>
	inline void warning(gsdk::LoggingChannelID_t chan, std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(chan, warning_clr, LS_WARNING) }
	template <typename ...Args>
	inline void error(gsdk::LoggingChannelID_t chan, std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(chan, error_clr, LS_WARNING) }
#endif

	template <typename ...Args>
	inline void print(std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(LOG_vmod, print_clr, LS_MESSAGE) }
	template <typename ...Args>
	inline void success(std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(LOG_vmod, success_clr, LS_MESSAGE) }
	template <typename ...Args>
	inline void info(std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(LOG_vmod, info_clr, LS_MESSAGE) }
	template <typename ...Args>
	inline void remark(std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(LOG_vmod, remark_clr, LS_MESSAGE) }
	template <typename ...Args>
	inline void warning(std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(LOG_vmod, warning_clr, LS_WARNING) }
	template <typename ...Args>
	inline void error(std::string_view fmt, Args &&...args) noexcept
	{ __VMOD_PRINT(LOG_vmod, error_clr, LS_WARNING) }

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	class gsdk_tier0_library final : public library
	{
	public:
		bool load(const std::filesystem::path &path) noexcept override;
	};
#endif

	class gsdk_launcher_library final : public gsdk_library
	{
	public:
		bool load(const std::filesystem::path &path) noexcept override;
	};

	class gsdk_engine_library final : public gsdk_library
	{
	public:
		gsdk_engine_library() noexcept = default;

		bool load(const std::filesystem::path &path) noexcept override;

		inline const symbol_cache &symbols() const noexcept
		{ return syms; }

	private:
		symbol_cache syms;

	private:
		gsdk_engine_library(const gsdk_engine_library &) = delete;
		gsdk_engine_library &operator=(const gsdk_engine_library &) = delete;
		gsdk_engine_library(gsdk_engine_library &&) = delete;
		gsdk_engine_library &operator=(gsdk_engine_library &&) = delete;
	};

	class gsdk_server_library final : public gsdk_library
	{
	public:
		gsdk_server_library() noexcept = default;

		bool load(const std::filesystem::path &path) noexcept override;

		inline const symbol_cache &symbols() const noexcept
		{ return syms; }

	private:
		symbol_cache syms;

	private:
		gsdk_server_library(const gsdk_server_library &) = delete;
		gsdk_server_library &operator=(const gsdk_server_library &) = delete;
		gsdk_server_library(gsdk_server_library &&) = delete;
		gsdk_server_library &operator=(gsdk_server_library &&) = delete;
	};

	class gsdk_filesystem_library final : public gsdk_library
	{
	public:
		bool load(const std::filesystem::path &path) noexcept override;
	};

	class gsdk_vstdlib_library final : public gsdk_library
	{
	public:
		bool load(const std::filesystem::path &path) noexcept override;
	};

	class gsdk_vscript_library final : public gsdk_library
	{
	public:
		gsdk_vscript_library() noexcept = default;

		bool load(const std::filesystem::path &path) noexcept override;

		inline const symbol_cache &symbols() const noexcept
		{ return syms; }

	private:
		symbol_cache syms;

	private:
		gsdk_vscript_library(const gsdk_vscript_library &) = delete;
		gsdk_vscript_library &operator=(const gsdk_vscript_library &) = delete;
		gsdk_vscript_library(gsdk_vscript_library &&) = delete;
		gsdk_vscript_library &operator=(gsdk_vscript_library &&) = delete;
	};
}

#include "gsdk.tpp"
