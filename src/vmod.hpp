#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include "gsdk.hpp"

namespace vmod
{
	class plugin;

	extern gsdk::IScriptVM *vm;
	extern gsdk::CVarDLLIdentifier_t cvar_dll_id;
	extern gsdk::HSCRIPT global_scope;
	extern std::filesystem::path root_dir;

	class vmod final
	{
		friend class vsp;

	public:
		inline bool map_is_loaded() const noexcept
		{ return is_map_loaded; }

		std::string_view to_string(gsdk::HSCRIPT value) noexcept;

	private:
		bool load_late() noexcept;
		bool load() noexcept;
		void unload() noexcept;

		void game_frame(bool simulating) noexcept;
		void map_loaded(std::string_view name) noexcept;
		void map_unloaded() noexcept;
		void map_active() noexcept;

		gsdk::HSCRIPT script_find_plugin(std::string_view name) noexcept;

		bool bindings() noexcept;
		void unbindings() noexcept;

		bool detours() noexcept;

		bool is_map_loaded;

		std::filesystem::path game_dir;
		std::filesystem::path plugins_dir;

		gsdk_engine_library engine_lib;

		gsdk_server_library server_lib;

		gsdk_launcher_library launcher_lib;

		gsdk_vstdlib_library vstdlib_lib;

		gsdk_vscript_library vscript_lib;

		std::string_view scripts_extension;

		gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};

		gsdk::HSCRIPT server_init_script{gsdk::INVALID_HSCRIPT};

		std::filesystem::path base_script_path;
		gsdk::HSCRIPT base_script{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT to_string_func{gsdk::INVALID_HSCRIPT};

		std::vector<std::unique_ptr<plugin>> plugins;
		bool plugins_loaded;
	};

	extern vmod vmod;
}
