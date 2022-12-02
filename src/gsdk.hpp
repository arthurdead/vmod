#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include "gsdk/tier1/interface.hpp"
#include "gsdk/launcher/launcher.hpp"
#include "gsdk/engine/sv_engine.hpp"
#include "gsdk/engine/globalvars.hpp"
#include "gsdk/server/server.hpp"
#include "gsdk/filesystem/filesystem.hpp"
#include "gsdk/vstdlib/convar.hpp"
#include "gsdk/vscript/vscript.hpp"
#include "symbol_cache.hpp"

namespace vmod
{
	extern gsdk::IDedicatedExports *dedicated;
	extern gsdk::IVEngineServer *sv_engine;
	extern gsdk::IFileSystem *filesystem;
	extern gsdk::ICvar *cvar;
	extern gsdk::IScriptManager *vsmgr;
	extern gsdk::CGlobalVars *sv_globals;
	extern gsdk::IServerGameDLL *gamedll;
	extern gsdk::IServerTools *servertools;
	extern gsdk::IEntityFactoryDictionary *entityfactorydict;

	template <typename ...Args>
	void print(std::string_view fmt, Args &&...args);

	template <typename ...Args>
	void success(std::string_view fmt, Args &&...args) noexcept;

	template <typename ...Args>
	void info(std::string_view fmt, Args &&...args) noexcept;

	template <typename ...Args>
	void warning(std::string_view fmt, Args &&...args) noexcept;

	template <typename ...Args>
	void error(std::string_view fmt, Args &&...args) noexcept;

	class gsdk_library
	{
	public:
		virtual bool load(const std::filesystem::path &path) noexcept;

		inline const std::string &error_string() const noexcept
		{ return err_str; }

		template <typename T>
		T *iface() noexcept;

		template <typename T>
		T *addr(std::string_view name) noexcept;

		inline void *base() noexcept
		{ return base_addr; }

		void unload() noexcept;

		virtual ~gsdk_library() noexcept;

	protected:
		std::string err_str;

		virtual void *find_addr(std::string_view name) noexcept;
		virtual void *find_iface(std::string_view name) noexcept;

	private:
		void *dl{nullptr};
		gsdk::CreateInterfaceFn iface_fac;
		void *base_addr;
	};

	template <typename T>
	inline T *gsdk_library::iface() noexcept
	{ return static_cast<T *>(find_iface(T::interface_name)); }

	class gsdk_launcher_library final : public gsdk_library
	{
	public:
		bool load(const std::filesystem::path &path) noexcept override;
	};

	class gsdk_engine_library final : public gsdk_library
	{
	public:
		bool load(const std::filesystem::path &path) noexcept override;
	};

	class gsdk_server_library final : public gsdk_library
	{
	public:
		bool load(const std::filesystem::path &path) noexcept override;

		inline const symbol_cache &symbols() const noexcept
		{ return syms; }

	private:
		symbol_cache syms;
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
		bool load(const std::filesystem::path &path) noexcept override;

		inline const symbol_cache &symbols() const noexcept
		{ return syms; }

	private:
		symbol_cache syms;
	};
}

#include "gsdk.tpp"
