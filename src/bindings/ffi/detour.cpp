#include "detour.hpp"
#include "../../main.hpp"

namespace vmod::bindings::ffi
{
	vscript::class_desc<detour> detour::desc{"ffi::detour"};

	bool detour::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&detour::script_call, "script_call"sv, "call"sv);
		desc.func(&detour::script_enable, "script_enable"sv, "enable"sv);
		desc.func(&detour::script_disable, "script_disable"sv, "disable"sv);

		if(!plugin::owned_instance::register_class(&desc)) {
			error("vmod: failed to register ffi detour class\n"sv);
			return false;
		}

		return true;
	}

	void detour::unbindings() noexcept
	{
		
	}

	detour::~detour() noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(script_target && script_target != gsdk::INVALID_HSCRIPT) {
			vm->ReleaseFunction(script_target);
		}

		if(old_target) {
			disable();
		}

		if(closure) {
			ffi_closure_free(closure);
		}
	}

	gsdk::ScriptVariant_t detour::script_call(const vscript::variant *args, std::size_t num_args, ...) noexcept
	{
		std::size_t required_args{args_types.size()};
		if(!args || num_args != required_args) {
			main::instance().vm()->RaiseException("vmod: wrong number of parameters expected %zu got %i", num_args, required_args);
			return vscript::null();
		}

		for(std::size_t i{0}; i < num_args; ++i) {
			ffi_type *arg_type{args_types[i]};
			const vscript::variant &arg_var{args[i]};
			auto &arg_ptr{args_storage[i]};

			vmod::ffi::script_var_to_ptr(arg_var, static_cast<void *>(arg_ptr.get()), arg_type);
		}

		{
			scope_enable sce{*this};
			call(reinterpret_cast<void(*)()>(old_target.mfp.addr));
		}

		gsdk::ScriptVariant_t ret;
		vmod::ffi::ptr_to_script_var(static_cast<void *>(ret_storage.get()), ret_type, ret);
		return ret;
	}

	void detour::closure_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr) noexcept
	{
		std::vector<vscript::variant> sargs;

		detour *det{static_cast<detour *>(userptr)};

		sargs.emplace_back(det->instance);

		for(std::size_t i{0}; i < closure_cif->nargs; ++i) {
			ffi_type *arg_type{closure_cif->arg_types[i]};
			vscript::variant &arg_var{sargs.emplace_back()};

			vmod::ffi::ptr_to_script_var(args[i], arg_type, arg_var);
		}

		gsdk::IScriptVM *vm{main::instance().vm()};

		gsdk::HSCRIPT scope{det->owner_scope()};

		vscript::variant ret_var;
		if(vm->ExecuteFunction(det->script_target, sargs.data(), static_cast<int>(sargs.size()), ((det->ret_type == &ffi_type_void) ? nullptr : &ret_var), scope, true) == gsdk::SCRIPT_ERROR) {
			return;
		}

		if(det->ret_type != &ffi_type_void) {
			vmod::ffi::script_var_to_ptr(ret_var, ret, det->ret_type);
		}
	}

	bool detour::initialize_shared(gsdk::HSCRIPT callback, ffi_abi abi) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		closure = static_cast<ffi_closure *>(ffi_closure_alloc(sizeof(ffi_closure), reinterpret_cast<void **>(&closure_func)));
		if(!closure) {
			vm->RaiseException("vmod: failed to allocate detour closure");
			return false;
		}

		if(!vmod::ffi::cif::initialize(abi)) {
			vm->RaiseException("vmod: failed to create detour cif");
			return false;
		}

	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		if(ffi_prep_closure_loc(closure, &cif_impl, closure_binding, this, reinterpret_cast<void *>(closure_func)) != FFI_OK) {
			vm->RaiseException("vmod: failed to prepare detour closure");
			return false;
		}
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif

		if(!register_instance(&desc, this)) {
			return false;
		}

		script_target = vm->ReferenceObject(callback);

		return true;
	}

	bool detour::initialize(mfp_or_func_t old_target_, gsdk::HSCRIPT callback, ffi_abi abi) noexcept
	{
		if(!initialize_shared(callback, abi)) {
			return false;
		}

		old_target = old_target_;

		backup_bytes();

		return true;
	}

	void detour::enable() noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_target.mfp.addr)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif

		bytes[0] = 0xE9;

		std::uintptr_t target{reinterpret_cast<std::uintptr_t>(closure_func) - (reinterpret_cast<std::uintptr_t>(old_target.mfp.addr) + sizeof(old_bytes))};
		std::memcpy(bytes + 1, &target, sizeof(std::uintptr_t));
	}

	void detour::backup_bytes() noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		page_info func_page{reinterpret_cast<void *>(old_target.mfp.addr), sizeof(old_bytes)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif
		func_page.protect(PROT_READ|PROT_WRITE|PROT_EXEC);

	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_target.mfp.addr)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif

		std::memcpy(old_bytes, bytes, sizeof(old_bytes));
	}
}
