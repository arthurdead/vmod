#include "ffi.hpp"
#include "gsdk/tier1/utlstring.hpp"

namespace vmod
{
	static void script_var_to_ffi_ptr(ffi_type *type_ptr, void *arg_ptr, const script_variant_t &arg_var) noexcept
	{
		switch(type_ptr->type) {
			case FFI_TYPE_INT:
			*reinterpret_cast<int *>(arg_ptr) = arg_var.get<int>();
			break;
			case FFI_TYPE_FLOAT:
			*reinterpret_cast<float *>(arg_ptr) = arg_var.get<float>();
			break;
			case FFI_TYPE_DOUBLE:
			*reinterpret_cast<double *>(arg_ptr) = arg_var.get<double>();
			break;
			case FFI_TYPE_LONGDOUBLE:
			*reinterpret_cast<long double *>(arg_ptr) = arg_var.get<long double>();
			break;
			case FFI_TYPE_UINT8:
			*reinterpret_cast<unsigned char *>(arg_ptr) = static_cast<unsigned char>(arg_var.get<unsigned short>());
			break;
			case FFI_TYPE_SINT8:
			*reinterpret_cast<signed char *>(arg_ptr) = static_cast<signed char>(arg_var.get<short>());
			break;
			case FFI_TYPE_UINT16:
			*reinterpret_cast<unsigned short *>(arg_ptr) = arg_var.get<unsigned short>();
			break;
			case FFI_TYPE_SINT16:
			*reinterpret_cast<short *>(arg_ptr) = arg_var.get<short>();
			break;
			case FFI_TYPE_UINT32:
			*reinterpret_cast<unsigned int *>(arg_ptr) = arg_var.get<unsigned int>();
			break;
			case FFI_TYPE_SINT32:
			*reinterpret_cast<int *>(arg_ptr) = arg_var.get<int>();
			break;
			case FFI_TYPE_UINT64:
			*reinterpret_cast<unsigned long long *>(arg_ptr) = arg_var.get<unsigned long long>();
			break;
			case FFI_TYPE_SINT64:
			*reinterpret_cast<long long *>(arg_ptr) = arg_var.get<long long>();
			break;
			case FFI_TYPE_POINTER:
			*reinterpret_cast<void **>(arg_ptr) = arg_var.get<void *>();
			break;
		}
	}

	static void ffi_ptr_to_script_var(ffi_type *type_ptr, void *arg_ptr, script_variant_t &arg_var) noexcept
	{
		switch(type_ptr->type) {
			case FFI_TYPE_INT:
			arg_var.assign<int>(*reinterpret_cast<int *>(arg_ptr));
			break;
			case FFI_TYPE_FLOAT:
			arg_var.assign<float>(*reinterpret_cast<float *>(arg_ptr));
			break;
			case FFI_TYPE_DOUBLE:
			arg_var.assign<double>(*reinterpret_cast<double *>(arg_ptr));
			break;
			case FFI_TYPE_LONGDOUBLE:
			arg_var.assign<long double>(*reinterpret_cast<long double *>(arg_ptr));
			break;
			case FFI_TYPE_UINT8:
			arg_var.assign<unsigned short>(*reinterpret_cast<unsigned char *>(arg_ptr));
			break;
			case FFI_TYPE_SINT8:
			arg_var.assign<short>(*reinterpret_cast<signed char *>(arg_ptr));
			break;
			case FFI_TYPE_UINT16:
			arg_var.assign<unsigned short>(*reinterpret_cast<unsigned short *>(arg_ptr));
			break;
			case FFI_TYPE_SINT16:
			arg_var.assign<short>(*reinterpret_cast<short *>(arg_ptr));
			break;
			case FFI_TYPE_UINT32:
			arg_var.assign<unsigned int>(*reinterpret_cast<unsigned int *>(arg_ptr));
			break;
			case FFI_TYPE_SINT32:
			arg_var.assign<int>(*reinterpret_cast<int *>(arg_ptr));
			break;
			case FFI_TYPE_UINT64:
			arg_var.assign<unsigned long long>(*reinterpret_cast<unsigned long long *>(arg_ptr));
			break;
			case FFI_TYPE_SINT64:
			arg_var.assign<long long>(*reinterpret_cast<long long *>(arg_ptr));
			break;
			case FFI_TYPE_POINTER:
			arg_var.assign<void *>(*reinterpret_cast<void **>(arg_ptr));
			break;
		}
	}

