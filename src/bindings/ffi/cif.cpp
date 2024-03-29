#include "cif.hpp"

namespace vmod::bindings::ffi
{
	vscript::class_desc<caller> caller::desc_base{"ffi::cif"};
	vscript::class_desc<caller> caller::desc_static{"ffi::cif_static"};
	vscript::class_desc<caller> caller::desc_member{"ffi::cif_member"};

	caller::~caller() noexcept {}

	bool caller::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc_base.base(plugin::owned_instance::desc);

		desc_base.func(&caller::script_call, "script_call"sv, "call"sv);

		desc_base.dtor();

		desc_static.base(desc_base);

		desc_static.func(&caller::script_set_func, "script_set_func"sv, "set_func"sv)
		.desc("(fp|target)"sv);

		desc_static.dtor();

		desc_member.base(desc_base);

		desc_member.func(&caller::script_set_mfp, "script_set_mfp"sv, "set_mfp"sv)
		.desc("(mfp|target)"sv);

		desc_member.func(&caller::script_set_vidx, "script_set_vidx"sv, "set_vidx"sv)
		.desc("(vidx)"sv);

		desc_member.dtor();

		if(!plugin::owned_instance::register_class(&desc_static)) {
			error("vmod: failed to register ffi cif class\n"sv);
			return false;
		}

		if(!plugin::owned_instance::register_class(&desc_member)) {
			error("vmod: failed to register ffi cif member class\n"sv);
			return false;
		}

		return true;
	}

	void caller::unbindings() noexcept
	{

	}

	void caller::script_set_func(generic_func_t func) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!func) {
			vm->RaiseException("vmod: invalid function");
			return;
		}

		target_ptr.func = func;
		virt = false;
	}

	void caller::script_set_mfp(generic_mfp_t func) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!func) {
			vm->RaiseException("vmod: invalid function");
			return;
		}

		target_ptr.mfp = func;
		virt = false;
	}

	void caller::script_set_vidx(std::size_t func) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(func == static_cast<std::size_t>(-1)) {
			vm->RaiseException("vmod: invalid function");
			return;
		}

		target_idx = func;
		virt = true;
	}

	vscript::variant caller::script_call(const vscript::variant *args, std::size_t num_args, ...) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!virt) {
			if(!target_ptr.mfp) {
				vm->RaiseException("vmod: invalid function");
				return vscript::null();
			}
		}

		std::size_t required_args{args_types.size()};
		if(!args || num_args != required_args) {
			vm->RaiseException("vmod: wrong number of parameters expected %zu got %zu", num_args, required_args);
			return vscript::null();
		}

		if(virt) {
			if(num_args == 0) {
				vm->RaiseException("vmod: missing this param");
				return vscript::null();
			}
		}

		for(std::size_t i{0}; i < num_args; ++i) {
			ffi_type *type{args_types[i]};
			const vscript::variant &var{args[i]};
			auto &ptr{args_storage[i]};

			vmod::ffi::script_var_to_ptr(var, static_cast<void *>(ptr.get()), type);
		}

		mfp_or_func_t func;

		if(!virt) {
			func = target_ptr;
		} else {
			auto obj{args_storage[0].get()};
			if(!obj) {
				vm->RaiseException("vmod: nullptr this param");
				return vscript::null();
			}

			auto vtabl{vtable_from_object(obj)};
			func = vtabl[target_idx];
		}

		call(reinterpret_cast<void(*)()>(func.mfp.addr));

		vscript::variant ret_var;
		if(ret_type != &ffi_type_void) {
			vmod::ffi::ptr_to_script_var(static_cast<void *>(ret_storage.get()), ret_type, ret_var);
		} else {
			ret_var = nullptr;
		}
		return ret_var;
	}

	bool caller::initialize(ffi_abi abi, bool member) noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		if(!vmod::ffi::cif::initialize(abi)) {
			vm->RaiseException("vmod: failed to initialize");
			return false;
		}

		auto target_desc{member ? &static_cast<gsdk::ScriptClassDesc_t &>(desc_member) : &static_cast<gsdk::ScriptClassDesc_t &>(desc_static)};
		if(!register_instance(target_desc, this)) {
			return false;
		}

		return true;
	}
}
