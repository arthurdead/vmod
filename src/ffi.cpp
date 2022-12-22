#include "ffi.hpp"
#include "gsdk/tier1/utlstring.hpp"
#include "vmod.hpp"

namespace vmod
{
	static ffi_type *ffi_type_vector_elements[3]{
		&ffi_type_float, &ffi_type_float, &ffi_type_float
	};
	static ffi_type *ffi_type_qangle_elements[3]{
		&ffi_type_float, &ffi_type_float, &ffi_type_float
	};
	static ffi_type *ffi_type_color32_elements[4]{
		&ffi_type_uchar, &ffi_type_uchar, &ffi_type_uchar, &ffi_type_uchar
	};
	static ffi_type *ffi_type_ehandle_elements[1]{
		&ffi_type_ulong
	};

	ffi_type ffi_type_vector{
		sizeof(gsdk::Vector),
		alignof(gsdk::Vector),
		FFI_TYPE_STRUCT,
		ffi_type_vector_elements
	};
	ffi_type ffi_type_qangle{
		sizeof(gsdk::QAngle),
		alignof(gsdk::QAngle),
		FFI_TYPE_STRUCT,
		ffi_type_qangle_elements
	};
	ffi_type ffi_type_color32{
		sizeof(gsdk::Color),
		alignof(gsdk::Color),
		FFI_TYPE_STRUCT,
		ffi_type_color32_elements
	};
	ffi_type ffi_type_ehandle{
		sizeof(gsdk::EHANDLE),
		alignof(gsdk::EHANDLE),
		FFI_TYPE_STRUCT,
		ffi_type_ehandle_elements
	};
	ffi_type ffi_type_bool{
		sizeof(bool),
		alignof(bool),
		FFI_TYPE_UINT8,
		nullptr
	};
	ffi_type ffi_type_cstr{
		sizeof(const char *),
		alignof(const char *),
		FFI_TYPE_POINTER,
		nullptr
	};

	void script_var_to_ptr(ffi_type *type_ptr, void *arg_ptr, const script_variant_t &arg_var) noexcept
	{
		switch(type_ptr->type) {
			case FFI_TYPE_INT:
			*static_cast<int *>(arg_ptr) = arg_var.get<int>();
			break;
			case FFI_TYPE_FLOAT:
			*static_cast<float *>(arg_ptr) = arg_var.get<float>();
			break;
			case FFI_TYPE_DOUBLE:
			*static_cast<double *>(arg_ptr) = arg_var.get<double>();
			break;
			case FFI_TYPE_LONGDOUBLE:
			*static_cast<long double *>(arg_ptr) = arg_var.get<long double>();
			break;
			case FFI_TYPE_UINT8:
			*static_cast<unsigned char *>(arg_ptr) = static_cast<unsigned char>(arg_var.get<unsigned short>());
			break;
			case FFI_TYPE_SINT8:
			*static_cast<signed char *>(arg_ptr) = static_cast<signed char>(arg_var.get<short>());
			break;
			case FFI_TYPE_UINT16:
			*static_cast<unsigned short *>(arg_ptr) = arg_var.get<unsigned short>();
			break;
			case FFI_TYPE_SINT16:
			*static_cast<short *>(arg_ptr) = arg_var.get<short>();
			break;
			case FFI_TYPE_UINT32:
			*static_cast<unsigned int *>(arg_ptr) = arg_var.get<unsigned int>();
			break;
			case FFI_TYPE_SINT32:
			*static_cast<int *>(arg_ptr) = arg_var.get<int>();
			break;
			case FFI_TYPE_UINT64:
			*static_cast<unsigned long long *>(arg_ptr) = arg_var.get<unsigned long long>();
			break;
			case FFI_TYPE_SINT64:
			*static_cast<long long *>(arg_ptr) = arg_var.get<long long>();
			break;
			case FFI_TYPE_POINTER:
			*static_cast<void **>(arg_ptr) = arg_var.get<void *>();
			break;
			case FFI_TYPE_STRUCT: {
				debugtrap();
			} break;
		}
	}

