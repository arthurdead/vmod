#include "vmod.hpp"
#include "symbol_cache.hpp"
#include "vscript.hpp"
#include "gsdk/engine/vsp.hpp"
#include "gsdk/tier0/dbg.hpp"
#include <cstring>

#include "plugin.hpp"
#include "filesystem.hpp"
#include "gsdk/server/gamerules.hpp"
#include "gsdk/server/baseentity.hpp"
#include "gsdk/server/datamap.hpp"

#include <filesystem>
#include <string_view>
#include <climits>

#include <iostream>
#include "glob.h"

#include "convar.hpp"
#include <utility>

#include "gsdk/tier1/utlstring.hpp"
#include "yaml.hpp"
#include "ffi.hpp"

namespace vmod
{
#if __has_include("vmod_base.nut.h")
#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
#endif
	#include "vmod_base.nut.h"
#ifdef __clang__
	#pragma clang diagnostic pop
#endif
	#define __VMOD_BASE_SCRIPT_HEADER_INCLUDED
#endif
}

namespace vmod
{
	class vmod vmod;

	struct entity_class_info
	{
		gsdk::ServerClass *sv_class;
		gsdk::SendTable *sendtable;
		gsdk::datamap_t *datamap;
		gsdk::ScriptClassDesc_t *script_desc;
	};

	static std::unordered_map<std::string, gsdk::ScriptClassDesc_t *> sv_script_class_descs;
	static std::unordered_map<std::string, gsdk::datamap_t *> sv_datamaps;
	static std::unordered_map<std::string, gsdk::SendTable *> sv_sendtables;
	static std::unordered_map<std::string, entity_class_info> sv_ent_class_info;

	gsdk::HSCRIPT vmod::script_find_plugin(std::string_view name) noexcept
	{
		using namespace std::literals::string_view_literals;

		if(name.empty()) {
			vm_->RaiseException("vmod: invalid name");
			return nullptr;
		}

		std::filesystem::path path{name};
		if(!path.is_absolute()) {
			path = (plugins_dir/path);
		}
		if(!path.has_extension()) {
			path.replace_extension(scripts_extension);
		}

		if(path.extension() != scripts_extension) {
			vm_->RaiseException("vmod: invalid extension");
			return nullptr;
		}

		auto it{plugins.find(path)};
		if(it != plugins.end()) {
			return it->second->instance();
		}

		return nullptr;
	}

	static singleton_class_desc_t<class vmod> vmod_desc{"__vmod_singleton_class"};

	inline class vmod &vmod::instance() noexcept
	{ return ::vmod::vmod; }

