#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include "gsdk.hpp"
#include "convar.hpp"
#include "vscript/vscript.hpp"
#include "preprocessor.hpp"
#include "bindings/singleton.hpp"
#include "bindings/strtables/string_table.hpp"
#include "gsdk/engine/dt_send.hpp"
#include "gsdk/server/datamap.hpp"
#include "plugin.hpp"

namespace vmod
{
	struct entity_class_info
	{
		gsdk::ServerClass *sv_class;
		gsdk::SendTable *sendtable;
		gsdk::datamap_t *datamap;
		gsdk::ScriptClassDesc_t *script_desc;
	};

	extern std::unordered_map<std::string, entity_class_info> sv_ent_class_info;

	class main final : public bindings::singleton_base
	{
		friend class vsp;
		friend void bindings::strtables::write_docs(const std::filesystem::path &) noexcept;

	public:
		inline main() noexcept
			: singleton_base{"vmod", true}
		{
		}

		~main() noexcept override;

		inline bool map_is_loaded() const noexcept
		{ return is_map_loaded; }
		inline bool map_is_active() const noexcept
		{ return is_map_active; }
		inline bool string_tables_created() const noexcept
		{ return are_string_tables_created; }

		std::string_view to_string(gsdk::HSCRIPT value) const noexcept;
		int to_int(gsdk::HSCRIPT value) const noexcept;
		float to_float(gsdk::HSCRIPT value) const noexcept;
		bool to_bool(gsdk::HSCRIPT value) const noexcept;
		std::string_view type_of(gsdk::HSCRIPT value) const noexcept;

		inline gsdk::CVarDLLIdentifier_t cvar_dll_id() const noexcept
		{ return cvar_dll_id_; }

		inline gsdk::IScriptVM *vm() noexcept
		{ return vm_; }

		inline const std::filesystem::path &game_dir() const noexcept
		{ return game_dir_; }
		inline const std::filesystem::path &root_dir() const noexcept
		{ return root_dir_; }
		inline const std::filesystem::path &plugins_dir() const noexcept
		{ return plugins_dir_; }

		inline squirrel_preprocessor &preprocessor() noexcept
		{ return pp; }

	#ifndef GSDK_NO_SYMBOLS
		inline gsdk::HSCRIPT symbols_table() noexcept
		{ return symbols_table_; }

		inline const symbol_cache &sv_syms() const noexcept
		{ return server_lib.symbols(); }

		inline const symbol_cache &eng_syms() const noexcept
		{ return engine_lib.symbols(); }
	#endif

		static main &instance() noexcept;

	private:
		static void CreateNetworkStringTables_detour_callback(gsdk::IServerGameDLL *dll);
		static void RemoveAllTables_detour_callback(gsdk::IServerNetworkStringTableContainer *cont);

		friend class squirrel_preprocessor;

		bool load_late() noexcept;
		bool load() noexcept;
		void unload() noexcept;

		void game_frame(bool simulating) noexcept;
		void map_loaded(std::string_view mapname) noexcept;
		void map_unloaded() noexcept;
		void map_active() noexcept;

		bool binding_mods() noexcept;
		bool bindings() noexcept;
		void unbindings() noexcept;

		bool detours_prevm() noexcept;
		bool detours() noexcept;

		bool assign_entity_class_info() noexcept;

		void write_docs(const std::filesystem::path &dir) const noexcept;

		void build_internal_docs() noexcept;

