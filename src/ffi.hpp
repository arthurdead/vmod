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

extern ffi_type ffi_type_vector;
extern ffi_type ffi_type_qangle;
extern ffi_type ffi_type_color32;
extern ffi_type ffi_type_ehandle;
extern ffi_type ffi_type_bool;
extern ffi_type ffi_type_cstr;

namespace vmod::ffi
{
	extern void script_var_to_ptr(const vscript::variant &var, void *ptr, ffi_type *type) noexcept;
	extern void ptr_to_script_var(void *ptr, ffi_type *type, gsdk::ScriptVariant_t &var) noexcept;
	extern ffi_type *type_id_to_ptr(int id) noexcept;
	extern short to_script_type(ffi_type *type);

	class cif
	{
	public:
		inline cif(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: args_types{std::move(args)}, ret_type{ret}
		{
		}

		bool initialize(ffi_abi abi) noexcept;

		inline ffi_cif *operator&() noexcept
		{ return &impl; }

	protected:
		void call(void(*func)()) noexcept;

		ffi_cif impl;

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
}