	bool vmod::Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value)
	{
		return vm_->GetValue(vs_instance_, name.c_str(), &value);
	}

	class lib_symbols_singleton
	{
	protected:
		lib_symbols_singleton() noexcept = default;

	public:
		virtual ~lib_symbols_singleton() noexcept;

	private:
		friend class vmod;

		static gsdk::HSCRIPT script_lookup_shared(symbol_cache::const_iterator it) noexcept;
		static gsdk::HSCRIPT script_lookup_shared(symbol_cache::qualification_info::const_iterator it) noexcept;

		struct script_qual_it_t final : public plugin::owned_instance
		{
			~script_qual_it_t() noexcept override;

			gsdk::HSCRIPT script_lookup(std::string_view name) const noexcept
			{
				using namespace std::literals::string_view_literals;

				gsdk::IScriptVM *vm{vmod.vm()};

				if(name.empty()) {
					vm->RaiseException("vmod: invalid name");
					return nullptr;
				}

				std::string name_tmp{name};

				auto tmp_it{it_->second->find(name_tmp)};
				if(tmp_it == it_->second->end()) {
					return nullptr;
				}

				return script_lookup_shared(tmp_it);
			}

			inline std::string_view script_name() const noexcept
			{ return it_->first; }

			symbol_cache::const_iterator it_;
			gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		};

		struct script_name_it_t final : public plugin::owned_instance
		{
			~script_name_it_t() noexcept override;

			gsdk::HSCRIPT script_lookup(std::string_view name) const noexcept
			{
				using namespace std::literals::string_view_literals;

				gsdk::IScriptVM *vm{vmod.vm()};

				if(name.empty()) {
					vm->RaiseException("vmod: invalid name");
					return nullptr;
				}

				std::string name_tmp{name};

				auto tmp_it{it_->second->find(name_tmp)};
				if(tmp_it == it_->second->end()) {
					return nullptr;
				}

				return script_lookup_shared(tmp_it);
			}

			inline std::string_view script_name() const noexcept
			{ return it_->first; }
			inline void *script_addr() const noexcept
			{ return it_->second->addr<void *>(); }
			inline generic_func_t script_func() const noexcept
			{ return it_->second->func<generic_func_t>(); }
			inline generic_mfp_t script_mfp() const noexcept
			{ return it_->second->mfp<generic_mfp_t>(); }
			inline std::size_t script_size() const noexcept
			{ return it_->second->size(); }
			inline std::size_t script_vindex() const noexcept
			{ return it_->second->virtual_index(); }

			symbol_cache::qualification_info::const_iterator it_;
			gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		};

		static inline class_desc_t<script_qual_it_t> qual_it_desc{"__vmod_sym_qual_it_class"};
		static inline class_desc_t<script_name_it_t> name_it_desc{"__vmod_sym_name_it_class"};

		virtual const symbol_cache &get_syms() const noexcept = 0;

		gsdk::HSCRIPT script_lookup(std::string_view name) const noexcept
		{
			using namespace std::literals::string_view_literals;

			gsdk::IScriptVM *vm{vmod.vm()};

			if(name.empty()) {
				vm->RaiseException("vmod: invalid name");
				return nullptr;
			}

			std::string name_tmp{name};

			const symbol_cache &syms{get_syms()};

			auto it{syms.find(name_tmp)};
			if(it == syms.end()) {
				return nullptr;
			}

			return script_lookup_shared(it);
		}

		gsdk::HSCRIPT script_lookup_global(std::string_view name) const noexcept
		{
			using namespace std::literals::string_view_literals;

			gsdk::IScriptVM *vm{vmod.vm()};

			if(name.empty()) {
				vm->RaiseException("vmod: invalid name");
				return nullptr;
			}

			std::string name_tmp{name};

			const symbol_cache::qualification_info &syms{get_syms().global()};

			auto it{syms.find(name_tmp)};
			if(it == syms.end()) {
				return nullptr;
			}

			return script_lookup_shared(it);
		}

		static bool bindings() noexcept;
		static void unbindings() noexcept;

		void unregister() noexcept
		{
			if(vs_instance_ && vs_instance_ != gsdk::INVALID_HSCRIPT) {
				vmod.vm()->RemoveInstance(vs_instance_);
			}
		}

	protected:
		gsdk::HSCRIPT vs_instance_{gsdk::INVALID_HSCRIPT};
	};

	lib_symbols_singleton::script_qual_it_t::~script_qual_it_t() noexcept
	{
		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vmod.vm()->RemoveInstance(instance);
		}
	}

	lib_symbols_singleton::script_name_it_t::~script_name_it_t() noexcept
	{
		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vmod.vm()->RemoveInstance(instance);
		}
	}

	lib_symbols_singleton::~lib_symbols_singleton() noexcept
	{
		
	}

	static singleton_class_desc_t<class lib_symbols_singleton> lib_symbols_desc{"__vmod_lib_symbols_class"};

	class server_symbols_singleton final : public lib_symbols_singleton
	{
	public:
		inline server_symbols_singleton() noexcept
		{
		}

		~server_symbols_singleton() noexcept override;

		const symbol_cache &get_syms() const noexcept override;

		static server_symbols_singleton &instance() noexcept;

		bool register_instance() noexcept
		{
			using namespace std::literals::string_view_literals;

			gsdk::IScriptVM *vm{vmod.vm()};

			vs_instance_ = vm->RegisterInstance(&lib_symbols_desc, this);
			if(!vs_instance_ || vs_instance_ == gsdk::INVALID_HSCRIPT) {
				error("vmod: failed to create server symbols instance\n"sv);
				return false;
			}

			vm->SetInstanceUniqeId(vs_instance_, "__vmod_server_symbols_singleton");

			if(!vm->SetValue(vmod.symbols_table(), "sv", vs_instance_)) {
				error("vmod: failed to set server symbols table value\n"sv);
				return false;
			}

			return true;
		}
	};

	server_symbols_singleton::~server_symbols_singleton() noexcept {}

	const symbol_cache &server_symbols_singleton::get_syms() const noexcept
	{ return vmod.server_lib.symbols(); }

	static class server_symbols_singleton server_symbols_singleton;

	inline class server_symbols_singleton &server_symbols_singleton::instance() noexcept
	{ return ::vmod::server_symbols_singleton; }

	gsdk::HSCRIPT lib_symbols_singleton::script_lookup_shared(symbol_cache::const_iterator it) noexcept
	{
		script_qual_it_t *script_it{new script_qual_it_t};
		script_it->it_ = it;

		gsdk::IScriptVM *vm{vmod.vm()};

		script_it->instance = vm->RegisterInstance(&qual_it_desc, script_it);
		if(!script_it->instance || script_it->instance == gsdk::INVALID_HSCRIPT) {
			delete script_it;
			vm->RaiseException("vmod: failed to create symbol qualification iterator instance");
			return nullptr;
		}

		//vm->SetInstanceUniqeId

		script_it->set_plugin();

		return script_it->instance;
	}

	gsdk::HSCRIPT lib_symbols_singleton::script_lookup_shared(symbol_cache::qualification_info::const_iterator it) noexcept
	{
		script_name_it_t *script_it{new script_name_it_t};
		script_it->it_ = it;

		gsdk::IScriptVM *vm{vmod.vm()};

		script_it->instance = vm->RegisterInstance(&name_it_desc, script_it);
		if(!script_it->instance || script_it->instance == gsdk::INVALID_HSCRIPT) {
			delete script_it;
			vm->RaiseException("vmod: failed to create symbol name iterator instance");
			return nullptr;
		}

		//vm->SetInstanceUniqeId

		script_it->set_plugin();

		return script_it->instance;
	}

	void lib_symbols_singleton::unbindings() noexcept
	{
		server_symbols_singleton.unregister();
	}

	bool lib_symbols_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		lib_symbols_desc.func(&lib_symbols_singleton::script_lookup, "script_lookup"sv, "lookup"sv);
		lib_symbols_desc.func(&lib_symbols_singleton::script_lookup_global, "script_lookup_global"sv, "lookup_global"sv);
		lib_symbols_desc.doc_class_name("symbol_cache"sv);

		if(!vm->RegisterClass(&lib_symbols_desc)) {
			error("vmod: failed to register lib symbols script class\n"sv);
			return false;
		}

		qual_it_desc.func(&script_qual_it_t::script_name, "script_name"sv, "name"sv);
		qual_it_desc.func(&script_qual_it_t::script_lookup, "script_lookup"sv, "lookup"sv);
		qual_it_desc.dtor();
		qual_it_desc.base(plugin::owned_instance_desc);
		qual_it_desc.doc_class_name("symbol_qualifier"sv);

		if(!vm->RegisterClass(&qual_it_desc)) {
			error("vmod: failed to register symbol qualification iterator script class\n"sv);
			return false;
		}

		name_it_desc.func(&script_name_it_t::script_name, "script_name"sv, "name"sv);
		name_it_desc.func(&script_name_it_t::script_addr, "script_addr"sv, "addr"sv);
		name_it_desc.func(&script_name_it_t::script_func, "script_func"sv, "func"sv);
		name_it_desc.func(&script_name_it_t::script_mfp, "script_mfp"sv, "mfp"sv);
		name_it_desc.func(&script_name_it_t::script_size, "script_size"sv, "size"sv);
		name_it_desc.func(&script_name_it_t::script_vindex, "script_vindex"sv, "vidx"sv);
		name_it_desc.func(&script_name_it_t::script_lookup, "script_lookup"sv, "lookup"sv);
		name_it_desc.dtor();
		name_it_desc.base(plugin::owned_instance_desc);
		name_it_desc.doc_class_name("symbol_name"sv);

		if(!vm->RegisterClass(&name_it_desc)) {
			error("vmod: failed to register symbol name iterator script class\n"sv);
			return false;
		}

		if(!server_symbols_singleton.register_instance()) {
			return false;
		}

		return true;
	}

	class filesystem_singleton final : public gsdk::ISquirrelMetamethodDelegate
	{
	public:
		~filesystem_singleton() noexcept override;

		static filesystem_singleton &instance() noexcept;

		bool bindings() noexcept;
		void unbindings() noexcept;

	private:
		static int script_globerr(const char *epath, int eerrno)
		{
			//TODO!!!
			//vmod.vm()->RaiseException("vmod: glob error %i on %s:", eerrno, epath);
			return 0;
		}

		static gsdk::HSCRIPT script_glob(std::filesystem::path pattern) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			if(pattern.empty()) {
				vm->RaiseException("vmod: invalid pattern");
				return nullptr;
			}

			glob_t glob;
			if(::glob(pattern.c_str(), GLOB_ERR|GLOB_NOSORT, script_globerr, &glob) != 0) {
				globfree(&glob);
				return nullptr;
			}

			gsdk::HSCRIPT arr{vm->CreateArray()};
			if(!arr || arr == gsdk::INVALID_HSCRIPT) {
				vm->RaiseException("vmod: failed to create array");
				return nullptr;
			}

			for(std::size_t i{0}; i < glob.gl_pathc; ++i) {
				std::string temp{glob.gl_pathv[i]};

				script_variant_t var;
				var.assign<std::string>(std::move(temp));
				vm->ArrayAddToTail(arr, std::move(var));
			}

			globfree(&glob);

			return arr;
		}

		static std::filesystem::path script_join_paths(const script_variant_t *va_args, std::size_t num_args, ...) noexcept
		{
			std::filesystem::path final_path;

			for(std::size_t i{0}; i < num_args; ++i) {
				final_path /= va_args[i].get<std::filesystem::path>();
			}

			return final_path;
		}

		bool Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value) override;

		gsdk::HSCRIPT vs_instance_{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT scope{gsdk::INVALID_HSCRIPT};
		gsdk::CSquirrelMetamethodDelegateImpl *get_impl{nullptr};
	};

	bool filesystem_singleton::Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value)
	{
		return vmod.vm()->GetValue(vs_instance_, name.c_str(), &value);
	}

	filesystem_singleton::~filesystem_singleton() {}

	static class filesystem_singleton filesystem_singleton;

	static singleton_class_desc_t<class filesystem_singleton> filesystem_singleton_desc{"__vmod_filesystem_singleton_class"};

	inline class filesystem_singleton &filesystem_singleton::instance() noexcept
	{ return ::vmod::filesystem_singleton; }

	bool filesystem_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		filesystem_singleton_desc.func(&filesystem_singleton::script_join_paths, "script_join_paths"sv, "join_paths"sv);
		filesystem_singleton_desc.func(&filesystem_singleton::script_glob, "script_glob"sv, "glob"sv);

		if(!vm->RegisterClass(&filesystem_singleton_desc)) {
			error("vmod: failed to register vmod filesystem script class\n"sv);
			return false;
		}

		vs_instance_ = vm->RegisterInstance(&filesystem_singleton_desc, this);
		if(!vs_instance_ || vs_instance_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create vmod filesystem instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(vs_instance_, "__vmod_filesystem_singleton");

		scope = vm->CreateScope("__vmod_fs_scope", nullptr);
		if(!scope || scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create filesystem scope\n"sv);
			return false;
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(!vm->SetValue(vmod_scope, "fs", scope)) {
			error("vmod: failed to set filesystem scope value\n"sv);
			return false;
		}

		if(!vm->SetValue(scope, "game_dir", vmod.game_dir().c_str())) {
			error("vmod: failed to set game dir value\n"sv);
			return false;
		}

		get_impl = vm->MakeSquirrelMetamethod_Get(vmod_scope, "fs", this, false);
		if(!get_impl) {
			error("vmod: failed to create filesystem _get metamethod\n"sv);
			return false;
		}

		return true;
	}

	void filesystem_singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(vs_instance_ && vs_instance_ != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(vs_instance_);
		}

		if(get_impl) {
			vm->DestroySquirrelMetamethod_Get(get_impl);
		}

		if(vm->ValueExists(scope, "game_dir")) {
			vm->ClearValue(scope, "game_dir");
		}

		if(scope && scope != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScope(scope);
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(vm->ValueExists(vmod_scope, "fs")) {
			vm->ClearValue(vmod_scope, "fs");
		}
	}

	class script_stringtable final
	{
	public:
		~script_stringtable() noexcept;

		script_stringtable(const script_stringtable &) noexcept = delete;
		script_stringtable &operator=(const script_stringtable &) noexcept = delete;
		inline script_stringtable(script_stringtable &&other) noexcept
		{ operator=(std::move(other)); }
		inline script_stringtable &operator=(script_stringtable &&other) noexcept
		{
			instance = other.instance;
			other.instance = gsdk::INVALID_HSCRIPT;
			table = other.table;
			other.table = nullptr;
			return *this;
		}

		std::size_t script_find_index(std::string_view name) const noexcept;
		std::size_t script_num_strings() const noexcept;
		std::string_view script_get_string(std::size_t i) const noexcept;
		std::size_t script_add_string(std::string_view name, ssize_t bytes, const void *data) noexcept;

	private:
		friend class vmod;

		script_stringtable() noexcept = default;

		gsdk::INetworkStringTable *table;
		gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
	};

	static class_desc_t<script_stringtable> stringtable_desc{"__vmod_stringtable_class"};

	static gsdk::INetworkStringTable *m_pDownloadableFileTable;
	static gsdk::INetworkStringTable *m_pModelPrecacheTable;
	static gsdk::INetworkStringTable *m_pGenericPrecacheTable;
	static gsdk::INetworkStringTable *m_pSoundPrecacheTable;
	static gsdk::INetworkStringTable *m_pDecalPrecacheTable;

	static gsdk::INetworkStringTable *g_pStringTableParticleEffectNames;
	static gsdk::INetworkStringTable *g_pStringTableEffectDispatch;
	static gsdk::INetworkStringTable *g_pStringTableVguiScreen;
	static gsdk::INetworkStringTable *g_pStringTableMaterials;
	static gsdk::INetworkStringTable *g_pStringTableInfoPanel;
	static gsdk::INetworkStringTable *g_pStringTableClientSideChoreoScenes;

	std::size_t script_stringtable::script_find_index(std::string_view name) const noexcept
	{
		gsdk::IScriptVM *vm{::vmod::vmod.vm()};

		if(name.empty()) {
			vm->RaiseException("vmod: invalid name");
			return static_cast<std::size_t>(-1);
		}

		if(!table) {
			vm->RaiseException("vmod: stringtable is not created yet");
			return static_cast<std::size_t>(-1);
		}

		return static_cast<std::size_t>(table->FindStringIndex(name.data()));
	}

	std::size_t script_stringtable::script_num_strings() const noexcept
	{
		gsdk::IScriptVM *vm{::vmod::vmod.vm()};

		if(!table) {
			vm->RaiseException("vmod: stringtable is not created yet");
			return static_cast<std::size_t>(-1);
		}

		return static_cast<std::size_t>(table->GetNumStrings());
	}

	std::string_view script_stringtable::script_get_string(std::size_t i) const noexcept
	{
		gsdk::IScriptVM *vm{::vmod::vmod.vm()};

		if(i == static_cast<std::size_t>(-1) || static_cast<int>(i) >= table->GetNumStrings()) {
			vm->RaiseException("vmod: invalid index");
			return {};
		}

		if(!table) {
			vm->RaiseException("vmod: stringtable is not created yet");
			return {};
		}

		return table->GetString(static_cast<int>(i));
	}

	std::size_t script_stringtable::script_add_string(std::string_view name, ssize_t bytes, const void *data) noexcept
	{
		gsdk::IScriptVM *vm{::vmod::vmod.vm()};

		if(name.empty()) {
			vm->RaiseException("vmod: invalid string");
			return static_cast<std::size_t>(-1);
		}

		if(!table) {
			vm->RaiseException("vmod: stringtable is not created yet");
			return static_cast<std::size_t>(-1);
		}

		return static_cast<std::size_t>(table->AddString(true, name.data(), bytes, data));
	}

	script_stringtable::~script_stringtable() noexcept
	{
		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			::vmod::vmod.vm()->RemoveInstance(instance);
		}
	}

	void vmod::stringtables_removed() noexcept
	{
		are_string_tables_created = false;

		for(const auto &it : script_stringtables) {
			it.second->table = nullptr;
		}
	}

	void vmod::recreate_script_stringtables() noexcept
	{
		are_string_tables_created = true;

		using namespace std::literals::string_literals;

		static auto set_table_value{
			[this](std::string &&name, gsdk::INetworkStringTable *value) noexcept -> void {
				auto it{script_stringtables.find(name)};
				if(it != script_stringtables.end()) {
					it->second->table = value;
				}
			}
		};

		set_table_value(gsdk::DOWNLOADABLE_FILE_TABLENAME, m_pDownloadableFileTable);
		set_table_value(gsdk::MODEL_PRECACHE_TABLENAME, m_pModelPrecacheTable);
		set_table_value(gsdk::GENERIC_PRECACHE_TABLENAME, m_pGenericPrecacheTable);
		set_table_value(gsdk::SOUND_PRECACHE_TABLENAME, m_pSoundPrecacheTable);
		set_table_value(gsdk::DECAL_PRECACHE_TABLENAME, m_pDecalPrecacheTable);

		set_table_value("ParticleEffectNames"s, g_pStringTableParticleEffectNames);
		set_table_value("EffectDispatch"s, g_pStringTableEffectDispatch);
		set_table_value("VguiScreen"s, g_pStringTableVguiScreen);
		set_table_value("Materials"s, g_pStringTableMaterials);
		set_table_value("InfoPanel"s, g_pStringTableInfoPanel);
		set_table_value("Scenes"s, g_pStringTableClientSideChoreoScenes);

		for(const auto &it : plugins) {
			if(!*it.second) {
				continue;
			}

			it.second->string_tables_created();
		}
	}

	bool vmod::create_script_stringtable(std::string &&name) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::unique_ptr<script_stringtable> ptr{new script_stringtable};

		gsdk::HSCRIPT table_instance{vm_->RegisterInstance(&stringtable_desc, ptr.get())};
		if(!table_instance || table_instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create stringtable '%s' instance\n"sv);
			return false;
		}

		if(!vm_->SetValue(stringtable_table, name.c_str(), table_instance)) {
			vm_->RemoveInstance(table_instance);
			error("vmod: failed to set stringtable '%s' value\n"sv);
			return false;
		}

		ptr->instance = table_instance;
		ptr->table = nullptr;

		script_stringtables.emplace(std::move(name), std::move(ptr));

		return true;
	}

	class cvar_singleton : public gsdk::ISquirrelMetamethodDelegate
	{
	public:
		~cvar_singleton() noexcept override;

		static cvar_singleton &instance() noexcept;

		bool bindings() noexcept;
		void unbindings() noexcept;

	private:
		friend class vmod;

		class script_cvar final : public plugin::owned_instance
		{
		public:
			~script_cvar() noexcept override;

		private:
			friend class cvar_singleton;

			inline int script_get_value_int() const noexcept
			{ return var->ConVar::GetInt(); }
			inline float script_get_value_float() const noexcept
			{ return var->ConVar::GetFloat(); }
			inline std::string_view script_get_value_string() const noexcept
			{ return var->ConVar::GetString(); }
			inline bool script_get_value_bool() const noexcept
			{ return var->ConVar::GetBool(); }

			inline void script_set_value_int(int value) const noexcept
			{ var->ConVar::SetValue(value); }
			inline void script_set_value_float(float value) const noexcept
			{ var->ConVar::SetValue(value); }
			inline void script_set_value_string(std::string_view value) const noexcept
			{ var->ConVar::SetValue(value.data()); }
			inline void script_set_value_bool(bool value) const noexcept
			{ var->ConVar::SetValue(value); }

			inline void script_set_value(script_variant_t value) const noexcept
			{
				switch(value.m_type) {
					case gsdk::FIELD_CSTRING:
					var->ConVar::SetValue(value.m_pszString);
					break;
					case gsdk::FIELD_INTEGER:
					var->ConVar::SetValue(value.m_int);
					break;
					case gsdk::FIELD_FLOAT:
					var->ConVar::SetValue(value.m_float);
					break;
					case gsdk::FIELD_BOOLEAN:
					var->ConVar::SetValue(value.m_bool);
					break;
					default:
					vmod.vm()->RaiseException("vmod: invalid type");
					break;
				}
			}

			inline script_variant_t script_get_value() const noexcept
			{
				const char *str{var->InternalGetString()};
				std::size_t len{var->InternalGetStringLength()};

				if(std::strncmp(str, "true", len) == 0) {
					return true;
				} else if(std::strncmp(str, "false", len) == 0) {
					return false;
				} else {
					bool is_float{false};

					for(std::size_t i{0}; i < len; ++i) {
						if(str[i] == '.') {
							is_float = true;
							continue;
						}

						if(!std::isdigit(str[i])) {
							return std::string_view{str};
						}
					}

					if(is_float) {
						return var->GetFloat();
					} else {
						return var->GetInt();
					}
				}
			}

			gsdk::ConVar *var;
			bool free_var;
			gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		};

		static inline class_desc_t<script_cvar> script_cvar_desc{"__vmod_script_cvar_class"};

		static gsdk::HSCRIPT script_create_cvar(std::string_view name, std::string_view value) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			if(name.empty()) {
				vm->RaiseException("vmod: invalid name");
				return nullptr;
			}

			if(cvar->FindCommandBase(name.data()) != nullptr) {
				vm->RaiseException("vmod: name already in use");
				return nullptr;
			}

			script_cvar *svar{new script_cvar};

			gsdk::HSCRIPT cvar_instance{vm->RegisterInstance(&script_cvar_desc, svar)};
			if(!cvar_instance || cvar_instance == gsdk::INVALID_HSCRIPT) {
				delete svar;
				vm->RaiseException("vmod: failed to register instance");
				return nullptr;
			}

			ConVar *var{new ConVar};
			var->initialize(name, value);

			svar->var = var;
			svar->free_var = true;
			svar->instance = cvar_instance;

			svar->set_plugin();

			return svar->instance;
		}

		static gsdk::HSCRIPT script_find_cvar(std::string_view name) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			if(name.empty()) {
				vm->RaiseException("vmod: invalid name");
				return nullptr;
			}

			gsdk::ConVar *var{cvar->FindVar(name.data())};
			if(!var) {
				return nullptr;
			}

			script_cvar *svar{new script_cvar};

			gsdk::HSCRIPT cvar_instance{vm->RegisterInstance(&script_cvar_desc, svar)};
			if(!cvar_instance || cvar_instance == gsdk::INVALID_HSCRIPT) {
				delete svar;
				vm->RaiseException("vmod: failed to register instance");
				return nullptr;
			}

			svar->var = var;
			svar->free_var = false;
			svar->instance = cvar_instance;

			svar->set_plugin();

			return svar->instance;
		}

		bool Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value) override;

		gsdk::HSCRIPT flags_table{gsdk::INVALID_HSCRIPT};

		gsdk::HSCRIPT vs_instance_{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT scope{gsdk::INVALID_HSCRIPT};
		gsdk::CSquirrelMetamethodDelegateImpl *get_impl{nullptr};
	};

	bool cvar_singleton::Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value)
	{
		return vmod.vm()->GetValue(vs_instance_, name.c_str(), &value);
	}

	cvar_singleton::~cvar_singleton() {}

	static class cvar_singleton cvar_singleton;

	static singleton_class_desc_t<class cvar_singleton> cvar_singleton_desc{"__vmod_cvar_singleton_class"};

	inline class cvar_singleton &cvar_singleton::instance() noexcept
	{ return ::vmod::cvar_singleton; }

	cvar_singleton::script_cvar::~script_cvar() noexcept
	{
		if(free_var) {
			delete var;
		}

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vmod.vm()->RemoveInstance(instance);
		}
	}

	bool cvar_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		cvar_singleton_desc.func(&cvar_singleton::script_create_cvar, "script_create_cvar"sv, "create_var"sv);
		cvar_singleton_desc.func(&cvar_singleton::script_find_cvar, "script_find_cvar"sv, "find_var"sv);

		if(!vm->RegisterClass(&cvar_singleton_desc)) {
			error("vmod: failed to register vmod cvar singleton script class\n"sv);
			return false;
		}

		script_cvar_desc.func(&script_cvar::script_set_value, "script_script_set_value"sv, "set"sv);
		script_cvar_desc.func(&script_cvar::script_set_value_string, "script_set_value_string"sv, "set_string"sv);
		script_cvar_desc.func(&script_cvar::script_set_value_float, "script_set_value_float"sv, "set_float"sv);
		script_cvar_desc.func(&script_cvar::script_set_value_int, "script_set_value_int"sv, "set_int"sv);
		script_cvar_desc.func(&script_cvar::script_set_value_bool, "script_set_value_bool"sv, "set_bool"sv);
		script_cvar_desc.func(&script_cvar::script_get_value, "script_script_get_value"sv, "get"sv);
		script_cvar_desc.func(&script_cvar::script_get_value_string, "script_get_value_string"sv, "string"sv);
		script_cvar_desc.func(&script_cvar::script_get_value_float, "script_get_value_float"sv, "float"sv);
		script_cvar_desc.func(&script_cvar::script_get_value_int, "script_get_value_int"sv, "int"sv);
		script_cvar_desc.func(&script_cvar::script_get_value_bool, "script_get_value_bool"sv, "bool"sv);
		script_cvar_desc.dtor();
		script_cvar_desc.base(plugin::owned_instance_desc);
		script_cvar_desc.doc_class_name("convar"sv);

		if(!vm->RegisterClass(&script_cvar_desc)) {
			error("vmod: failed to register vmod cvar script class\n"sv);
			return false;
		}

		vs_instance_ = vm->RegisterInstance(&cvar_singleton_desc, this);
		if(!vs_instance_ || vs_instance_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create vmod cvar singleton instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(vs_instance_, "__vmod_cvar_singleton");

		scope = vm->CreateScope("__vmod_cvar_scope", nullptr);
		if(!scope || scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create cvar scope\n"sv);
			return false;
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(!vm->SetValue(vmod_scope, "cvar", scope)) {
			error("vmod: failed to set cvar scope value\n"sv);
			return false;
		}

		get_impl = vm->MakeSquirrelMetamethod_Get(vmod_scope, "cvar", this, false);
		if(!get_impl) {
			error("vmod: failed to create cvar _get metamethod\n"sv);
			return false;
		}

		flags_table = vm->CreateTable();
		if(!flags_table || flags_table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create cvar flags table\n"sv);
			return false;
		}

		if(!vm->SetValue(scope, "flags", flags_table)) {
			error("vmod: failed to set cvar flags table value\n"sv);
			return false;
		}

		{
			if(!vm->SetValue(flags_table, "none", script_variant_t{gsdk::FCVAR_NONE})) {
				error("vmod: failed to set cvar none flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "development", script_variant_t{gsdk::FCVAR_DEVELOPMENTONLY})) {
				error("vmod: failed to set cvar development flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "hidden", script_variant_t{gsdk::FCVAR_HIDDEN})) {
				error("vmod: failed to set cvar hidden flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "protected", script_variant_t{gsdk::FCVAR_PROTECTED})) {
				error("vmod: failed to set cvar protected flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "singleplayer", script_variant_t{gsdk::FCVAR_SPONLY})) {
				error("vmod: failed to set cvar singleplayer flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "printable_only", script_variant_t{gsdk::FCVAR_PRINTABLEONLY})) {
				error("vmod: failed to set cvar printable_only flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "never_string", script_variant_t{gsdk::FCVAR_NEVER_AS_STRING})) {
				error("vmod: failed to set cvar never_string flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "server", script_variant_t{gsdk::FCVAR_GAMEDLL})) {
				error("vmod: failed to set cvar server flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "client", script_variant_t{gsdk::FCVAR_CLIENTDLL})) {
				error("vmod: failed to set cvar client flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "archive", script_variant_t{gsdk::FCVAR_ARCHIVE})) {
				error("vmod: failed to set cvar archive flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "notify", script_variant_t{gsdk::FCVAR_NOTIFY})) {
				error("vmod: failed to set cvar notify flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "userinfo", script_variant_t{gsdk::FCVAR_USERINFO})) {
				error("vmod: failed to set cvar userinfo flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "cheat", script_variant_t{gsdk::FCVAR_CHEAT})) {
				error("vmod: failed to set cvar cheat flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "replicated", script_variant_t{gsdk::FCVAR_REPLICATED})) {
				error("vmod: failed to set cvar replicated flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "only_unconnected", script_variant_t{gsdk::FCVAR_NOT_CONNECTED})) {
				error("vmod: failed to set cvar only_unconnected flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "allowed_in_competitive", script_variant_t{gsdk::FCVAR_ALLOWED_IN_COMPETITIVE})) {
				error("vmod: failed to set cvar allowed_in_competitive flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "internal", script_variant_t{gsdk::FCVAR_INTERNAL_USE})) {
				error("vmod: failed to set cvar internal flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "server_can_exec", script_variant_t{gsdk::FCVAR_SERVER_CAN_EXECUTE})) {
				error("vmod: failed to set cvar server_can_exec flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "server_cant_query", script_variant_t{gsdk::FCVAR_SERVER_CANNOT_QUERY})) {
				error("vmod: failed to set cvar server_cant_query flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "client_can_exec", script_variant_t{gsdk::FCVAR_CLIENTCMD_CAN_EXECUTE})) {
				error("vmod: failed to set cvar client_can_exec flag value\n"sv);
				return false;
			}

			if(!vm->SetValue(flags_table, "exec_in_default", script_variant_t{gsdk::FCVAR_EXEC_DESPITE_DEFAULT})) {
				error("vmod: failed to set cvar exec_in_default flag value\n"sv);
				return false;
			}
		}

		return true;
	}

	void cvar_singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(vs_instance_ && vs_instance_ != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(vs_instance_);
		}

		if(get_impl) {
			vm->DestroySquirrelMetamethod_Get(get_impl);
		}

		if(flags_table && flags_table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(flags_table);
		}

		if(vm->ValueExists(scope, "flags")) {
			vm->ClearValue(scope, "flags");
		}

		if(scope && scope != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScope(scope);
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(vm->ValueExists(vmod_scope, "cvar")) {
			vm->ClearValue(vmod_scope, "cvar");
		}
	}

	class entities_singleton : public gsdk::ISquirrelMetamethodDelegate
	{
	public:
		~entities_singleton() noexcept override;

		static entities_singleton &instance() noexcept;

		bool bindings() noexcept;
		void unbindings() noexcept;

	private:
		friend class vmod;

		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
		class script_factory_impl final : public gsdk::IEntityFactory, public plugin::owned_instance
		{
		public:
			~script_factory_impl() noexcept override;

			inline gsdk::IServerNetworkable *Create(const char *classname) override
			{ return create(classname, size); }

			void Destroy(gsdk::IServerNetworkable *net) override;
			size_t GetEntitySize() override;

			gsdk::IServerNetworkable *create(std::string_view classname, std::size_t size_) noexcept;

			gsdk::IServerNetworkable *script_create(std::string_view classname) noexcept
			{
				gsdk::IScriptVM *vm{::vmod::vmod.vm()};

				if(classname.empty()) {
					vm->RaiseException("vmod: invalid classname");
					return nullptr;
				}

				return create(classname, size);
			}

			gsdk::IServerNetworkable *script_create_sized(std::string_view classname, std::size_t size_) noexcept
			{
				gsdk::IScriptVM *vm{::vmod::vmod.vm()};

				if(classname.empty()) {
					vm->RaiseException("vmod: invalid classname");
					return nullptr;
				}

				if(size_ == 0 || size_ == static_cast<std::size_t>(-1) || size_ < size) {
					vm->RaiseException("vmod: invalid size");
					return nullptr;
				}

				return create(classname, size_);
			}

			inline std::size_t script_size() noexcept
			{ return GetEntitySize(); }

			std::vector<std::string> names;
			std::size_t size;
			gsdk::HSCRIPT func;
			gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		};
		#pragma GCC diagnostic pop

		static inline class_desc_t<script_factory_impl> script_factory_impl_desc{"__vmod_script_entity_factory_impl_class"};

		class script_factory final : public plugin::owned_instance
		{
		public:
			~script_factory() noexcept override;

		private:
			friend class entities_singleton;

			gsdk::IServerNetworkable *script_create(std::string_view classname) noexcept
			{
				gsdk::IScriptVM *vm{::vmod::vmod.vm()};

				if(classname.empty()) {
					vm->RaiseException("vmod: invalid classname");
					return nullptr;
				}

				return factory->Create(classname.data());
			}

			inline std::size_t script_size() const noexcept
			{ return factory->GetEntitySize(); }

			gsdk::IEntityFactory *factory;
			bool free_factory;
			gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		};

		static inline class_desc_t<script_factory> script_factory_desc{"__vmod_script_entity_factory_class"};

		static gsdk::HSCRIPT script_find_factory(std::string_view name) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			if(name.empty()) {
				vm->RaiseException("vmod: invalid name");
				return nullptr;
			}

			gsdk::IEntityFactory *factory{entityfactorydict->FindFactory(name.data())};
			if(!factory) {
				return nullptr;
			}

			script_factory_impl *impl{dynamic_cast<script_factory_impl *>(factory)};
			if(impl) {
				return impl->instance;
			}

			script_factory *sfac{new script_factory};

			gsdk::HSCRIPT factory_instance{vm->RegisterInstance(&script_factory_desc, sfac)};
			if(!factory_instance || factory_instance == gsdk::INVALID_HSCRIPT) {
				delete sfac;
				vm->RaiseException("vmod: failed to register instance");
				return nullptr;
			}

			sfac->factory = factory;
			sfac->free_factory = false;
			sfac->instance = factory_instance;

			sfac->set_plugin();

			return sfac->instance;
		}

		static gsdk::HSCRIPT script_create_factory(std::string_view name, gsdk::HSCRIPT func, std::size_t size) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			if(name.empty()) {
				vm->RaiseException("vmod: invalid name");
				return nullptr;
			}

			if(size == 0 || size == static_cast<std::size_t>(-1)) {
				vm->RaiseException("vmod: invalid size");
				return nullptr;
			}

			if(!func || func == gsdk::INVALID_HSCRIPT) {
				vm->RaiseException("vmod: null function");
				return nullptr;
			}

			gsdk::IEntityFactory *factory{entityfactorydict->FindFactory(name.data())};
			if(factory) {
				vm->RaiseException("vmod: name already in use");
				return nullptr;
			}

			script_factory_impl *sfac{new script_factory_impl};

			gsdk::HSCRIPT factory_instance{vm->RegisterInstance(&script_factory_impl_desc, sfac)};
			if(!factory_instance || factory_instance == gsdk::INVALID_HSCRIPT) {
				delete sfac;
				vm->RaiseException("vmod: failed to register instance");
				return nullptr;
			}

			sfac->names.emplace_back(name);
			sfac->func = vm->ReferenceObject(func);
			sfac->size = size;
			sfac->instance = factory_instance;

			sfac->set_plugin();

			entityfactorydict->InstallFactory(sfac, name.data());

			return sfac->instance;
		}

		static gsdk::HSCRIPT script_from_ptr(gsdk::CBaseEntity *ptr) noexcept
		{
			gsdk::IScriptVM *vm{::vmod::vmod.vm()};

			if(!ptr) {
				vm->RaiseException("vmod: invalid ptr");
				return nullptr;
			}

			return ptr->GetScriptInstance();
		}

		enum class entity_prop_result_type : unsigned char
		{
			none,
			prop,
			table
		};

		struct entity_prop_data_result
		{
			entity_prop_result_type type;

			union {
				gsdk::datamap_t *table;
				gsdk::typedescription_t *prop;
			};
		};

		struct entity_prop_send_result
		{
			entity_prop_result_type type;

			union {
				gsdk::SendTable *table;
				gsdk::SendProp *prop;
			};
		};

		struct entity_prop_result
		{
			enum class which : unsigned char
			{
				none,
				send,
				data,
				both
			};

			enum which which;

			entity_prop_result_type type;

			union {
				gsdk::datamap_t *datatable;
				gsdk::typedescription_t *dataprop;

				gsdk::SendTable *sendtable;
				gsdk::SendProp *sendprop;
			};

			entity_prop_result &operator+=(const entity_prop_data_result &other) noexcept;
			entity_prop_result &operator+=(const entity_prop_send_result &other) noexcept;

			inline explicit operator entity_prop_data_result() const noexcept
			{ return entity_prop_data_result{type, {datatable}}; }
			inline explicit operator entity_prop_send_result() const noexcept
			{ return entity_prop_send_result{type, {sendtable}}; }
		};

		enum class entity_prop_tree_flags : unsigned char
		{
			data =             (1 << 0),
			send =             (1 << 1),
			lazy =             (1 << 2),
			ignore_exclude =   (1 << 3),
			only_prop =        (1 << 4),
			only_table =       (1 << 5),
			both =             (data|send),
		};
		friend constexpr inline bool operator&(entity_prop_tree_flags lhs, entity_prop_tree_flags rhs) noexcept
		{ return static_cast<bool>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); }
		friend constexpr inline entity_prop_tree_flags operator|(entity_prop_tree_flags lhs, entity_prop_tree_flags rhs) noexcept
		{ return static_cast<entity_prop_tree_flags>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); }
		friend constexpr inline entity_prop_tree_flags operator~(entity_prop_tree_flags lhs) noexcept
		{ return static_cast<entity_prop_tree_flags>(~static_cast<unsigned char>(lhs)); }
		friend constexpr inline entity_prop_tree_flags &operator&=(entity_prop_tree_flags &lhs, entity_prop_tree_flags rhs) noexcept
		{ lhs = static_cast<entity_prop_tree_flags>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); return lhs; }
		friend constexpr inline entity_prop_tree_flags &operator|=(entity_prop_tree_flags &lhs, entity_prop_tree_flags rhs) noexcept
		{ lhs = static_cast<entity_prop_tree_flags>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); return lhs; }

		bool walk_entity_prop_tree(std::string_view path, entity_prop_tree_flags flags, entity_prop_result &result) noexcept;

		struct entity_prop_tree_cache_t
		{
			std::unordered_map<std::string, entity_prop_data_result> data;
			std::unordered_map<std::string, entity_prop_send_result> send;

			std::unordered_map<std::string, std::string> lazy_to_full;
		};

		entity_prop_tree_cache_t entity_prop_tree_cache;

		struct proxyhook_info_t;

		struct proxyhook_instance_t final : public plugin::owned_instance
		{
			~proxyhook_instance_t() noexcept override
			{
				if(func && func != gsdk::INVALID_HSCRIPT) {
					auto it{info->instances.find(func)};
					if(it != info->instances.end()) {
						info->instances.erase(it);
					}

					vmod.vm()->ReleaseFunction(func);
				}
			}

			proxyhook_info_t *info;
			gsdk::HSCRIPT func;
			gsdk::HSCRIPT instance;
		};

		static inline cif proxy_cif{&ffi_type_void, {&ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer, &ffi_type_pointer, &ffi_type_sint, &ffi_type_sint}};

		static ffi_type *guess_prop_type(const gsdk::SendProp *prop, gsdk::SendVarProxyFn proxy, const gsdk::SendTable *table) noexcept;

		struct proxyhook_info_t
		{
			static void closure_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr) noexcept;

			inline proxyhook_info_t(gsdk::SendProp *prop_) noexcept
				: prop{prop_},
				old_proxy{prop_->m_ProxyFn},
				type_ptr{guess_prop_type(prop_, prop_->m_ProxyFn, nullptr)}
			{
			}

			bool initialize() noexcept
			{
				closure = static_cast<ffi_closure *>(ffi_closure_alloc(sizeof(ffi_closure), reinterpret_cast<void **>(&prop->m_ProxyFn)));
				if(!closure) {
					return false;
				}

				if(ffi_prep_closure_loc(closure, &proxy_cif, closure_binding, this, reinterpret_cast<void *>(prop->m_ProxyFn)) != FFI_OK) {
					return false;
				}

				return true;
			}

			inline ~proxyhook_info_t() noexcept
			{
				if(closure) {
					ffi_closure_free(closure);
				}
			}

			gsdk::SendProp *prop;
			gsdk::SendVarProxyFn old_proxy;
			ffi_type *type_ptr;

			std::unordered_map<gsdk::HSCRIPT, proxyhook_instance_t *> instances;

			ffi_closure *closure{nullptr};
		};

		std::unordered_map<gsdk::SendProp *, std::unique_ptr<proxyhook_info_t>> proxyhooks;

		gsdk::SendProp *script_lookup_sendprop(std::string_view path) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			if(path.empty()) {
				vm->RaiseException("vmod: invalid path");
				return nullptr;
			}

			entity_prop_result res;

			entity_prop_tree_flags flags{
				entity_prop_tree_flags::only_prop|
				entity_prop_tree_flags::send|
				entity_prop_tree_flags::ignore_exclude|
				entity_prop_tree_flags::lazy
			};
			if(!walk_entity_prop_tree(path, flags, res)) {
				vm->RaiseException("vmod: lookup failed");
				return nullptr;
			}

			return res.sendprop;
		}

		gsdk::HSCRIPT script_hook_send_proxy(gsdk::SendProp *prop, gsdk::HSCRIPT func, bool per_client) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			if(!prop) {
				vm->RaiseException("vmod: invalid prop");
				return nullptr;
			}

			if(!func || func == gsdk::INVALID_HSCRIPT) {
				vm->RaiseException("vmod: invalid function");
				return nullptr;
			}

			auto info_it{proxyhooks.find(prop)};
			if(info_it == proxyhooks.end()) {
				std::unique_ptr<proxyhook_info_t> info{new proxyhook_info_t{prop}};
				if(!info->initialize()) {
					vm->RaiseException("vmod: failed to initialize hook");
					return nullptr;
				}

				info_it = proxyhooks.emplace(prop, std::move(info)).first;
			}

			return nullptr;
		}

		bool Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value) override;

		gsdk::HSCRIPT vs_instance_{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT scope{gsdk::INVALID_HSCRIPT};
		gsdk::CSquirrelMetamethodDelegateImpl *get_impl{nullptr};
	};

	ffi_type *entities_singleton::guess_prop_type(const gsdk::SendProp *prop, gsdk::SendVarProxyFn proxy, const gsdk::SendTable *table) noexcept
	{
		switch(prop->m_Type) {
			case gsdk::DPT_Int: {
				if(prop->m_Flags & gsdk::SPROP_UNSIGNED) {
					if(proxy == std_proxies->m_UInt8ToInt32) {
						if(prop->m_nBits == 1) {
							return &ffi_type_bool;
						}

						return &ffi_type_uchar;
					} else if(proxy == std_proxies->m_UInt16ToInt32) {
						return &ffi_type_ushort;
					} else if(proxy == std_proxies->m_UInt32ToInt32) {
						if(table && std::strcmp(table->m_pNetTableName, "DT_BaseEntity") == 0 && std::strcmp(prop->m_pVarName, "m_clrRender") == 0) {
							return &ffi_type_color32;
						}

						return &ffi_type_uint;
					} else {
						{
							if(prop->m_nBits == 32) {
								struct dummy_t {
									unsigned int val{256};
								} dummy;

								gsdk::DVariant out{};
								proxy(prop, static_cast<const void *>(&dummy), static_cast<const void *>(&dummy.val), &out, 0, static_cast<int>(gsdk::INVALID_EHANDLE_INDEX));
								if(out.m_Int == 65536) {
									return &ffi_type_color32;
								}
							}
						}

						{
							if(prop->m_nBits == gsdk::NUM_NETWORKED_EHANDLE_BITS) {
								struct dummy_t {
									gsdk::EHANDLE val{};
								} dummy;

								gsdk::DVariant out{};
								proxy(prop, static_cast<const void *>(&dummy), static_cast<const void *>(&dummy.val), &out, 0, static_cast<int>(gsdk::INVALID_EHANDLE_INDEX));
								if(out.m_Int == gsdk::INVALID_NETWORKED_EHANDLE_VALUE) {
									return &ffi_type_ehandle;
								}
							}
						}

						return &ffi_type_uint;
					}
				} else {
					if(proxy == std_proxies->m_Int8ToInt32) {
						return &ffi_type_schar;
					} else if(proxy == std_proxies->m_Int16ToInt32) {
						return &ffi_type_sshort;
					} else if(proxy == std_proxies->m_Int32ToInt32) {
						return &ffi_type_sint;
					} else {
						{
							struct dummy_t {
								short val{SHRT_MAX-1};
							} dummy;

							gsdk::DVariant out{};
							proxy(prop, static_cast<const void *>(&dummy), static_cast<const void *>(&dummy.val), &out, 0, static_cast<int>(gsdk::INVALID_EHANDLE_INDEX));
							if(out.m_Int == dummy.val+1) {
								return &ffi_type_sshort;
							}
						}

						return &ffi_type_sint;
					}
				}
			}
			case gsdk::DPT_Float:
			return &ffi_type_float;
			case gsdk::DPT_Vector: {
				if(prop->m_fLowValue == 0.0f && prop->m_fHighValue == 360.0f) {
					return &ffi_type_qangle;
				} else {
					return &ffi_type_vector;
				}
			}
			case gsdk::DPT_VectorXY:
			return &ffi_type_vector;
			case gsdk::DPT_String: {
				return &ffi_type_cstr;
			}
			case gsdk::DPT_Array:
			return nullptr;
			case gsdk::DPT_DataTable:
			return nullptr;
			default:
			return nullptr;
		}
	}

	void entities_singleton::proxyhook_info_t::closure_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr) noexcept
	{
		proxyhook_info_t *prop_info{static_cast<proxyhook_info_t *>(userptr)};

		//TODO!!!

		ffi_call(closure_cif, reinterpret_cast<void(*)()>(prop_info->old_proxy), ret, args);
	}

	bool entities_singleton::Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value)
	{
		return vmod.vm()->GetValue(vs_instance_, name.c_str(), &value);
	}

	entities_singleton::~entities_singleton() {}

	static class entities_singleton entities_singleton;

	static singleton_class_desc_t<class entities_singleton> entities_singleton_desc{"__vmod_entities_singleton_class"};

	inline class entities_singleton &entities_singleton::instance() noexcept
	{ return ::vmod::entities_singleton; }

	entities_singleton::entity_prop_result &entities_singleton::entity_prop_result::operator+=(const entity_prop_data_result &other) noexcept
	{
		switch(which) {
			case which::none:
			which = which::data; break;
			case which::send:
			which = which::both; break;
			default: break;
		}

		switch(other.type) {
			case entity_prop_result_type::prop:
			dataprop = other.prop; break;
			case entity_prop_result_type::table:
			datatable = other.table; break;
			default: break;
		}

		return *this;
	}

	entities_singleton::entity_prop_result &entities_singleton::entity_prop_result::operator+=(const entity_prop_send_result &other) noexcept
	{
		switch(which) {
			case which::none:
			which = which::send; break;
			case which::data:
			which = which::both; break;
			default: break;
		}

		switch(other.type) {
			case entity_prop_result_type::prop:
			sendprop = other.prop; break;
			case entity_prop_result_type::table:
			sendtable = other.table; break;
			default: break;
		}

		return *this;
	}

	bool entities_singleton::walk_entity_prop_tree(std::string_view path, entity_prop_tree_flags flags, entity_prop_result &result) noexcept
	{
		using namespace std::literals::string_view_literals;

		if(!(flags & entity_prop_tree_flags::send) && !(flags & entity_prop_tree_flags::data)) {
			error("vmod: no tree type specified\n"sv);
			return false;
		}

		if((flags & entity_prop_tree_flags::only_prop) && (flags & entity_prop_tree_flags::only_table)) {
			error("vmod: cannot exclude both prop and table\n"sv);
			return false;
		}

		if((flags & entity_prop_tree_flags::send) && (flags & entity_prop_tree_flags::data)) {
			error("vmod: walking both send and data is not supported yet\n"sv);
			return false;
		}

		if(flags & entity_prop_tree_flags::lazy) {
			auto full_it{entity_prop_tree_cache.lazy_to_full.find(std::string{path})};
			if(full_it != entity_prop_tree_cache.lazy_to_full.end()) {
				flags &= ~entity_prop_tree_flags::lazy;
				path = full_it->second;
			}
		}

		if(flags & entity_prop_tree_flags::data) {
			auto datares_it{entity_prop_tree_cache.data.find(std::string{path})};
			if(datares_it != entity_prop_tree_cache.data.end()) {
				flags &= ~entity_prop_tree_flags::data;
				result += datares_it->second;
			}
		}

		if(flags & entity_prop_tree_flags::send) {
			auto sendres_it{entity_prop_tree_cache.send.find(std::string{path})};
			if(sendres_it != entity_prop_tree_cache.send.end()) {
				flags &= ~entity_prop_tree_flags::send;
				result += sendres_it->second;
			}
		}

		if((result.which == entity_prop_result::which::both) ||
			(!(flags & entity_prop_tree_flags::send) && !(flags & entity_prop_tree_flags::data))) {
			return true;
		}

		std::size_t path_len{path.length()};

		std::size_t name_start{path.find('.')};
		if(name_start == std::string_view::npos) {
			name_start = path_len;
		}

		std::string_view classname{path.substr(0, name_start)};

		auto sv_class_it{sv_ent_class_info.find(std::string{classname})};
		if(sv_class_it == sv_ent_class_info.end()) {
			error("vmod: invalid class '%.*s'\n"sv, classname.length(), classname.data());
			return false;
		}

		entity_class_info &class_info{sv_class_it->second};

		gsdk::ServerClass *sv_class{class_info.sv_class};

		gsdk::datamap_t *curr_datamap{nullptr};
		gsdk::typedescription_t *curr_dataprop{nullptr};

		gsdk::SendTable *curr_sendtable{nullptr};
		gsdk::SendProp *curr_sendprop{nullptr};

		std::string_view last_send_name{classname};
		std::string_view last_data_name{classname};

		if(flags & entity_prop_tree_flags::send) {
			curr_sendtable = sv_class->m_pTable;
		}

		if(flags & entity_prop_tree_flags::data) {
			curr_datamap = class_info.datamap;
		}

		std::string full_path;

		if(flags & entity_prop_tree_flags::lazy) {
			full_path = classname;
		}

		if(name_start < path_len) {
			while(true) {
				std::size_t name_end{path.find('.', name_start+1)};
				bool done{name_end == std::string_view::npos};
				if(done) {
					name_end = path_len;
				}

				++name_start;
				std::string_view name{path.substr(name_start, name_end-name_start)};

				std::size_t subscript_start{name.find('[')};
				if(subscript_start != std::string_view::npos) {
					std::size_t subscript_end{name.find(']', subscript_start+1)};
					if(subscript_end == std::string_view::npos) {
						error("vmod: subscript started but not ended\n"sv);
						return false;
					} else if(subscript_end != name.length()) {
						error("vmod: subscript ending must be the last character\n"sv);
						return false;
					}

					std::string_view subscript_num{name.substr(subscript_start, subscript_end-subscript_start)};

					const char *num_begin{subscript_num.data()};
					const char *num_end{subscript_num.data() + subscript_num.length()};

					std::size_t num;
					std::from_chars(num_begin, num_end, num);

					//TODO!!!!
					error("vmod: subscripts are not supported yet\n"sv);
					return false;
				} else {
					if((flags & entity_prop_tree_flags::send) && !curr_sendtable) {
						error("vmod: '%.*s' cannot contain members\n"sv, last_send_name.length(), last_send_name.data());
						return false;
					}

					if((flags & entity_prop_tree_flags::data) && !curr_datamap) {
						error("vmod: '%.*s' cannot contain members\n"sv, last_data_name.length(), last_data_name.data());
						return false;
					}

					unsigned char found{0};

					if(flags & entity_prop_tree_flags::send) {
						bool send_found{false};

						gsdk::SendTable *temp_table{curr_sendtable};

						std::function<void()> loop_props{
							[flags,&full_path,&send_found,&temp_table,&curr_sendprop,name,&loop_props]() noexcept -> void {
								gsdk::SendTable *baseclass{nullptr};
								std::vector<std::pair<const char *, gsdk::SendTable *>> check_later;

								std::size_t num_props{static_cast<std::size_t>(temp_table->m_nProps)};
								for(std::size_t i{0}; i < num_props; ++i) {
									gsdk::SendProp &prop{temp_table->m_pProps[i]};
									if(flags & entity_prop_tree_flags::ignore_exclude) {
										if(prop.m_Flags & gsdk::SPROP_EXCLUDE) {
											continue;
										}
									}

									if(std::strncmp(prop.m_pVarName, name.data(), name.length()) == 0) {
										curr_sendprop = &prop;
										send_found = true;
										return;
									} else if(flags & entity_prop_tree_flags::lazy) {
										if(std::strcmp(prop.m_pVarName, "baseclass") == 0) {
											baseclass = prop.m_pDataTable;
										} else if(prop.m_Type == gsdk::DPT_DataTable) {
											check_later.emplace_back(std::pair<const char *, gsdk::SendTable *>{prop.m_pVarName, prop.m_pDataTable});
										}
									}
								}

								if(flags & entity_prop_tree_flags::lazy) {
									if(!check_later.empty()) {
										for(const auto &it : check_later) {
											temp_table = it.second;
											loop_props();
											if(send_found) {
												full_path += '.';
												full_path += it.first;
												return;
											}
										}
									}

									if(baseclass) {
										temp_table = baseclass;
										full_path += '.';
										full_path += "baseclass"sv;
										loop_props();
										if(send_found) {
											return;
										}
									}
								}
							}
						};

						loop_props();

						if(flags & entity_prop_tree_flags::lazy) {
							full_path += '.';
							full_path += curr_sendprop->m_pVarName;
						}

						if(send_found) {
							found |= (1 << 0);
						}

						if(!send_found) {
							error("vmod: member '%.*s' was not found in '%.*s'\n"sv, name.length(), name.data(), last_send_name.length(), last_send_name.data());
							return false;
						}
					}

					if(flags & entity_prop_tree_flags::data) {
						bool data_found{false};

						gsdk::datamap_t *temp_table{curr_datamap};

						std::function<void()> loop_props{
							[flags,&full_path,&data_found,&temp_table,&curr_dataprop,&curr_datamap,name,&loop_props]() noexcept -> void {
								gsdk::datamap_t *baseclass{temp_table->baseMap};

								if(name == "baseclass"sv) {
									if(baseclass) {
										curr_datamap = baseclass;
										data_found = true;
									} else {
										data_found = false;
									}
									return;
								}

								std::vector<std::pair<const char *, gsdk::datamap_t *>> check_later;

								std::size_t num_props{static_cast<std::size_t>(temp_table->dataNumFields)};
								for(std::size_t i{0}; i < num_props; ++i) {
									gsdk::typedescription_t &prop{temp_table->dataDesc[i]};
									if(std::strncmp(prop.fieldName, name.data(), name.length()) == 0) {
										curr_dataprop = &prop;
										data_found = true;
										return;
									} else if(flags & entity_prop_tree_flags::lazy) {
										if(prop.td) {
											check_later.emplace_back(std::pair<const char *, gsdk::datamap_t *>{prop.fieldName, prop.td});
										}
									}
								}

								if(flags & entity_prop_tree_flags::lazy) {
									full_path += '.';
									full_path += temp_table->dataClassName;

									if(!check_later.empty()) {
										for(const auto &it : check_later) {
											temp_table = it.second;
											loop_props();
											if(data_found) {
												full_path += '.';
												full_path += it.first;
												return;
											}
										}
									}

									if(baseclass) {
										temp_table = baseclass;
										full_path += '.';
										full_path += "baseclass"sv;
										loop_props();
										if(data_found) {
											return;
										}
									}
								}
							}
						};

						loop_props();

						if(flags & entity_prop_tree_flags::lazy) {
							full_path += '.';
							full_path += curr_dataprop->fieldName;
						}

						if(data_found) {
							found |= (1 << 1);
						}

						if(!data_found) {
							error("vmod: member '%.*s' was not found in '%.*s'\n"sv, name.length(), name.data(), last_data_name.length(), last_data_name.data());
							return false;
						}
					}
				}

				if(flags & entity_prop_tree_flags::send) {
					switch(curr_sendprop->m_Type) {
						case gsdk::DPT_DataTable: {
							curr_sendtable = curr_sendprop->m_pDataTable;
							curr_sendprop = nullptr;
						} break;
						default: {
							curr_sendtable = nullptr;
						} break;
					}
				}

				if(flags & entity_prop_tree_flags::data) {
					if(curr_dataprop) {
						if(curr_dataprop->td) {
							curr_datamap = curr_dataprop->td;
							curr_dataprop = nullptr;
						} else {
							curr_datamap = nullptr;
						}
					} else if(curr_datamap) {
						curr_dataprop = nullptr;
					}
				}

				last_send_name = name;
				last_data_name = name;

				name_start = name_end;

				if(done) {
					break;
				}
			}
		}

		if(flags & entity_prop_tree_flags::lazy) {
			entity_prop_tree_cache.lazy_to_full.emplace(path, std::move(full_path));
		}

		if(flags & entity_prop_tree_flags::send) {
			if(flags & entity_prop_tree_flags::only_prop) {
				if(!curr_sendprop) {
					error("vmod: '%.*s' is not a prop\n"sv, last_send_name.length(), last_send_name.data());
					return false;
				}
			} else if(flags & entity_prop_tree_flags::only_table) {
				if(!curr_sendtable) {
					error("vmod: '%.*s' is not a table\n"sv, last_send_name.length(), last_send_name.data());
					return false;
				}
			}
		}

		if(flags & entity_prop_tree_flags::data) {
			if(flags & entity_prop_tree_flags::only_prop) {
				if(!curr_dataprop) {
					error("vmod: '%.*s' is not a prop\n"sv, last_data_name.length(), last_data_name.data());
					return false;
				}
			} else if(flags & entity_prop_tree_flags::only_table) {
				if(!curr_datamap) {
					error("vmod: '%.*s' is not a table\n"sv, last_data_name.length(), last_data_name.data());
					return false;
				}
			}
		}

		if(flags & entity_prop_tree_flags::send) {
			if(curr_sendtable) {
				result.type = entity_prop_result_type::table;
				result.sendtable = curr_sendtable;
			} else if(curr_sendprop) {
				result.type = entity_prop_result_type::prop;
				result.sendprop = curr_sendprop;
			}

			switch(result.which) {
				case entity_prop_result::which::none:
				result.which = entity_prop_result::which::send; break;
				case entity_prop_result::which::data:
				result.which = entity_prop_result::which::both; break;
				default: break;
			}
		}

		if(flags & entity_prop_tree_flags::data) {
			if(curr_datamap) {
				result.type = entity_prop_result_type::table;
				result.datatable = curr_datamap;
			} else if(curr_dataprop) {
				result.type = entity_prop_result_type::prop;
				result.dataprop = curr_dataprop;
			}

			switch(result.which) {
				case entity_prop_result::which::none:
				result.which = entity_prop_result::which::data; break;
				case entity_prop_result::which::send:
				result.which = entity_prop_result::which::both; break;
				default: break;
			}
		}

		if(flags & entity_prop_tree_flags::send) {
			entity_prop_tree_cache.send.emplace(path, static_cast<entity_prop_send_result>(result));
		}

		if(flags & entity_prop_tree_flags::data) {
			entity_prop_tree_cache.data.emplace(path, static_cast<entity_prop_data_result>(result));
		}

		return true;
	}

	entities_singleton::script_factory_impl::~script_factory_impl() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance);
		}

		if(func && func != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseFunction(func);
		}

		for(const std::string &name : names) {
			std::size_t i{entityfactorydict->m_Factories.find(name)};
			if(i != entityfactorydict->m_Factories.npos) {
				entityfactorydict->m_Factories.erase(i);
			}
		}
	}

	gsdk::IServerNetworkable *entities_singleton::script_factory_impl::create(std::string_view classname, std::size_t size_) noexcept
	{
		if(func && func != gsdk::INVALID_HSCRIPT) {
			gsdk::IScriptVM *vm{vmod.vm()};

			gsdk::HSCRIPT pl_scope{owner_scope()};

			std::vector<script_variant_t> args;
			args.emplace_back(instance);
			args.emplace_back(size_);
			args.emplace_back(classname);

			script_variant_t ret_var;
			if(vm->ExecuteFunction(func, args.data(), static_cast<int>(args.size()), &ret_var, pl_scope, true) == gsdk::SCRIPT_ERROR) {
				return nullptr;
			}

			return ret_var.get<gsdk::IServerNetworkable *>();
		}

		return nullptr;
	}

	void entities_singleton::script_factory_impl::Destroy(gsdk::IServerNetworkable *net)
	{
		if(net) {
			net->Release();
		}
	}

	size_t entities_singleton::script_factory_impl::GetEntitySize()
	{ return size; }

	entities_singleton::script_factory::~script_factory() noexcept
	{
		if(free_factory) {
			script_factory_impl *impl{dynamic_cast<script_factory_impl *>(factory)};
			if(impl != nullptr) {
				delete impl;
			}
		}

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vmod.vm()->RemoveInstance(instance);
		}
	}

	bool entities_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		if(!proxy_cif.initialize(FFI_SYSV)) {
			error("vmod: failed to initialize send proxy cif\n"sv);
			return false;
		}

		entities_singleton_desc.func(&entities_singleton::script_hook_send_proxy, "script_hook_send_proxy"sv, "hook_sendprop_proxy"sv);
		entities_singleton_desc.func(&entities_singleton::script_lookup_sendprop, "script_lookup_sendprop"sv, "lookup_sendprop"sv);
		entities_singleton_desc.func(&entities_singleton::script_from_ptr, "script_from_ptr"sv, "from_ptr"sv);
		entities_singleton_desc.func(&entities_singleton::script_find_factory, "script_find_factory"sv, "find_factory"sv);
		entities_singleton_desc.func(&entities_singleton::script_create_factory, "script_create_factory"sv, "create_factory"sv);

		if(!vm->RegisterClass(&entities_singleton_desc)) {
			error("vmod: failed to register vmod entities singleton script class\n"sv);
			return false;
		}

		script_factory_impl_desc.func(&script_factory_impl::script_create_sized, "script_create_sized"sv, "create_sized"sv);
		script_factory_impl_desc.func(&script_factory_impl::script_create, "script_create"sv, "create"sv);
		script_factory_impl_desc.func(&script_factory_impl::script_size, "script_size"sv, "size"sv);
		script_factory_impl_desc.dtor();
		script_factory_impl_desc.base(plugin::owned_instance_desc);
		script_factory_impl_desc.doc_class_name("entity_factory_impl"sv);

		if(!vm->RegisterClass(&script_factory_impl_desc)) {
			error("vmod: failed to register entity factory impl script class\n"sv);
			return false;
		}

		script_factory_desc.func(&script_factory::script_create, "script_create"sv, "create"sv);
		script_factory_desc.func(&script_factory::script_size, "script_size"sv, "size"sv);
		script_factory_desc.dtor();
		script_factory_desc.base(plugin::owned_instance_desc);
		script_factory_desc.doc_class_name("entity_factory_ref"sv);

		if(!vm->RegisterClass(&script_factory_desc)) {
			error("vmod: failed to register entity factory script class\n"sv);
			return false;
		}

		vs_instance_ = vm->RegisterInstance(&entities_singleton_desc, this);
		if(!vs_instance_ || vs_instance_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create vmod entities singleton instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(vs_instance_, "__vmod_entities_singleton");

		scope = vm->CreateScope("__vmod_entities_scope", nullptr);
		if(!scope || scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create entities scope\n"sv);
			return false;
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(!vm->SetValue(vmod_scope, "ent", scope)) {
			error("vmod: failed to set entities scope value\n"sv);
			return false;
		}

		get_impl = vm->MakeSquirrelMetamethod_Get(vmod_scope, "ent", this, false);
		if(!get_impl) {
			error("vmod: failed to create entities _get metamethod\n"sv);
			return false;
		}

		return true;
	}

	void entities_singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(vs_instance_ && vs_instance_ != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(vs_instance_);
		}

		if(get_impl) {
			vm->DestroySquirrelMetamethod_Get(get_impl);
		}

		if(scope && scope != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScope(scope);
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(vm->ValueExists(vmod_scope, "ent")) {
			vm->ClearValue(vmod_scope, "ent");
		}
	}

	bool vmod::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;
		using namespace std::literals::string_literals;

		vmod_desc.func(&vmod::script_find_plugin, "script_find_plugin"sv, "find_plugin"sv);
		vmod_desc.func(&vmod::script_is_map_active, "script_is_map_active"sv, "is_map_active"sv);
		vmod_desc.func(&vmod::script_is_map_loaded, "script_is_map_loaded"sv, "is_map_loaded"sv);
		vmod_desc.func(&vmod::script_are_stringtables_created, "script_are_stringtables_created"sv, "are_stringtables_created"sv);

		vmod_desc.func(&vmod::script_success, "script_success"sv, "success"sv);
		vmod_desc.func(&vmod::script_print, "script_print"sv, "print"sv);
		vmod_desc.func(&vmod::script_info, "script_info"sv, "info"sv);
		vmod_desc.func(&vmod::script_remark, "script_remark"sv, "remark"sv);
		vmod_desc.func(&vmod::script_error, "script_error"sv, "error"sv);
		vmod_desc.func(&vmod::script_warning, "script_warning"sv, "warning"sv);

		vmod_desc.func(&vmod::script_successl, "script_successl"sv, "successl"sv);
		vmod_desc.func(&vmod::script_printl, "script_printl"sv, "printl"sv);
		vmod_desc.func(&vmod::script_infol, "script_infol"sv, "infol"sv);
		vmod_desc.func(&vmod::script_remarkl, "script_remarkl"sv, "remarkl"sv);
		vmod_desc.func(&vmod::script_errorl, "script_errorl"sv, "errorl"sv);
		vmod_desc.func(&vmod::script_warningl, "script_warningl"sv, "warningl"sv);

		if(!vm_->RegisterClass(&vmod_desc)) {
			error("vmod: failed to register vmod script class\n"sv);
			return false;
		}

		vs_instance_ = vm_->RegisterInstance(&vmod_desc, this);
		if(!vs_instance_ || vs_instance_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create vmod instance\n"sv);
			return false;
		}

		vm_->SetInstanceUniqeId(vs_instance_, "__vmod_singleton");

		plugins_table_ = vm_->CreateTable();
		if(!plugins_table_ || plugins_table_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create plugins table\n"sv);
			return false;
		}

		if(!vm_->SetValue(scope_, "plugins", plugins_table_)) {
			error("vmod: failed to set plugins table value\n"sv);
			return false;
		}

		symbols_table_ = vm_->CreateTable();
		if(!symbols_table_ || symbols_table_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create symbols table\n"sv);
			return false;
		}

		if(!vm_->SetValue(scope_, "syms", symbols_table_)) {
			error("vmod: failed to set symbols table value\n"sv);
			return false;
		}

		stringtable_table = vm_->CreateTable();
		if(!stringtable_table || stringtable_table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create stringtable table\n"sv);
			return false;
		}

		if(!vm_->SetValue(scope_, "strtables", stringtable_table)) {
			error("vmod: failed to set stringtable table value\n"sv);
			return false;
		}

		stringtable_desc.func(&script_stringtable::script_find_index, "script_find_index"sv, "find"sv);
		stringtable_desc.func(&script_stringtable::script_num_strings, "script_num_strings"sv, "num"sv);
		stringtable_desc.func(&script_stringtable::script_get_string, "script_get_string"sv, "get"sv);
		stringtable_desc.func(&script_stringtable::script_add_string, "script_add_string"sv, "add"sv);
		stringtable_desc.doc_class_name("string_table"sv);

		if(!vm_->RegisterClass(&stringtable_desc)) {
			error("vmod: failed to register stringtable script class\n"sv);
			return false;
		}

		{
			if(!create_script_stringtable(gsdk::DOWNLOADABLE_FILE_TABLENAME)) {
				return false;
			}

			if(!create_script_stringtable(gsdk::MODEL_PRECACHE_TABLENAME)) {
				return false;
			}

			if(!create_script_stringtable(gsdk::GENERIC_PRECACHE_TABLENAME)) {
				return false;
			}

			if(!create_script_stringtable(gsdk::SOUND_PRECACHE_TABLENAME)) {
				return false;
			}

			if(!create_script_stringtable(gsdk::DECAL_PRECACHE_TABLENAME)) {
				return false;
			}

			if(!create_script_stringtable("ParticleEffectNames"s)) {
				return false;
			}

			if(!create_script_stringtable("EffectDispatch"s)) {
				return false;
			}

			if(!create_script_stringtable("VguiScreen"s)) {
				return false;
			}

			if(!create_script_stringtable("Materials"s)) {
				return false;
			}

			if(!create_script_stringtable("InfoPanel"s)) {
				return false;
			}

			if(!create_script_stringtable("Scenes"s)) {
				return false;
			}
		}

		if(!plugin::bindings()) {
			return false;
		}

		if(!server_symbols_singleton.bindings()) {
			return false;
		}

		if(!filesystem_singleton.bindings()) {
			return false;
		}

		if(!cvar_singleton.bindings()) {
			return false;
		}

		if(!entities_singleton.bindings()) {
			return false;
		}

		get_impl = vm_->MakeSquirrelMetamethod_Get(nullptr, "vmod", this, false);
		if(!get_impl) {
			error("vmod: failed to create vmod _get metamethod\n"sv);
			return false;
		}

		if(!vm_->SetValue(scope_, "root_dir", root_dir_.c_str())) {
			error("vmod: failed to set root dir value\n"sv);
			return false;
		}

		if(!yaml::bindings()) {
			return false;
		}

		if(!ffi_bindings()) {
			return false;
		}

		return true;
	}

	bool vmod::binding_mods() noexcept
	{
		return true;
	}

	void vmod::unbindings() noexcept
	{
		plugin::unbindings();

		yaml::unbindings();

		server_symbols_singleton.unbindings();

		filesystem_singleton.unbindings();

		cvar_singleton.unbindings();

		entities_singleton.unbindings();

		ffi_unbindings();

		if(plugins_table_ && plugins_table_ != gsdk::INVALID_HSCRIPT) {
			vm_->ReleaseTable(plugins_table_);
		}

		if(vm_->ValueExists(scope_, "plugins")) {
			vm_->ClearValue(scope_, "plugins");
		}

		if(symbols_table_ && symbols_table_ != gsdk::INVALID_HSCRIPT) {
			vm_->ReleaseTable(symbols_table_);
		}

		if(vm_->ValueExists(scope_, "syms")) {
			vm_->ClearValue(scope_, "syms");
		}

		script_stringtables.clear();

		if(stringtable_table && stringtable_table != gsdk::INVALID_HSCRIPT) {
			vm_->ReleaseTable(stringtable_table);
		}

		if(vm_->ValueExists(scope_, "strtables")) {
			vm_->ClearValue(scope_, "strtables");
		}

		if(vm_->ValueExists(scope_, "cvar")) {
			vm_->ClearValue(scope_, "cvar");
		}

		if(vs_instance_ && vs_instance_ != gsdk::INVALID_HSCRIPT) {
			vm_->RemoveInstance(vs_instance_);
		}

		if(get_impl) {
			vm_->DestroySquirrelMetamethod_Get(get_impl);
		}

		if(vm_->ValueExists(scope_, "root_dir")) {
			vm_->ClearValue(scope_, "root_dir");
		}
	}

	static const unsigned char *g_Script_init;
	static const unsigned char *g_Script_vscript_server;
	static gsdk::IScriptVM **g_pScriptVM_ptr;
	static bool(*VScriptServerInit)();
	static void(*VScriptServerTerm)();
	static bool(*VScriptRunScript)(const char *, gsdk::HSCRIPT, bool);
	static void(gsdk::CTFGameRules::*RegisterScriptFunctions)();
	static void(*PrintFunc)(HSQUIRRELVM, const SQChar *, ...);
	static void(*ErrorFunc)(HSQUIRRELVM, const SQChar *, ...);
	static void(gsdk::IScriptVM::*RegisterFunctionGuts)(gsdk::ScriptFunctionBinding_t *, gsdk::ScriptClassDesc_t *);
	static SQRESULT(*sq_setparamscheck)(HSQUIRRELVM, SQInteger, const SQChar *);
	static gsdk::ScriptClassDesc_t **sv_classdesc_pHead;
	static gsdk::CUtlVector<gsdk::SendTable *> *g_SendTables;

	static bool in_vscript_server_init;
	static bool in_vscript_print;
	static bool in_vscript_error;

	static void vscript_output(const char *txt)
	{
		using namespace std::literals::string_view_literals;

		info("%s"sv, txt);
	}

	static gsdk::ScriptErrorFunc_t server_vs_error_cb;
	static bool vscript_error_output(gsdk::ScriptErrorLevel_t lvl, const char *txt)
	{
		using namespace std::literals::string_view_literals;

		in_vscript_error = true;
		bool ret{server_vs_error_cb(lvl, txt)};
		in_vscript_error = false;

		return ret;
	}

#if GSDK_ENGINE == GSDK_ENGINE_TF2
	static gsdk::SpewOutputFunc_t old_spew;
	static gsdk::SpewRetval_t new_spew(gsdk::SpewType_t type, const char *str)
	{
		if(in_vscript_error || in_vscript_print || in_vscript_server_init) {
			switch(type) {
				case gsdk::SPEW_LOG: {
					return gsdk::SPEW_CONTINUE;
				}
				case gsdk::SPEW_WARNING: {
					if(in_vscript_print) {
						return gsdk::SPEW_CONTINUE;
					}
				} break;
				default: break;
			}
		}

		const gsdk::Color *clr{GetSpewOutputColor()};

		if(!clr || (clr->r == 255 && clr->g == 255 && clr->b == 255)) {
			switch(type) {
				case gsdk::SPEW_MESSAGE: {
					if(in_vscript_error) {
						clr = &error_clr;
					} else {
						clr = &print_clr;
					}
				} break;
				case gsdk::SPEW_WARNING: {
					clr = &warning_clr;
				} break;
				case gsdk::SPEW_ASSERT: {
					clr = &error_clr;
				} break;
				case gsdk::SPEW_ERROR: {
					clr = &error_clr;
				} break;
				case gsdk::SPEW_LOG: {
					clr = &info_clr;
				} break;
				default: break;
			}
		}

		if(clr) {
			std::printf("\033[38;2;%hhu;%hhu;%hhum", clr->r, clr->g, clr->b);
			std::fflush(stdout);
		}

		gsdk::SpewRetval_t ret{old_spew(type, str)};

		if(clr) {
			std::fputs("\033[0m", stdout);
			std::fflush(stdout);
		}

		return ret;
	}
#endif

	static bool vscript_server_init_called;

	static bool in_vscript_server_term;
	static gsdk::IScriptVM *(gsdk::IScriptManager::*CreateVM_original)(gsdk::ScriptLanguage_t);
	static gsdk::IScriptVM *CreateVM_detour_callback(gsdk::IScriptManager *pthis, gsdk::ScriptLanguage_t lang) noexcept
	{
		if(in_vscript_server_init) {
			gsdk::IScriptVM *vmod_vm{vmod.vm()};
			if(lang == vmod_vm->GetLanguage()) {
				return vmod_vm;
			} else {
				return nullptr;
			}
		}

		return (pthis->*CreateVM_original)(lang);
	}

	static void(gsdk::IScriptManager::*DestroyVM_original)(gsdk::IScriptVM *);
	static void DestroyVM_detour_callback(gsdk::IScriptManager *pthis, gsdk::IScriptVM *vm) noexcept
	{
		if(in_vscript_server_term) {
			if(vm == vmod.vm()) {
				return;
			}
		}

		(pthis->*DestroyVM_original)(vm);
	}

	static gsdk::ScriptStatus_t(gsdk::IScriptVM::*Run_original)(const char *, bool);
	static gsdk::ScriptStatus_t Run_detour_callback(gsdk::IScriptVM *pthis, const char *script, bool wait) noexcept
	{
		if(in_vscript_server_init) {
			if(script == reinterpret_cast<const char *>(g_Script_vscript_server)) {
				return gsdk::SCRIPT_DONE;
			}
		}

		return (pthis->*Run_original)(script, wait);
	}

	static detour<decltype(VScriptRunScript)> VScriptRunScript_detour;
	static bool VScriptRunScript_detour_callback(const char *script, gsdk::HSCRIPT scope, bool warn) noexcept
	{
		if(!vscript_server_init_called) {
			if(std::strcmp(script, "mapspawn") == 0) {
				return true;
			}
		}

		return VScriptRunScript_detour(script, scope, warn);
	}

	static detour<decltype(VScriptServerInit)> VScriptServerInit_detour;
	static bool VScriptServerInit_detour_callback() noexcept
	{
		in_vscript_server_init = true;
		bool ret{vscript_server_init_called ? true : VScriptServerInit_detour()};
		gsdk::IScriptVM *vm{vmod.vm()};
		*g_pScriptVM_ptr = vm;
		gsdk::g_pScriptVM = vm;
		if(vscript_server_init_called) {
			VScriptRunScript_detour("mapspawn", nullptr, false);
		}
		in_vscript_server_init = false;
		return ret;
	}

	static detour<decltype(VScriptServerTerm)> VScriptServerTerm_detour;
	static void VScriptServerTerm_detour_callback() noexcept
	{
		in_vscript_server_term = true;
		//VScriptServerTerm_detour();
		gsdk::IScriptVM *vm{vmod.vm()};
		*g_pScriptVM_ptr = vm;
		gsdk::g_pScriptVM = vm;
		in_vscript_server_term = false;
	}

	static char __vscript_printfunc_buffer[2048];
	static detour<decltype(PrintFunc)> PrintFunc_detour;
	static void PrintFunc_detour_callback(HSQUIRRELVM m_hVM, const SQChar *s, ...)
	{
		va_list varg_list;
		va_start(varg_list, s);
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wformat-nonliteral"
	#endif
		std::vsnprintf(__vscript_printfunc_buffer, sizeof(__vscript_printfunc_buffer), s, varg_list);
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		in_vscript_print = true;
		PrintFunc_detour(m_hVM, "%s", __vscript_printfunc_buffer);
		in_vscript_print = false;
		va_end(varg_list);
	}

	static char __vscript_errorfunc_buffer[2048];
	static detour<decltype(ErrorFunc)> ErrorFunc_detour;
	static void ErrorFunc_detour_callback(HSQUIRRELVM m_hVM, const SQChar *s, ...)
	{
		va_list varg_list;
		va_start(varg_list, s);
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wformat-nonliteral"
	#endif
		std::vsnprintf(__vscript_errorfunc_buffer, sizeof(__vscript_errorfunc_buffer), s, varg_list);
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		in_vscript_error = true;
		ErrorFunc_detour(m_hVM, "%s", __vscript_errorfunc_buffer);
		in_vscript_error = false;
		va_end(varg_list);
	}

	static gsdk::ScriptFunctionBinding_t *current_binding;

	static detour<decltype(RegisterFunctionGuts)> RegisterFunctionGuts_detour;
	static void RegisterFunctionGuts_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptFunctionBinding_t *binding, gsdk::ScriptClassDesc_t *classdesc)
	{
		current_binding = binding;

		RegisterFunctionGuts_detour(vm, binding, classdesc);

		if(binding->m_flags & func_desc_t::SF_VA_FUNC) {
			constexpr std::size_t arglimit{14};
			constexpr std::size_t va_args{arglimit};

			std::size_t current_size{binding->m_desc.m_Parameters.size()};
			if(current_size < arglimit) {
				std::size_t new_size{current_size + va_args};
				if(new_size > arglimit) {
					new_size = arglimit;
				}
				for(std::size_t i{current_size}; i < new_size; ++i) {
					binding->m_desc.m_Parameters.emplace_back(gsdk::FIELD_VARIANT);
				}
			}
		}

		current_binding = nullptr;
	}

	static detour<decltype(sq_setparamscheck)> sq_setparamscheck_detour;
	static SQRESULT sq_setparamscheck_detour_callback(HSQUIRRELVM v, SQInteger nparamscheck, const SQChar *typemask)
	{
		using namespace std::literals::string_view_literals;

		std::string temp_typemask{typemask};

		if(current_binding) {
			if(current_binding->m_flags & func_desc_t::SF_VA_FUNC) {
				nparamscheck = -nparamscheck;
			} else if(current_binding->m_flags & func_desc_t::SF_OPT_FUNC) {
				nparamscheck = -(nparamscheck-1);
				temp_typemask += "|o"sv;
			}
		}

		return sq_setparamscheck_detour(v, nparamscheck, temp_typemask.c_str());
	}

	static void (gsdk::IServerGameDLL::*CreateNetworkStringTables_original)();
	void vmod::CreateNetworkStringTables_detour_callback(gsdk::IServerGameDLL *dll)
	{
		(dll->*CreateNetworkStringTables_original)();

		m_pDownloadableFileTable = sv_stringtables->FindTable(gsdk::DOWNLOADABLE_FILE_TABLENAME);
		m_pModelPrecacheTable = sv_stringtables->FindTable(gsdk::MODEL_PRECACHE_TABLENAME);
		m_pGenericPrecacheTable = sv_stringtables->FindTable(gsdk::GENERIC_PRECACHE_TABLENAME);
		m_pSoundPrecacheTable = sv_stringtables->FindTable(gsdk::SOUND_PRECACHE_TABLENAME);
		m_pDecalPrecacheTable = sv_stringtables->FindTable(gsdk::DECAL_PRECACHE_TABLENAME);

		g_pStringTableParticleEffectNames = sv_stringtables->FindTable("ParticleEffectNames");
		g_pStringTableEffectDispatch = sv_stringtables->FindTable("EffectDispatch");
		g_pStringTableVguiScreen = sv_stringtables->FindTable("VguiScreen");
		g_pStringTableMaterials = sv_stringtables->FindTable("Materials");
		g_pStringTableInfoPanel = sv_stringtables->FindTable("InfoPanel");
		g_pStringTableClientSideChoreoScenes = sv_stringtables->FindTable("Scenes");

		::vmod::vmod.recreate_script_stringtables();
	}

	static void (gsdk::IServerNetworkStringTableContainer::*RemoveAllTables_original)();
	void vmod::RemoveAllTables_detour_callback(gsdk::IServerNetworkStringTableContainer *cont)
	{
		::vmod::vmod.stringtables_removed();

		m_pDownloadableFileTable = nullptr;
		m_pModelPrecacheTable = nullptr;
		m_pGenericPrecacheTable = nullptr;
		m_pSoundPrecacheTable = nullptr;
		m_pDecalPrecacheTable = nullptr;

		g_pStringTableParticleEffectNames = nullptr;
		g_pStringTableEffectDispatch = nullptr;
		g_pStringTableVguiScreen = nullptr;
		g_pStringTableMaterials = nullptr;
		g_pStringTableInfoPanel = nullptr;
		g_pStringTableClientSideChoreoScenes = nullptr;

		(cont->*RemoveAllTables_original)();
	}

	static void (gsdk::IScriptVM::*SetErrorCallback_original)(gsdk::ScriptErrorFunc_t);
	static void SetErrorCallback_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptErrorFunc_t func)
	{
		if(in_vscript_server_init) {
			server_vs_error_cb = func;
			return;
		}

		(vm->*SetErrorCallback_original)(func);
	}

	static std::vector<const gsdk::ScriptFunctionBinding_t *> game_vscript_func_bindings;
	static std::vector<const gsdk::ScriptClassDesc_t *> game_vscript_class_bindings;

	static void (gsdk::IScriptVM::*RegisterFunction_original)(gsdk::ScriptFunctionBinding_t *);
	static void RegisterFunction_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptFunctionBinding_t *func)
	{
		(vm->*RegisterFunction_original)(func);
		if(!vscript_server_init_called) {
			std::vector<const gsdk::ScriptFunctionBinding_t *> &vec{game_vscript_func_bindings};
			vec.emplace_back(func);
		}
	}

	static bool (gsdk::IScriptVM::*RegisterClass_original)(gsdk::ScriptClassDesc_t *);
	static bool RegisterClass_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptClassDesc_t *desc)
	{
		bool ret{(vm->*RegisterClass_original)(desc)};
		if(!vscript_server_init_called) {
			std::vector<const gsdk::ScriptClassDesc_t *> &vec{game_vscript_class_bindings};
			auto it{std::find(vec.begin(), vec.end(), desc)};
			if(it == vec.end()) {
				vec.emplace_back(desc);
			}
		}
		return ret;
	}

	static gsdk::HSCRIPT (gsdk::IScriptVM::*RegisterInstance_original)(gsdk::ScriptClassDesc_t *, void *);
	static gsdk::HSCRIPT RegisterInstance_detour_callback(gsdk::IScriptVM *vm, gsdk::ScriptClassDesc_t *desc, void *ptr)
	{
		gsdk::HSCRIPT ret{(vm->*RegisterInstance_original)(desc, ptr)};
		if(!vscript_server_init_called) {
			std::vector<const gsdk::ScriptClassDesc_t *> &vec{game_vscript_class_bindings};
			auto it{std::find(vec.begin(), vec.end(), desc)};
			if(it == vec.end()) {
				vec.emplace_back(desc);
			}
		}
		return ret;
	}

	bool vmod::detours() noexcept
	{
		RegisterFunctionGuts_detour.initialize(RegisterFunctionGuts, RegisterFunctionGuts_detour_callback);
		RegisterFunctionGuts_detour.enable();

		sq_setparamscheck_detour.initialize(sq_setparamscheck, sq_setparamscheck_detour_callback);
		sq_setparamscheck_detour.enable();

		PrintFunc_detour.initialize(PrintFunc, PrintFunc_detour_callback);
		PrintFunc_detour.enable();

		ErrorFunc_detour.initialize(ErrorFunc, ErrorFunc_detour_callback);
		ErrorFunc_detour.enable();

		VScriptServerInit_detour.initialize(VScriptServerInit, VScriptServerInit_detour_callback);
		VScriptServerInit_detour.enable();

		VScriptServerTerm_detour.initialize(VScriptServerTerm, VScriptServerTerm_detour_callback);
		VScriptServerTerm_detour.enable();

		VScriptRunScript_detour.initialize(VScriptRunScript, VScriptRunScript_detour_callback);
		VScriptRunScript_detour.enable();

		CreateVM_original = swap_vfunc(vsmgr, &gsdk::IScriptManager::CreateVM, CreateVM_detour_callback);
		DestroyVM_original = swap_vfunc(vsmgr, &gsdk::IScriptManager::DestroyVM, DestroyVM_detour_callback);

		Run_original = swap_vfunc(vm_, static_cast<decltype(Run_original)>(&gsdk::IScriptVM::Run), Run_detour_callback);
		SetErrorCallback_original = swap_vfunc(vm_, &gsdk::IScriptVM::SetErrorCallback, SetErrorCallback_detour_callback);

		RegisterFunction_original = swap_vfunc(vm_, &gsdk::IScriptVM::RegisterFunction, RegisterFunction_detour_callback);
		RegisterClass_original = swap_vfunc(vm_, &gsdk::IScriptVM::RegisterClass, RegisterClass_detour_callback);
		RegisterInstance_original = swap_vfunc(vm_, &gsdk::IScriptVM::RegisterInstance_impl, RegisterInstance_detour_callback);

		CreateNetworkStringTables_original = swap_vfunc(gamedll, &gsdk::IServerGameDLL::CreateNetworkStringTables, CreateNetworkStringTables_detour_callback);

		RemoveAllTables_original = swap_vfunc(sv_stringtables, &gsdk::IServerNetworkStringTableContainer::RemoveAllTables, RemoveAllTables_detour_callback);

		return true;
	}

	bool vmod::load() noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		gsdk::ScriptLanguage_t script_language{gsdk::SL_SQUIRREL};

		switch(script_language) {
			case gsdk::SL_NONE: break;
			case gsdk::SL_GAMEMONKEY: scripts_extension = ".gm"sv; break;
			case gsdk::SL_SQUIRREL: scripts_extension = ".nut"sv; break;
			case gsdk::SL_LUA: scripts_extension = ".lua"sv; break;
			case gsdk::SL_PYTHON: scripts_extension = ".py"sv; break;
		}

		if(!symbol_cache::initialize()) {
			std::cout << "\033[0;31m"sv << "vmod: failed to initialize symbol cache\n"sv << "\033[0m"sv;
			return false;
		}

		std::filesystem::path exe_filename;
		std::filesystem::path bin_folder;

		{
			char exe[PATH_MAX];
			ssize_t len{readlink("/proc/self/exe", exe, sizeof(exe))};
			exe[len] = '\0';

			exe_filename = exe;

			bin_folder = exe_filename.parent_path();
			bin_folder /= "bin"sv;

			exe_filename = exe_filename.filename();
			exe_filename.replace_extension();

			if(exe_filename == "portal2_linux"sv) {
				bin_folder /= "linux32"sv;
			}
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || \
		GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(exe_filename != "hl2_linux"sv && exe_filename != "srcds_linux"sv) {
			std::cout << "\033[0;31m"sv << "vmod: unsupported exe filename: '"sv << exe_filename << "'\n"sv << "\033[0m"sv;
			return false;
		}
	#elif GSDK_ENGINE == GSDK_ENGINE_PORTAL2
		if(exe_filename != "portal2_linux"sv) {
			std::cout << "\033[0;31m"sv << "vmod: unsupported exe filename: '"sv << exe_filename << "'\n"sv << "\033[0m"sv;
			return false;
		}
	#else
		#error
	#endif

		std::string_view launcher_lib_name;
		if(exe_filename == "hl2_linux"sv ||
			exe_filename == "portal2_linux"sv) {
			launcher_lib_name = "launcher.so"sv;
		} else if(exe_filename == "srcds_linux"sv) {
			launcher_lib_name = "dedicated_srv.so"sv;
		} else {
			std::cout << "\033[0;31m"sv << "vmod: unsupported exe filename: '"sv << exe_filename << "'\n"sv << "\033[0m"sv;
			return false;
		}

		if(!launcher_lib.load(bin_folder/launcher_lib_name)) {
			std::cout << "\033[0;31m"sv << "vmod: failed to open launcher library: '"sv << launcher_lib.error_string() << "'\n"sv << "\033[0m"sv;
			return false;
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		old_spew = GetSpewOutputFunc();
		SpewOutputFunc(new_spew);
	#elif GSDK_ENGINE == GSDK_ENGINE_PORTAL2 || \
			GSDK_ENGINE == GSDK_ENGINE_L4D2
		//TODO!!!
		//LoggingSystem
	#endif

		std::string_view engine_lib_name{"engine.so"sv};
		if(dedicated) {
			engine_lib_name = "engine_srv.so"sv;
		}
		if(!engine_lib.load(bin_folder/engine_lib_name)) {
			error("vmod: failed to open engine library: '%s'\n", engine_lib.error_string().c_str());
			return false;
		}

		std::string_view vstdlib_lib_name{"libvstdlib.so"sv};
		if(sv_engine->IsDedicatedServer()) {
			vstdlib_lib_name = "libvstdlib_srv.so"sv;
		}
		if(!vstdlib_lib.load(bin_folder/vstdlib_lib_name)) {
			error("vmod: failed to open vstdlib library: %s\n"sv, vstdlib_lib.error_string().c_str());
			return false;
		}

		cvar_dll_id_ = cvar->AllocateDLLIdentifier();

		{
			char gamedir[PATH_MAX];
			sv_engine->GetGameDir(gamedir, sizeof(gamedir));

			game_dir_ = gamedir;
		}

		root_dir_ = game_dir_;
		root_dir_ /= "addons/vmod"sv;

		plugins_dir = root_dir_;
		plugins_dir /= "plugins"sv;

		if(!pp.initialize()) {
			return false;
		}

		base_script_path = root_dir_;
		base_script_path /= "base/vmod_base"sv;
		base_script_path.replace_extension(scripts_extension);

		std::filesystem::path server_lib_name{game_dir_};
		server_lib_name /= "bin"sv;
		if(sv_engine->IsDedicatedServer()) {
			server_lib_name /= "server_srv.so";
		} else {
			server_lib_name /= "server.so";
		}
		if(!server_lib.load(server_lib_name)) {
			error("vmod: failed to open server library: '%s'\n"sv, server_lib.error_string().c_str());
			return false;
		}

		const auto &eng_symbols{engine_lib.symbols()};
		const auto &eng_global_qual{eng_symbols.global()};

		const auto &sv_symbols{server_lib.symbols()};
		const auto &sv_global_qual{sv_symbols.global()};

		auto g_Script_vscript_server_it{sv_global_qual.find("g_Script_vscript_server"s)};
		if(g_Script_vscript_server_it == sv_global_qual.end()) {
			error("vmod: missing 'g_Script_vscript_server' symbol\n"sv);
			return false;
		}

		auto g_pScriptVM_it{sv_global_qual.find("g_pScriptVM"s)};
		if(g_pScriptVM_it == sv_global_qual.end()) {
			error("vmod: missing 'g_pScriptVM' symbol\n"sv);
			return false;
		}

		auto VScriptServerInit_it{sv_global_qual.find("VScriptServerInit()"s)};
		if(VScriptServerInit_it == sv_global_qual.end()) {
			error("vmod: missing 'VScriptServerInit' symbol\n"sv);
			return false;
		}

		auto VScriptServerTerm_it{sv_global_qual.find("VScriptServerTerm()"s)};
		if(VScriptServerTerm_it == sv_global_qual.end()) {
			error("vmod: missing 'VScriptServerTerm' symbol\n"sv);
			return false;
		}

		auto VScriptRunScript_it{sv_global_qual.find("VScriptRunScript(char const*, HSCRIPT__*, bool)"s)};
		if(VScriptRunScript_it == sv_global_qual.end()) {
			error("vmod: missing 'VScriptRunScript' symbol\n"sv);
			return false;
		}

		auto CTFGameRules_it{sv_symbols.find("CTFGameRules"s)};
		if(CTFGameRules_it == sv_symbols.end()) {
			error("vmod: missing 'CTFGameRules' symbols\n"sv);
			return false;
		}

		auto RegisterScriptFunctions_it{CTFGameRules_it->second->find("RegisterScriptFunctions()"s)};
		if(RegisterScriptFunctions_it == CTFGameRules_it->second->end()) {
			error("vmod: missing 'CTFGameRules::RegisterScriptFunctions()' symbol\n"sv);
			return false;
		}

		auto CBaseEntity_it{sv_symbols.find("CBaseEntity"s)};
		if(CBaseEntity_it == sv_symbols.end()) {
			error("vmod: missing 'CBaseEntity' symbols\n"sv);
			return false;
		}

		auto GetScriptInstance_it{CBaseEntity_it->second->find("GetScriptInstance()"s)};
		if(GetScriptInstance_it == CBaseEntity_it->second->end()) {
			error("vmod: missing 'CBaseEntity::GetScriptInstance()' symbol\n"sv);
			return false;
		}

		auto sv_ScriptClassDesc_t_it{sv_symbols.find("ScriptClassDesc_t"s)};
		if(sv_ScriptClassDesc_t_it == sv_symbols.end()) {
			error("vmod: missing 'ScriptClassDesc_t' symbol\n"sv);
			return false;
		}

		auto sv_GetDescList_it{sv_ScriptClassDesc_t_it->second->find("GetDescList()"s)};
		if(sv_GetDescList_it == sv_ScriptClassDesc_t_it->second->end()) {
			error("vmod: missing 'ScriptClassDesc_t::GetDescList()' symbol\n"sv);
			return false;
		}

		auto sv_pHead_it{sv_GetDescList_it->second->find("pHead"s)};
		if(sv_pHead_it == sv_GetDescList_it->second->end()) {
			error("vmod: missing 'ScriptClassDesc_t::GetDescList()::pHead' symbol\n"sv);
			return false;
		}

		std::string_view vscript_lib_name{"vscript.so"sv};
		if(sv_engine->IsDedicatedServer()) {
			vscript_lib_name = "vscript_srv.so"sv;
		}
		if(!vscript_lib.load(bin_folder/vscript_lib_name)) {
			error("vmod: failed to open vscript library: '%s'\n"sv, vscript_lib.error_string().c_str());
			return false;
		}

		const auto &vscript_symbols{vscript_lib.symbols()};
		const auto &vscript_global_qual{vscript_symbols.global()};

		auto sq_getversion_it{vscript_global_qual.find("sq_getversion"s)};
		if(sq_getversion_it == vscript_global_qual.end()) {
			error("vmod: missing 'sq_getversion' symbol\n"sv);
			return false;
		}

		vm_ = vsmgr->CreateVM(script_language);
		if(!vm_) {
			error("vmod: failed to create VM\n"sv);
			return false;
		}

		{
			SQInteger game_sq_ver{sq_getversion_it->second->func<decltype(::sq_getversion)>()()};
			SQInteger curr_sq_ver{::sq_getversion()};

			if(curr_sq_ver != SQUIRREL_VERSION_NUMBER) {
				error("vmod: mismatched squirrel header '%i' vs '%i'\n"sv, curr_sq_ver, SQUIRREL_VERSION_NUMBER);
				return false;
			}

			if(game_sq_ver != curr_sq_ver) {
				error("vmod: mismatched squirrel versions '%i' vs '%i'\n"sv, game_sq_ver, curr_sq_ver);
				return false;
			}

			script_variant_t game_sq_versionnumber;
			if(!vm_->GetValue(nullptr, "_versionnumber_", &game_sq_versionnumber)) {
				error("vmod: failed to get _versionnumber_ value\n"sv);
				return false;
			}

			script_variant_t game_sq_version;
			if(!vm_->GetValue(nullptr, "_version_", &game_sq_version)) {
				error("vmod: failed to get _version_ value\n"sv);
				return false;
			}

			//TODO!!! make this a cmd/cvar/cmdline opt
		#if 0
			info("vmod: squirrel info:\n");
			info("vmod:   vmod:\n");
			info("vmod:    SQUIRREL_VERSION: %s\n", SQUIRREL_VERSION);
			info("vmod:    SQUIRREL_VERSION_NUMBER: %i\n", SQUIRREL_VERSION_NUMBER);
			info("vmod:    sq_getversion: %i\n", curr_sq_ver);
			info("vmod:   game:\n");
			info("vmod:    _version_: %s\n", game_sq_version.get<std::string_view>().data());
			info("vmod:    _versionnumber_: %i\n", game_sq_versionnumber.get<int>());
			info("vmod:    sq_getversion: %i\n", game_sq_ver);
		#endif
		}

		auto g_Script_init_it{vscript_global_qual.find("g_Script_init"s)};

		auto sq_setparamscheck_it{vscript_global_qual.find("sq_setparamscheck"s)};
		if(sq_setparamscheck_it == vscript_global_qual.end()) {
			error("vmod: missing 'sq_setparamscheck' symbol\n"sv);
			return false;
		}

		auto CSquirrelVM_it{vscript_symbols.find("CSquirrelVM"s)};
		if(CSquirrelVM_it == vscript_symbols.end()) {
			error("vmod: missing 'CSquirrelVM' symbols\n"sv);
			return false;
		}

		auto CreateArray_it{CSquirrelVM_it->second->find("CreateArray(CVariantBase<CVariantDefaultAllocator>&)"s)};
		if(CreateArray_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::CreateArray(CVariantBase<CVariantDefaultAllocator>&)' symbol\n"sv);
			return false;
		}

		auto GetArrayCount_it{CSquirrelVM_it->second->find("GetArrayCount(HSCRIPT__*)"s)};
		if(GetArrayCount_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::GetArrayCount(HSCRIPT__*)' symbol\n"sv);
			return false;
		}

		auto IsArray_it{CSquirrelVM_it->second->find("IsArray(HSCRIPT__*)"s)};
		if(IsArray_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::IsArray(HSCRIPT__*)' symbol\n"sv);
			return false;
		}

		auto IsTable_it{CSquirrelVM_it->second->find("IsTable(HSCRIPT__*)"s)};
		if(IsTable_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::IsTable(HSCRIPT__*)' symbol\n"sv);
			return false;
		}

		auto PrintFunc_it{CSquirrelVM_it->second->find("PrintFunc(SQVM*, char const*, ...)"s)};
		if(PrintFunc_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::PrintFunc(SQVM*, char const*, ...)' symbol\n"sv);
			return false;
		}

		auto ErrorFunc_it{CSquirrelVM_it->second->find("ErrorFunc(SQVM*, char const*, ...)"s)};
		if(ErrorFunc_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::ErrorFunc(SQVM*, char const*, ...)' symbol\n"sv);
			return false;
		}

		auto RegisterFunctionGuts_it{CSquirrelVM_it->second->find("RegisterFunctionGuts(ScriptFunctionBinding_t*, ScriptClassDesc_t*)"s)};
		if(RegisterFunctionGuts_it == CSquirrelVM_it->second->end()) {
			error("vmod: missing 'CSquirrelVM::RegisterFunctionGuts(ScriptFunctionBinding_t*, ScriptClassDesc_t*)' symbol\n"sv);
			return false;
		}

		auto g_SendTables_it{eng_global_qual.find("g_SendTables"s)};
		if(g_SendTables_it == eng_global_qual.end()) {
			error("vmod: missing 'g_SendTables' symbol\n"sv);
			return false;
		}

		if(g_Script_init_it != vscript_global_qual.end()) {
			g_Script_init = g_Script_init_it->second->addr<const unsigned char *>();
		}

		RegisterScriptFunctions = RegisterScriptFunctions_it->second->mfp<decltype(RegisterScriptFunctions)>();
		VScriptServerInit = VScriptServerInit_it->second->func<decltype(VScriptServerInit)>();
		VScriptServerTerm = VScriptServerTerm_it->second->func<decltype(VScriptServerTerm)>();
		VScriptRunScript = VScriptRunScript_it->second->func<decltype(VScriptRunScript)>();
		g_Script_vscript_server = g_Script_vscript_server_it->second->addr<const unsigned char *>();
		g_pScriptVM_ptr = g_pScriptVM_it->second->addr<gsdk::IScriptVM **>();
		gsdk::g_pScriptVM = *g_pScriptVM_ptr;

		gsdk::IScriptVM::CreateArray_ptr = CreateArray_it->second->mfp<decltype(gsdk::IScriptVM::CreateArray_ptr)>();
		gsdk::IScriptVM::GetArrayCount_ptr = GetArrayCount_it->second->mfp<decltype(gsdk::IScriptVM::GetArrayCount_ptr)>();
		gsdk::IScriptVM::IsArray_ptr = IsArray_it->second->mfp<decltype(gsdk::IScriptVM::IsArray_ptr)>();
		gsdk::IScriptVM::IsTable_ptr = IsTable_it->second->mfp<decltype(gsdk::IScriptVM::IsTable_ptr)>();

		PrintFunc = PrintFunc_it->second->func<decltype(PrintFunc)>();
		ErrorFunc = ErrorFunc_it->second->func<decltype(ErrorFunc)>();
		RegisterFunctionGuts = RegisterFunctionGuts_it->second->mfp<decltype(RegisterFunctionGuts)>();
		sq_setparamscheck = sq_setparamscheck_it->second->func<decltype(sq_setparamscheck)>();

		sv_classdesc_pHead = sv_pHead_it->second->addr<gsdk::ScriptClassDesc_t **>();

		g_SendTables = g_SendTables_it->second->addr<gsdk::CUtlVector<gsdk::SendTable *> *>();

		gsdk::ScriptClassDesc_t *tmp_desc{*sv_classdesc_pHead};
		while(tmp_desc) {
			std::string name{tmp_desc->m_pszClassname};
			sv_script_class_descs.emplace(std::move(name), tmp_desc);
			tmp_desc = tmp_desc->m_pNextDesc;
		}

		for(const auto &it : sv_classes) {
			auto info_it{sv_ent_class_info.find(it.first)};
			if(info_it == sv_ent_class_info.end()) {
				info_it = sv_ent_class_info.emplace(it.first, entity_class_info{}).first;
			}

			info_it->second.sv_class = it.second;
			info_it->second.sendtable = it.second->m_pTable;

			auto script_desc_it{sv_script_class_descs.find(it.first)};
			if(script_desc_it != sv_script_class_descs.end()) {
				info_it->second.script_desc = script_desc_it->second;
			}

			auto sv_sym_it{sv_symbols.find(it.first)};
			if(sv_sym_it == sv_symbols.end()) {
				error("vmod: missing '%s' symbols\n"sv, it.first.c_str());
				return false;
			}

			auto GetDataDescMap_it{sv_sym_it->second->find("GetDataDescMap()"s)};
			if(GetDataDescMap_it != sv_sym_it->second->end()) {
				using GetDataDescMap_t = gsdk::datamap_t *(gsdk::CBaseEntity::*)();
				GetDataDescMap_t GetDataDescMap{GetDataDescMap_it->second->mfp<GetDataDescMap_t>()};
				gsdk::datamap_t *map{(reinterpret_cast<gsdk::CBaseEntity *>(uninitialized_memory)->*GetDataDescMap)()};

				info_it->second.datamap = map;
			}
		}

		gsdk::CBaseEntity::GetScriptInstance_ptr = GetScriptInstance_it->second->mfp<decltype(gsdk::CBaseEntity::GetScriptInstance_ptr)>();

		if(!assign_entity_class_info()) {
			return false;
		}

		vm_->SetOutputCallback(vscript_output);
		vm_->SetErrorCallback(vscript_error_output);

		//TODO!!! make this a cmd/cvar/cmdline opt
		{
			if(g_Script_init) {
				write_file(root_dir_/"internal_scripts"sv/"init.nut"sv, g_Script_init, std::strlen(reinterpret_cast<const char *>(g_Script_init)+1));
			}

			write_file(root_dir_/"internal_scripts"sv/"vscript_server.nut"sv, g_Script_vscript_server, std::strlen(reinterpret_cast<const char *>(g_Script_vscript_server)+1));
		}

		if(!detours()) {
			return false;
		}

		if(!binding_mods()) {
			return false;
		}

		if(!VScriptServerInit_detour_callback()) {
			error("vmod: VScriptServerInit failed\n"sv);
			return false;
		}

		(reinterpret_cast<gsdk::CTFGameRules *>(uninitialized_memory)->*RegisterScriptFunctions)();

		vscript_server_init_called = true;

		if(vm_->GetLanguage() == gsdk::SL_SQUIRREL) {
			server_init_script = vm_->CompileScript(reinterpret_cast<const char *>(g_Script_vscript_server), "vscript_server.nut");
			if(!server_init_script || server_init_script == gsdk::INVALID_HSCRIPT) {
				error("vmod: failed to compile server init script\n"sv);
				return false;
			}
		} else {
			error("vmod: server init script not supported on this language\n"sv);
			return false;
		}

		if(vm_->Run(server_init_script, nullptr, true) == gsdk::SCRIPT_ERROR) {
			error("vmod: failed to run server init script\n"sv);
			return false;
		}

		std::string base_script_name{"vmod_base"sv};
		base_script_name += scripts_extension;

		if(std::filesystem::exists(base_script_path)) {
			{
				std::unique_ptr<unsigned char[]> script_data{read_file(base_script_path)};

				base_script = vm_->CompileScript(reinterpret_cast<const char *>(script_data.get()), base_script_path.c_str());
				if(!base_script || base_script == gsdk::INVALID_HSCRIPT) {
				#ifndef __VMOD_BASE_SCRIPT_HEADER_INCLUDED
					error("vmod: failed to compile base script '%s'\n"sv, base_script_path.c_str());
					return false;
				#else
					base_script = vm_->CompileScript(reinterpret_cast<const char *>(__vmod_base_script), base_script_name.c_str());
					if(!base_script || base_script == gsdk::INVALID_HSCRIPT) {
						error("vmod: failed to compile base script\n"sv);
						return false;
					}
				#endif
				} else {
					base_script_from_file = true;
				}
			}
		} else {
		#ifndef __VMOD_BASE_SCRIPT_HEADER_INCLUDED
			error("vmod: missing base script '%s'\n"sv, base_script_path.c_str());
			return false;
		#else
			base_script = vm_->CompileScript(reinterpret_cast<const char *>(__vmod_base_script), base_script_name.c_str());
			if(!base_script || base_script == gsdk::INVALID_HSCRIPT) {
				error("vmod: failed to compile base script\n"sv);
				return false;
			}
		#endif
		}

		base_script_scope = vm_->CreateScope("__vmod_base_script_scope__", nullptr);
		if(!base_script_scope || base_script_scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create base script scope\n"sv);
			return false;
		}

		if(vm_->Run(base_script, base_script_scope, true) == gsdk::SCRIPT_ERROR) {
			if(base_script_from_file) {
				error("vmod: failed to run base script '%s'\n"sv, base_script_path.c_str());
			} else {
				error("vmod: failed to run base script\n"sv);
			}
			return false;
		}

		auto get_func_from_base_script{[this](gsdk::HSCRIPT &func, std::string_view name) noexcept -> bool {
			func = vm_->LookupFunction(name.data(), base_script_scope);
			if(!func || func == gsdk::INVALID_HSCRIPT) {
				if(base_script_from_file) {
					error("vmod: base script '%s' missing '%s' function\n"sv, base_script_path.c_str(), name.data());
				} else {
					error("vmod: base script missing '%s' function\n"sv, name.data());
				}
				return false;
			}
			return true;
		}};

		if(!get_func_from_base_script(to_string_func, "__to_string__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(to_int_func, "__to_int__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(to_float_func, "__to_float__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(to_bool_func, "__to_bool__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(typeof_func, "__typeof__"sv)) {
			return false;
		}

		if(!get_func_from_base_script(funcisg_func, "__get_func_sig__"sv)) {
			return false;
		}

		scope_ = vm_->CreateScope("vmod", nullptr);
		if(!scope_ || scope_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create vmod scope\n"sv);
			return false;
		}

		vmod_reload_plugins.initialize("vmod_reload_plugins"sv, [this](const gsdk::CCommand &) noexcept -> void {
			auto it{plugins.begin()};
			while(it != plugins.end()) {
				if(it->second->reload() == plugin::load_status::disabled) {
					it = plugins.erase(it);
					continue;
				}
				++it;
			}

			if(plugins_loaded) {
				it = plugins.begin();
				while(it != plugins.end()) {
					if(!*it->second) {
						continue;
					}

					it->second->all_plugins_loaded();
				}
			}
		});

		vmod_unload_plugins.initialize("vmod_unload_plugins"sv, [this](const gsdk::CCommand &) noexcept -> void {
			for(const auto &it : plugins) {
				it.second->unload();
			}

			plugins.clear();

			for(const std::filesystem::path &it : added_paths) {
				filesystem->RemoveSearchPath(it.c_str(), "GAME");
			}
			added_paths.clear();

			plugins_loaded = false;
		});

		vmod_unload_plugin.initialize("vmod_unload_plugin"sv, [this](const gsdk::CCommand &args) noexcept -> void {
			if(args.m_nArgc != 2) {
				error("vmod: usage: vmod_unload_plugin <path>\n");
				return;
			}

			std::filesystem::path path{args.m_ppArgv[1]};
			if(!path.is_absolute()) {
				path = (plugins_dir/path);
			}
			if(!path.has_extension()) {
				path.replace_extension(scripts_extension);
			}

			if(path.extension() != scripts_extension) {
				error("vmod: invalid extension\n");
				return;
			}

			auto it{plugins.find(path)};
			if(it != plugins.end()) {
				error("vmod: unloaded plugin '%s'\n", it->first.c_str());
				plugins.erase(it);
			} else {
				error("vmod: plugin '%s' not found\n", path.c_str());
			}
		});

		vmod_load_plugin.initialize("vmod_load_plugin"sv, [this](const gsdk::CCommand &args) noexcept -> void {
			if(args.m_nArgc != 2) {
				error("vmod: usage: vmod_load_plugin <path>\n");
				return;
			}

			std::filesystem::path path{args.m_ppArgv[1]};
			if(!path.is_absolute()) {
				path = (plugins_dir/path);
			}
			if(!path.has_extension()) {
				path.replace_extension(scripts_extension);
			}

			if(path.extension() != scripts_extension) {
				error("vmod: invalid extension\n");
				return;
			}

			auto it{plugins.find(path)};
			if(it != plugins.end()) {
				switch(it->second->reload()) {
					case plugin::load_status::success: {
						success("vmod: plugin '%s' reloaded\n", it->first.c_str());
						if(plugins_loaded) {
							it->second->all_plugins_loaded();
						}
					} break;
					case plugin::load_status::disabled: {
						plugins.erase(it);
					} break;
					default: break;
				}
				return;
			}

			std::unique_ptr<plugin> pl{new plugin{path}};
			switch(pl->load()) {
				case plugin::load_status::success: {
					success("vmod: plugin '%s' loaded\n", path.c_str());
					if(plugins_loaded) {
						pl->all_plugins_loaded();
					}
				} break;
				case plugin::load_status::disabled: {
					return;
				}
				default: break;
			}

			plugins.emplace(std::move(path), std::move(pl));
		});

		vmod_list_plugins.initialize("vmod_list_plugins"sv, [this](const gsdk::CCommand &args) noexcept -> void {
			if(args.m_nArgc != 1) {
				error("vmod: usage: vmod_list_plugins\n");
				return;
			}

			if(plugins.empty()) {
				info("vmod: no plugins loaded\n");
				return;
			}

			for(const auto &it : plugins) {
				if(*it.second) {
					success("'%s'\n", it.first.c_str());
				} else {
					error("'%s'\n", it.first.c_str());
				}
			}
		});

		vmod_refresh_plugins.initialize("vmod_refresh_plugins"sv, [this](const gsdk::CCommand &) noexcept -> void {
			vmod_unload_plugins();

			load_plugins(plugins_dir, load_plugins_flags::none);

			for(const auto &it : plugins) {
				if(!*it.second) {
					continue;
				}

				it.second->all_plugins_loaded();
			}

			plugins_loaded = true;
		});

		sv_engine->InsertServerCommand("exec vmod/load.cfg\n");
		sv_engine->ServerExecute();

		return true;
	}

	bool vmod::assign_entity_class_info() noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		{
			auto CBaseEntity_desc_it{sv_script_class_descs.find("CBaseEntity"s)};
			if(CBaseEntity_desc_it == sv_script_class_descs.end()) {
				error("vmod: failed to find baseentity script class\n"sv);
				return false;
			}

			gsdk::CBaseEntity::g_pScriptDesc = CBaseEntity_desc_it->second;
		}

		return true;
	}

	void vmod::load_plugins(const std::filesystem::path &dir, load_plugins_flags flags) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::filesystem::path dir_name{dir.filename()};

		for(const auto &file : std::filesystem::directory_iterator{dir}) {
			std::filesystem::path path{file.path()};
			std::filesystem::path name{path.filename()};

			if(name.native()[0] == '.') {
				continue;
			}

			if(file.is_directory()) {
				if(std::filesystem::exists(path/".ignored"sv)) {
					continue;
				}

				if(name == "include"sv) {
					if(dir_name == "assets"sv) {
						remark("vmod: include folder inside assets folder: '%s'\n"sv, path.c_str());
					} else if(dir_name == "docs"sv) {
						remark("vmod: include folder inside docs folder: '%s'\n"sv, path.c_str());
					}
					continue;
				} else if(name == "docs"sv) {
					if(dir_name == "assets"sv) {
						remark("vmod: docs folder inside assets folder: '%s'\n"sv, path.c_str());
					} else if(dir_name == "include"sv) {
						remark("vmod: docs folder inside include folder: '%s'\n"sv, path.c_str());
					}
					continue;
				} else if(name == "assets"sv) {
					if(!(flags & load_plugins_flags::src_folder)) {
						filesystem->AddSearchPath(path.c_str(), "GAME");
						added_paths.emplace_back(std::move(path));
					} else {
						remark("vmod: assets folder inside src folder: '%s'\n"sv, path.c_str());
					}
					continue;
				} else if(name == "src"sv) {
					if(!(flags & load_plugins_flags::src_folder)) {
						load_plugins(path, load_plugins_flags::src_folder|load_plugins_flags::no_recurse);
					} else {
						remark("vmod: src folder inside another src folder: '%s'\n"sv, path.c_str());
					}
					continue;
				}

				if(!(flags & load_plugins_flags::no_recurse)) {
					load_plugins(path, load_plugins_flags::none);
				}
				continue;
			} else if(!file.is_regular_file()) {
				continue;
			}

			if(name.extension() != scripts_extension) {
				continue;
			}

			std::unique_ptr<plugin> pl{new plugin{path}};
			if(pl->load() == plugin::load_status::disabled) {
				continue;
			}

			plugins.emplace(std::move(path), std::move(pl));
		}
	}

	static script_variant_t call_to_func(gsdk::HSCRIPT func, gsdk::HSCRIPT value) noexcept
	{
		script_variant_t ret;
		script_variant_t arg{value};

		if(vmod.vm()->ExecuteFunction(func, &arg, 1, &ret, nullptr, true) == gsdk::SCRIPT_ERROR) {
			null_variant(ret);
			return ret;
		}

		return ret;
	}

	std::string_view vmod::to_string(gsdk::HSCRIPT value) const noexcept
	{
		script_variant_t ret{call_to_func(to_string_func, value)};

		if(ret.m_type != gsdk::FIELD_CSTRING) {
			return {};
		}

		static std::string temp_buffer;

		temp_buffer = ret.m_pszString;

		return temp_buffer;
	}

	int vmod::to_int(gsdk::HSCRIPT value) const noexcept
	{
		script_variant_t ret{call_to_func(to_int_func, value)};

		if(ret.m_type != gsdk::FIELD_INTEGER) {
			return 0;
		}

		return ret.m_int;
	}

	float vmod::to_float(gsdk::HSCRIPT value) const noexcept
	{
		script_variant_t ret{call_to_func(to_float_func, value)};

		if(ret.m_type != gsdk::FIELD_FLOAT) {
			return 0.0f;
		}

		return ret.m_float;
	}

	bool vmod::to_bool(gsdk::HSCRIPT value) const noexcept
	{
		script_variant_t ret{call_to_func(to_bool_func, value)};

		if(ret.m_type != gsdk::FIELD_BOOLEAN) {
			return false;
		}

		return ret.m_bool;
	}

	std::string_view vmod::typeof_(gsdk::HSCRIPT value) const noexcept
	{
		script_variant_t ret{call_to_func(typeof_func, value)};

		if(ret.m_type != gsdk::FIELD_CSTRING) {
			return {};
		}

		return ret.m_pszString;
	}

	bool __vmod_to_bool(gsdk::HSCRIPT object) noexcept
	{ return vmod.to_bool(object); }
	float __vmod_to_float(gsdk::HSCRIPT object) noexcept
	{ return vmod.to_float(object); }
	int __vmod_to_int(gsdk::HSCRIPT object) noexcept
	{ return vmod.to_int(object); }
	std::string_view __vmod_to_string(gsdk::HSCRIPT object) noexcept
	{ return vmod.to_string(object); }
	std::string_view __vmod_typeof(gsdk::HSCRIPT object) noexcept
	{ return vmod.typeof_(object); }
	void __vmod_raiseexception(const char *str) noexcept
	{ vmod.vm()->RaiseException(str); }

	static inline void ident(std::string &file, std::size_t num) noexcept
	{ file.insert(file.end(), num, '\t'); }

	static std::string_view datatype_to_raw_str(gsdk::ScriptDataType_t type) noexcept
	{
		using namespace std::literals::string_view_literals;

		static std::string temp_buffer;

		switch(type) {
			case gsdk::FIELD_VOID:
			return "FIELD_VOID"sv;
			case gsdk::FIELD_FLOAT:
			return "FIELD_FLOAT"sv;
			case gsdk::FIELD_STRING:
			return "FIELD_STRING"sv;
			case gsdk::FIELD_VECTOR:
			return "FIELD_VECTOR"sv;
			case gsdk::FIELD_QUATERNION:
			return "FIELD_QUATERNION"sv;
			case gsdk::FIELD_INTEGER:
			return "FIELD_INTEGER"sv;
			case gsdk::FIELD_BOOLEAN:
			return "FIELD_BOOLEAN"sv;
			case gsdk::FIELD_SHORT:
			return "FIELD_SHORT"sv;
			case gsdk::FIELD_CHARACTER:
			return "FIELD_CHARACTER"sv;
			case gsdk::FIELD_COLOR32:
			return "FIELD_COLOR32"sv;
			case gsdk::FIELD_EMBEDDED:
			return "FIELD_EMBEDDED"sv;
			case gsdk::FIELD_CUSTOM:
			return "FIELD_CUSTOM"sv;
			case gsdk::FIELD_CLASSPTR:
			return "FIELD_CLASSPTR"sv;
			case gsdk::FIELD_EHANDLE:
			return "FIELD_EHANDLE"sv;
			case gsdk::FIELD_EDICT:
			return "FIELD_EDICT"sv;
			case gsdk::FIELD_POSITION_VECTOR:
			return "FIELD_POSITION_VECTOR"sv;
			case gsdk::FIELD_TIME:
			return "FIELD_TIME"sv;
			case gsdk::FIELD_TICK:
			return "FIELD_TICK"sv;
			case gsdk::FIELD_MODELNAME:
			return "FIELD_MODELNAME"sv;
			case gsdk::FIELD_SOUNDNAME:
			return "FIELD_SOUNDNAME"sv;
			case gsdk::FIELD_INPUT:
			return "FIELD_INPUT"sv;
			case gsdk::FIELD_FUNCTION:
			return "FIELD_FUNCTION"sv;
			case gsdk::FIELD_VMATRIX:
			return "FIELD_VMATRIX"sv;
			case gsdk::FIELD_VMATRIX_WORLDSPACE:
			return "FIELD_VMATRIX_WORLDSPACE"sv;
			case gsdk::FIELD_MATRIX3X4_WORLDSPACE:
			return "FIELD_MATRIX3X4_WORLDSPACE"sv;
			case gsdk::FIELD_INTERVAL:
			return "FIELD_INTERVAL"sv;
			case gsdk::FIELD_MODELINDEX:
			return "FIELD_MODELINDEX"sv;
			case gsdk::FIELD_MATERIALINDEX:
			return "FIELD_MATERIALINDEX"sv;
			case gsdk::FIELD_VECTOR2D:
			return "FIELD_VECTOR2D"sv;
			//case gsdk::FIELD_TYPECOUNT:
			//return "FIELD_TYPECOUNT"sv;
			case gsdk::FIELD_TYPEUNKNOWN:
			return "FIELD_TYPEUNKNOWN"sv;
			case gsdk::FIELD_CSTRING:
			return "FIELD_CSTRING"sv;
			case gsdk::FIELD_HSCRIPT:
			return "FIELD_HSCRIPT"sv;
			case gsdk::FIELD_VARIANT:
			return "FIELD_VARIANT"sv;
			case gsdk::FIELD_UINT64:
			return "FIELD_UINT64"sv;
			case gsdk::FIELD_DOUBLE:
			return "FIELD_DOUBLE"sv;
			case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
			return "FIELD_POSITIVEINTEGER_OR_NULL"sv;
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			return "FIELD_HSCRIPT_NEW_INSTANCE"sv;
			case gsdk::FIELD_UINT:
			return "FIELD_UINT"sv;
			case gsdk::FIELD_UTLSTRINGTOKEN:
			return "FIELD_UTLSTRINGTOKEN"sv;
			case gsdk::FIELD_QANGLE:
			return "FIELD_QANGLE"sv;
			case gsdk::FIELD_INTEGER64:
			return "FIELD_INTEGER64"sv;
			case gsdk::FIELD_VECTOR4D:
			return "FIELD_VECTOR4D"sv;
			case gsdk::FIELD_RESOURCE:
			return "FIELD_RESOURCE"sv;
			default: {
				temp_buffer = "<<unknown: "sv;

				temp_buffer.reserve(temp_buffer.length() + 6);

				char *begin{temp_buffer.data() + temp_buffer.length()};
				char *end{begin + 6};

				std::to_chars_result tc_res{std::to_chars(begin, end, type)};
				tc_res.ptr[0] = '\0';

				return temp_buffer.data();
			}
		}
	}

	static std::string_view datatype_to_str(gsdk::ScriptDataType_t type) noexcept
	{
		using namespace std::literals::string_view_literals;

		switch(type) {
			case gsdk::FIELD_VOID:
			return "void"sv;
			case gsdk::FIELD_CHARACTER:
			return "char"sv;
			case gsdk::FIELD_SHORT:
			case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_UINT:
			case gsdk::FIELD_INTEGER64:
			case gsdk::FIELD_UINT64:
			case gsdk::FIELD_MODELINDEX:
			case gsdk::FIELD_MATERIALINDEX:
			case gsdk::FIELD_TICK:
			return "int"sv;
			case gsdk::FIELD_DOUBLE:
			case gsdk::FIELD_FLOAT:
			case gsdk::FIELD_INTERVAL:
			case gsdk::FIELD_TIME:
			return "float"sv;
			case gsdk::FIELD_BOOLEAN:
			return "bool"sv;
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT:
			return "handle"sv;
			case gsdk::FIELD_POSITION_VECTOR:
			case gsdk::FIELD_VECTOR:
			return "Vector"sv;
			case gsdk::FIELD_VECTOR2D:
			return "Vector2D"sv;
			case gsdk::FIELD_QANGLE:
			return "QAngle"sv;
			case gsdk::FIELD_QUATERNION:
			return "Quaternion"sv;
			case gsdk::FIELD_STRING:
			case gsdk::FIELD_CSTRING:
			case gsdk::FIELD_MODELNAME:
			case gsdk::FIELD_SOUNDNAME:
			return "string"sv;
			case gsdk::FIELD_VARIANT:
			return "variant"sv;
			case gsdk::FIELD_EHANDLE:
			return "ehandle"sv;
			case gsdk::FIELD_EDICT:
			return "edict"sv;
			case gsdk::FIELD_FUNCTION:
			return "function"sv;
			case gsdk::FIELD_CLASSPTR:
			return "object"sv;
			case gsdk::FIELD_TYPEUNKNOWN:
			return "unknown"sv;
			default:
			return datatype_to_raw_str(type);
		}
	}

	static void add_gen_date(std::string &file) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::time_t time{std::time(nullptr)};

		char timestr[256];
		std::strftime(timestr, sizeof(timestr), "//Generated on %Y-%m-%d %H:%M:%S UTC\n\n", std::gmtime(&time));

		file += timestr;
	}

	static std::string_view get_func_desc_desc(const gsdk::ScriptFuncDescriptor_t *desc) noexcept
	{
		if(desc->m_pszDescription[0] == '#') {
			const char *ptr{desc->m_pszDescription};
			while(*ptr != ':') {
				++ptr;
			}
			++ptr;
			return ptr;
		} else {
			return desc->m_pszDescription;
		}
	}

	bool vmod::write_func(const gsdk::ScriptFunctionBinding_t *func, bool global, std::size_t depth, std::string &file, bool respect_hide) const noexcept
	{
		using namespace std::literals::string_view_literals;

		const gsdk::ScriptFuncDescriptor_t &func_desc{func->m_desc};

		if(respect_hide) {
			if(func_desc.m_pszDescription && func_desc.m_pszDescription[0] == '@') {
				return false;
			}
		}

		if(func_desc.m_pszDescription && func_desc.m_pszDescription[0] != '\0' && func_desc.m_pszDescription[0] != '@') {
			ident(file, depth);
			file += "//"sv;
			file += get_func_desc_desc(&func_desc);
			file += '\n';
		}

		ident(file, depth);

		if(global) {
			if(!(func->m_flags & gsdk::SF_MEMBER_FUNC)) {
				file += "static "sv;
			}
		}

		file += datatype_to_str(func_desc.m_ReturnType);
		file += ' ';
		file += func_desc.m_pszScriptName;

		file += '(';
		std::size_t num_args{func_desc.m_Parameters.size()};
		for(std::size_t j{0}; j < num_args; ++j) {
			file += datatype_to_str(func_desc.m_Parameters[j]);
			file += ", "sv;
		}
		if(func->m_flags & func_desc_t::SF_VA_FUNC) {
			file += "..."sv;
		} else {
			if(num_args > 0) {
				file.erase(file.end()-2, file.end());
			}
		}
		file += ");\n\n"sv;

		return true;
	}

	static std::string_view get_class_desc_name(const gsdk::ScriptClassDesc_t *desc) noexcept
	{
		if(desc->m_pNextDesc == reinterpret_cast<const gsdk::ScriptClassDesc_t *>(uninitialized_memory)) {
			const extra_class_desc_t &extra{static_cast<const base_class_desc_t<empty_class> *>(desc)->extra()};
			if(!extra.doc_class_name.empty()) {
				return extra.doc_class_name;
			}
		}

		return desc->m_pszScriptName;
	}

	static std::string_view get_class_desc_desc(const gsdk::ScriptClassDesc_t *desc) noexcept
	{
		if(desc->m_pszDescription[0] == '!') {
			return desc->m_pszDescription + 1;
		} else {
			return desc->m_pszDescription;
		}
	}

	bool vmod::write_class(const gsdk::ScriptClassDesc_t *desc, bool global, std::size_t depth, std::string &file, bool respect_hide) const noexcept
	{
		using namespace std::literals::string_view_literals;

		if(respect_hide) {
			if(desc->m_pszDescription && desc->m_pszDescription[0] == '@') {
				return false;
			}
		}

		if(global) {
			if(desc->m_pszDescription && desc->m_pszDescription[0] != '\0' && desc->m_pszDescription[0] != '@') {
				ident(file, depth);
				file += "//"sv;
				file += get_class_desc_desc(desc);
				file += '\n';
			}

			ident(file, depth);
			file += "class "sv;
			file += get_class_desc_name(desc);

			if(desc->m_pBaseDesc) {
				file += " : "sv;
				file += get_class_desc_name(desc->m_pBaseDesc);
			}

			file += '\n';
			ident(file, depth);
			file += "{\n"sv;

			if(desc->m_pfnConstruct) {
				ident(file, depth+1);
				file += get_class_desc_name(desc);
				file += "();\n\n"sv;
			}

			if(desc->m_pfnDestruct) {
				ident(file, depth+1);
				file += '~';
				file += get_class_desc_name(desc);
				file += "();\n\n"sv;
			}
		}

		std::size_t written{0};
		for(std::size_t i{0}; i < desc->m_FunctionBindings.size(); ++i) {
			if(write_func(&desc->m_FunctionBindings[i], global, global ? depth+1 : depth, file, respect_hide)) {
				++written;
			}
		}
		if(written > 0) {
			file.erase(file.end()-1, file.end());
		}

		if(global) {
			ident(file, depth);
			file += "};"sv;
		}

		return true;
	}

	void vmod::write_docs(const std::filesystem::path &dir, const std::vector<const gsdk::ScriptClassDesc_t *> &vec, bool respect_hide) const noexcept
	{
		using namespace std::literals::string_view_literals;

		for(const gsdk::ScriptClassDesc_t *desc : vec) {
			std::string file;

			add_gen_date(file);

			if(!write_class(desc, true, 0, file, respect_hide)) {
				continue;
			}

			std::filesystem::path doc_path{dir};
			doc_path /= get_class_desc_name(desc);
			doc_path.replace_extension(".txt"sv);

			write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
		}
	}

	void vmod::write_docs(const std::filesystem::path &dir, const std::vector<const gsdk::ScriptFunctionBinding_t *> &vec, bool respect_hide) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		add_gen_date(file);

		std::size_t written{0};
		for(const gsdk::ScriptFunctionBinding_t *desc : vec) {
			if(write_func(desc, false, 0, file, respect_hide)) {
				++written;
			}
		}
		if(written > 0) {
			file.erase(file.end()-1, file.end());
		}

		std::filesystem::path doc_path{dir};
		doc_path /= "globals"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void vmod::write_syms_docs(const std::filesystem::path &dir) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		add_gen_date(file);

		file += "namespace syms\n{\n"sv;

		write_class(&lib_symbols_desc, true, 1, file, false);
		file += "\n\n"sv;

		write_class(&lib_symbols_singleton::qual_it_desc, true, 1, file, false);
		file += "\n\n"sv;

		write_class(&lib_symbols_singleton::name_it_desc, true, 1, file, false);
		file += "\n\n"sv;

		ident(file, 1);
		file += get_class_desc_name(&lib_symbols_desc);
		file += " sv;"sv;

		file += "\n}"sv;

		std::filesystem::path doc_path{dir};
		doc_path /= "syms"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void vmod::write_strtables_docs(const std::filesystem::path &dir) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		add_gen_date(file);

		file += "namespace strtables\n{\n"sv;

		write_class(&stringtable_desc, true, 1, file, false);
		file += "\n\n"sv;

		for(const auto &it : script_stringtables) {
			ident(file, 1);
			file += get_class_desc_name(&stringtable_desc);
			file += ' ';
			file += it.first;
			file += ";\n"sv;
		}

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "strtables"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void vmod::write_yaml_docs(const std::filesystem::path &dir) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		add_gen_date(file);

		file += "namespace yaml\n{\n"sv;

		write_class(&yaml_desc, true, 1, file, false);
		file += "\n\n"sv;

		write_class(&yaml_singleton_desc, false, 1, file, false);

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "yaml"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void vmod::write_fs_docs(const std::filesystem::path &dir) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		add_gen_date(file);

		file += "namespace fs\n{\n"sv;

		write_class(&filesystem_singleton_desc, false, 1, file, false);
		file += '\n';

		ident(file, 1);
		file += "string game_dir;\n"sv;

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "fs"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void vmod::write_cvar_docs(const std::filesystem::path &dir) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		add_gen_date(file);

		file += "namespace cvar\n{\n"sv;

		write_class(&cvar_singleton::script_cvar_desc, true, 1, file, false);
		file += "\n\n"sv;

		ident(file, 1);
		file += "enum class flags\n"sv;
		ident(file, 1);
		file += "{\n"sv;
		write_enum_table(file, 2, ::vmod::cvar_singleton.flags_table, write_enum_how::flags);
		ident(file, 1);
		file += "};\n\n"sv;

		write_class(&cvar_singleton_desc, false, 1, file, false);

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "cvar"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void vmod::write_mem_docs(const std::filesystem::path &dir) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		add_gen_date(file);

		file += "namespace mem\n{\n"sv;

		write_class(&mem_block_desc, true, 1, file, false);
		file += "\n\n"sv;

		write_class(&memory_desc, false, 1, file, false);

		file += '\n';

		ident(file, 1);
		file += "namespace types\n"sv;
		ident(file, 1);
		file += "{\n"sv;
		ident(file, 2);
		file += "struct type\n"sv;
		ident(file, 2);
		file += "{\n"sv;
		ident(file, 3);
		file += "int size;\n"sv;
		ident(file, 3);
		file += "int alignment;\n"sv;
		ident(file, 3);
		file += "int id;\n"sv;
		ident(file, 3);
		file += "string name;\n"sv;
		ident(file, 2);
		file += "};\n\n"sv;
		for(const memory_singleton::mem_type &type : memory_singleton.types) {
			ident(file, 2);
			file += "type "sv;
			file += type.name;
			file += ";\n"sv;
		}
		ident(file, 1);
		file += "}\n}"sv;

		std::filesystem::path doc_path{dir};
		doc_path /= "mem"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void vmod::write_ffi_docs(const std::filesystem::path &dir) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		add_gen_date(file);

		file += "namespace ffi\n{\n"sv;

		write_class(&detour_desc, true, 1, file, false);
		file += "\n\n"sv;

		write_class(&cif_desc, true, 1, file, false);
		file += "\n\n"sv;

		write_class(&ffi_singleton_desc, false, 1, file, false);

		file += '\n';

		ident(file, 1);
		file += "enum class types\n"sv;
		ident(file, 1);
		file += "{\n"sv;
		write_enum_table(file, 2, ::vmod::ffi_singleton.types_table, write_enum_how::name);
		ident(file, 1);
		file += "};\n\n"sv;

		ident(file, 1);
		file += "enum class abi\n"sv;
		ident(file, 1);
		file += "{\n"sv;
		write_enum_table(file, 2, ::vmod::ffi_singleton.abi_table, write_enum_how::normal);
		ident(file, 1);
		file += "};\n}"sv;

		std::filesystem::path doc_path{dir};
		doc_path /= "ffi"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void vmod::write_ent_docs(const std::filesystem::path &dir) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		add_gen_date(file);

		file += "namespace ent\n{\n"sv;

		write_class(&entities_singleton::script_factory_impl_desc, true, 1, file, false);
		file += "\n\n"sv;

		write_class(&entities_singleton::script_factory_desc, true, 1, file, false);
		file += "\n\n"sv;

		write_class(&entities_singleton_desc, false, 1, file, false);

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "ent"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void vmod::write_vmod_docs(const std::filesystem::path &dir) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string file;

		add_gen_date(file);

		file += "namespace vmod\n{\n"sv;
		write_class(&vmod_desc, false, 1, file, false);
		file += '\n';

		ident(file, 1);
		file += "string root_dir;\n\n"sv;

		write_class(&plugin::owned_instance_desc, true, 1, file, false);
		file += "\n\n"sv;

		write_class(&plugin_desc, true, 1, file, false);
		file += "\n\n"sv;

		ident(file, 1);
		file += "namespace plugins\n"sv;
		ident(file, 1);
		file += "{\n"sv;
		ident(file, 1);
		file += "}\n\n"sv;

		ident(file, 1);
		file += "namespace syms;\n\n"sv;
		write_syms_docs(dir);

		ident(file, 1);
		file += "namespace yaml;\n\n"sv;
		write_yaml_docs(dir);

		ident(file, 1);
		file += "namespace ffi;\n\n"sv;
		write_ffi_docs(dir);

		ident(file, 1);
		file += "namespace fs;\n\n"sv;
		write_fs_docs(dir);

		ident(file, 1);
		file += "namespace cvar;\n\n"sv;
		write_cvar_docs(dir);

		ident(file, 1);
		file += "namespace mem;\n\n"sv;
		write_mem_docs(dir);

		ident(file, 1);
		file += "namespace strtables;\n\n"sv;
		write_strtables_docs(dir);

		ident(file, 1);
		file += "namespace ent;\n"sv;
		write_ent_docs(dir);

		file += '}';

		std::filesystem::path doc_path{dir};
		doc_path /= "vmod"sv;
		doc_path.replace_extension(".txt"sv);

		write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
	}

	void vmod::write_enum_table(std::string &file, std::size_t depth, gsdk::HSCRIPT enum_table, write_enum_how how) const noexcept
	{
		using namespace std::literals::string_view_literals;

		std::unordered_map<int, std::string> bit_str_map;

		int num2{vm_->GetNumTableEntries(enum_table)};
		for(int j{0}, it2{0}; it2 != -1 && j < num2; ++j) {
			script_variant_t key2;
			script_variant_t value2;
			it2 = vm_->GetKeyValue(enum_table, it2, &key2, &value2);

			std::string_view value_name{key2.get<std::string_view>()};

			ident(file, depth);
			file += value_name;
			if(how != write_enum_how::name) {
				file += " = "sv;
			}

			std::string_view value_str;
			if(value2.m_type == gsdk::FIELD_VOID) {
				value_str = "void"sv;
			} else {
				value_str = value2.get<std::string_view>();
			}

			if(value2.m_type == gsdk::FIELD_VOID) {
				if(how != write_enum_how::name) {
					file += value_str;
				}
			} else {
				if(how == write_enum_how::flags) {
					unsigned int val{value2.get<unsigned int>()};

					std::vector<int> bits;

					for(int k{0}; val; val >>= 1, ++k) {
						if(val & 1) {
							bits.emplace_back(k);
						}
					}

					std::size_t num_bits{bits.size()};

					if(num_bits > 0) {
						std::string temp_bit_str;

						if(num_bits > 1) {
							file += '(';
						}
						for(int bit : bits) {
							auto name_it{bit_str_map.find(bit)};
							if(name_it != bit_str_map.end()) {
								file += name_it->second;
								file += '|';
							} else {
								file += "(1 << "sv;

								temp_bit_str.resize(6);

								char *begin{temp_bit_str.data()};
								char *end{temp_bit_str.data() + 6};

								std::to_chars_result tc_res{std::to_chars(begin, end, bit)};
								tc_res.ptr[0] = '\0';

								file += begin;
								file += ")|"sv;
							}
						}
						file.pop_back();
						if(num_bits > 1) {
							file += ')';
						}

						if(num_bits == 1) {
							bit_str_map.emplace(bits[0], value_name);
						}
					} else {
						file += value_str;
					}
				} else if(how == write_enum_how::normal) {
					file += value_str;
				}
			}

			if(j < num2-1) {
				file += ',';
			}

			if(value2.m_type != gsdk::FIELD_VOID) {
				if(how == write_enum_how::flags) {
					file += " //"sv;
					file += value_str;
				}
			}

			file += '\n';
		}
	}

	bool vmod::load_late() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(!bindings()) {
			return false;
		}

		sv_engine->InsertServerCommand("exec vmod/load_late.cfg\n");
		sv_engine->ServerExecute();

		//TODO!!! make this a cmd/cvar/cmdline opt
		{
			std::filesystem::path game_docs{root_dir_/"docs"sv/"game"sv};
			write_docs(game_docs, game_vscript_class_bindings, false);
			write_docs(game_docs, game_vscript_func_bindings, false);

			gsdk::HSCRIPT const_table;
			if(vm_->GetValue(nullptr, "Constants", &const_table)) {
				std::string file;

				add_gen_date(file);

				int num{vm_->GetNumTableEntries(const_table)};
				for(int i{0}, it{0}; it != -1 && i < num; ++i) {
					script_variant_t key;
					script_variant_t value;
					it = vm_->GetKeyValue(const_table, it, &key, &value);

					std::string_view enum_name{key.get<std::string_view>()};

					if(enum_name[0] == 'E' || enum_name[0] == 'F') {
						file += "enum class "sv;
					} else {
						file += "namespace "sv;
					}
					file += enum_name;
					file += "\n{\n"sv;

					gsdk::HSCRIPT enum_table{value.get<gsdk::HSCRIPT>()};
					write_enum_table(file, 1, enum_table, enum_name[0] == 'F' ? write_enum_how::flags : write_enum_how::normal);

					file += '}';

					if(enum_name[0] == 'E' || enum_name[0] == 'F') {
						file += ';';
					}

					file += "\n\n"sv;
				}
				if(num > 0) {
					file.erase(file.end()-1, file.end());
				}

				std::filesystem::path doc_path{game_docs};
				doc_path /= "Constants"sv;
				doc_path.replace_extension(".txt"sv);

				write_file(doc_path, reinterpret_cast<const unsigned char *>(file.c_str()), file.length());
			}

			std::filesystem::path vmod_docs{root_dir_/"docs"sv/"vmod"sv};
			write_vmod_docs(vmod_docs);
		}

		vmod_refresh_plugins();

		return true;
	}

	void vmod::map_loaded(std::string_view name) noexcept
	{
		is_map_loaded = true;

		for(const auto &it : plugins) {
			if(!*it.second) {
				continue;
			}

			it.second->map_loaded(name);
		}
	}

	void vmod::map_active() noexcept
	{
		is_map_active = true;

		for(const auto &it : plugins) {
			if(!*it.second) {
				continue;
			}

			it.second->map_active();
		}
	}

	void vmod::map_unloaded() noexcept
	{
		if(is_map_loaded) {
			for(const auto &it : plugins) {
				if(!*it.second) {
					continue;
				}

				it.second->map_unloaded();
			}
		}

		is_map_loaded = false;
		is_map_active = false;
	}

	void vmod::game_frame(bool simulating) noexcept
	{
	#if 0
		vm_->Frame(sv_globals->frametime);
	#endif

		for(const auto &it : plugins) {
			it.second->game_frame(simulating);
		}
	}

	void vmod::unload() noexcept
	{
		vmod_unload_plugins();

		if(vm_) {
			unbindings();
		}

		vmod_reload_plugins.unregister();
		vmod_unload_plugins.unregister();
		vmod_unload_plugin.unregister();
		vmod_load_plugin.unregister();
		vmod_list_plugins.unregister();
		vmod_refresh_plugins.unregister();

		if(cvar_dll_id_ != gsdk::INVALID_CVAR_DLL_IDENTIFIER) {
			cvar->UnregisterConCommands(cvar_dll_id_);
		}

		if(vm_) {
			if(to_string_func && to_string_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(to_string_func);
			}

			if(to_int_func && to_int_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(to_int_func);
			}

			if(to_float_func && to_float_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(to_float_func);
			}

			if(to_bool_func && to_bool_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(to_bool_func);
			}

			if(typeof_func && typeof_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(typeof_func);
			}

			if(funcisg_func && funcisg_func != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseFunction(funcisg_func);
			}

			if(base_script && base_script != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseScript(base_script);
			}

			if(base_script_scope && base_script_scope != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseScope(base_script_scope);
			}

			if(server_init_script && server_init_script != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseScript(server_init_script);
			}

			if(scope_ && scope_ != gsdk::INVALID_HSCRIPT) {
				vm_->ReleaseScope(scope_);
			}

			vsmgr->DestroyVM(vm_);

			if(*g_pScriptVM_ptr == vm_) {
				*g_pScriptVM_ptr = nullptr;
			}

			if(gsdk::g_pScriptVM == vm_) {
				gsdk::g_pScriptVM = nullptr;
			}
		}

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		if(old_spew) {
			SpewOutputFunc(old_spew);
		}
	#endif
	}
}

