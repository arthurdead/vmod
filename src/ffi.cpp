#include "ffi.hpp"
#include "gsdk/tier1/utlstring.hpp"

namespace vmod
{
	memory_block::~memory_block() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(ptr) {
			if(dtor_func && dtor_func != gsdk::INVALID_HSCRIPT) {
				script_variant_t args{instance};
				vm->ExecuteFunction(dtor_func, &args, 1, nullptr, nullptr, true);
			}
		}

		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(instance);
		}

		if(ptr) {
			free(ptr);
		}
	}

	gsdk::HSCRIPT memory_block::script_allocate(std::size_t size) noexcept
	{
		memory_block *block{new memory_block{size}};

		if(!block->register_instance()) {
			return nullptr;
		}

		return block->instance;
	}

	gsdk::HSCRIPT memory_block::script_allocate_aligned(std::size_t align, std::size_t size) noexcept
	{
		memory_block *block{new memory_block{static_cast<std::align_val_t>(align), size}};

		if(!block->register_instance()) {
			return nullptr;
		}

		return block->instance;
	}

	gsdk::HSCRIPT memory_block::script_allocate_zero(std::size_t num, std::size_t size) noexcept
	{
		memory_block *block{new memory_block{num, size}};

		if(!block->register_instance()) {
			return nullptr;
		}

		return block->instance;
	}

	static class_desc_t<memory_block> mem_block_desc{"memory_block"};

	bool memory_block::register_instance() noexcept
	{
		using namespace std::literals::string_view_literals;

		instance = vmod.vm()->RegisterInstance(&mem_block_desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to register memory block instance\n"sv);
			return false;
		}

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

		script_cif(ffi_abi abi, ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: arg_type_ptrs{std::move(args)}, ret_type_ptr{ret}
		{
			ffi_prep_cif(&cif_, abi, arg_type_ptrs.size(), ret_type_ptr, const_cast<ffi_type **>(arg_type_ptrs.data()));

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
		}

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
				switch(arg_type->type) {
					case FFI_TYPE_INT:
					*reinterpret_cast<int *>(arg_ptr.get()) = arg_var.get<int>();
					break;
					case FFI_TYPE_FLOAT:
					*reinterpret_cast<float *>(arg_ptr.get()) = arg_var.get<float>();
					break;
					case FFI_TYPE_DOUBLE:
					*reinterpret_cast<double *>(arg_ptr.get()) = arg_var.get<double>();
					break;
					case FFI_TYPE_LONGDOUBLE:
					*reinterpret_cast<long double *>(arg_ptr.get()) = arg_var.get<long double>();
					break;
					case FFI_TYPE_UINT8:
					*reinterpret_cast<unsigned char *>(arg_ptr.get()) = static_cast<unsigned char>(arg_var.get<unsigned short>());
					break;
					case FFI_TYPE_SINT8:
					*reinterpret_cast<signed char *>(arg_ptr.get()) = static_cast<signed char>(arg_var.get<short>());
					break;
					case FFI_TYPE_UINT16:
					*reinterpret_cast<unsigned short *>(arg_ptr.get()) = arg_var.get<unsigned short>();
					break;
					case FFI_TYPE_SINT16:
					*reinterpret_cast<short *>(arg_ptr.get()) = arg_var.get<short>();
					break;
					case FFI_TYPE_UINT32:
					*reinterpret_cast<unsigned int *>(arg_ptr.get()) = arg_var.get<unsigned int>();
					break;
					case FFI_TYPE_SINT32:
					*reinterpret_cast<int *>(arg_ptr.get()) = arg_var.get<int>();
					break;
					case FFI_TYPE_UINT64:
					*reinterpret_cast<unsigned long long *>(arg_ptr.get()) = arg_var.get<unsigned long long>();
					break;
					case FFI_TYPE_SINT64:
					*reinterpret_cast<long long *>(arg_ptr.get()) = arg_var.get<long long>();
					break;
					case FFI_TYPE_POINTER:
					*reinterpret_cast<void **>(arg_ptr.get()) = arg_var.get<void *>();
					break;
				}
			}

			ffi_call(&cif_, reinterpret_cast<void(*)()>(func), reinterpret_cast<void *>(ret_storage.get()), const_cast<void **>(args_storage_ptrs.data()));

			script_variant_t ret_var;
			switch(ret_type_ptr->type) {
				case FFI_TYPE_INT:
				ret_var.assign<int>(*reinterpret_cast<int *>(ret_storage.get()));
				break;
				case FFI_TYPE_FLOAT:
				ret_var.assign<float>(*reinterpret_cast<float *>(ret_storage.get()));
				break;
				case FFI_TYPE_DOUBLE:
				ret_var.assign<double>(*reinterpret_cast<double *>(ret_storage.get()));
				break;
				case FFI_TYPE_LONGDOUBLE:
				ret_var.assign<long double>(*reinterpret_cast<long double *>(ret_storage.get()));
				break;
				case FFI_TYPE_UINT8:
				ret_var.assign<unsigned short>(*reinterpret_cast<unsigned char *>(ret_storage.get()));
				break;
				case FFI_TYPE_SINT8:
				ret_var.assign<short>(*reinterpret_cast<signed char *>(ret_storage.get()));
				break;
				case FFI_TYPE_UINT16:
				ret_var.assign<unsigned short>(*reinterpret_cast<unsigned short *>(ret_storage.get()));
				break;
				case FFI_TYPE_SINT16:
				ret_var.assign<short>(*reinterpret_cast<short *>(ret_storage.get()));
				break;
				case FFI_TYPE_UINT32:
				ret_var.assign<unsigned int>(*reinterpret_cast<unsigned int *>(ret_storage.get()));
				break;
				case FFI_TYPE_SINT32:
				ret_var.assign<int>(*reinterpret_cast<int *>(ret_storage.get()));
				break;
				case FFI_TYPE_UINT64:
				ret_var.assign<unsigned long long>(*reinterpret_cast<unsigned long long *>(ret_storage.get()));
				break;
				case FFI_TYPE_SINT64:
				ret_var.assign<long long>(*reinterpret_cast<long long *>(ret_storage.get()));
				break;
				case FFI_TYPE_POINTER:
				ret_var.assign<void *>(*reinterpret_cast<void **>(ret_storage.get()));
				break;
			}

			return ret_var;
		}

		bool register_instance() noexcept;

		ffi_cif cif_;

		std::vector<ffi_type *> arg_type_ptrs;
		ffi_type *ret_type_ptr;

		std::unique_ptr<unsigned char[]> ret_storage;
		std::vector<std::unique_ptr<unsigned char[]>> args_storage;
		std::vector<void *> args_storage_ptrs;

		generic_func_t func;
		gsdk::HSCRIPT instance;
	};

	static class_desc_t<script_cif> cif_desc{"cif"};

	bool script_cif::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		cif_desc.func(&script_cif::script_call, "script_call"sv, "call"sv);
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

	bool script_cif::register_instance() noexcept
	{
		using namespace std::literals::string_view_literals;

		instance = vmod.vm()->RegisterInstance(&cif_desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to register cif instance\n"sv);
			return false;
		}

		return true;
	}

	static class ffi_singleton final : public gsdk::ISquirrelMetamethodDelegate
	{
	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

	private:
		static ffi_type *type_id_to_ptr(int id) noexcept
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

		static gsdk::HSCRIPT script_create_cif(generic_func_t func, ffi_abi abi, int ret, gsdk::HSCRIPT args) noexcept
		{
			ffi_type *ret_ptr{type_id_to_ptr(ret)};
			if(!ret_ptr) {
				return nullptr;
			}

			gsdk::IScriptVM *vm{vmod.vm()};

			std::vector<ffi_type *> args_ptrs;

			int num_args{vm->GetArrayCount(args)};
			for(int i{0}; i < num_args; ++i) {
				script_variant_t value;
				vm->GetArrayValue(args, i, &value);

				if(value.m_type != gsdk::FIELD_INTEGER) {
					return nullptr;
				}

				ffi_type *arg_ptr{type_id_to_ptr(value.m_int)};
				if(!arg_ptr) {
					return nullptr;
				}

				args_ptrs.emplace_back(arg_ptr);
			}

			class script_cif *cif{new script_cif{abi, ret_ptr, std::move(args_ptrs)}};
			if(!cif->register_instance()) {
				delete cif;
				return nullptr;
			}

			cif->func = func;

			return cif->instance;
		}

		bool Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value) override;

		static inline gsdk::HSCRIPT scope{gsdk::INVALID_HSCRIPT};
		static inline gsdk::HSCRIPT types_table{gsdk::INVALID_HSCRIPT};
		static inline gsdk::HSCRIPT abi_table{gsdk::INVALID_HSCRIPT};
		static inline gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		static inline gsdk::CSquirrelMetamethodDelegateImpl *get_impl{nullptr};
	} ffi_singleton;

	bool ffi_singleton::Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value)
	{
		using namespace std::literals::string_view_literals;

		return vmod.vm()->GetValue(instance, name.c_str(), &value);
	}

	static class_desc_t<class ffi_singleton> ffi_singleton_desc{"ffi"};

	bool memory_block::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		mem_block_desc.func(&memory_block::script_allocate, "script_allocate"sv, "allocate"sv);
		mem_block_desc.func(&memory_block::script_allocate_aligned, "script_allocate_aligned"sv, "allocate_aligned"sv);
		mem_block_desc.func(&memory_block::script_allocate_zero, "script_allocate_zero"sv, "allocate_zero"sv);
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

	bool ffi_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		ffi_singleton_desc.func(&ffi_singleton::script_create_cif, "script_create_cif"sv, "cif"sv);

		if(!vm->RegisterClass(&ffi_singleton_desc)) {
			error("vmod: failed to register ffi script class\n"sv);
			return false;
		}

		instance = vm->RegisterInstance(&ffi_singleton_desc, &::vmod::ffi_singleton);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create ffi instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(instance, "ffi_singleton");

		scope = vm->CreateScope("ffi", nullptr);
		if(!scope || scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create ffi scope\n"sv);
			return false;
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(!vm->SetValue(vmod_scope, "ffi", scope)) {
			error("vmod: failed to set ffi instance value\n"sv);
			return false;
		}

		get_impl = vm->MakeSquirrelMetamethod_Get(vmod_scope, "ffi", &::vmod::ffi_singleton, false);
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

		return true;
	}

	void ffi_singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(types_table && types_table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(types_table);
		}

		if(vm->ValueExists(instance, "types")) {
			vm->ClearValue(instance, "types");
		}

		if(abi_table && abi_table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(abi_table);
		}

		if(vm->ValueExists(instance, "abi")) {
			vm->ClearValue(instance, "abi");
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
	}

	bool ffi_bindings() noexcept
	{
		if(!script_cif::bindings()) {
			return false;
		}

		if(!memory_block::bindings()) {
			return false;
		}

		if(!ffi_singleton::bindings()) {
			return false;
		}

		return true;
	}

	void ffi_unbindings() noexcept
	{
		script_cif::unbindings();

		memory_block::unbindings();

		ffi_singleton::unbindings();
	}

	dynamic_detour::~dynamic_detour() noexcept
	{
		if(old_func) {
			disable();
		}

		if(closure) {
			ffi_closure_free(closure);
		}
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
