#include "singleton.hpp"
#include "container.hpp"

namespace vmod::bindings::mem
{
	vscript::singleton_class_desc<singleton> singleton::desc{"mem"};

	static singleton mem_;

	singleton &singleton::instance() noexcept
	{ return mem_; }

	singleton::~singleton() noexcept {}

	bool singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		desc.func(&singleton::script_allocate, "script_allocate"sv, "allocate"sv)
		.desc("[container](size)"sv);

		desc.func(&singleton::script_allocate_aligned, "script_allocate_aligned"sv, "allocate_aligned"sv)
		.desc("[container](align, size)"sv);

		desc.func(&singleton::script_allocate_zero, "script_allocate_zero"sv, "allocate_zero"sv)
		.desc("[container](num, size)"sv);

		desc.func(&singleton::script_allocate_type, "script_allocate_type"sv, "allocate_type"sv)
		.desc("[container](types::type|type)"sv);

		desc.func(&singleton::script_allocate_ent, "script_allocate_ent"sv, "allocate_ent"sv)
		.desc("[container](size)"sv);

		desc.func(&singleton::script_read, "script_read"sv, "read"sv)
		.desc("(ptr|, types::type|type)"sv);

		desc.func(&singleton::script_write, "script_write"sv, "write"sv)
		.desc("(ptr|, types::type|type, value)"sv);

		desc.func(&singleton::script_add, "script_add"sv, "add"sv)
		.desc("[ptr](ptr|, offset)"sv);

		desc.func(&singleton::script_sub, "script_sub"sv, "sub"sv)
		.desc("[ptr](ptr|, offset)"sv);

		desc.func(&singleton::script_get_vtable, "script_get_vtable"sv, "get_vtable"sv)
		.desc("[vtable](ptr|)"sv);

		desc.func(&singleton::script_get_vfunc, "script_get_vfunc"sv, "get_vfunc"sv)
		.desc("[fp](ptr|, index)"sv);

		if(!singleton_base::bindings(&desc)) {
			return false;
		}

		types_table = vm->CreateTable();
		if(!types_table) {
			error("vmod: failed to create mem types table\n"sv);
			return false;
		}

