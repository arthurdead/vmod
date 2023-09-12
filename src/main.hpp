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
#ifdef __VMOD_USING_PREPROCESSOR
#include "preprocessor.hpp"
#endif
#include "bindings/singleton.hpp"
#include "bindings/strtables/string_table.hpp"
#include "gsdk/engine/dt_send.hpp"
#include "gsdk/server/datamap.hpp"
#include "plugin.hpp"
#include "mod.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Wredundant-tags"
#pragma GCC diagnostic ignored "-Wconditionally-supported"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <steam/isteamremotestorage.h>
#include <steam/isteamugc.h>
#pragma GCC diagnostic pop

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

	extern std::unordered_map<std::string, gsdk::ScriptClassDesc_t *> sv_script_class_descs;

	class main final : public bindings::singleton_base
	{
		friend class vsp;
		friend void bindings::strtables::write_docs(const std::filesystem::path &) noexcept;

	public:
		main() noexcept;
		~main() noexcept override;

		inline bool map_is_loaded() const noexcept
		{ return is_map_loaded; }
		inline bool map_is_active() const noexcept
		{ return is_map_active; }
		inline bool string_tables_created() const noexcept
		{ return are_string_tables_created; }

		std::string_view to_string(vscript::handle_ref value) const noexcept;
		int to_int(vscript::handle_ref value) const noexcept;
		float to_float(vscript::handle_ref value) const noexcept;
		bool to_bool(vscript::handle_ref value) const noexcept;
		std::string_view type_of(vscript::handle_ref value) const noexcept;

		inline gsdk::CVarDLLIdentifier_t cvar_dll_id() const noexcept
		{ return cvar_dll_id_; }

		inline IScriptVM *vm() noexcept
		{ return vm_; }

		inline const std::filesystem::path &game_dir() const noexcept
		{ return game_dir_; }
		inline const std::filesystem::path &root_dir() const noexcept
		{ return root_dir_; }
		inline const std::filesystem::path &mods_dir() const noexcept
		{ return mods_dir_; }
		inline std::string_view scripts_extension() const noexcept
		{ return scripts_extension_; }

	#ifdef __VMOD_USING_PREPROCESSOR
		inline squirrel_preprocessor &preprocessor() noexcept
		{ return pp; }
	#endif

	#ifndef GSDK_NO_SYMBOLS
		inline vscript::handle_ref symbols_table() noexcept
		{ return symbols_table_; }

		inline const symbol_cache &sv_syms() const noexcept
		{ return server_lib.symbols(); }

		inline const symbol_cache &eng_syms() const noexcept
		{ return engine_lib.symbols(); }
	#endif

		static main &instance() noexcept;

		bool dump_scripts() const noexcept;

		void add_search_path(const std::filesystem::path &path, std::string_view id = {}, gsdk::SearchPathAdd_t priority = gsdk::PATH_ADD_TO_TAIL) noexcept;
		void remove_search_path(const std::filesystem::path &path, std::string_view id = {}) noexcept;

	private:
		static void CreateNetworkStringTables_detour_callback(gsdk::IServerGameDLL *dll);
		static void RemoveAllTables_detour_callback(gsdk::IServerNetworkStringTableContainer *cont);

	#ifdef __VMOD_USING_PREPROCESSOR
		friend class squirrel_preprocessor;
	#endif

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

		void load_mods(const std::filesystem::path &dir) noexcept;

		static vscript::singleton_class_desc<main> desc;

		static vscript::variant call_to_func(vscript::handle_ref func, vscript::handle_ref value) noexcept;

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

		vscript::handle_ref script_find_mod(std::string_view mdname) noexcept;
		bool script_is_map_loaded() const noexcept;
		bool script_is_map_active() const noexcept;
		bool script_are_stringtables_created() const noexcept;

		std::filesystem::path build_mod_path(std::string_view plname) const noexcept;

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
		std::filesystem::path addons_dir_;
		std::filesystem::path mods_dir_;
		std::filesystem::path root_dir_;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		std::filesystem::path mount_dir_;
	#endif

		gsdk::CVarDLLIdentifier_t cvar_dll_id_{gsdk::INVALID_CVAR_DLL_IDENTIFIER};

		static ISteamUGC *ugc() noexcept;

		struct workshop_item_t
		{
			workshop_item_t() = delete;

			workshop_item_t(workshop_item_t &&) = delete;
			workshop_item_t &operator=(workshop_item_t &&) = delete;

			workshop_item_t(const workshop_item_t &) = delete;
			workshop_item_t &operator=(const workshop_item_t &) = delete;

			~workshop_item_t() noexcept;

			inline workshop_item_t(PublishedFileId_t id_) noexcept
				: id{id_}
			{
			}

			void query_details() noexcept;
			void download() noexcept;
			void refresh() noexcept;
			void upkeep() noexcept;
			void cancel() noexcept;
			void unmount() noexcept;
			void mount() noexcept;
			void init() noexcept;

			enum class state : unsigned char
			{
				unknown =                    0,
				initialized =          (1 << 0),
				details_queried =      (1 << 1),
				metadata_queried =     (1 << 2),
				details_query_active = (1 << 3),
				downloaded =           (1 << 4),
				download_active =      (1 << 5),
				installed =            (1 << 6),
				mounted =              (1 << 7)
			};
			friend constexpr inline bool operator&(state lhs, state rhs) noexcept
			{ return static_cast<bool>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); }
			friend constexpr inline state operator|(state lhs, state rhs) noexcept
			{ return static_cast<enum state>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); }
			friend constexpr inline state operator~(state lhs) noexcept
			{ return static_cast<enum state>(~static_cast<unsigned char>(lhs)); }
			friend constexpr inline state &operator&=(state &lhs, state rhs) noexcept
			{ lhs = static_cast<enum state>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); return lhs; }
			friend constexpr inline state &operator|=(state &lhs, state rhs) noexcept
			{ lhs = static_cast<enum state>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); return lhs; }

			enum class mounttype : unsigned char
			{
				unknown,
				vpk,
				search_path
			};

			enum class itemtype : unsigned char
			{
				unknown,
				file,
				collection
			};

			PublishedFileId_t id;
			AppId_t app{k_uAppIdInvalid};
			std::string title{};
			std::string metadata{};
			std::filesystem::path cloud_path{};
			std::filesystem::path disk_path{};
			std::filesystem::path mount_path{};
			std::size_t cloud_filesize{0};
			std::size_t disk_filesize{0};
			std::size_t disk_timestamp{0};
			state state{state::unknown};
			itemtype itemtype{itemtype::unknown};
			mounttype mounttype{mounttype::unknown};
			UGCQueryHandle_t query_details_handle{k_UGCQueryHandleInvalid};
			CCallResult<workshop_item_t, SteamUGCQueryCompleted_t> query_details_call{};

			void on_installed() noexcept;
			void on_details_queried(SteamUGCQueryCompleted_t *result, bool err) noexcept;
		};
		std::unordered_map<PublishedFileId_t, std::unique_ptr<workshop_item_t>> workshop_items;

		float next_workshop_upkeep{0.0f};

		std::filesystem::path workshop_dir_;
		bool workshop_initalized{false};
		ConCommand vmod_workshop_track;
		ConCommand vmod_workshop_untrack;
		ConCommand vmod_workshop_status;
		ConCommand vmod_workshop_refresh;
		ConCommand vmod_workshop_clear;

		void workshop_item_downloaded(DownloadItemResult_t *result) noexcept;
		CCallback<main, DownloadItemResult_t, false> cl_downloaditem_callback;
		CCallback<main, DownloadItemResult_t, true> sv_downloaditem_callback;

		void workshop_item_installed(ItemInstalled_t *result) noexcept;
		CCallback<main, ItemInstalled_t, false> cl_iteminstalled_callback;
		CCallback<main, ItemInstalled_t, true> sv_iteminstalled_callback;

		void logged_on(SteamServersConnected_t *result) noexcept;
		CCallback<main, SteamServersConnected_t, true> cl_loggedon_callback;
		CCallback<main, SteamServersConnected_t, true> sv_loggedon_callback;

		void logon_failed(SteamServerConnectFailure_t *result) noexcept;
		CCallback<main, SteamServerConnectFailure_t, true> cl_logon_failed_callback;
		CCallback<main, SteamServerConnectFailure_t, true> sv_logon_failed_callback;

		void logged_off(SteamServersDisconnected_t *result) noexcept;
		CCallback<main, SteamServersDisconnected_t, true> cl_logged_off_callback;
		CCallback<main, SteamServersDisconnected_t, true> sv_logged_off_callback;

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		gsdk_tier0_library tier0_lib;
	#endif
		gsdk_engine_library engine_lib;
		gsdk_server_library server_lib;
		gsdk_launcher_library launcher_lib;
		gsdk_vstdlib_library vstdlib_lib;
		gsdk_vscript_library vscript_lib;
		gsdk_filesystem_library filesystem_lib;

		std::string_view scripts_extension_;

		IScriptVM *vm_{nullptr};

		vscript::handle_wrapper return_flags_table{};

		vscript::handle_wrapper stringtable_table{};
		std::unordered_map<std::string, std::unique_ptr<bindings::strtables::string_table>> script_stringtables;

	#ifndef GSDK_NO_SYMBOLS
		vscript::handle_wrapper symbols_table_{};
	#endif

		vscript::handle_wrapper server_init_script{};

		vscript::handle_wrapper base_script_scope{};
		vscript::handle_wrapper base_script{};

		vscript::handle_wrapper to_string_func{};
		vscript::handle_wrapper to_int_func{};
		vscript::handle_wrapper to_float_func{};
		vscript::handle_wrapper to_bool_func{};
		vscript::handle_wrapper typeof_func{};
		vscript::handle_wrapper funcisg_func{};

	#ifdef __VMOD_USING_PREPROCESSOR
		squirrel_preprocessor pp;
	#endif

		std::unordered_map<std::filesystem::path, std::unique_ptr<mod>> mods;
		bool mods_loaded{false};

		ConCommand vmod_reload_mods;
		ConCommand vmod_unload_mods;
		ConCommand vmod_unload_mod;
		ConCommand vmod_load_mod;
		ConCommand vmod_list_mods;
		ConCommand vmod_refresh_mods;

		ConCommand vmod_dump_internal_scripts;
		ConVar vmod_auto_dump_internal_scripts;

		ConCommand vmod_dump_squirrel_ver;
		ConVar vmod_auto_dump_squirrel_ver;

		ConCommand vmod_dump_netprops;
		ConVar vmod_auto_dump_netprops;

		ConCommand vmod_dump_datamaps;
		ConVar vmod_auto_dump_datamaps;

		ConCommand vmod_dump_keyvalues;
		ConVar vmod_auto_dump_keyvalues;

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

		ConCommand vmod_gen_script_docs;
		ConVar vmod_auto_gen_script_docs;

	private:
		main(const main &) = delete;
		main &operator=(const main &) = delete;
		main(main &&) = delete;
		main &operator=(main &&) = delete;
	};
}
