#include "singleton.hpp"
#include "../../main.hpp"
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

		gsdk::IScriptVM *vm{main::instance().vm()};

		desc.func(&singleton::script_allocate, "script_allocate"sv, "allocate"sv);
		desc.func(&singleton::script_allocate_aligned, "script_allocate_aligned"sv, "allocate_aligned"sv);
		desc.func(&singleton::script_allocate_zero, "script_allocate_zero"sv, "allocate_zero"sv);
		desc.func(&singleton::script_allocate_type, "script_allocate_type"sv, "allocate_type"sv);
		desc.func(&singleton::script_read, "script_read"sv, "read"sv);
		desc.func(&singleton::script_write, "script_write"sv, "write"sv);
		desc.func(&singleton::script_add, "script_add"sv, "add"sv);
		desc.func(&singleton::script_sub, "script_sub"sv, "sub"sv);
		desc.func(&singleton::script_get_vtable, "script_get_vtable"sv, "get_vtable"sv);
		desc.func(&singleton::script_get_vfunc, "script_get_vfunc"sv, "get_vfunc"sv);

		if(!singleton_base::bindings(&desc)) {
			return false;
		}

		types_table = vm->CreateTable();
		if(!types_table || types_table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create mem types table\n"sv);
			return false;
		}

		{
			if(!register_type(&ffi_type_void, "void"sv)) {
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
		}

		if(!vm->SetValue(scope, "types", types_table)) {
			error("vmod: failed to set mem types table value\n"sv);
			return false;
		}

		return true;
	}

	void singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		types.clear();

		if(types_table && types_table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(types_table);
		}

		if(vm->ValueExists(scope, "types")) {
			vm->ClearValue(scope, "types");
		}

		singleton_base::unbindings();
	}

	singleton::type::~type() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!name_.empty()) {
			singleton &mem{instance()};
			if(vm->ValueExists(mem.types_table, name_.c_str())) {
				vm->ClearValue(mem.types_table, name_.c_str());
			}
		}

		if(table && table != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseTable(table);
		}
	}

	bool singleton::register_type(ffi_type *ptr, std::string_view type_name) noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		gsdk::HSCRIPT table{vm->CreateTable()};
		if(!table || table == gsdk::INVALID_HSCRIPT) {
			error("vmod: failed to create type '%s' table\n", type_name.data());
			return false;
		}

		types.emplace_back(type{type_name, ptr, table});

		if(!vm->SetValue(table, "size", vscript::variant{ptr->size})) {
			error("vmod: failed to set type '%s' size value\n", type_name.data());
			return false;
		}

		if(!vm->SetValue(table, "alignment", vscript::variant{ptr->alignment})) {
			error("vmod: failed to set type '%s' alignment value\n", type_name.data());
			return false;
		}

		if(!vm->SetValue(table, "id", vscript::variant{ptr->type})) {
			error("vmod: failed to set type '%s' id value\n", type_name.data());
			return false;
		}

		if(!vm->SetValue(table, "__internal_ptr__", vscript::variant{ptr})) {
			error("vmod: failed to set type '%s' internal ptr value\n", type_name.data());
			return false;
		}

		if(!vm->SetValue(table, "name", vscript::variant{type_name})) {
			error("vmod: failed to set type '%s' name value\n", type_name.data());
			return false;
		}

		if(!vm->SetValue(types_table, type_name.data(), table)) {
			error("vmod: failed to set type '%s' name value\n", type_name.data());
			return false;
		}

		return true;
	}

	gsdk::HSCRIPT singleton::script_allocate(std::size_t size) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size");
			return gsdk::INVALID_HSCRIPT;
		}

		container *block{new container{size}};

		if(!block->initialize()) {
			delete block;
			return gsdk::INVALID_HSCRIPT;
		}

		return block->instance;
	}

	gsdk::HSCRIPT singleton::script_allocate_aligned(std::align_val_t align, std::size_t size) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size");
			return gsdk::INVALID_HSCRIPT;
		}

		if(static_cast<std::size_t>(align) == 0 || static_cast<std::size_t>(align) == static_cast<std::size_t>(-1) || (static_cast<std::size_t>(align) % 2) != 0) {
			vm->RaiseException("vmod: invalid align");
			return gsdk::INVALID_HSCRIPT;
		}

		container *block{new container{align, size}};

		if(!block->initialize()) {
			delete block;
			return gsdk::INVALID_HSCRIPT;
		}

		return block->instance;
	}

	gsdk::HSCRIPT singleton::script_allocate_type(gsdk::HSCRIPT type_table) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!type_table || type_table == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid type");
			return gsdk::INVALID_HSCRIPT;
		}

		if(!vm->IsTable(type_table)) {
			vm->RaiseException("vmod: invalid type");
			return gsdk::INVALID_HSCRIPT;
		}

		vscript::variant type_var;
		if(!vm->GetValue(type_table, "__internal_ptr__", &type_var)) {
			vm->RaiseException("vmod: invalid type");
			return gsdk::INVALID_HSCRIPT;
		}

		ffi_type *type{type_var.get<ffi_type *>()};
		if(!type) {
			vm->RaiseException("vmod: invalid type");
			return gsdk::INVALID_HSCRIPT;
		}

		container *block{new container{static_cast<std::align_val_t>(type->alignment), type->size}};
		if(!block->initialize()) {
			delete block;
			return gsdk::INVALID_HSCRIPT;
		}

		return block->instance;
	}

	gsdk::HSCRIPT singleton::script_allocate_zero(std::size_t num, std::size_t size) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(size == 0 || size == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid size");
			return gsdk::INVALID_HSCRIPT;
		}

		if(num == 0 || num == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid num");
			return gsdk::INVALID_HSCRIPT;
		}

		container *block{new container{num, size}};
		if(!block->initialize()) {
			delete block;
			return gsdk::INVALID_HSCRIPT;
		}

		return block->instance;
	}

	gsdk::ScriptVariant_t singleton::script_read(unsigned char *ptr, gsdk::HSCRIPT type_table) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!ptr) {
			vm->RaiseException("vmod: invalid ptr");
			return vscript::null();
		}

		if(!type_table || type_table == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid type");
			return vscript::null();
		}

		if(!vm->IsTable(type_table)) {
			vm->RaiseException("vmod: invalid type");
			return vscript::null();
		}

		vscript::variant type_var;
		if(!vm->GetValue(type_table, "__internal_ptr__", &type_var)) {
			vm->RaiseException("vmod: invalid type");
			return vscript::null();
		}

		ffi_type *type{type_var.get<ffi_type *>()};
		if(!type) {
			vm->RaiseException("vmod: invalid type");
			return vscript::null();
		}

		gsdk::ScriptVariant_t ret;
		ffi::ptr_to_script_var(ptr, type, ret);
		return ret;
	}

	void singleton::script_write(unsigned char *ptr, gsdk::HSCRIPT type_table, const vscript::variant &var) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!ptr) {
			vm->RaiseException("vmod: invalid ptr");
			return;
		}

		if(!type_table || type_table == gsdk::INVALID_HSCRIPT) {
			vm->RaiseException("vmod: invalid type");
			return;
		}

		if(!vm->IsTable(type_table)) {
			vm->RaiseException("vmod: invalid type");
			return;
		}

		vscript::variant type_var;
		if(!vm->GetValue(type_table, "__internal_ptr__", &type_var)) {
			vm->RaiseException("vmod: invalid type");
			return;
		}

		ffi_type *type{type_var.get<ffi_type *>()};
		if(!type) {
			vm->RaiseException("vmod: invalid type");
			return;
		}

		ffi::script_var_to_ptr(var, ptr, type);
	}

	unsigned char *singleton::script_add(unsigned char *ptr, std::ptrdiff_t off) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

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
		gsdk::IScriptVM *vm{main::instance().vm()};

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
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!obj) {
			vm->RaiseException("vmod: invalid object");
			return nullptr;
		}

		return vtable_from_object(obj);
	}

	generic_plain_mfp_t singleton::script_get_vfunc(generic_object_t *obj, std::size_t index) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!obj) {
			vm->RaiseException("vmod: invalid object");
			return nullptr;
		}

		if(index == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid index");
			return nullptr;
		}

		generic_plain_mfp_t *vtable{vtable_from_object<generic_object_t>(obj)};
		return vtable[index];
	}
}