		{
			if(!register_type(&ffi_type_void, "void"sv)) {
				error("vmod: failed to register mem void type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_nullptr, "nullptr"sv)) {
				error("vmod: failed to register mem void type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sint, "int"sv)) {
				error("vmod: failed to register mem int type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uint, "uint"sv)) {
				error("vmod: failed to register mem uint type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_schar, "char"sv)) {
				error("vmod: failed to register mem char type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uchar, "uchar"sv)) {
				error("vmod: failed to register mem uchar type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sshort, "short"sv)) {
				error("vmod: failed to register mem short type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_ushort, "ushort"sv)) {
				error("vmod: failed to register mem ushort type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_slong, "long"sv)) {
				error("vmod: failed to register mem long type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_ulong, "ulong"sv)) {
				error("vmod: failed to register mem ulong type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_float, "float"sv)) {
				error("vmod: failed to register mem float type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_double, "double"sv)) {
				error("vmod: failed to register mem double type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_longdouble, "long_double"sv)) {
				error("vmod: failed to register mem long double type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uint8, "uint8"sv)) {
				error("vmod: failed to register mem uint8 type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sint8, "int8"sv)) {
				error("vmod: failed to register mem int8 type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uint16, "uint16"sv)) {
				error("vmod: failed to register mem uint16 type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sint16, "int16"sv)) {
				error("vmod: failed to register mem int16 type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uint32, "uint32"sv)) {
				error("vmod: failed to register mem uint32 type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sint32, "int32"sv)) {
				error("vmod: failed to register mem int32 type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_uint64, "uint64"sv)) {
				error("vmod: failed to register mem uint64 type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_sint64, "int64"sv)) {
				error("vmod: failed to register mem int64 type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_pointer, "ptr"sv)) {
				error("vmod: failed to register mem ptr type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_bool, "bool"sv)) {
				error("vmod: failed to register mem bool type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_cstr, "cstr"sv)) {
				error("vmod: failed to register mem cstr type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_object_tstr, "tstr_obj"sv)) {
				error("vmod: failed to register mem tstr type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_plain_tstr, "tstr_plain"sv)) {
				error("vmod: failed to register mem tstr type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_weak_tstr, "tstr_weak"sv)) {
				error("vmod: failed to register mem tstr type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_vector, "vec"sv)) {
				error("vmod: failed to register mem vec type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_qangle, "ang"sv)) {
				error("vmod: failed to register mem ang type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_color32, "clr32"sv)) {
				error("vmod: failed to register mem clr32 type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_ehandle, "ehandle"sv)) {
				error("vmod: failed to register mem ehandle type\n"sv);
				return false;
			}

			if(!register_type(&ffi_type_ent_ptr, "ent_ptr"sv)) {
				error("vmod: failed to register mem ent_ptr type\n"sv);
				return false;
			}
		}

		if(!vm->SetValue(*scope, "types", *types_table)) {
			error("vmod: failed to set mem types table value\n"sv);
			return false;
		}

		return true;
	}

	void singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		types.clear();

		types_table.free();

		if(scope) {
			if(vm->ValueExists(*scope, "types")) {
				vm->ClearValue(*scope, "types");
			}
		}

		singleton_base::unbindings();
	}

	singleton::type::~type() noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!name_.empty()) {
			singleton &mem{instance()};
			if(vm->ValueExists(*mem.types_table, name_.c_str())) {
				vm->ClearValue(*mem.types_table, name_.c_str());
			}
		}

		table_.free();
	}

	singleton::type *singleton::find_type(ffi_type *ptr) noexcept
	{
		for(const auto &it :types) {
			if(it->ptr == ptr) {
				return it.get();
			}
		}

		return nullptr;
	}

	ffi_type *singleton::read_type(vscript::table_handle_ref type_table) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!type_table) {
			vm->RaiseException("vmod: invalid type");
			return nullptr;
		}

		vscript::variant value;
		if(!vm->GetValue(*type_table, "__internal_ptr__", &value)) {
			vm->RaiseException("vmod: type table missing __internal_ptr__ key");
			return nullptr;
		}

		ffi_type *type{value.get<ffi_type *>()};
		if(!type) {
			vm->RaiseException("vmod: failed to read type");
			return nullptr;
		}

		return type;
	}

	bool singleton::register_type(ffi_type *ptr, std::string_view type_name) noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		vscript::table_handle_wrapper table{vm->CreateTable()};
		if(!table) {
			error("vmod: failed to create type '%s' table\n", type_name.data());
			return false;
		}

		{
			if(!vm->SetValue(*table, "size", vscript::variant{ptr->size})) {
				error("vmod: failed to set type '%s' size value\n", type_name.data());
				return false;
			}

			if(!vm->SetValue(*table, "alignment", vscript::variant{ptr->alignment})) {
				error("vmod: failed to set type '%s' alignment value\n", type_name.data());
				return false;
			}

			if(!vm->SetValue(*table, "__internal_ptr__", vscript::variant{ptr})) {
				error("vmod: failed to set type '%s' internal ptr value\n", type_name.data());
				return false;
			}
		}

		if(!vm->SetValue(*types_table, type_name.data(), *table)) {
			error("vmod: failed to set type '%s' value\n", type_name.data());
			return false;
		}

		types.emplace_back(new type{type_name, ptr, std::move(table)});

		return true;
	}

	vscript::instance_handle_ref singleton::script_allocate(std::size_t size) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size %zu", size);
			return nullptr;
		}

		container *block{new container{size, container::type::normal}};

		if(!block->initialize()) {
			delete block;
			return nullptr;
		}

		return block->instance_;
	}

	vscript::instance_handle_ref singleton::script_allocate_ent(std::size_t size) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size %zu", size);
			return nullptr;
		}

		container *block{new container{size, container::type::entity}};

		if(!block->initialize()) {
			delete block;
			return nullptr;
		}

		return block->instance_;
	}

	vscript::instance_handle_ref singleton::script_allocate_aligned(std::align_val_t align, std::size_t size) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size: %zu", size);
			return nullptr;
		}

		if(static_cast<std::size_t>(align) == 0 || static_cast<std::size_t>(align) == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid align: %zu", static_cast<std::size_t>(align));
			return nullptr;
		}

		if((static_cast<std::size_t>(align) % 2) != 0) {
			vm->RaiseException("vmod: align is not divisible by 2: %zu", static_cast<std::size_t>(align));
			return nullptr;
		}

		container *block{new container{align, size, false}};

		if(!block->initialize()) {
			delete block;
			return nullptr;
		}

		return block->instance_;
	}

	vscript::instance_handle_ref singleton::script_allocate_type(vscript::table_handle_ref type_table) noexcept
	{
		ffi_type *type{read_type(type_table)};
		if(!type) {
			return nullptr;
		}

		container *block{new container{static_cast<std::align_val_t>(type->alignment), type->size, false}};
		if(!block->initialize()) {
			delete block;
			return nullptr;
		}

		return block->instance_;
	}

	vscript::instance_handle_ref singleton::script_allocate_zero(std::size_t num, std::size_t size) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size: %zu", size);
			return nullptr;
		}

		if(num == 0 || num == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid num: %zu", num);
			return nullptr;
		}

		container *block{new container{num, size, false}};
		if(!block->initialize()) {
			delete block;
			return nullptr;
		}

		return block->instance_;
	}

	vscript::variant singleton::script_read(unsigned char *ptr, vscript::table_handle_ref type_table) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!ptr) {
			vm->RaiseException("vmod: invalid ptr");
			return vscript::null();
		}

		ffi_type *type{read_type(type_table)};
		if(!type) {
			return vscript::null();
		}

		vscript::variant ret;
		ffi::ptr_to_script_var(ptr, type, ret);
		return ret;
	}

	void singleton::script_write(unsigned char *ptr, vscript::table_handle_ref type_table, const vscript::variant &var) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!ptr) {
			vm->RaiseException("vmod: invalid ptr");
			return;
		}

		ffi_type *type{read_type(type_table)};
		if(!type) {
			return;
		}

		ffi::script_var_to_ptr(var, ptr, type);
	}

	unsigned char *singleton::script_add(unsigned char *ptr, std::ptrdiff_t off) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!ptr) {
			vm->RaiseException("vmod: invalid ptr");
			return nullptr;
		}

		unsigned char *temp{ptr};
		temp += off;
		return temp;
	}

	unsigned char *singleton::script_sub(unsigned char *ptr, std::ptrdiff_t off) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!ptr) {
			vm->RaiseException("vmod: invalid ptr");
			return nullptr;
		}

		unsigned char *temp{ptr};
		temp -= off;
		return temp;
	}

	generic_vtable_t singleton::script_get_vtable(generic_object_t *obj) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!obj) {
			vm->RaiseException("vmod: invalid object");
			return nullptr;
		}

		return vtable_from_object(obj);
	}

	generic_plain_mfp_t singleton::script_get_vfunc(generic_object_t *obj, std::size_t index) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!obj) {
			vm->RaiseException("vmod: invalid object");
			return nullptr;
		}

		if(index == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid index %zu", index);
			return nullptr;
		}

		generic_plain_mfp_t *vtable{vtable_from_object<generic_object_t>(obj)};
		return vtable[index];
	}
}
