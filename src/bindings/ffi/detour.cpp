#include "detour.hpp"
#include "../../main.hpp"

namespace vmod::bindings::ffi
{
	vscript::class_desc<detour> detour::desc{"ffi::detour"};

	std::unordered_map<generic_func_t, std::unique_ptr<detour>> detour::static_detours;
	std::unordered_map<generic_internal_mfp_t, std::unique_ptr<detour>> detour::member_detours;

	bool detour::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{main::instance().vm()};

		desc.base(plugin::shared_instance::desc);
		desc.func(&detour::script_call, "script_call"sv, "call"sv);

		desc.func(&detour::script_hook, "script_hook"sv, "hook"sv)
		.desc("[callback_instance](function|callback, post)"sv);

		desc.dtor();

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register ffi detour class\n"sv);
			return false;
		}

		return true;
	}

	void detour::unbindings() noexcept
	{
		
	}

	vscript::instance_handle_ref detour::script_hook(vscript::func_handle_ref callback, bool post) noexcept
	{
		gsdk::IScriptVM *vm{main::instance().vm()};

		if(!callback) {
			vm->RaiseException("vmod: invalid callback");
			return nullptr;
		}

		vscript::func_handle_wrapper callback_copy{vm->ReferenceFunction(*callback)};
		if(!callback_copy) {
			vm->RaiseException("vmod: failed to get callback reference");
			return nullptr;
		}

		detour::callback_instance *clbk_instance{new detour::callback_instance{this, std::move(callback_copy), post}};
		if(!clbk_instance->initialize()) {
			delete clbk_instance;
			return nullptr;
		}

		return clbk_instance->instance_;
	}

	detour::callback_instance::callback_instance(detour *owner_, vscript::func_handle_wrapper &&callback_, bool post_) noexcept
		: plugin::callback_instance{owner_, std::move(callback_), post_}, owner{owner_}
	{
	}

	detour::callback_instance::~callback_instance() noexcept {}

	detour::~detour() noexcept
	{
		if(member) {
			auto it{member_detours.find(old_target.mfp)};
			if(it != member_detours.end()) {
				it->second.release();
				member_detours.erase(it);
			}
		} else {
			auto it{static_detours.find(old_target.func)};
			if(it != static_detours.end()) {
				it->second.release();
				static_detours.erase(it);
			}
		}

		if(old_target) {
			disable();
		}

		if(closure) {
			ffi_closure_free(closure);
		}
	}

	vscript::variant detour::script_call(const vscript::variant *args, std::size_t num_args, ...) noexcept
	{
		std::size_t required_args{args_types.size()};
		if(!args || num_args != required_args) {
			vscript::vm()->RaiseException("vmod: wrong number of parameters expected %zu got %zu", num_args, required_args);
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
			vmod::ffi::cif::call(reinterpret_cast<void(*)()>(old_target.mfp.addr));
		}

		vscript::variant ret;
		if(ret_type != &ffi_type_void) {
			vmod::ffi::ptr_to_script_var(static_cast<void *>(ret_storage.get()), ret_type, ret);
		} else {
			ret = nullptr;
		}
		return ret;
	}

	std::vector<vscript::variant> detour::sargs;
	const char detour::uninitalized_str[17]{"<<uninitalized>>"};

	void detour::closure_binding(ffi_cif *closure_cif, void *ret, void *args[], void *userptr) noexcept
	{
		detour *det{static_cast<detour *>(userptr)};

		bool has_ret{det->ret_type != &ffi_type_void};

		sargs.resize(
			has_ret ?
			closure_cif->nargs + 1
			:
			closure_cif->nargs
		);

		for(std::size_t i{0}; i < closure_cif->nargs; ++i) {
			ffi_type *arg_type{closure_cif->arg_types[i]};

			vscript::variant &arg_var{sargs[i]};
			arg_var.free();

			vmod::ffi::ptr_to_script_var(args[i], arg_type, arg_var);
		}

		if(has_ret) {
			vscript::variant &ret_var{sargs[closure_cif->nargs]};
			ret_var.free();

			ret_var = static_cast<const void *>(uninitalized_str);
		}

		gsdk::IScriptVM *vm{vscript::vm()};

		return_value retflags{det->call_pre(sargs.data(), sargs.size(), true)};

		if(retflags & return_value::call_orig) {
			if(retflags & return_value::changed) {
				for(std::size_t i{0}; i < closure_cif->nargs; ++i) {
					ffi_type *arg_type{closure_cif->arg_types[i]};

					vmod::ffi::script_var_to_ptr(sargs[i], args[i], arg_type);
				}
			}

			{
				scope_enable sce{*det};
				det->vmod::ffi::cif::call(reinterpret_cast<void(*)()>(det->old_target.mfp.addr), ret, args);
			}
		} else if(has_ret) {
			vmod::ffi::init_ptr(ret, det->ret_type);
		}

		if(has_ret) {
			vscript::variant &ret_var{sargs[closure_cif->nargs]};

			if(ret_var.m_ccstr == uninitalized_str || !(retflags & return_value::changed)) {
				vmod::ffi::ptr_to_script_var(ret, det->ret_type, ret_var);
			} else {
				vmod::ffi::script_var_to_ptr(ret_var, ret, det->ret_type);
			}
		}

		retflags = det->call_post(sargs.data(), sargs.size(), true);

		if(has_ret && (retflags & return_value::changed)) {
			vscript::variant &ret_var{sargs[closure_cif->nargs]};

			if(ret_var.m_ccstr != uninitalized_str) {
				vmod::ffi::script_var_to_ptr(ret_var, ret, det->ret_type);
			}
		}
	}

	bool detour::initialize(mfp_or_func_t old_target_, ffi_abi abi, bool member_) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		old_target = old_target_;

		member = member_;

		backup_bytes();

		if(!register_instance(&desc, this)) {
			vm->RaiseException("vmod: failed to register detour instance");
			return false;
		}

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

		return true;
	}

	void detour::on_sleep() noexcept
	{
		disable();
	}

	void detour::on_wake() noexcept
	{
		enable();
	}

	void detour::enable() noexcept
	{
		if(enabled) {
			return;
		}

	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_target.mfp.addr)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif

	#ifdef __x86_64__
		bytes[0] = 0x49;
		bytes[1] = 0xBA;

		std::uintptr_t target{reinterpret_cast<std::uintptr_t>(closure_func)};
		std::memcpy(bytes + 2, &target, sizeof(std::uintptr_t));

		bytes[10] = 0x41;
		bytes[11] = 0xFF;
		bytes[12] = 0xE2;
	#else
		bytes[0] = 0xE9;

		std::uintptr_t target{reinterpret_cast<std::uintptr_t>(closure_func) - (reinterpret_cast<std::uintptr_t>(old_target.mfp.addr) + sizeof(old_bytes))};
		std::memcpy(bytes + 1, &target, sizeof(std::uintptr_t));
	#endif

		enabled = true;
	}

	void detour::disable() noexcept
	{
		if(!enabled) {
			return;
		}

	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_target.mfp.addr)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif
		std::memcpy(bytes, old_bytes, sizeof(old_bytes));

		enabled = false;
	}

	void detour::backup_bytes() noexcept
	{
	#ifndef __clang__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wconditionally-supported"
	#endif
		unsigned char *bytes{reinterpret_cast<unsigned char *>(old_target.mfp.addr)};
	#ifndef __clang__
		#pragma GCC diagnostic pop
	#endif

		page_info func_page{bytes, sizeof(old_bytes)};
		func_page.protect(PROT_READ|PROT_WRITE|PROT_EXEC);

		std::memcpy(old_bytes, bytes, sizeof(old_bytes));
	}
}
