#include "singleton.hpp"
#include "cif.hpp"
#include "detour.hpp"

//TODO!!!! rethink how detours are created

namespace vmod::bindings::ffi
{
	vscript::singleton_class_desc<singleton> singleton::desc{"ffi"};

	static singleton ffi_;

	singleton::~singleton() noexcept {}

	singleton &singleton::instance() noexcept
	{ return ffi_; }

	bool singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		desc.func(&singleton::script_create_static_cif, "script_create_static_cif"sv, "cif_static"sv)
		.desc("[cif_static](abi|, types|ret, array<types>|args)"sv);

		desc.func(&singleton::script_create_member_cif, "script_create_member_cif"sv, "cif_member"sv)
		.desc("[cif_member](types|ret, this_types|this, array<types>|args)"sv);

		desc.func(&singleton::script_create_detour_static, "script_create_detour_static"sv, "detour_static"sv)
		.desc("[detour](fp|target, abi|, types|ret, array<types>|args, post)"sv);

		desc.func(&singleton::script_create_detour_member, "script_create_detour_member"sv, "detour_member"sv)
		.desc("[detour](mfp|target, types|ret, this_types|this, array<types>|args, post)"sv);

		if(!singleton_base::bindings(&desc)) {
			return false;
		}

		types_table = vm->CreateTable();
		if(!types_table) {
			error("vmod: failed to create ffi types table\n"sv);
			return false;
		}

