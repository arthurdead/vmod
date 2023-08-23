#pragma once

#include "vscript/vscript.hpp"
#include "vscript/variant.hpp"
#include <vector>
#include <memory>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-W#warnings"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#endif
#include <ffi.h>
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

#define __VMOD_DECLARE_FFI_TYPE_WITH_PTR(name) \
	extern ffi_type ffi_type_##name; \
	extern ffi_type ffi_type_##name##_ptr;

__VMOD_DECLARE_FFI_TYPE_WITH_PTR(vector)
__VMOD_DECLARE_FFI_TYPE_WITH_PTR(qangle)
extern ffi_type ffi_type_color32;
extern ffi_type ffi_type_ehandle;
extern ffi_type ffi_type_bool;
extern ffi_type ffi_type_cstr;
extern ffi_type ffi_type_object_tstr;
extern ffi_type ffi_type_ent_ptr;
#define ffi_type_plain_tstr ffi_type_cstr
#define ffi_type_weak_tstr ffi_type_sint

namespace vmod::ffi
{
	extern void script_var_to_ptr(const vscript::variant &var, void *ptr, ffi_type *type) noexcept;
	extern void ptr_to_script_var(void *ptr, ffi_type *type, gsdk::ScriptVariant_t &var) noexcept;
	extern ffi_type *type_id_to_ptr(int id) noexcept;
	extern int to_field_type(ffi_type *type);

	class cif
	{
	public:
		inline cif(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: args_types{std::move(args)}, ret_type{ret}
		{
		}

		virtual ~cif() noexcept;

		bool initialize(ffi_abi abi) noexcept;

		inline ffi_cif *operator&() noexcept
		{ return &cif_impl; }

	protected:
		inline void call(void(*func)()) noexcept
		{ call(func, static_cast<void *>(ret_storage.get()), args_ptrs.data()); }
		void call(void(*func)(), void *ret) noexcept;
		void call(void(*func)(), void *ret, void **args) noexcept;
		void call(void(*func)(), void **args) noexcept;

		ffi_cif cif_impl;

		std::vector<ffi_type *> args_types;
		ffi_type *ret_type;

		struct free_deleter
		{
			inline void operator()(unsigned char *ptr) noexcept
			{ std::free(ptr); }
		};

		std::unique_ptr<unsigned char[], free_deleter> ret_storage;
		std::vector<std::unique_ptr<unsigned char[], free_deleter>> args_storage;
		std::vector<void *> args_ptrs;

	private:
		cif() = delete;
		cif(const cif &) = delete;
		cif &operator=(const cif &) = delete;
		cif(cif &&) = delete;
		cif &operator=(cif &&) = delete;
	};

	class closure final : cif
	{
	public:
		inline closure(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: cif{ret, std::move(args)}
		{
		}

		~closure() noexcept override;

		using binding_func = void (*)(ffi_cif *closure_cif, void *ret, void *args[], void *userptr);

		template <typename U>
		bool initialize(ffi_abi abi, generic_plain_mfp_t &func, binding_func binding, U &userptr) noexcept
		{
			return initialize_impl(abi, reinterpret_cast<void **>(&func), binding, static_cast<void *>(&userptr));
		}

		template <typename R, typename U, typename ...Args>
		bool initialize(ffi_abi abi, R(*&func)(Args...), binding_func binding, U &userptr) noexcept
		{
			return initialize_impl(abi, reinterpret_cast<void **>(&func), binding, static_cast<void *>(&userptr));
		}

		template <typename R, typename C, typename U, typename ...Args>
		bool initialize(ffi_abi abi, R(C::*&func)(Args...), binding_func binding, U &userptr) noexcept
		{
			return initialize_impl(abi, reinterpret_cast<void **>(&func), binding, static_cast<void *>(&userptr));
		}

	private:
		bool initialize_impl(ffi_abi abi, void **func, binding_func binding, void *userptr) noexcept;

	private:
		ffi_closure *closure_impl{nullptr};

	private:
		closure() = delete;
		closure(const closure &) = delete;
		closure &operator=(const closure &) = delete;
		closure(closure &&) = delete;
		closure &operator=(closure &&) = delete;
	};
}