	void ptr_to_script_var(ffi_type *type_ptr, void *arg_ptr, script_variant_t &arg_var) noexcept
	{
		switch(type_ptr->type) {
			case FFI_TYPE_INT:
			arg_var.assign<int>(*static_cast<int *>(arg_ptr));
			break;
			case FFI_TYPE_FLOAT:
			arg_var.assign<float>(*static_cast<float *>(arg_ptr));
			break;
			case FFI_TYPE_DOUBLE:
			arg_var.assign<double>(*static_cast<double *>(arg_ptr));
			break;
			case FFI_TYPE_LONGDOUBLE:
			arg_var.assign<long double>(*static_cast<long double *>(arg_ptr));
			break;
			case FFI_TYPE_UINT8:
			arg_var.assign<unsigned short>(*static_cast<unsigned char *>(arg_ptr));
			break;
			case FFI_TYPE_SINT8:
			arg_var.assign<short>(*static_cast<signed char *>(arg_ptr));
			break;
			case FFI_TYPE_UINT16:
			arg_var.assign<unsigned short>(*static_cast<unsigned short *>(arg_ptr));
			break;
			case FFI_TYPE_SINT16:
			arg_var.assign<short>(*static_cast<short *>(arg_ptr));
			break;
			case FFI_TYPE_UINT32:
			arg_var.assign<unsigned int>(*static_cast<unsigned int *>(arg_ptr));
			break;
			case FFI_TYPE_SINT32:
			arg_var.assign<int>(*static_cast<int *>(arg_ptr));
			break;
			case FFI_TYPE_UINT64:
			arg_var.assign<unsigned long long>(*static_cast<unsigned long long *>(arg_ptr));
			break;
			case FFI_TYPE_SINT64:
			arg_var.assign<long long>(*static_cast<long long *>(arg_ptr));
			break;
			case FFI_TYPE_POINTER:
			arg_var.assign<void *>(*static_cast<void **>(arg_ptr));
			break;
			case FFI_TYPE_STRUCT: {
				debugtrap();
			} break;
		}
	}