		enum class load_plugins_flags : unsigned char
		{
			none =             0,
			no_recurse = (1 << 0),
			src_folder = (1 << 1)
		};
		friend constexpr inline bool operator&(load_plugins_flags lhs, load_plugins_flags rhs) noexcept
		{ return static_cast<bool>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); }
		friend constexpr inline load_plugins_flags operator|(load_plugins_flags lhs, load_plugins_flags rhs) noexcept
		{ return static_cast<load_plugins_flags>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); }

		void load_plugins(const std::filesystem::path &dir, load_plugins_flags flags) noexcept;

		static vscript::singleton_class_desc<main> desc;

		static gsdk::ScriptVariant_t call_to_func(gsdk::HSCRIPT func, gsdk::HSCRIPT value) noexcept;

		void script_print(const vscript::variant &var) noexcept;
		void script_success(const vscript::variant &var) noexcept;
		void script_error(const vscript::variant &var) noexcept;
		void script_warning(const vscript::variant &var) noexcept;
		void script_info(const vscript::variant &var) noexcept;
		void script_remark(const vscript::variant &var) noexcept;

		void script_printl(const vscript::variant &var) noexcept;
		void script_successl(const vscript::variant &var) noexcept;
		void script_errorl(const vscript::variant &var) noexcept;
		void script_warningl(const vscript::variant &var) noexcept;
		void script_infol(const vscript::variant &var) noexcept;
		void script_remarkl(const vscript::variant &var) noexcept;

		gsdk::HSCRIPT script_find_plugin(std::string_view plname) noexcept;
		bool script_is_map_loaded() const noexcept;
		bool script_is_map_active() const noexcept;
		bool script_are_stringtables_created() const noexcept;

		std::filesystem::path build_plugin_path(std::string_view plname) const noexcept;

		void recreate_script_stringtables() noexcept;
		bool create_script_stringtables() noexcept;
		void clear_script_stringtables() noexcept;
		bool create_script_stringtable(std::string &&tablename) noexcept;

	#ifndef GSDK_NO_SYMBOLS
		bool create_script_symbols() noexcept;
	#endif

		bool is_map_loaded{false};
		bool is_map_active{false};
		bool are_string_tables_created{false};

		bool internal_docs_built{false};

		std::filesystem::path game_dir_;
		std::filesystem::path plugins_dir_;
		std::filesystem::path root_dir_;

		gsdk::CVarDLLIdentifier_t cvar_dll_id_{gsdk::INVALID_CVAR_DLL_IDENTIFIER};

		gsdk_engine_library engine_lib;
		gsdk_server_library server_lib;
		gsdk_launcher_library launcher_lib;
		gsdk_vstdlib_library vstdlib_lib;
		gsdk_vscript_library vscript_lib;
		gsdk_filesystem_library filesystem_lib;

		std::string_view scripts_extension;

		gsdk::IScriptVM *vm_{nullptr};

		gsdk::HSCRIPT return_flags_table{gsdk::INVALID_HSCRIPT};

		gsdk::HSCRIPT stringtable_table{gsdk::INVALID_HSCRIPT};
		std::unordered_map<std::string, std::unique_ptr<bindings::strtables::string_table>> script_stringtables;

	#ifndef GSDK_NO_SYMBOLS
		gsdk::HSCRIPT symbols_table_{gsdk::INVALID_HSCRIPT};
	#endif

		gsdk::HSCRIPT server_init_script{gsdk::INVALID_HSCRIPT};

		std::filesystem::path base_script_path;
		gsdk::HSCRIPT base_script_scope{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT base_script{gsdk::INVALID_HSCRIPT};
		bool base_script_from_file{false};

		gsdk::HSCRIPT to_string_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT to_int_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT to_float_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT to_bool_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT typeof_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT funcisg_func{gsdk::INVALID_HSCRIPT};

		squirrel_preprocessor pp;

		std::vector<std::filesystem::path> added_paths;

		std::unordered_map<std::filesystem::path, std::unique_ptr<plugin>> plugins;
		bool plugins_loaded{false};

		ConCommand vmod_reload_plugins;
		ConCommand vmod_unload_plugins;
		ConCommand vmod_unload_plugin;
		ConCommand vmod_load_plugin;
		ConCommand vmod_list_plugins;
		ConCommand vmod_refresh_plugins;

		ConCommand vmod_dump_internal_scripts;
		ConVar vmod_auto_dump_internal_scripts;

		ConCommand vmod_dump_squirrel_ver;
		ConVar vmod_auto_dump_squirrel_ver;

		ConCommand vmod_dump_netprops;
		ConVar vmod_auto_dump_netprops;

		ConCommand vmod_dump_datamaps;
		ConVar vmod_auto_dump_datamaps;

		ConCommand vmod_dump_entity_classes;
		ConVar vmod_auto_dump_entity_classes;

	#ifndef GSDK_NO_SYMBOLS
		ConCommand vmod_dump_entity_vtables;
		ConVar vmod_auto_dump_entity_vtables;

		ConCommand vmod_dump_entity_funcs;
		ConVar vmod_auto_dump_entity_funcs;
	#endif

		bool can_gen_docs{false};
		ConCommand vmod_gen_docs;
		ConVar vmod_auto_gen_docs;

	private:
		main(const main &) = delete;
		main &operator=(const main &) = delete;
		main(main &&) = delete;
		main &operator=(main &&) = delete;
	};
}