namespace vmod
{
	class vsp final : public gsdk::IServerPluginCallbacks
	{
	public:
		inline vsp() noexcept
		{
			
		}

		virtual inline ~vsp() noexcept
		{
			if(!unloaded) {
				vmod.unload();
			}
		}

	private:
		const char *GetPluginDescription() override;
		bool Load(gsdk::CreateInterfaceFn, gsdk::CreateInterfaceFn) override;
		void Unload() override;
		void GameFrame(bool simulating) override;
		void ServerActivate([[maybe_unused]] gsdk::edict_t *edicts, [[maybe_unused]] int num_edicts, [[maybe_unused]] int max_clients) override;
		void LevelInit(const char *name) override;
		void LevelShutdown() override;

		bool load_return;
		bool unloaded;
	};

	const char *vsp::GetPluginDescription()
	{ return "vmod"; }

	bool vsp::Load(gsdk::CreateInterfaceFn, gsdk::CreateInterfaceFn)
	{
		load_return = vmod.load();

		if(!load_return) {
			return false;
		}

		if(!vmod.load_late()) {
			return false;
		}

		return true;
	}

	void vsp::Unload()
	{
		vmod.unload();
		unloaded = true;
	}

	void vsp::GameFrame(bool simulating)
	{ vmod.game_frame(simulating); }

	void vsp::ServerActivate([[maybe_unused]] gsdk::edict_t *edicts, [[maybe_unused]] int num_edicts, [[maybe_unused]] int max_clients)
	{ vmod.map_active(); }

	void vsp::LevelInit(const char *name)
	{ vmod.map_loaded(name); }

	void vsp::LevelShutdown()
	{ vmod.map_unloaded(); }

	static class vsp vsp;
}

extern "C" __attribute__((__visibility__("default"))) void * __attribute__((__cdecl__)) CreateInterface(const char *name, int *status)
{
	using namespace gsdk;

	if(std::strncmp(name, IServerPluginCallbacks::interface_name.data(), IServerPluginCallbacks::interface_name.length()) == 0) {
		if(status) {
			*status = IFACE_OK;
		}
		return static_cast<IServerPluginCallbacks *>(&vmod::vsp);
	} else {
		if(status) {
			*status = IFACE_FAILED;
		}
		return nullptr;
	}
}