	ffi_type *ffi_type_id_to_ptr(int id) noexcept
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
			case FFI_TYPE_STRUCT: return nullptr;
			default: return nullptr;
		}
	}

	void memory_singleton::unbindings() noexcept
	{
		memory_block::unbindings();

		gsdk::IScriptVM *vm{vmod.vm()};

		types.clear();

		if(get_impl) {
			vm->DestroySquirrelMetamethod_Get(get_impl);
		}

		if(vs_instance_ && vs_instance_ != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(vs_instance_);
		}

		if(types_table && types_table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(types_table);
		}

		if(vm->ValueExists(scope, "types")) {
			vm->ClearValue(scope, "types");
		}

		if(scope && scope != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseScope(scope);
		}

		gsdk::HSCRIPT vmod_scope{vmod.scope()};
		if(vm->ValueExists(vmod_scope, "mem")) {
			vm->ClearValue(vmod_scope, "mem");
		}
	}

	gsdk::HSCRIPT memory_singleton::script_allocate(std::size_t size) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size");
			return nullptr;
		}

		memory_block *block{new memory_block{size}};

		if(!block->initialize()) {
			delete block;
			return nullptr;
		}

		block->set_plugin();

		return block->instance;
	}

	gsdk::HSCRIPT memory_singleton::script_allocate_aligned(std::size_t size, std::size_t align) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size");
			return nullptr;
		}

		if(align == 0 || align == static_cast<std::size_t>(-1) || (align % 2) != 0) {
			vm->RaiseException("vmod: invalid align");
			return nullptr;
		}

		memory_block *block{new memory_block{static_cast<std::align_val_t>(align), size}};

		if(!block->initialize()) {
			delete block;
			return nullptr;
		}

		block->set_plugin();

		return block->instance;
	}

	gsdk::HSCRIPT memory_singleton::script_allocate_type(gsdk::HSCRIPT type) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!type || type == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid type");
			return nullptr;
		}

		if(!vm->IsTable(type)) {
			vm->RaiseException("vmod: invalid type");
			return nullptr;
		}

		script_variant_t type_var;
		if(!vm->GetValue(type, "__internal_ptr__", &type_var)) {
			vm->RaiseException("vmod: invalid type");
			return nullptr;
		}

		ffi_type *type_ptr{type_var.get<ffi_type *>()};

		memory_block *block{new memory_block{static_cast<std::align_val_t>(type_ptr->alignment), type_ptr->size}};

		if(!block->initialize()) {
			delete block;
			return nullptr;
		}

		block->set_plugin();

		return block->instance;
	}

	gsdk::HSCRIPT memory_singleton::script_allocate_zero(std::size_t num, std::size_t size) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size");
			return nullptr;
		}

		if(num == 0 || num == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid num");
			return nullptr;
		}

		memory_block *block{new memory_block{num, size}};

		if(!block->initialize()) {
			delete block;
			return nullptr;
		}

		block->set_plugin();

		return block->instance;
	}

	script_variant_t memory_singleton::script_read(void *ptr, gsdk::HSCRIPT type) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!ptr) {
			vm->RaiseException("vmod: invalid ptr");
			return {};
		}

		if(!type || type == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid type");
			return {};
		}

		if(!vm->IsTable(type)) {
			vm->RaiseException("vmod: invalid type");
			return {};
		}

		script_variant_t type_id;
		if(!vm->GetValue(type, "__internal_ptr__", &type_id)) {
			vm->RaiseException("vmod: invalid type");
			return {};
		}

		ffi_type *type_ptr{type_id.get<ffi_type *>()};

		script_variant_t ret_var;
		ptr_to_script_var(type_ptr, ptr, ret_var);
		return ret_var;
	}

	void memory_singleton::script_write(void *ptr, gsdk::HSCRIPT type, script_variant_t arg_var) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!ptr) {
			vm->RaiseException("vmod: invalid ptr");
			return;
		}

		if(!type || type == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid type");
			return;
		}

		if(!vm->IsTable(type)) {
			vm->RaiseException("vmod: invalid type");
			return;
		}

		script_variant_t type_id;
		if(!vm->GetValue(type, "__internal_ptr__", &type_id)) {
			vm->RaiseException("vmod: invalid type");
			return;
		}

		ffi_type *type_ptr{type_id.get<ffi_type *>()};

		script_var_to_ptr(type_ptr, ptr, arg_var);
	}

	bool memory_singleton::register_type(ffi_type *type_ptr, std::string_view name) noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		gsdk::HSCRIPT type_table{vm->CreateTable()};
		if(!type_table || type_table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create type '%s' table\n", name.data());
			return false;
		}

		mem_type type;
		type.table = type_table;
		type.type_ptr = type_ptr;
		type.name = name;
		types.emplace_back(std::move(type));

		if(!vm->SetValue(type_table, "size", script_variant_t{type_ptr->size})) {
			error("vmod: failed to set type '%s' size value\n", name.data());
			return false;
		}

		if(!vm->SetValue(type_table, "alignment", script_variant_t{type_ptr->alignment})) {
			error("vmod: failed to set type '%s' alignment value\n", name.data());
			return false;
		}

		if(!vm->SetValue(type_table, "id", script_variant_t{type_ptr->type})) {
			error("vmod: failed to set type '%s' id value\n", name.data());
			return false;
		}

		if(!vm->SetValue(type_table, "__internal_ptr__", script_variant_t{type_ptr})) {
			error("vmod: failed to set type '%s' internal ptr value\n", name.data());
			return false;
		}

		if(!vm->SetValue(type_table, "name", script_variant_t{name})) {
			error("vmod: failed to set type '%s' name value\n", name.data());
			return false;
		}

		if(!vm->SetValue(types_table, name.data(), type_table)) {
			error("vmod: failed to set type '%s' name value\n", name.data());
			return false;
		}

		return true;
	}

	class memory_singleton memory_singleton;

	memory_singleton::mem_type::~mem_type() noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!name.empty()) {
			if(vm->ValueExists(::vmod::memory_singleton.types_table, name.c_str())) {
				vm->ClearValue(::vmod::memory_singleton.types_table, name.c_str());
			}
		}

		if(table && table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(table);
		}
	}

	bool memory_singleton::Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value)
	{
		using namespace std::literals::string_view_literals;

		return vmod.vm()->GetValue(vs_instance_, name.c_str(), &value);
	}

	singleton_class_desc_t<class memory_singleton> memory_desc{"__vmod_memory_singleton_class"};

	inline class memory_singleton &memory_singleton::instance() noexcept
	{ return ::vmod::memory_singleton; }

	class_desc_t<memory_block> mem_block_desc{"__vmod_memory_block_class"};

	bool memory_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		memory_desc.func(&memory_singleton::script_allocate, "script_allocate"sv, "allocate"sv);
		memory_desc.func(&memory_singleton::script_allocate_aligned, "script_allocate_aligned"sv, "allocate_aligned"sv);
		memory_desc.func(&memory_singleton::script_allocate_zero, "script_allocate_zero"sv, "allocate_zero"sv);
		memory_desc.func(&memory_singleton::script_allocate_type, "script_allocate_type"sv, "allocate_type"sv);
		memory_desc.func(&memory_singleton::script_read, "script_read"sv, "read"sv);
		memory_desc.func(&memory_singleton::script_write, "script_write"sv, "write"sv);
		memory_desc.func(&memory_singleton::script_add, "script_add"sv, "add"sv);
		memory_desc.func(&memory_singleton::script_sub, "script_sub"sv, "sub"sv);
		memory_desc.func(&memory_singleton::script_get_vtable, "script_get_vtable"sv, "get_vtable"sv);
		memory_desc.func(&memory_singleton::script_get_vfunc, "script_get_vfunc"sv, "get_vfunc"sv);

		if(!vm->RegisterClass(&memory_desc)) {
			error("vmod: failed to register memory singleton script class\n"sv);
			return false;
		}

		vs_instance_ = vm->RegisterInstance(&memory_desc, &::vmod::memory_singleton);
		if(!vs_instance_ || vs_instance_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create memory singleton instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(vs_instance_, "__vmod_memory_singleton");

		gsdk::HSCRIPT vmod_scope{vmod.scope()};

		scope = vm->CreateScope("__vmod_memory_scope", nullptr);
		if(!scope || scope == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create memory scope\n"sv);
			return false;
		}

		if(!vm->SetValue(vmod_scope, "mem", scope)) {
			error("vmod: failed to set memory scope value\n"sv);
			return false;
		}

		get_impl = vm->MakeSquirrelMetamethod_Get(vmod_scope, "mem", &static_cast<gsdk::ISquirrelMetamethodDelegate &>(::vmod::memory_singleton), false);
		if(!get_impl) {
			error("vmod: failed to create memory _get metamethod\n"sv);
			return false;
		}

		types_table = vm->CreateTable();
		if(!types_table || types_table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create memory types table\n"sv);
			return false;
		}

		if(!vm->SetValue(scope, "types", types_table)) {
			error("vmod: failed to set memory types table value\n"sv);
			return false;
		}

		{
			if(!register_type(&ffi_type_void, "void"sv)) {
				error("vmod: failed to register memory type void\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sint, "int"sv)) {
				error("vmod: failed to register memory type int\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uint, "uint"sv)) {
				error("vmod: failed to register memory type uint\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_schar, "char"sv)) {
				error("vmod: failed to register memory type char\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uchar, "uchar"sv)) {
				error("vmod: failed to register memory type uchar\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sshort, "short"sv)) {
				error("vmod: failed to register memory type short\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_ushort, "ushort"sv)) {
				error("vmod: failed to register memory type ushort\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_slong, "long"sv)) {
				error("vmod: failed to register memory type long\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_ulong, "ulong"sv)) {
				error("vmod: failed to register memory type ulong\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_float, "float"sv)) {
				error("vmod: failed to register memory type float\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_double, "double"sv)) {
				error("vmod: failed to register memory type double\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_longdouble, "long_double"sv)) {
				error("vmod: failed to register memory type long double\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uint8, "uint8"sv)) {
				error("vmod: failed to register memory type uint8\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sint8, "int8"sv)) {
				error("vmod: failed to register memory type int8\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uint16, "uint16"sv)) {
				error("vmod: failed to register memory type uint16\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sint16, "int16"sv)) {
				error("vmod: failed to register memory type int16\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uint32, "uint32"sv)) {
				error("vmod: failed to register memory type uint32\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sint32, "int32"sv)) {
				error("vmod: failed to register memory type int32\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uint64, "uint64"sv)) {
				error("vmod: failed to register memory type uint64\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sint64, "int64"sv)) {
				error("vmod: failed to register memory type int64\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_pointer, "ptr"sv)) {
				error("vmod: failed to register memory type ptr\n"sv);
				return false;
			}
		}

		if(!memory_block::bindings()) {
			return false;
		}

		return true;
	}

	void memory_block::script_set_dtor_func(gsdk::HSCRIPT func) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!func || func == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: null function");
			return;
		}

		dtor_func = vm->ReferenceObject(func);
	}

	void *memory_block::script_release() noexcept
	{
		unsigned char *temp_ptr{ptr};
		ptr = nullptr;
		delete this;
		return static_cast<void *>(temp_ptr);
	}

	bool memory_block::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		mem_block_desc.func(&memory_block::script_set_dtor_func, "script_set_dtor_func"sv, "hook_free"sv);
		mem_block_desc.func(&memory_block::script_release, "script_release"sv, "release"sv);
		mem_block_desc.func(&memory_block::script_ptr, "script_ptr"sv, "ptr"sv);
		mem_block_desc.func(&memory_block::script_get_size, "script_get_size"sv, "size"sv);
		mem_block_desc.dtor();
		mem_block_desc.base(plugin::owned_instance_desc);
		mem_block_desc.doc_class_name("memory_block"sv);

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

	void script_cif::script_set_func(generic_func_t func_) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!func_) {
			vm->RaiseException("vmod: null function");
			return;
		}

		func = func_;
	}

	void script_cif::script_set_mfp(generic_mfp_t func_) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!func_) {
			vm->RaiseException("vmod: null function");
			return;
		}

		mfp = func_;
	}

	script_variant_t script_cif::script_call(const script_variant_t *va_args, std::size_t num_args, ...) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!mfp) {
			vm->RaiseException("vmod: null function");
			return {};
		}

		if(!va_args || num_args != arg_type_ptrs.size()) {
			vm->RaiseException("wrong number of parameters");
			return {};
		}

		for(std::size_t i{0}; i < num_args; ++i) {
			ffi_type *arg_type{arg_type_ptrs[i]};
			const script_variant_t &arg_var{va_args[i]};
			auto &arg_ptr{args_storage[i]};

			script_var_to_ptr(arg_type, static_cast<void *>(arg_ptr.get()), arg_var);
		}

		ffi_call(&cif_, reinterpret_cast<void(*)()>(mfp.addr), static_cast<void *>(ret_storage.get()), const_cast<void **>(args_storage_ptrs.data()));

		script_variant_t ret_var;
		ptr_to_script_var(ret_type_ptr, static_cast<void *>(ret_storage.get()), ret_var);

		return ret_var;
	}

	script_cif::~script_cif() noexcept
	{
		if(instance && instance != gsdk::INVALID_HSCRIPT) {
			vmod.vm()->RemoveInstance(instance);
		}
	}

	class_desc_t<script_cif> cif_desc{"__vmod_ffi_cif_class"};

	bool script_cif::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		cif_desc.func(&script_cif::script_call, "script_call"sv, "call"sv);
		cif_desc.func(&script_cif::script_set_func, "script_set_func"sv, "set_func"sv);
		cif_desc.func(&script_cif::script_set_mfp, "script_set_mfp"sv, "set_mfp"sv);
		cif_desc.dtor();
		cif_desc.base(plugin::owned_instance_desc);
		cif_desc.doc_class_name("cif"sv);

		if(!vm->RegisterClass(&cif_desc)) {
			error("vmod: failed to register cif script class\n"sv);
			return false;
		}

		return true;
	}

	void script_cif::unbindings() noexcept
	{

	}

	bool cif::initialize(ffi_abi abi) noexcept
	{
		if(ffi_prep_cif(&cif_, abi, arg_type_ptrs.size(), ret_type_ptr, const_cast<ffi_type **>(arg_type_ptrs.data())) != FFI_OK) {
			return false;
		}

		for(ffi_type *arg_type_ptr : arg_type_ptrs) {
			auto &arg_ptr{args_storage.emplace_back()};
			arg_ptr.reset(static_cast<unsigned char *>(std::aligned_alloc(arg_type_ptr->alignment, arg_type_ptr->size)));
		}

		for(auto &ptr : args_storage) {
			args_storage_ptrs.emplace_back(ptr.get());
		}

		if(ret_type_ptr != &ffi_type_void) {
			ret_storage.reset(static_cast<unsigned char *>(std::aligned_alloc(ret_type_ptr->alignment, ret_type_ptr->size)));
		}

		return true;
	}

	cif::~cif() noexcept
	{
		
	}

	bool script_cif::initialize(ffi_abi abi) noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		if(!cif::initialize(abi)) {
			vm->RaiseException("vmod: failed to create cif");
			return false;
		}

		instance = vm->RegisterInstance(&cif_desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: failed to register cif instance");
			return false;
		}

		//vm->SetInstanceUniqeId

		return true;
	}

	bool ffi_singleton::script_create_cif_shared(std::vector<ffi_type *> &args_ptrs, gsdk::HSCRIPT args) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!args || args == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid args");
			return false;
		}

		if(!vm->IsArray(args)) {
			vm->RaiseException("vmod: invalid args");
			return false;
		}

		int num_args{vm->GetArrayCount(args)};
		for(int i{0}, it{0}; it != -1 && i < num_args; ++i) {
			script_variant_t value;
			it = vm->GetArrayValue(args, it, &value);

			ffi_type *arg_ptr{value.get<ffi_type *>()};

			args_ptrs.emplace_back(arg_ptr);
		}

		return true;
	}

	gsdk::HSCRIPT ffi_singleton::script_create_static_cif(ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!ret) {
			vm->RaiseException("vmod: invalid ret");
			return nullptr;
		}

		ffi_type *ret_ptr{ret};

		std::vector<ffi_type *> args_ptrs;
		if(!script_create_cif_shared(args_ptrs, args)) {
			return nullptr;
		}

		script_cif *cif{new script_cif{ret_ptr, std::move(args_ptrs)}};
		if(!cif->initialize(abi)) {
			delete cif;
			vm->RaiseException("vmod: failed to register ffi cif instance");
			return nullptr;
		}

		cif->set_plugin();

		return cif->instance;
	}

	gsdk::HSCRIPT ffi_singleton::script_create_member_cif(ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!ret) {
			vm->RaiseException("vmod: invalid ret");
			return nullptr;
		}

		ffi_type *ret_ptr{ret};

		std::vector<ffi_type *> args_ptrs;
		if(!script_create_cif_shared(args_ptrs, args)) {
			return nullptr;
		}

		args_ptrs.insert(args_ptrs.begin(), &ffi_type_pointer);

		script_cif *cif{new script_cif{ret_ptr, std::move(args_ptrs)}};
		if(!cif->initialize(abi)) {
			delete cif;
			vm->RaiseException("vmod: failed to register ffi cif instance");
			return nullptr;
		}

		cif->set_plugin();

		return cif->instance;
	}

	dynamic_detour *ffi_singleton::script_create_detour_shared(ffi_type *ret, gsdk::HSCRIPT args) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!ret) {
			vm->RaiseException("vmod: invalid ret");
			return nullptr;
		}

		if(!args || args == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid args");
			return nullptr;
		}

		if(!vm->IsArray(args)) {
			vm->RaiseException("vmod: invalid args");
			return nullptr;
		}

		ffi_type *ret_ptr{ret};

		std::vector<ffi_type *> args_ptrs;

		int num_args{vm->GetArrayCount(args)};
		for(int i{0}, it{0}; it != -1 && i < num_args; ++i) {
			script_variant_t value;
			it = vm->GetArrayValue(args, it, &value);

			ffi_type *arg_ptr{value.get<ffi_type *>()};

			args_ptrs.emplace_back(arg_ptr);
		}

		dynamic_detour *det{new dynamic_detour{ret_ptr, std::move(args_ptrs)}};
		return det;
	}

	gsdk::HSCRIPT ffi_singleton::script_create_detour_member(generic_mfp_t old_func, gsdk::HSCRIPT new_func, ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!old_func) {
			vm->RaiseException("vmod: null function");
			return nullptr;
		}

		if(!new_func || new_func == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: null function");
			return nullptr;
		}

		dynamic_detour *det{script_create_detour_shared(ret, args)};
		if(!det) {
			return nullptr;
		}

		if(!det->initialize(old_func, new_func, abi)) {
			delete det;
			vm->RaiseException("vmod: failed to register detour instance");
			return nullptr;
		}

		det->set_plugin();

		return det->instance;
	}

	gsdk::HSCRIPT ffi_singleton::script_create_detour_static(generic_func_t old_func, gsdk::HSCRIPT new_func, ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		if(!old_func) {
			vm->RaiseException("vmod: null function");
			return nullptr;
		}

		if(!new_func || new_func == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: null function");
			return nullptr;
		}

		dynamic_detour *det{script_create_detour_shared(ret, args)};
		if(!det) {
			return nullptr;
		}

		if(!det->initialize(old_func, new_func, abi)) {
			delete det;
			vm->RaiseException("vmod: failed to register detour instance");
			return nullptr;
		}

		det->set_plugin();

		return det->instance;
	}

	class ffi_singleton ffi_singleton;

	inline class ffi_singleton &ffi_singleton::instance() noexcept
	{ return ::vmod::ffi_singleton; }

	bool ffi_singleton::Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value)
	{
		using namespace std::literals::string_view_literals;

		return vmod.vm()->GetValue(vs_instance_, name.c_str(), &value);
	}

	singleton_class_desc_t<class ffi_singleton> ffi_singleton_desc{"__vmod_ffi_singleton_class"};

	bool ffi_singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		ffi_singleton_desc.func(&ffi_singleton::script_create_static_cif, "script_create_static_cif"sv, "cif_static"sv);
		ffi_singleton_desc.func(&ffi_singleton::script_create_member_cif, "script_create_member_cif"sv, "cif_member"sv);
		ffi_singleton_desc.func(&ffi_singleton::script_create_detour_static, "script_create_detour_static"sv, "detour_static"sv);
		ffi_singleton_desc.func(&ffi_singleton::script_create_detour_member, "script_create_detour_member"sv, "detour_member"sv);

		if(!vm->RegisterClass(&ffi_singleton_desc)) {
			error("vmod: failed to register ffi script class\n"sv);
			return false;
		}

		vs_instance_ = vm->RegisterInstance(&ffi_singleton_desc, &::vmod::ffi_singleton);
		if(!vs_instance_ || vs_instance_ == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create ffi instance\n"sv);
			return false;
		}

		vm->SetInstanceUniqeId(vs_instance_, "__vmod_ffi_singleton");

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

		{
			if(!vm->SetValue(types_table, "void", script_variant_t{&ffi_type_void})) {
				error("vmod: failed to set ffi types void value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "int", script_variant_t{&ffi_type_sint})) {
				error("vmod: failed to set ffi types int value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "uint", script_variant_t{&ffi_type_uint})) {
				error("vmod: failed to set ffi types uint value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "char", script_variant_t{&ffi_type_schar})) {
				error("vmod: failed to set ffi types char value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "uchar", script_variant_t{&ffi_type_uchar})) {
				error("vmod: failed to set ffi types uchar value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "short", script_variant_t{&ffi_type_sshort})) {
				error("vmod: failed to set ffi types short value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "ushort", script_variant_t{&ffi_type_ushort})) {
				error("vmod: failed to set ffi types ushort value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "long", script_variant_t{&ffi_type_slong})) {
				error("vmod: failed to set ffi types long value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "ulong", script_variant_t{&ffi_type_ulong})) {
				error("vmod: failed to set ffi types ulong value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "float", script_variant_t{&ffi_type_float})) {
				error("vmod: failed to set ffi types float value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "double", script_variant_t{&ffi_type_double})) {
				error("vmod: failed to set ffi types double value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "long_double", script_variant_t{&ffi_type_longdouble})) {
				error("vmod: failed to set ffi types long double value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "uint8", script_variant_t{&ffi_type_uint8})) {
				error("vmod: failed to set ffi types uint8 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "int8", script_variant_t{&ffi_type_sint8})) {
				error("vmod: failed to set ffi types int8 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "uint16", script_variant_t{&ffi_type_uint16})) {
				error("vmod: failed to set ffi types uint16 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "int16", script_variant_t{&ffi_type_sint16})) {
				error("vmod: failed to set ffi types int16 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "uint32", script_variant_t{&ffi_type_uint32})) {
				error("vmod: failed to set ffi types uint32 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "int32", script_variant_t{&ffi_type_sint32})) {
				error("vmod: failed to set ffi types int32 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "uint64", script_variant_t{&ffi_type_uint64})) {
				error("vmod: failed to set ffi types uint64 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "int64", script_variant_t{&ffi_type_sint64})) {
				error("vmod: failed to set ffi types int64 value\n"sv);
				return false;
			}

			if(!vm->SetValue(types_table, "ptr", script_variant_t{&ffi_type_pointer})) {
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

		if(vs_instance_ && vs_instance_ != gsdk::INVALID_HSCRIPT) {
			vm->RemoveInstance(vs_instance_);
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

	class_desc_t<class dynamic_detour> detour_desc{"__vmod_detour_class"};

	bool dynamic_detour::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vmod.vm()};

		detour_desc.func(&dynamic_detour::script_call, "script_call"sv, "call"sv);
		detour_desc.func(&dynamic_detour::script_enable, "script_enable"sv, "enable"sv);
		detour_desc.func(&dynamic_detour::script_disable, "script_disable"sv, "disable"sv);
		detour_desc.dtor();
		detour_desc.base(plugin::owned_instance_desc);
		detour_desc.doc_class_name("detour"sv);

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
		if(!va_args || num_args != arg_type_ptrs.size()) {
			vmod.vm()->RaiseException("wrong number of parameters");
			return {};
		}

		for(std::size_t i{0}; i < num_args; ++i) {
			ffi_type *arg_type{arg_type_ptrs[i]};
			const script_variant_t &arg_var{va_args[i]};
			auto &arg_ptr{args_storage[i]};

			script_var_to_ptr(arg_type, static_cast<void *>(arg_ptr.get()), arg_var);
		}

		{
			scope_enable sce{*this};
			ffi_call(&cif_, static_cast<void(*)()>(old_func), static_cast<void *>(ret_storage.get()), const_cast<void **>(args_storage_ptrs.data()));
		}

		script_variant_t ret_var;
		ptr_to_script_var(ret_type_ptr, static_cast<void *>(ret_storage.get()), ret_var);

		return ret_var;
	}

	void dynamic_detour::closure_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr) noexcept
	{
		std::vector<script_variant_t> sargs;

		dynamic_detour *det{static_cast<dynamic_detour *>(userptr)};

		sargs.emplace_back(det->instance);

		for(std::size_t i{0}; i < closure_cif->nargs; ++i) {
			ffi_type *arg_type_ptr{closure_cif->arg_types[i]};
			script_variant_t &arg_var{sargs.emplace_back()};

			ptr_to_script_var(arg_type_ptr, args[i], arg_var);
		}

		gsdk::IScriptVM *vm{vmod.vm()};

		gsdk::HSCRIPT scope{det->owner_scope()};

		script_variant_t ret_var;
		if(vm->ExecuteFunction(det->new_func, sargs.data(), static_cast<int>(sargs.size()), ((det->ret_type_ptr == &ffi_type_void) ? nullptr : &ret_var), scope, true) == gsdk::SCRIPT_ERROR) {
			return;
		}

		if(det->ret_type_ptr != &ffi_type_void) {
			script_var_to_ptr(det->ret_type_ptr, ret, ret_var);
		}
	}

	bool dynamic_detour::initialize_shared(gsdk::HSCRIPT new_func_, ffi_abi abi) noexcept
	{
		gsdk::IScriptVM *vm{vmod.vm()};

		closure = static_cast<ffi_closure *>(ffi_closure_alloc(sizeof(ffi_closure), reinterpret_cast<void **>(&closure_func)));
		if(!closure) {
			vm->RaiseException("vmod: failed to allocate detour closure");
			return false;
		}

		if(!cif::initialize(abi)) {
			vm->RaiseException("vmod: failed to create detour cif");
			return false;
		}

		if(ffi_prep_closure_loc(closure, &cif_, closure_binding, this, reinterpret_cast<void *>(closure_func)) != FFI_OK) {
			vm->RaiseException("vmod: failed to prepare detour closure");
			return false;
		}

		instance = vm->RegisterInstance(&detour_desc, this);
		if(!instance || instance == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: failed to register cif instance");
			return false;
		}

		//vm->SetInstanceUniqeId

		new_func = vm->ReferenceObject(new_func_);

		return true;
	}

	bool dynamic_detour::initialize(generic_func_t old_func_, gsdk::HSCRIPT new_func_, ffi_abi abi) noexcept
	{
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
