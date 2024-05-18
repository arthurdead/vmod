#include "factory.hpp"

namespace vmod::bindings::ent
{
	vscript::class_desc<factory_base> factory_base::desc{"ent::factory_base"};
	vscript::class_desc<factory_ref> factory_ref::desc{"ent::factory_ref"};
	vscript::class_desc<factory_impl> factory_impl::desc{"ent::factory_impl"};

	factory_ref::~factory_ref() noexcept {}
	factory_base::~factory_base() noexcept {}

	bool factory_base::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		desc.func(&factory_base::script_create, "script_create"sv, "create"sv)
		.desc("[entity](classname)"sv);

		desc.func(&factory_base::script_size, "script_size"sv, "size"sv);

		factory_ref::desc.func(&factory_ref::script_create_sized, "script_create_sized"sv, "create_sized"sv)
		.desc("[entity](classname, size)"sv);

		factory_ref::desc.base(desc);

		factory_impl::desc.func(&factory_impl::script_create, "script_create"sv, "create"sv)
		.desc("[entity](classname, create_options|opts)"sv);

		factory_impl::desc.dtor();

		factory_impl::desc.base(desc);

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register entity factory class\n"sv);
			return false;
		}

		if(!plugin::owned_instance::register_class(&factory_impl::desc)) {
			return false;
		}

		if(!vm->RegisterClass(&factory_ref::desc)) {
			error("vmod: failed to register entity factory ref class\n"sv);
			return false;
		}

		return true;
	}

	void factory_base::unbindings() noexcept
	{
		
	}

	gsdk::IServerNetworkable *factory_base::script_create(std::string_view classname) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(classname.empty()) {
			vm->RaiseException("vmod: empty classname");
			return nullptr;
		}

		return factory->Create(classname.data());
	}

	bool factory_impl::initialize(std::vector<std::string> &&names_, vscript::func_handle_wrapper &&callback_, vscript::func_handle_wrapper &&size_callback_) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!register_instance(&desc, this)) {
			return false;
		}

		names = std::move(names_);

		callback = std::move(callback_);
		size_callback = std::move(size_callback_);

		for(const auto &name : names) {
			entityfactorydict->InstallFactory(this, name.data());
		}

		return true;
	}

	static std::size_t tmp_ent_size{0};
	static std::size_t last_ent_size{0};

	static void *(gsdk::IVEngineServer::*PvAllocEntPrivateData_original)(long) {nullptr};
	static void *PvAllocEntPrivateData_detour_callback(gsdk::IVEngineServer *pthis, long cb) noexcept
	{
		if(tmp_ent_size > 0) {
			cb = static_cast<long>(tmp_ent_size);
		}

		last_ent_size = static_cast<std::size_t>(cb);

		void *ent{(pthis->*PvAllocEntPrivateData_original)(cb)};

		tmp_ent_size = 0;

		return ent;
	}

	gsdk::IServerNetworkable *factory_ref::script_create_sized(std::string_view classname, std::size_t size_) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(classname.empty()) {
			vm->RaiseException("vmod: invalid classname: '%s'", classname.data());
			return nullptr;
		}

		if(size_ == 0 || size_ == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size: %zu", size_);
			return nullptr;
		}

		std::size_t size{factory->GetEntitySize()};
		if(size_ < size) {
			vm->RaiseException("vmod: new size is less than base size: %zu vs %zu", size_, size);
			return nullptr;
		}

		tmp_ent_size = size_;

		gsdk::IServerNetworkable *net{factory->Create(classname.data())};

		tmp_ent_size = 0;

		return net;
	}

	bool factory_ref::detours() noexcept
	{
		generic_vtable_t sv_engine_vtable{vtable_from_object(sv_engine)};

		PvAllocEntPrivateData_original = swap_vfunc(sv_engine_vtable, &gsdk::IVEngineServer::PvAllocEntPrivateData, PvAllocEntPrivateData_detour_callback);

		return true;
	}

	bool factory_impl::detours() noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		return true;
	}

	gsdk::IServerNetworkable *factory_impl::script_create(std::string_view classname, std::optional<vscript::table_handle_wrapper> script_opts) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(classname.empty()) {
			vm->RaiseException("vmod: invalid classname: '%s'", classname.data());
			return nullptr;
		}

		std::optional<create_options> opts;

		std::size_t new_size{0};
		if(script_opts && *script_opts) {
			opts.emplace();

			vscript::variant additional_size;
			if(vm->GetValue(*(*script_opts), "additional_size", &additional_size)) {
				auto val{additional_size.get<std::size_t>()};
				if(val == static_cast<std::size_t>(-1)) {
					vm->RaiseException("vmod: invalid additional size");
					return nullptr;
				}

				opts->new_size.emplace(factory_impl::GetEntitySize());
				*opts->new_size += val;
			}
		}

		return create(classname, std::move(opts));
	}

	struct entity_custom_props_info_t
	{
		entity_custom_props_info_t() noexcept = default;

		std::vector<XXH64_hash_t> datamaps_contained;
		std::unique_ptr<allocated_datamap> datamap;

		inline bool empty() const noexcept
		{ return static_cast<bool>(datamap); }

		void set_datamap(const allocated_datamap &start, gsdk::datamap_t *base, std::size_t size) noexcept
		{
			datamaps_contained.emplace_back(start.hash());
			datamap = start.calculate_offsets(size);
			datamap->map->baseMap = base;
		}

		void append_datamap(const allocated_datamap &other) noexcept
		{
			XXH64_hash_t other_hash{other.hash()};

			if(std::find(datamaps_contained.begin(), datamaps_contained.end(), other_hash) == datamaps_contained.end()) {
				return;
			}

			datamaps_contained.emplace_back(other_hash);
			datamap->append(other);
		}

	private:
		entity_custom_props_info_t(const entity_custom_props_info_t &) = delete;
		entity_custom_props_info_t &operator=(const entity_custom_props_info_t &) = delete;
		entity_custom_props_info_t(entity_custom_props_info_t &&) = delete;
		entity_custom_props_info_t &operator=(entity_custom_props_info_t &&) = delete;
	};

	static std::unordered_map<std::size_t, std::unique_ptr<entity_custom_props_info_t>> entity_custom_props;

	struct entity_vtable_info_t
	{
		entity_vtable_info_t() noexcept = default;

		generic_plain_mfp_t ent_GetServerClass_original{nullptr};
		ffi::closure ent_GetServerClass_closure{&ffi_type_pointer, {&ffi_type_pointer}};

		generic_plain_mfp_t net_GetServerClass_original{nullptr};
		ffi::closure net_GetServerClass_closure{&ffi_type_pointer, {&ffi_type_pointer}};

		generic_plain_mfp_t GetDataDescMap_original{nullptr};
		ffi::closure GetDataDescMap_closure{&ffi_type_pointer, {&ffi_type_pointer}};

		std::unique_ptr<__cxxabiv1::vtable_prefix> ent_prefix;
		std::unique_ptr<__cxxabiv1::vtable_prefix> net_prefix;

		entity_custom_props_info_t *props{nullptr};

		allocated_server_class *svclass{nullptr};

	private:
		entity_vtable_info_t(const entity_vtable_info_t &) = delete;
		entity_vtable_info_t &operator=(const entity_vtable_info_t &) = delete;
		entity_vtable_info_t(entity_vtable_info_t &&) = delete;
		entity_vtable_info_t &operator=(entity_vtable_info_t &&) = delete;
	};

	static std::unordered_map<generic_vtable_t, std::unique_ptr<entity_vtable_info_t>> entity_vtables;

	static std::unordered_map<generic_vtable_t, std::size_t> entity_vtable_sizes;

	static void GetDataDescMap_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr)
	{
		entity_vtable_info_t *info{static_cast<entity_vtable_info_t *>(userptr)};

		if(info->props && !info->props->empty()) {
			*static_cast<gsdk::datamap_t **>(ret) = info->props->datamap->maps[0].get();
		} else {
			ffi_call(closure_cif, reinterpret_cast<void(*)()>(info->GetDataDescMap_original), ret, args);
		}
	}

	static void ent_GetServerClass_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr)
	{
		entity_vtable_info_t *info{static_cast<entity_vtable_info_t *>(userptr)};

		if(info->svclass) {
			*static_cast<gsdk::ServerClass **>(ret) = info->svclass->svclass.get();
		} else {
			ffi_call(closure_cif, reinterpret_cast<void(*)()>(info->ent_GetServerClass_original), ret, args);
		}
	}

	static void net_GetServerClass_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr)
	{
		entity_vtable_info_t *info{static_cast<entity_vtable_info_t *>(userptr)};

		if(info->svclass) {
			*static_cast<gsdk::ServerClass **>(ret) = info->svclass->svclass.get();
		} else {
			ffi_call(closure_cif, reinterpret_cast<void(*)()>(info->net_GetServerClass_original), ret, args);
		}
	}

	gsdk::IServerNetworkable *factory_impl::create(std::string_view classname, std::optional<create_options> opts) noexcept
	{
		if(callback) {
			gsdk::IScriptVM *vm{vscript::vm()};

			vscript::handle_ref pl_scope{owner_scope()};

			vscript::variant args[]{
				instance_,
				classname,
				{}
			};

			if(opts && opts->new_size) {
				args[2] = *opts->new_size;
			} else {
				args[2] = nullptr;
			}

			vscript::variant ret;
			if(vm->ExecuteFunction(*callback, args, std::size(args), &ret, *pl_scope, true) == gsdk::SCRIPT_ERROR) {
				return nullptr;
			}

			gsdk::IServerNetworkable *net{ret.get<gsdk::IServerNetworkable *>()};
			if(!net) {
				return nullptr;
			}

			gsdk::CBaseEntity *ent{net->GetBaseEntity()};

		#if 0
			gsdk::datamap_t *base_datamap{ent->GetDataDescMap()};

			__cxxabiv1::vtable_prefix *ent_prefix{vtable_prefix_from_object(ent)};
			__cxxabiv1::vtable_prefix *net_prefix{vtable_prefix_from_object(net)};
			generic_vtable_t old_ent_vtable{vtable_from_prefix(ent_prefix)};
			const __cxxabiv1::__class_type_info *ent_type{ent_prefix->whole_type};

			std::size_t type_hash{ent_type->hash_code()};

			auto vtable_it{entity_vtables.find(old_ent_vtable)};
			if(vtable_it == entity_vtables.end()) {
				auto ent_vtable_size_it{entity_vtable_sizes.find(old_ent_vtable)};
				if(ent_vtable_size_it == entity_vtable_sizes.end()) {
				#ifndef GSDK_NO_SYMBOLS
					if(symbols_available) {
						const auto &sv_symbols{main::instance().sv_syms()};

						std::size_t ent_vtable_size{sv_symbols.vtable_size(demangle(ent_type->name()))};
						if(ent_vtable_size == static_cast<std::size_t>(-1)) {
							debugtrap();
						}

						ent_vtable_size_it = entity_vtable_sizes.emplace(old_ent_vtable, ent_vtable_size).first;
					} else {
				#endif
						
				#ifndef GSDK_NO_SYMBOLS
					}
				#endif
				}

				std::size_t ent_vtable_size{ent_vtable_size_it->second};
				std::size_t net_vtable_size{gsdk::IServerNetworkable::vtable_size};

				std::unique_ptr<entity_vtable_info_t> info{new entity_vtable_info_t};

				info->ent_prefix = copy_prefix(ent_prefix, ent_vtable_size);
				info->net_prefix = copy_prefix(net_prefix, net_vtable_size);

				generic_vtable_t new_ent_vtable{vtable_from_prefix(info->ent_prefix.get())};
				generic_vtable_t new_net_vtable{vtable_from_prefix(info->net_prefix.get())};

				info->GetDataDescMap_original = new_ent_vtable[gsdk::CBaseEntity::GetDataDescMap_vindex];
				info->GetDataDescMap_closure.initialize(FFI_DEFAULT_ABI, new_ent_vtable[gsdk::CBaseEntity::GetDataDescMap_vindex], GetDataDescMap_binding, *info);

				info->ent_GetServerClass_original = new_ent_vtable[gsdk::CBaseEntity::GetServerClass_vindex];
				info->ent_GetServerClass_closure.initialize(FFI_DEFAULT_ABI, new_ent_vtable[gsdk::CBaseEntity::GetServerClass_vindex], ent_GetServerClass_binding, *info);

				info->net_GetServerClass_original = new_net_vtable[gsdk::IServerNetworkable::GetServerClass_vindex];
				info->net_GetServerClass_closure.initialize(FFI_DEFAULT_ABI, new_net_vtable[gsdk::IServerNetworkable::GetServerClass_vindex], net_GetServerClass_binding, *info);

				vtable_it = entity_vtables.emplace(old_ent_vtable, std::move(info)).first;
			}

			swap_prefix(ent, vtable_it->second->ent_prefix.get());
			swap_prefix(net, vtable_it->second->net_prefix.get());

			auto props_it{entity_custom_props.find(type_hash)};
			if(props_it == entity_custom_props.end()) {
				props_it = entity_custom_props.emplace(type_hash, new entity_custom_props_info_t).first;
				vtable_it->second->props = props_it->second.get();
			}

			if(props_it->second->empty()) {
				props_it->second->set_datamap(*datamap, base_datamap, last_ent_size);
			} else {
				props_it->second->append_datamap(*datamap);
			}
		#endif

			return net;
		}

		return nullptr;
	}

	factory_impl::~factory_impl() noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		for(const std::string &name : names) {
			std::size_t i{entityfactorydict->m_Factories.find(name)};
			if(i != entityfactorydict->m_Factories.npos) {
				entityfactorydict->m_Factories.erase(i);
			}
		}

		callback.free();

		size_callback.free();
	}

	gsdk::IServerNetworkable *factory_impl::Create(const char *classname)
	{ return create(classname); }

	void factory_impl::Destroy(gsdk::IServerNetworkable *net)
	{
		if(net) {
			net->Release();
		}
	}

	size_t factory_impl::GetEntitySize()
	{
		if(size_callback) {
			gsdk::IScriptVM *vm{vscript::vm()};

			vscript::scope_handle_ref pl_scope{owner_scope()};

			vscript::variant args[]{
				instance_
			};

			vscript::variant ret;
			if(vm->ExecuteFunction(*size_callback, args, std::size(args), &ret, *pl_scope, true) == gsdk::SCRIPT_ERROR) {
				return 0;
			}

			return ret.get<std::size_t>();
		}

		return 0;
	}
}
