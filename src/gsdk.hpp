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
#include "gsdk/server/server.hpp"
#include "gsdk/filesystem/filesystem.hpp"
#include "gsdk/vstdlib/convar.hpp"
#include "vscript/vscript.hpp"
#include "gsdk/tier0/dbg.hpp"
#include "gsdk/server/baseentity.hpp"
#ifndef GSDK_NO_SYMBOLS
#include "symbol_cache.hpp"
#endif

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
#ifndef GSDK_NO_SYMBOLS
	extern bool symbols_available;
#endif

	constexpr gsdk::Color print_clr{255, 255, 255, 255};
	constexpr gsdk::Color success_clr{0, 255, 0, 255};
	constexpr gsdk::Color info_clr{0, 150, 150, 255};
	constexpr gsdk::Color remark_clr{150, 0, 255, 255};
	constexpr gsdk::Color warning_clr{255, 255, 0, 255};
	constexpr gsdk::Color error_clr{255, 0, 0, 255};

	template <typename ...Args>
	inline void print(std::string_view fmt, Args &&...args) noexcept
	{
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		ConColorMsg(0, print_clr, fmt.data(), std::forward<Args>(args)...);
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		ConColorMsg(print_clr, fmt.data(), std::forward<Args>(args)...);
	#else
		#error
	#endif
	}

	template <typename ...Args>
	inline void success(std::string_view fmt, Args &&...args) noexcept
	{
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		ConColorMsg(0, success_clr, fmt.data(), std::forward<Args>(args)...);
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		ConColorMsg(success_clr, fmt.data(), std::forward<Args>(args)...);
	#else
		#error
	#endif
	}

	template <typename ...Args>
	inline void info(std::string_view fmt, Args &&...args) noexcept
	{
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		ConColorMsg(0, info_clr, fmt.data(), std::forward<Args>(args)...);
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		ConColorMsg(info_clr, fmt.data(), std::forward<Args>(args)...);
	#else
		#error
	#endif
	}

	template <typename ...Args>
	inline void remark(std::string_view fmt, Args &&...args) noexcept
	{
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		ConColorMsg(0, remark_clr, fmt.data(), std::forward<Args>(args)...);
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		ConColorMsg(remark_clr, fmt.data(), std::forward<Args>(args)...);
	#else
		#error
	#endif
	}

	template <typename ...Args>
	inline void warning(std::string_view fmt, Args &&...args) noexcept
	{
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		ConColorMsg(0, warning_clr, fmt.data(), std::forward<Args>(args)...);
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		ConColorMsg(warning_clr, fmt.data(), std::forward<Args>(args)...);
	#else
		#error
	#endif
	}

	template <typename ...Args>
	inline void error(std::string_view fmt, Args &&...args) noexcept
	{
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		ConColorMsg(0, error_clr, fmt.data(), std::forward<Args>(args)...);
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		ConColorMsg(error_clr, fmt.data(), std::forward<Args>(args)...);
	#else
		#error
	#endif
	}

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

	#ifndef GSDK_NO_SYMBOLS
		inline const symbol_cache &symbols() const noexcept
		{ return syms; }

	private:
		symbol_cache syms;
	#endif

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

	#ifndef GSDK_NO_SYMBOLS
		inline const symbol_cache &symbols() const noexcept
		{ return syms; }

	private:
		symbol_cache syms;
	#endif

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

	#ifndef GSDK_NO_SYMBOLS
		inline const symbol_cache &symbols() const noexcept
		{ return syms; }

	private:
		symbol_cache syms;
	#endif

	private:
		gsdk_vscript_library(const gsdk_vscript_library &) = delete;
		gsdk_vscript_library &operator=(const gsdk_vscript_library &) = delete;
		gsdk_vscript_library(gsdk_vscript_library &&) = delete;
		gsdk_vscript_library &operator=(gsdk_vscript_library &&) = delete;
	};
}

#include "gsdk.tpp"
