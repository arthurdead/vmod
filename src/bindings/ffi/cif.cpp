#include "cif.hpp"
#include "../../main.hpp"

namespace vmod::bindings::ffi
{
	vscript::class_desc<caller> caller::desc{"ffi::cif"};

	caller::~caller() noexcept {}

	bool caller::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&caller::script_call, "script_call"sv, "call"sv);

		desc.func(&caller::script_set_func, "script_set_func"sv, "set_func"sv)
		.desc("(fp|target)"sv);

		desc.func(&caller::script_set_mfp, "script_set_mfp"sv, "set_mfp"sv)
		.desc("(mfp|target)"sv);

		desc.dtor();

		if(!plugin::owned_instance::register_class(&desc)) {
			error("vmod: failed to register ffi cif class\n"sv);
			return false;
		}

		return true;
	}

	void caller::unbindings() noexcept
	{

	}

	void caller::script_set_func(generic_func_t func) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!func) {
			vm->RaiseException("vmod: invalid function");
			return;
		}

		target.func = func;
	}

	void caller::script_set_mfp(generic_mfp_t func) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!func) {
			vm->RaiseException("vmod: invalid function");
			return;
		}

		target.mfp = func;
	}

	vscript::variant caller::script_call(const vscript::variant *args, std::size_t num_args, ...) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!target.mfp) {
			vm->RaiseException("vmod: invalid function");
			return vscript::null();
		}

		std::size_t required_args{args_types.size()};
		if(!args || num_args != required_args) {
			vm->RaiseException("vmod: wrong number of parameters expected %zu got %zu", num_args, required_args);
			return vscript::null();
		}

		for(std::size_t i{0}; i < num_args; ++i) {
			ffi_type *type{args_types[i]};
			const vscript::variant &var{args[i]};
			auto &ptr{args_storage[i]};

			vmod::ffi::script_var_to_ptr(var, static_cast<void *>(ptr.get()), type);
		}

		call(reinterpret_cast<void(*)()>(target.mfp.addr));

		vscript::variant ret_var;
		if(ret_type != &ffi_type_void) {
			vmod::ffi::ptr_to_script_var(static_cast<void *>(ret_storage.get()), ret_type, ret_var);
		} else {
			ret_var = nullptr;
		}
		return ret_var;
	}

	bool caller::initialize(ffi_abi abi) noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!vmod::ffi::cif::initialize(abi)) {
			vm->RaiseException("vmod: failed to initialize");
			return false;
		}

		if(!register_instance(&desc, this)) {
			return false;
		}

		return true;
	}
}
