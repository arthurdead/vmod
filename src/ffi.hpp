#pragma once

#include <cstring>
#include <cstdint>
#include "vscript.hpp"
#include "hacking.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-W#warnings"
#include <ffi.h>
#pragma GCC diagnostic pop

namespace vmod
{
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<ffi_abi>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, ffi_abi value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline ffi_abi variant_to_value<ffi_abi>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			return static_cast<ffi_abi>(var.m_int);
		}

		return {};
	}

	extern bool ffi_bindings() noexcept;
	extern void ffi_unbindings() noexcept;

	struct memory_block final
	{
	public:
		memory_block() = delete;
		memory_block(const memory_block &) = delete;
		memory_block &operator=(const memory_block &) = delete;
		memory_block(memory_block &&) = delete;
		memory_block &operator=(memory_block &&) = delete;

		~memory_block() noexcept;

	private:
		friend bool ffi_bindings() noexcept;
		friend void ffi_unbindings() noexcept;

		static bool bindings() noexcept;
		static void unbindings() noexcept;

		bool register_instance() noexcept;

		inline memory_block(std::size_t size_) noexcept
			: ptr{static_cast<unsigned char *>(std::malloc(size_))}, size{size_}
		{
		}

		inline memory_block(std::align_val_t align, std::size_t size_) noexcept
			: ptr{static_cast<unsigned char *>(std::aligned_alloc(static_cast<std::size_t>(align), size_))}, size{size_}
		{
		}

		inline memory_block(std::size_t num, std::size_t size_) noexcept
			: ptr{static_cast<unsigned char *>(std::calloc(num, size_))}, size{num * size_}
		{
		}

		static gsdk::HSCRIPT script_allocate(std::size_t size) noexcept;
		static gsdk::HSCRIPT script_allocate_aligned(std::size_t align, std::size_t size) noexcept;
		static gsdk::HSCRIPT script_allocate_zero(std::size_t num, std::size_t size) noexcept;

		inline void script_set_dtor_func(gsdk::HSCRIPT func) noexcept
		{
			dtor_func = func;
		}

		inline void script_disown() noexcept
		{ ptr = nullptr; }

		inline void script_delete() noexcept
		{ delete this; }

		inline std::uintptr_t script_ptr_as_int() noexcept
		{ return reinterpret_cast<std::uintptr_t>(ptr); }

		inline std::size_t script_get_size() const noexcept
		{ return size; }

		unsigned char *ptr;
		std::size_t size;
		gsdk::HSCRIPT dtor_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
	};

	class dynamic_detour
	{
	public:
		inline dynamic_detour() noexcept
		{
		}

		virtual ~dynamic_detour() noexcept;

		bool initialize(generic_func_t old_func_, const ffi_type *ret = nullptr, const std::vector<ffi_type *> &args = {}) noexcept
		{
			closure = static_cast<ffi_closure *>(ffi_closure_alloc(sizeof(ffi_closure), reinterpret_cast<void **>(&closure_func)));
			if(!closure) {
				return false;
			}

			if(ffi_prep_cif(&cif_, FFI_SYSV, args.empty() ? 0 : args.size(), ret ? const_cast<ffi_type *>(ret) : &ffi_type_void, args.empty() ? nullptr : const_cast<ffi_type **>(args.data())) != FFI_OK) {
				return false;
			}

			if(ffi_prep_closure_loc(closure, &cif_, closure_binding, this, reinterpret_cast<void *>(closure_func)) != FFI_OK) {
				return false;
			}

			old_mfp.addr = reinterpret_cast<generic_plain_mfp_t>(old_func_);
			old_mfp.adjustor = 0;

			backup_bytes();

			return true;
		}

		void enable() noexcept;

		inline void disable() noexcept
		{
			unsigned char *bytes{reinterpret_cast<unsigned char *>(old_func)};
			std::memcpy(bytes, old_bytes, sizeof(old_bytes));
		}

	protected:
		inline void call(void *ret, void *args[]) noexcept
		{
			scope_enable se{*this};
			ffi_call(&cif_, reinterpret_cast<void(*)()>(old_func), ret, args);
		}

	private:
		virtual void handle_detour(void *ret, void *args[]) noexcept = 0;

		static void closure_binding(ffi_cif *cif_, void *ret, void *args[], void *userptr) noexcept
		{ reinterpret_cast<dynamic_detour *>(userptr)->handle_detour(ret, args); }

		struct scope_enable final {
			inline scope_enable(dynamic_detour &det_) noexcept
				: det{det_} {
				det.disable();
			}
			inline ~scope_enable() noexcept {
				det.enable();
			}
			dynamic_detour &det;
		};

		void backup_bytes() noexcept;

		ffi_cif cif_;

		ffi_closure *closure{nullptr};
		generic_func_t closure_func;

		union {
			generic_func_t old_func;
			generic_internal_mfp_t old_mfp;
		};

		unsigned char old_bytes[1 + sizeof(std::uintptr_t)];
	};
}