	static ffi_type *ffi_type_id_to_ptr(int id) noexcept
	{
		switch(id) {
			case FFI_TYPE_VOID: return &ffi_type_void;
			case FFI_TYPE_INT: return &ffi_type_sint;
			case FFI_TYPE_FLOAT: return &ffi_type_float;
			case FFI_TYPE_DOUBLE: return &ffi_type_double;
			case FFI_TYPE_LONGDOUBLE: return &ffi_type_longdouble;
			case FFI_TYPE_UINT8: return &ffi_type_uint8;
			case FFI_TYPE_SINT8: return &ffi_type_sint8;
			case FFI_TYPE_UINT16: return &ffi_type_uint16;
			case FFI_TYPE_SINT16: return &ffi_type_sint16;
			case FFI_TYPE_UINT32: return &ffi_type_uint32;
			case FFI_TYPE_SINT32: return &ffi_type_sint32;
			case FFI_TYPE_UINT64: return &ffi_type_uint64;
			case FFI_TYPE_SINT64: return &ffi_type_sint64;
			case FFI_TYPE_POINTER: return &ffi_type_pointer;
			default: return nullptr;
		}
	}

	class memory_singleton final : public gsdk::ISquirrelMetamethodDelegate
	{
	public:
		bool bindings() noexcept;

		void unbindings() noexcept
		{
			memory_block::unbindings();

			gsdk::IScriptVM *vm{vmod.vm()};

			if(get_impl) {
				vm->DestroySquirrelMetamethod_Get(get_impl);
			}

			if(instance && instance != gsdk::INVALID_HSCRIPT) {
				vm->RemoveInstance(instance);
			}

			if(scope && scope != gsdk::INVALID_HSCRIPT) {
				vm->ReleaseScope(scope);
			}

			gsdk::HSCRIPT vmod_scope{vmod.scope()};
			if(vm->ValueExists(vmod_scope, "memory")) {
				vm->ClearValue(vmod_scope, "memory");
			}
		}

		static gsdk::HSCRIPT script_allocate(std::size_t size) noexcept
		{
			memory_block *block{new memory_block{size}};

			if(!block->initialize()) {
				delete block;
				return nullptr;
			}

			return block->instance;
		}

		static gsdk::HSCRIPT script_allocate_aligned(std::size_t align, std::size_t size) noexcept
		{
			memory_block *block{new memory_block{static_cast<std::align_val_t>(align), size}};

			if(!block->initialize()) {
				delete block;
				return nullptr;
			}

			return block->instance;
		}

		static gsdk::HSCRIPT script_allocate_zero(std::size_t num, std::size_t size) noexcept
		{
			memory_block *block{new memory_block{num, size}};

			if(!block->initialize()) {
				delete block;
				return nullptr;
			}

			return block->instance;
		}

		static script_variant_t script_read(void *ptr, int type) noexcept
		{
			ffi_type *type_ptr{ffi_type_id_to_ptr(type)};
			if(!type_ptr) {
				vmod.vm()->RaiseException("vmod: invalid type");
				return {};
			}

			script_variant_t ret_var;
			ffi_ptr_to_script_var(type_ptr, ptr, ret_var);
			return ret_var;
		}

		static void script_write(void *ptr, int type, script_variant_t arg_var) noexcept
		{
			ffi_type *type_ptr{ffi_type_id_to_ptr(type)};
			if(!type_ptr) {
				vmod.vm()->RaiseException("vmod: invalid type");
				return;
			}

			script_var_to_ffi_ptr(type_ptr, ptr, arg_var);
		}

		bool Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value) override;

		gsdk::HSCRIPT scope{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		gsdk::CSquirrelMetamethodDelegateImpl *get_impl{nullptr};
	};

	static class memory_singleton memory_singleton;

