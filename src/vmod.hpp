#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include "gsdk.hpp"
#include "convar.hpp"
#include "vscript.hpp"

namespace vmod
{
	class plugin;

	class script_variant_t;

	class vmod final : public gsdk::ISquirrelMetamethodDelegate, public singleton_instance_helper<vmod>
	{
		friend class vsp;

	public:
		inline bool map_is_loaded() const noexcept
		{ return is_map_loaded; }

		std::string_view to_string(gsdk::HSCRIPT value) const noexcept;
		int to_int(gsdk::HSCRIPT value) const noexcept;
		float to_float(gsdk::HSCRIPT value) const noexcept;
		bool to_bool(gsdk::HSCRIPT value) const noexcept;

		inline gsdk::CVarDLLIdentifier_t cvar_dll_id() const noexcept
		{ return cvar_dll_id_; }

		inline gsdk::IScriptVM *vm() noexcept
		{ return vm_; }

		inline gsdk::HSCRIPT scope() noexcept
		{ return scope_; }

		inline gsdk::HSCRIPT plugins_table() noexcept
		{ return plugins_table_; }

		inline gsdk::HSCRIPT symbols_table() noexcept
		{ return symbols_table_; }

		static vmod &instance() noexcept;

	private:
		bool load_late() noexcept;
		bool load() noexcept;
		void unload() noexcept;

		void game_frame(bool simulating) noexcept;
		void map_loaded(std::string_view name) noexcept;
		void map_unloaded() noexcept;
		void map_active() noexcept;

		gsdk::HSCRIPT script_find_plugin(std::string_view name) noexcept;

		bool binding_mods() noexcept;
		bool bindings() noexcept;
		void unbindings() noexcept;

		bool detours() noexcept;

		bool Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value) override;

		bool is_map_loaded;

		std::filesystem::path game_dir;
		std::filesystem::path plugins_dir;
		std::filesystem::path root_dir;

		gsdk::CVarDLLIdentifier_t cvar_dll_id_{gsdk::INVALID_CVAR_DLL_IDENTIFIER};

		friend class server_symbols_singleton;

		gsdk_engine_library engine_lib;

		gsdk_server_library server_lib;

		gsdk_launcher_library launcher_lib;

		gsdk_vstdlib_library vstdlib_lib;

		gsdk_vscript_library vscript_lib;

		std::string_view scripts_extension;

		gsdk::IScriptVM *vm_;

		gsdk::CSquirrelMetamethodDelegateImpl *get_impl{nullptr};
		gsdk::HSCRIPT vs_instance_{gsdk::INVALID_HSCRIPT};

		gsdk::HSCRIPT scope_{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT plugins_table_{gsdk::INVALID_HSCRIPT};

		gsdk::HSCRIPT symbols_table_{gsdk::INVALID_HSCRIPT};

		gsdk::HSCRIPT server_init_script{gsdk::INVALID_HSCRIPT};

		std::filesystem::path base_script_path;
		gsdk::HSCRIPT base_script_scope{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT base_script{gsdk::INVALID_HSCRIPT};
		bool base_script_from_file;

		gsdk::HSCRIPT to_string_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT to_int_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT to_float_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT to_bool_func{gsdk::INVALID_HSCRIPT};

		std::vector<std::unique_ptr<plugin>> plugins;
		bool plugins_loaded;

		ConCommand vmod_reload_plugins;
		ConCommand vmod_unload_plugins;
		ConCommand vmod_unload_plugin;
		ConCommand vmod_load_plugin;
		ConCommand vmod_list_plugins;
		ConCommand vmod_refresh_plugins;
	};

	extern vmod vmod;
}
