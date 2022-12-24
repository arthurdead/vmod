#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include "gsdk.hpp"
#include "convar.hpp"
#include "gsdk/vscript/vscript.hpp"
#include "preprocessor.hpp"

namespace vmod
{
	class plugin;

	class script_variant_t;

	class script_stringtable;

	enum class callback_result_flags : unsigned char
	{
		continue_ =        0,
		halt =       (1 << 0),
		handled =    (1 << 1)
	};
	constexpr inline bool operator&(callback_result_flags lhs, callback_result_flags rhs) noexcept
	{ return static_cast<bool>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); }
	constexpr inline callback_result_flags operator|(callback_result_flags lhs, callback_result_flags rhs) noexcept
	{ return static_cast<callback_result_flags>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); }

	static_assert(static_cast<unsigned char>(callback_result_flags::continue_) == 0);

	class vmod final : public gsdk::ISquirrelMetamethodDelegate
	{
		friend class vsp;

	public:
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
		std::string_view typeof_(gsdk::HSCRIPT value) const noexcept;

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

		inline const std::filesystem::path &game_dir() const noexcept
		{ return game_dir_; }

		inline squirrel_preprocessor &preprocessor() noexcept
		{ return pp; }

		inline const std::filesystem::path &root_dir() const noexcept
		{ return root_dir_; }

		static vmod &instance() noexcept;

	private:
		static void CreateNetworkStringTables_detour_callback(gsdk::IServerGameDLL *dll);
		static void RemoveAllTables_detour_callback(gsdk::IServerNetworkStringTableContainer *cont);

		friend class squirrel_preprocessor;

		bool load_late() noexcept;
		bool load() noexcept;
		void unload() noexcept;

		void game_frame(bool simulating) noexcept;
		void map_loaded(std::string_view name) noexcept;
		void map_unloaded() noexcept;
		void map_active() noexcept;

		gsdk::HSCRIPT script_find_plugin(std::string_view name) noexcept;
		inline bool script_is_map_loaded() const noexcept
		{ return is_map_loaded; }
		inline bool script_is_map_active() const noexcept
		{ return is_map_active; }
		inline bool script_are_stringtables_created() const noexcept
		{ return are_string_tables_created; }

		static inline void script_print(std::string_view str) noexcept
		{ print("%s", str.data()); }
		static inline void script_success(std::string_view str) noexcept
		{ success("%s", str.data()); }
		static inline void script_error(std::string_view str) noexcept
		{ error("%s", str.data()); }
		static inline void script_warning(std::string_view str) noexcept
		{ warning("%s", str.data()); }
		static inline void script_info(std::string_view str) noexcept
		{ info("%s", str.data()); }
		static inline void script_remark(std::string_view str) noexcept
		{ remark("%s", str.data()); }

		static inline void script_printl(std::string_view str) noexcept
		{ print("%s\n", str.data()); }
		static inline void script_successl(std::string_view str) noexcept
		{ success("%s\n", str.data()); }
		static inline void script_errorl(std::string_view str) noexcept
		{ error("%s\n", str.data()); }
		static inline void script_warningl(std::string_view str) noexcept
		{ warning("%s\n", str.data()); }
		static inline void script_infol(std::string_view str) noexcept
		{ info("%s\n", str.data()); }
		static inline void script_remarkl(std::string_view str) noexcept
		{ remark("%s\n", str.data()); }

		bool binding_mods() noexcept;
		bool bindings() noexcept;
		void unbindings() noexcept;

		bool detours_prevm() noexcept;
		bool detours() noexcept;

		bool assign_entity_class_info() noexcept;

		bool Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value) override;

		void recreate_script_stringtables() noexcept;
		void stringtables_removed() noexcept;

		bool write_func(const gsdk::ScriptFunctionBinding_t *func, bool global, std::size_t ident, std::string &file, bool respect_hide) const noexcept;
		bool write_class(const gsdk::ScriptClassDesc_t *desc, bool global, std::size_t ident, std::string &file, bool respect_hide) const noexcept;
		void write_docs(const std::filesystem::path &dir, const std::vector<const gsdk::ScriptClassDesc_t *> &vec, bool respect_hide) const noexcept;
		void write_docs(const std::filesystem::path &dir, const std::vector<const gsdk::ScriptFunctionBinding_t *> &vec, bool respect_hide) const noexcept;
		void write_vmod_docs(const std::filesystem::path &dir) const noexcept;
		void write_strtables_docs(const std::filesystem::path &dir) const noexcept;
		void write_syms_docs(const std::filesystem::path &dir) const noexcept;
		void write_yaml_docs(const std::filesystem::path &dir) const noexcept;
		void write_fs_docs(const std::filesystem::path &dir) const noexcept;
		void write_cvar_docs(const std::filesystem::path &dir) const noexcept;
		enum class write_enum_how : unsigned char
		{
			flags,
			name,
			normal
		};
		void write_enum_table(std::string &file, std::size_t depth, gsdk::HSCRIPT enum_table, write_enum_how how) const noexcept;
		void write_mem_docs(const std::filesystem::path &dir) const noexcept;
		void write_ffi_docs(const std::filesystem::path &dir) const noexcept;
		void write_ent_docs(const std::filesystem::path &dir) const noexcept;

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

		bool is_map_loaded;
		bool is_map_active;
		bool are_string_tables_created;

		std::filesystem::path game_dir_;
		std::filesystem::path plugins_dir;
		std::filesystem::path root_dir_;

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

		gsdk::HSCRIPT result_table{gsdk::INVALID_HSCRIPT};

		gsdk::HSCRIPT stringtable_table{gsdk::INVALID_HSCRIPT};
		std::unordered_map<std::string, std::unique_ptr<script_stringtable>> script_stringtables;

		bool create_script_stringtable(std::string &&name) noexcept;

		gsdk::HSCRIPT server_init_script{gsdk::INVALID_HSCRIPT};

		std::filesystem::path base_script_path;
		gsdk::HSCRIPT base_script_scope{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT base_script{gsdk::INVALID_HSCRIPT};
		bool base_script_from_file;

		gsdk::HSCRIPT to_string_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT to_int_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT to_float_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT to_bool_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT typeof_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT funcisg_func{gsdk::INVALID_HSCRIPT};

		squirrel_preprocessor pp;

		std::vector<std::filesystem::path> added_paths;

		std::unordered_map<std::filesystem::path, std::unique_ptr<plugin>> plugins;
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