	bool memory_singleton::Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value)
	{
		using namespace std::literals::string_view_literals;

		return vmod.vm()->GetValue(instance, name.c_str(), &value);
	}

	static class_desc_t<class memory_singleton> memory_desc{"__vmod_memory_singleton_class"};

	static class_desc_t<memory_block> mem_block_desc{"__vmod_memory_block_class"};

	bool memory_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		memory_desc.func(&memory_singleton::script_allocate, "__script_allocate"sv, "allocate"sv);
		memory_desc.func(&memory_singleton::script_allocate_aligned, "__script_allocate_aligned"sv, "allocate_aligned"sv);
		memory_desc.func(&memory_singleton::script_allocate_zero, "__script_allocate_zero"sv, "allocate_zero"sv);
		memory_desc.func(&memory_singleton::script_read, "__script_read"sv, "read"sv);
		memory_desc.func(&memory_singleton::script_write, "__script_write"sv, "write"sv);

		if(!vm->RegisterClass(&memory_desc)) {
			error("vmod: failed to register memory singleton script class\n"sv);
			return false;
		}

		instance = vm->RegisterInstance(&memory_desc, &::vmod::memory_singleton);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create memory singleton instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(instance, "__vmod_memory_singleton");

		gsdk::HSCRIPT vmod_scope{vmod.scope()};

	#if 0
		scope = vm->CreateScope("__vmod_memory_scope", nullptr);
		if(!scope || scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create memory scope\n"sv);
			return false;
		}

		if(!vm->SetValue(vmod_scope, "memory", scope)) {
			error("vmod: failed to set memory scope value\n"sv);
			return false;
		}

		get_impl = vm->MakeSquirrelMetamethod_Get(vmod_scope, "memory", &static_cast<gsdk::ISquirrelMetamethodDelegate &>(::vmod::memory_singleton), false);
		if(!get_impl) {
			error("vmod: failed to create memory _get metamethod\n"sv);
			return false;
		}
	#else
		if(!vm->SetValue(vmod_scope, "memory", instance)) {
			error("vmod: failed to set memory instance value\n"sv);
			return false;
		}
	#endif

		if(!memory_block::bindings()) {
			return false;
		}

		return true;
	}

	void memory_block::script_set_dtor_func(gsdk::HSCRIPT func) noexcept
	{
		dtor_func = vmod.vm()->ReferenceObject(func);
	}

	bool memory_block::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		mem_block_desc.func(&memory_block::script_set_dtor_func, "__script_set_dtor_func"sv, "hook_free"sv);
		mem_block_desc.func(&memory_block::script_disown, "__script_disown"sv, "disown"sv);
		mem_block_desc.func(&memory_block::script_delete, "__script_delete"sv, "free"sv);
		mem_block_desc.func(&memory_block::script_ptr_as_int, "__script_ptr_as_int"sv, "get_ptr_as_int"sv);
		mem_block_desc.func(&memory_block::script_ptr, "__script_ptr"sv, "get_ptr"sv);
		mem_block_desc.func(&memory_block::script_get_size, "__script_get_size"sv, "get_size"sv);
		mem_block_desc.dtor();

		if(!vm->RegisterClass(&mem_block_desc)) {
			error("vmod: failed to register memory block script class\n"sv);
			return false;
		}

		return true;
	}

	void memory_block::unbindings() noexcept
	{

	}

	memory_block::~memory_block() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(ptr) {
			if(dtor_func && dtor_func != gsdk::INVALID_HSCRIPT) {
				script_variant_t args{instance};
				vm->ExecuteFunction(dtor_func, &args, 1, nullptr, nullptr, true);

				vm->ReleaseFunction(dtor_func);
			}
		}

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance);
		}

		if(ptr) {
			free(ptr);
		}
	}

	bool memory_block::initialize() noexcept
	{
		using namespace std::literals::string_view_literals;

		instance = vmod.vm()->RegisterInstance(&mem_block_desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to register memory block instance\n"sv);
			return false;
		}

		//vm->SetInstanceUniqeId

		return true;
	}

	class script_cif final
	{
	public:
		inline ~script_cif() noexcept
		{
			if(instance && instance != gsdk::INVALID_HSCRIPT) {
				vmod.vm()->RemoveInstance(instance);
			}
		}

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	private:
		friend class ffi_singleton;

		inline script_cif(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: arg_type_ptrs{std::move(args)}, ret_type_ptr{ret}
		{
		}

		bool initialize(generic_func_t func_, ffi_abi abi) noexcept;

		script_variant_t script_call(const script_variant_t *va_args, std::size_t num_args, ...) noexcept
		{
			if(num_args != arg_type_ptrs.size()) {
				vmod.vm()->RaiseException("wrong number of parameters");
				return {};
			}

			for(std::size_t i{0}; i < num_args; ++i) {
				ffi_type *arg_type{arg_type_ptrs[i]};
				const script_variant_t &arg_var{va_args[i]};
				std::unique_ptr<unsigned char[]> &arg_ptr{args_storage[i]};

				script_var_to_ffi_ptr(arg_type, reinterpret_cast<void *>(arg_ptr.get()), arg_var);
			}

			ffi_call(&cif_, reinterpret_cast<void(*)()>(func), reinterpret_cast<void *>(ret_storage.get()), const_cast<void **>(args_storage_ptrs.data()));

			script_variant_t ret_var;
			ffi_ptr_to_script_var(ret_type_ptr, reinterpret_cast<void *>(ret_storage.get()), ret_var);

			return ret_var;
		}

		ffi_cif cif_;

		std::vector<ffi_type *> arg_type_ptrs;
		ffi_type *ret_type_ptr;

		std::unique_ptr<unsigned char[]> ret_storage;
		std::vector<std::unique_ptr<unsigned char[]>> args_storage;
		std::vector<void *> args_storage_ptrs;

		generic_func_t func;
		gsdk::HSCRIPT instance;
	};

	static class_desc_t<script_cif> cif_desc{"__vmod_ffi_cif_class"};

	bool script_cif::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		cif_desc.func(&script_cif::script_call, "__script_call"sv, "call"sv);
		cif_desc.dtor();

		if(!vm->RegisterClass(&cif_desc)) {
			error("vmod: failed to register cif script class\n"sv);
			return false;
		}

		return true;
	}

	void script_cif::unbindings() noexcept
	{

	}

	bool script_cif::initialize(generic_func_t func_, ffi_abi abi) noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		if(ffi_prep_cif(&cif_, abi, arg_type_ptrs.size(), ret_type_ptr, const_cast<ffi_type **>(arg_type_ptrs.data())) != FFI_OK) {
			return false;
		}

		instance = vm->RegisterInstance(&cif_desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: failed to register cif instance");
			return false;
		}

		//vm->SetInstanceUniqeId

		func = func_;

		for(ffi_type *arg_type_ptr : arg_type_ptrs) {
			std::unique_ptr<unsigned char[]> &arg_ptr{args_storage.emplace_back()};
			arg_ptr.reset(reinterpret_cast<unsigned char *>(std::aligned_alloc(arg_type_ptr->alignment, arg_type_ptr->size)));
		}

		for(std::unique_ptr<unsigned char[]> &ptr : args_storage) {
			args_storage_ptrs.emplace_back(ptr.get());
		}

		if(ret_type_ptr != &ffi_type_void) {
			ret_storage.reset(reinterpret_cast<unsigned char *>(std::aligned_alloc(ret_type_ptr->alignment, ret_type_ptr->size)));
		}

		return true;
	}

	class ffi_singleton final : public gsdk::ISquirrelMetamethodDelegate
	{
	public:
		inline ~ffi_singleton() noexcept override
		{
		}

		bool bindings() noexcept;
		void unbindings() noexcept;

	private:
		static gsdk::HSCRIPT script_create_cif(generic_func_t func, ffi_abi abi, int ret, gsdk::HSCRIPT args) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			ffi_type *ret_ptr{ffi_type_id_to_ptr(ret)};
			if(!ret_ptr) {
				vm->RaiseException("vmod: invalid return type");
				return nullptr;
			}

			std::vector<ffi_type *> args_ptrs;

			int num_args{vm->GetArrayCount(args)};
			for(int i{0}; i < num_args; ++i) {
				script_variant_t value;
				vm->GetArrayValue(args, i, &value);

				if(value.m_type != gsdk::FIELD_INTEGER) {
					vm->RaiseException("vmod: not a ffi type");
					return nullptr;
				}

				ffi_type *arg_ptr{ffi_type_id_to_ptr(value.m_int)};
				if(!arg_ptr) {
					vm->RaiseException("vmod: invalid argument type");
					return nullptr;
				}

				args_ptrs.emplace_back(arg_ptr);
			}

			script_cif *cif{new script_cif{ret_ptr, std::move(args_ptrs)}};
			if(!cif->initialize(func, abi)) {
				delete cif;
				vm->RaiseException("vmod: failed to register ffi cif instance");
				return nullptr;
			}

			return cif->instance;
		}

		static dynamic_detour *script_create_detour_shared(int ret, gsdk::HSCRIPT args) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			ffi_type *ret_ptr{ffi_type_id_to_ptr(ret)};
			if(!ret_ptr) {
				vm->RaiseException("vmod: invalid return type");
				return nullptr;
			}

			std::vector<ffi_type *> args_ptrs;

			int num_args{vm->GetArrayCount(args)};
			for(int i{0}; i < num_args; ++i) {
				script_variant_t value;
				vm->GetArrayValue(args, i, &value);

				if(value.m_type != gsdk::FIELD_INTEGER) {
					vm->RaiseException("vmod: not a ffi type");
					return nullptr;
				}

				ffi_type *arg_ptr{ffi_type_id_to_ptr(value.m_int)};
				if(!arg_ptr) {
					vm->RaiseException("vmod: invalid argument type");
					return nullptr;
				}

				args_ptrs.emplace_back(arg_ptr);
			}

			dynamic_detour *det{new dynamic_detour{ret_ptr, std::move(args_ptrs)}};
			return det;
		}

		static gsdk::HSCRIPT script_create_detour_member(generic_mfp_t old_func, gsdk::HSCRIPT new_func, ffi_abi abi, int ret, gsdk::HSCRIPT args) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			dynamic_detour *det{script_create_detour_shared(ret, args)};
			if(!det) {
				return nullptr;
			}

			if(!det->initialize(old_func, new_func, abi)) {
				delete det;
				vm->RaiseException("vmod: failed to register detour instance");
				return nullptr;
			}

			return det->instance;
		}

		static gsdk::HSCRIPT script_create_detour_static(generic_func_t old_func, gsdk::HSCRIPT new_func, ffi_abi abi, int ret, gsdk::HSCRIPT args) noexcept
		{
			gsdk::IScriptVM *vm{vmod.vm()};

			dynamic_detour *det{script_create_detour_shared(ret, args)};
			if(!det) {
				return nullptr;
			}

			if(!det->initialize(old_func, new_func, abi)) {
				delete det;
				vm->RaiseException("vmod: failed to register detour instance");
				return nullptr;
			}

			return det->instance;
		}

		bool Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value) override;

		gsdk::HSCRIPT scope{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT types_table{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT abi_table{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		gsdk::CSquirrelMetamethodDelegateImpl *get_impl{nullptr};
	};

	static class ffi_singleton ffi_singleton;

	bool ffi_singleton::Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value)
	{
		using namespace std::literals::string_view_literals;

		return vmod.vm()->GetValue(instance, name.c_str(), &value);
	}

	static class_desc_t<class ffi_singleton> ffi_singleton_desc{"__vmod_ffi_singleton_class"};

	bool ffi_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		ffi_singleton_desc.func(&ffi_singleton::script_create_cif, "__script_create_cif"sv, "cif"sv);
		ffi_singleton_desc.func(&ffi_singleton::script_create_detour_static, "__script_create_detour_static"sv, "detour_static"sv);
		ffi_singleton_desc.func(&ffi_singleton::script_create_detour_member, "__script_create_detour_member"sv, "detour_member"sv);

		if(!vm->RegisterClass(&ffi_singleton_desc)) {
			error("vmod: failed to register ffi script class\n"sv);
			return false;
		}

		instance = vm->RegisterInstance(&ffi_singleton_desc, &::vmod::ffi_singleton);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create ffi instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(instance, "__vmod_ffi_singleton");

		scope = vm->CreateScope("__vmod_ffi_scope", nullptr);
		if(!scope || scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create ffi scope\n"sv);
			return false;
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(!vm->SetValue(vmod_scope, "ffi", scope)) {
			error("vmod: failed to set ffi instance value\n"sv);
			return false;
		}

		get_impl = vm->MakeSquirrelMetamethod_Get(vmod_scope, "ffi", &static_cast<gsdk::ISquirrelMetamethodDelegate &>(::vmod::ffi_singleton), false);
		if(!get_impl) {
			error("vmod: failed to create ffi _get metamethod\n"sv);
			return false;
		}

		types_table = vm->CreateTable();
		if(!types_table || types_table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create ffi types table\n"sv);
			return false;
		}

		abi_table = vm->CreateTable();
		if(!abi_table || abi_table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create ffi abi table\n"sv);
			return false;
		}

		if(!vm->SetValue(scope, "types", types_table)) {
			error("vmod: failed to set ffi types table value\n"sv);
			return false;
		}

		//TODO!!!! make these instances and add get_alignment/size/id
		{
			if(!vm->SetValue(types_table, "void", script_variant_t{FFI_TYPE_VOID})) {
				error("vmod: failed to set ffi types void value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "int", script_variant_t{FFI_TYPE_INT})) {
				error("vmod: failed to set ffi types int value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "float", script_variant_t{FFI_TYPE_FLOAT})) {
				error("vmod: failed to set ffi types float value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "double", script_variant_t{FFI_TYPE_DOUBLE})) {
				error("vmod: failed to set ffi types double value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "long_double", script_variant_t{FFI_TYPE_LONGDOUBLE})) {
				error("vmod: failed to set ffi types long double value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "uint8", script_variant_t{FFI_TYPE_UINT8})) {
				error("vmod: failed to set ffi types uint8 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "int8", script_variant_t{FFI_TYPE_SINT8})) {
				error("vmod: failed to set ffi types int8 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "uint16", script_variant_t{FFI_TYPE_UINT16})) {
				error("vmod: failed to set ffi types uint16 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "int16", script_variant_t{FFI_TYPE_SINT16})) {
				error("vmod: failed to set ffi types int16 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "uint32", script_variant_t{FFI_TYPE_UINT32})) {
				error("vmod: failed to set ffi types uint32 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "int32", script_variant_t{FFI_TYPE_SINT32})) {
				error("vmod: failed to set ffi types int32 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "uint64", script_variant_t{FFI_TYPE_UINT64})) {
				error("vmod: failed to set ffi types uint64 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "int64", script_variant_t{FFI_TYPE_SINT64})) {
				error("vmod: failed to set ffi types int64 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "ptr", script_variant_t{FFI_TYPE_POINTER})) {
				error("vmod: failed to set ffi types ptr value\n"sv);
				return false;
			}
		}

		if(!vm->SetValue(scope, "abi", abi_table)) {
			error("vmod: failed to set ffi abi table value\n"sv);
			return false;
		}

		{
			if(!vm->SetValue(abi_table, "sysv", script_variant_t{FFI_SYSV})) {
				error("vmod: failed to set ffi abi sysv value\n"sv);
				return false;
			}

			if(!vm->SetValue(abi_table, "cdecl_ms", script_variant_t{FFI_MS_CDECL})) {
				error("vmod: failed to set ffi abi cdecl_ms value\n"sv);
				return false;
			}

			if(!vm->SetValue(abi_table, "thiscall", script_variant_t{FFI_THISCALL})) {
				error("vmod: failed to set ffi abi thiscall value\n"sv);
				return false;
			}

			if(!vm->SetValue(abi_table, "fastcall", script_variant_t{FFI_FASTCALL})) {
				error("vmod: failed to set ffi abi fastcall value\n"sv);
				return false;
			}

			if(!vm->SetValue(abi_table, "stdcall", script_variant_t{FFI_STDCALL})) {
				error("vmod: failed to set ffi abi stdcall value\n"sv);
				return false;
			}

			if(!vm->SetValue(abi_table, "current", script_variant_t{FFI_DEFAULT_ABI})) {
				error("vmod: failed to set ffi abi current value\n"sv);
				return false;
			}
		}

		if(!script_cif::bindings()) {
			return false;
		}

		if(!dynamic_detour::bindings()) {
			return false;
		}

		return true;
	}

	void ffi_singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(types_table && types_table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(types_table);
		}

		if(vm->ValueExists(scope, "types")) {
			vm->ClearValue(scope, "types");
		}

		if(abi_table && abi_table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(abi_table);
		}

		if(vm->ValueExists(scope, "abi")) {
			vm->ClearValue(scope, "abi");
		}

		if(get_impl) {
			vm->DestroySquirrelMetamethod_Get(get_impl);
		}

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance);
		}

		if(scope && scope != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScope(scope);
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(vm->ValueExists(vmod_scope, "ffi")) {
			vm->ClearValue(vmod_scope, "ffi");
		}

		dynamic_detour::unbindings();

		script_cif::unbindings();
	}

	bool ffi_bindings() noexcept
	{
		if(!memory_singleton.bindings()) {
			return false;
		}

		if(!ffi_singleton.bindings()) {
			return false;
		}

		return true;
	}

	void ffi_unbindings() noexcept
	{
		memory_singleton.unbindings();

		ffi_singleton.unbindings();
	}

	static class_desc_t<class dynamic_detour> detour_desc{"__vmod_detour_class"};

	bool dynamic_detour::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		detour_desc.func(&dynamic_detour::script_call, "__script_call"sv, "call"sv);
		detour_desc.func(&dynamic_detour::script_enable, "__script_enable"sv, "enable"sv);
		detour_desc.func(&dynamic_detour::script_disable, "__script_disable"sv, "disable"sv);
		detour_desc.func(&dynamic_detour::script_delete, "__script_delete"sv, "free"sv);
		detour_desc.dtor();

		if(!vm->RegisterClass(&detour_desc)) {
			error("vmod: failed to register detour script class\n"sv);
			return false;
		}

		return true;
	}

	void dynamic_detour::unbindings() noexcept
	{
		
	}

	dynamic_detour::~dynamic_detour() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(new_func && new_func != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseFunction(new_func);
		}

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance);
		}

		if(old_func) {
			disable();
		}

		if(closure) {
			ffi_closure_free(closure);
		}
	}

	script_variant_t dynamic_detour::script_call(const script_variant_t *va_args, std::size_t num_args, ...) noexcept
	{
		if(num_args != arg_type_ptrs.size()) {
			vmod.vm()->RaiseException("wrong number of parameters");
			return {};
		}

		for(std::size_t i{0}; i < num_args; ++i) {
			ffi_type *arg_type{arg_type_ptrs[i]};
			const script_variant_t &arg_var{va_args[i]};
			std::unique_ptr<unsigned char[]> &arg_ptr{args_storage[i]};

			script_var_to_ffi_ptr(arg_type, reinterpret_cast<void *>(arg_ptr.get()), arg_var);
		}

		{
			scope_enable sce{*this};
			ffi_call(&cif_, reinterpret_cast<void(*)()>(old_func), reinterpret_cast<void *>(ret_storage.get()), const_cast<void **>(args_storage_ptrs.data()));
		}

		script_variant_t ret_var;
		ffi_ptr_to_script_var(ret_type_ptr, reinterpret_cast<void *>(ret_storage.get()), ret_var);

		return ret_var;
	}

	void dynamic_detour::closure_binding(ffi_cif *cif_, void *ret, void *args[], void *userptr) noexcept
	{
		std::vector<script_variant_t> sargs;

		dynamic_detour *det{reinterpret_cast<dynamic_detour *>(userptr)};

		sargs.emplace_back(det->instance);

		for(std::size_t i{0}; i < cif_->nargs; ++i) {
			ffi_type *arg_type_ptr{cif_->arg_types[i]};
			script_variant_t &arg_var{sargs.emplace_back()};

			ffi_ptr_to_script_var(arg_type_ptr, args[i], arg_var);
		}

		gsdk::IScriptVM *vm{vmod.vm()};

		gsdk::HSCRIPT scope{nullptr};

		script_variant_t ret_var;
		if(vm->ExecuteFunction(det->new_func, sargs.data(), static_cast<int>(sargs.size()), ((det->ret_type_ptr == &ffi_type_void) ? nullptr : &ret_var), scope, true) == gsdk::SCRIPT_ERROR) {
			return;
		}

		if(det->ret_type_ptr != &ffi_type_void) {
			script_var_to_ffi_ptr(det->ret_type_ptr, ret, ret_var);
		}
	}

	bool dynamic_detour::initialize_shared(gsdk::HSCRIPT new_func_, ffi_abi abi) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		closure = static_cast<ffi_closure *>(ffi_closure_alloc(sizeof(ffi_closure), reinterpret_cast<void **>(&closure_func)));
		if(!closure) {
			vm->RaiseException("vmod:: failed to allocate detour closure");
			return false;
		}

		if(ffi_prep_cif(&cif_, abi, arg_type_ptrs.size(), ret_type_ptr, const_cast<ffi_type **>(arg_type_ptrs.data())) != FFI_OK) {
			vm->RaiseException("vmod:: failed to create detour cif");
			return false;
		}

		if(ffi_prep_closure_loc(closure, &cif_, closure_binding, this, reinterpret_cast<void *>(closure_func)) != FFI_OK) {
			vm->RaiseException("vmod:: failed to prepare detour closure");
			return false;
		}

		instance = vm->RegisterInstance(&detour_desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: failed to register cif instance");
			return false;
		}

		//vm->SetInstanceUniqeId

		for(ffi_type *arg_type_ptr : arg_type_ptrs) {
			std::unique_ptr<unsigned char[]> &arg_ptr{args_storage.emplace_back()};
			arg_ptr.reset(reinterpret_cast<unsigned char *>(std::aligned_alloc(arg_type_ptr->alignment, arg_type_ptr->size)));
		}

		for(std::unique_ptr<unsigned char[]> &ptr : args_storage) {
			args_storage_ptrs.emplace_back(ptr.get());
		}

		if(ret_type_ptr != &ffi_type_void) {
			ret_storage.reset(reinterpret_cast<unsigned char *>(std::aligned_alloc(ret_type_ptr->alignment, ret_type_ptr->size)));
		}

		new_func = vm->ReferenceObject(new_func_);

		return true;
	}

	bool dynamic_detour::initialize(generic_func_t old_func_, gsdk::HSCRIPT new_func_, ffi_abi abi) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!initialize_shared(new_func_, abi)) {
			return false;
		}

		old_mfp.addr = reinterpret_cast<generic_plain_mfp_t>(old_func_);
		old_mfp.adjustor = 0;

		backup_bytes();

		return true;
	}

	bool dynamic_detour::initialize(generic_mfp_t old_func_, gsdk::HSCRIPT new_func_, ffi_abi abi) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		arg_type_ptrs.insert(arg_type_ptrs.begin(), &ffi_type_pointer);

		if(!initialize_shared(new_func_, abi)) {
			return false;
		}

		old_mfp.func = old_func_;

		backup_bytes();

		return true;
	}

	void dynamic_detour::enable() noexcept
	{
		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_func)};

		bytes[0] = 0xE9;

		std::uintptr_t target{reinterpret_cast<std::uintptr_t>(closure_func) - (reinterpret_cast<std::uintptr_t>(old_func) + sizeof(old_bytes))};
		std::memcpy(bytes + 1, &target, sizeof(std::uintptr_t));
	}

	void dynamic_detour::backup_bytes() noexcept
	{
		page_info func_page{reinterpret_cast<void *>(old_func), sizeof(old_bytes)};
		func_page.protect(PROT_READ|PROT_WRITE|PROT_EXEC);

		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_func)};

		std::memcpy(old_bytes, bytes, sizeof(old_bytes));
	}
}