		{
			if(!vm->SetValue(*types_table, "void", vscript::variant{&ffi_type_void})) {
				error("vmod: failed to set ffi void type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "int", vscript::variant{&ffi_type_sint})) {
				error("vmod: failed to set ffi int type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "uint", vscript::variant{&ffi_type_uint})) {
				error("vmod: failed to set ffi uint type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "char", vscript::variant{&ffi_type_schar})) {
				error("vmod: failed to set ffi char type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "uchar", vscript::variant{&ffi_type_uchar})) {
				error("vmod: failed to set ffi uchar type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "short", vscript::variant{&ffi_type_sshort})) {
				error("vmod: failed to set ffi short type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "ushort", vscript::variant{&ffi_type_ushort})) {
				error("vmod: failed to set ffi ushort type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "long", vscript::variant{&ffi_type_slong})) {
				error("vmod: failed to set ffi long type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "ulong", vscript::variant{&ffi_type_ulong})) {
				error("vmod: failed to set ffi ulong type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "float", vscript::variant{&ffi_type_float})) {
				error("vmod: failed to set ffi float type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "double", vscript::variant{&ffi_type_double})) {
				error("vmod: failed to set ffi double type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "long_double", vscript::variant{&ffi_type_longdouble})) {
				error("vmod: failed to set ffi long double type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "uint8", vscript::variant{&ffi_type_uint8})) {
				error("vmod: failed to set ffi uint8 type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "int8", vscript::variant{&ffi_type_sint8})) {
				error("vmod: failed to set ffi int8 type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "uint16", vscript::variant{&ffi_type_uint16})) {
				error("vmod: failed to set ffi uint16 type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "int16", vscript::variant{&ffi_type_sint16})) {
				error("vmod: failed to set ffi int16 type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "uint32", vscript::variant{&ffi_type_uint32})) {
				error("vmod: failed to set ffi uint32 type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "int32", vscript::variant{&ffi_type_sint32})) {
				error("vmod: failed to set ffi int32 type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "uint64", vscript::variant{&ffi_type_uint64})) {
				error("vmod: failed to set ffi uint64 type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "int64", vscript::variant{&ffi_type_sint64})) {
				error("vmod: failed to set ffi int64 type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "ptr", vscript::variant{&ffi_type_pointer})) {
				error("vmod: failed to set ffi ptr type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "bool", vscript::variant{&ffi_type_bool})) {
				error("vmod: failed to set ffi bool type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "cstr", vscript::variant{&ffi_type_cstr})) {
				error("vmod: failed to set ffi cstr type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "tstr_obj", vscript::variant{&ffi_type_object_tstr})) {
				error("vmod: failed to set ffi tstr type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "tstr_weak", vscript::variant{&ffi_type_weak_tstr})) {
				error("vmod: failed to set ffi tstr type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "tstr_plain", vscript::variant{&ffi_type_plain_tstr})) {
				error("vmod: failed to set ffi tstr type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "vec", vscript::variant{&ffi_type_vector})) {
				error("vmod: failed to set ffi vec type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "ang", vscript::variant{&ffi_type_qangle})) {
				error("vmod: failed to set ffi ang type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "clr32", vscript::variant{&ffi_type_color32})) {
				error("vmod: failed to set ffi clr32 type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "ehandle", vscript::variant{&ffi_type_ehandle})) {
				error("vmod: failed to set ffi ehandle type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "ent_ptr", vscript::variant{&ffi_type_ent_ptr})) {
				error("vmod: failed to set ffi ent_ptr type value\n"sv);
				return false;
			}
		}

		if(!vm->SetValue(*scope, "types", *types_table)) {
			error("vmod: failed to set ffi types table value\n"sv);
			return false;
		}

		this_types_table = vm->CreateTable();
		if(!this_types_table) {
			error("vmod: failed to create ffi this types table\n"sv);
			return false;
		}

		{
			if(!vm->SetValue(*this_types_table, "ptr", vscript::variant{&ffi_type_pointer})) {
				error("vmod: failed to set ffi ptr type value\n"sv);
				return false;
			}

			if(!vm->SetValue(*this_types_table, "ent_ptr", vscript::variant{&ffi_type_ent_ptr})) {
				error("vmod: failed to set ffi ent_ptr type value\n"sv);
				return false;
			}
		}

		if(!vm->SetValue(*scope, "this_types", *this_types_table)) {
			error("vmod: failed to set ffi this types table value\n"sv);
			return false;
		}

		abi_table = vm->CreateTable();
		if(!abi_table) {
			error("vmod: failed to create ffi abi table\n"sv);
			return false;
		}

		{
		#ifdef __x86_64__
			if(!vm->SetValue(*abi_table, "unix64", vscript::variant{FFI_UNIX64})) {
				error("vmod: failed to set ffi unix64 abi value\n"sv);
				return false;
			}

			if(!vm->SetValue(*abi_table, "win64", vscript::variant{FFI_WIN64})) {
				error("vmod: failed to set ffi win64 abi value\n"sv);
				return false;
			}

			if(!vm->SetValue(*abi_table, "gnuw64", vscript::variant{FFI_GNUW64})) {
				error("vmod: failed to set ffi gnuw64 abi value\n"sv);
				return false;
			}
		#else
			if(!vm->SetValue(*abi_table, "sysv", vscript::variant{FFI_SYSV})) {
				error("vmod: failed to set ffi sysv abi value\n"sv);
				return false;
			}

			if(!vm->SetValue(*abi_table, "cdecl_ms", vscript::variant{FFI_MS_CDECL})) {
				error("vmod: failed to set ffi cdecl_ms abi value\n"sv);
				return false;
			}

			if(!vm->SetValue(*abi_table, "thiscall", vscript::variant{FFI_THISCALL})) {
				error("vmod: failed to set ffi thiscall abi value\n"sv);
				return false;
			}

			if(!vm->SetValue(*abi_table, "fastcall", vscript::variant{FFI_FASTCALL})) {
				error("vmod: failed to set ffi fastcall abi value\n"sv);
				return false;
			}

			if(!vm->SetValue(*abi_table, "stdcall", vscript::variant{FFI_STDCALL})) {
				error("vmod: failed to set ffi stdcall abi value\n"sv);
				return false;
			}
		#endif

			if(!vm->SetValue(*abi_table, "current", vscript::variant{FFI_DEFAULT_ABI})) {
				error("vmod: failed to set ffi current abi value\n"sv);
				return false;
			}
		}

		if(!vm->SetValue(*scope, "abi", *abi_table)) {
			error("vmod: failed to set ffi abi table value\n"sv);
			return false;
		}

		return true;
	}

	void singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		types_table.free();

		this_types_table.free();

		if(scope) {
			if(vm->ValueExists(*scope, "types")) {
				vm->ClearValue(*scope, "types");
			}
		}

		abi_table.free();

		if(scope) {
			if(vm->ValueExists(*scope, "abi")) {
				vm->ClearValue(*scope, "abi");
			}
		}

		singleton_base::unbindings();
	}

	bool singleton::script_create_cif_shared(std::vector<ffi_type *> &args_types, vscript::array_handle_ref args) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!args) {
			vm->RaiseException("vmod: invalid args");
			return false;
		}

		int num_args{vm->GetArrayCount(*args)};
		for(int i{0}, it{0}; it != -1 && i < num_args; ++i) {
			vscript::variant value;
			it = vm->GetArrayValue(*args, it, &value);

			ffi_type *type{value.get<ffi_type *>()};
			if(!type) {
				vm->RaiseException("vmod: arg %i is invalid", i);
				return false;
			}

			args_types.emplace_back(type);
		}

		return true;
	}

	vscript::instance_handle_ref singleton::script_create_static_cif(ffi_abi abi, ffi_type *ret, vscript::array_handle_ref args) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!ret) {
			vm->RaiseException("vmod: invalid ret");
			return nullptr;
		}

		std::vector<ffi_type *> args_types;
		if(!script_create_cif_shared(args_types, args)) {
			return nullptr;
		}

		caller *cif{new caller{ret, std::move(args_types)}};
		if(!cif->initialize(abi, false)) {
			delete cif;
			return nullptr;
		}

		return cif->instance_;
	}

	//TODO!!! should you be able to change the abi?
	vscript::instance_handle_ref singleton::script_create_member_cif(ffi_type *ret, ffi_type *this_type, vscript::array_handle_ref args) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!ret) {
			vm->RaiseException("vmod: invalid ret");
			return nullptr;
		}

		std::vector<ffi_type *> args_types;

		args_types.emplace_back(this_type);

		if(!script_create_cif_shared(args_types, args)) {
			return nullptr;
		}

		caller *cif{new caller{ret, std::move(args_types)}};
		if(!cif->initialize(vmod::ffi::target_abi, true)) {
			delete cif;
			return nullptr;
		}

		return cif->instance_;
	}

	bool singleton::script_create_detour_shared(mfp_or_func_t old_target, ffi_type *ret, vscript::array_handle_ref args, std::vector<ffi_type *> &args_types, std::vector<std::string> &args_names) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!old_target) {
			vm->RaiseException("vmod: invalid function");
			return false;
		}

		if(!ret) {
			vm->RaiseException("vmod: invalid ret");
			return false;
		}

		if(!args) {
			vm->RaiseException("vmod: invalid args");
			return false;
		}

		int num_args{vm->GetArrayCount(*args)};

		bool member{!args_types.empty()};

		args_types.resize(member ? num_args+1 : num_args);
		args_names.resize(num_args);

		auto type_it{member ? args_types.begin()+1 : args_types.begin()};

		for(int i{0}, it{0}; it != -1 && i < num_args; ++i) {
			vscript::variant value;
			it = vm->GetArrayValue(*args, it, &value);

			ffi_type *arg_type{nullptr};

			if(value.is_object()) {
				vscript::handle_ref value_obj{value};
				if(vm->IsTable(*value_obj)) {
					if(vm->GetNumTableEntries(*value_obj) != 2) {
						vm->RaiseException("vmod: arg %i has invalid table", i);
						return false;
					}

					if(vm->GetValue(*value_obj, "name", &value)) {
						args_names[i] = value.get<std::string>();
					}

					if(vm->GetValue(*value_obj, "type", &value)) {
						arg_type = value.get<ffi_type *>();
					} else {
						vm->RaiseException("vmod: arg %i has invalid table", i);
						return false;
					}
				} else if(vm->IsArray(*value_obj)) {
					int count{vm->GetArrayCount(*value_obj)};
					if(count < 1 || count > 2) {
						vm->RaiseException("vmod: arg %i has invalid array", i);
						return false;
					}

					vm->GetArrayValue(*value_obj, 0, &value);
					if(count == 2) {
						if(value.m_type == gsdk::FIELD_CSTRING) {
							args_names[i] = value.get<std::string>();
							vm->GetArrayValue(*value_obj, 1, &value);
							arg_type = value.get<ffi_type *>();
						} else {
							arg_type = value.get<ffi_type *>();
							vm->GetArrayValue(*value_obj, 1, &value);
							args_names[i] = value.get<std::string>();
						}
					} else {
						arg_type = value.get<ffi_type *>();
					}
				}
			} else {
				arg_type = value.get<ffi_type *>();
			}

			if(!arg_type) {
				vm->RaiseException("vmod: arg %i is invalid", i);
				return false;
			}

			*type_it++ = arg_type;
		}

		return true;
	}

	//TODO!!! should you be able to change the abi?
	vscript::instance_handle_ref singleton::script_create_detour_member(mfp_or_func_t old_target, ffi_type *ret, ffi_type *this_type, vscript::array_handle_ref args) noexcept
	{
		using namespace std::literals::string_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		std::vector<ffi_type *> args_types;
		std::vector<std::string> args_names;

		if(!this_type) {
			vm->RaiseException("vmod: invalid this type");
			return nullptr;
		}

		args_types.emplace_back(this_type);

		if(!script_create_detour_shared(old_target, ret, args, args_types, args_names)) {
			return nullptr;
		}

		auto &members{detour::member_detours};
		using members_t = std::remove_reference_t<decltype(members)>;

		detour *det{nullptr};

		auto it{members.find(old_target.mfp)};
		if(it == members.end()) {
			det = new detour{ret, std::move(args_types), std::move(args_names)};
			if(!det->initialize(old_target, vmod::ffi::target_abi, true)) {
				delete det;
				return nullptr;
			}

			members.emplace(members_t::value_type{old_target.mfp, det});
		} else {
			det = it->second.get();

			det->add_plugin();
		}

		return det->instance_;
	}

	vscript::instance_handle_ref singleton::script_create_detour_static(mfp_or_func_t old_target, ffi_abi abi, ffi_type *ret, vscript::array_handle_ref args) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		std::vector<ffi_type *> args_types;
		std::vector<std::string> args_names;

		if(!script_create_detour_shared(old_target, ret, args, args_types, args_names)) {
			return nullptr;
		}

		auto &statics{detour::static_detours};
		using statics_t = std::remove_reference_t<decltype(statics)>;

		detour *det{nullptr};

		auto it{statics.find(old_target.func)};
		if(it == statics.end()) {
			det = new detour{ret, std::move(args_types), std::move(args_names)};
			if(!det->initialize(old_target, abi, false)) {
				delete det;
				return nullptr;
			}

			statics.emplace(statics_t::value_type{old_target.func, det});
		} else {
			det = it->second.get();

			det->add_plugin();
		}

		return det->instance_;
	}
}
