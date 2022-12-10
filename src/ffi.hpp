#pragma once

#include <cstring>
#include <cstdint>
#include "vscript.hpp"
#include "hacking.hpp"
#include "plugin.hpp"

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

	static_assert(sizeof(ffi_type *) == sizeof(int));
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<ffi_type *>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, ffi_type *value) noexcept
	{ var.m_hScript = reinterpret_cast<gsdk::HSCRIPT>(value); }
	template <>
	inline ffi_type *variant_to_value<ffi_type *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			return reinterpret_cast<ffi_type *>(var.m_hScript);
		}

		return {};
	}

	extern bool ffi_bindings() noexcept;
	extern void ffi_unbindings() noexcept;

	struct memory_block final : public plugin::owned_instance
	{
	public:
		memory_block() = delete;
		memory_block(const memory_block &) = delete;
		memory_block &operator=(const memory_block &) = delete;
		memory_block(memory_block &&) = delete;
		memory_block &operator=(memory_block &&) = delete;

		~memory_block() noexcept override;

	private:
		friend class memory_singleton;

		static bool bindings() noexcept;
		static void unbindings() noexcept;

		bool initialize() noexcept;

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

		void script_set_dtor_func(gsdk::HSCRIPT func) noexcept;

		inline void script_disown() noexcept
		{ ptr = nullptr; }

		inline void script_delete() noexcept
		{ delete this; }

		inline std::uintptr_t script_ptr_as_int() noexcept
		{ return reinterpret_cast<std::uintptr_t>(ptr); }

		inline void *script_ptr() noexcept
		{ return ptr; }

		inline std::size_t script_get_size() const noexcept
		{ return size; }

		unsigned char *ptr;
		std::size_t size;
		gsdk::HSCRIPT dtor_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
	};

	class dynamic_detour final : public plugin::owned_instance
	{
	public:
		inline dynamic_detour(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: cif_{}, arg_type_ptrs{std::move(args)}, ret_type_ptr{ret}
		{
		}

		~dynamic_detour() noexcept override;

		bool initialize(generic_func_t old_func_, gsdk::HSCRIPT new_func_, ffi_abi abi) noexcept;
		bool initialize(generic_mfp_t old_func_, gsdk::HSCRIPT new_func_, ffi_abi abi) noexcept;

		void enable() noexcept;

		inline void disable() noexcept
		{
			unsigned char *bytes{reinterpret_cast<unsigned char *>(old_func)};
			std::memcpy(bytes, old_bytes, sizeof(old_bytes));
		}

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	private:
		friend class ffi_singleton;

		bool initialize_shared(gsdk::HSCRIPT new_func_, ffi_abi abi) noexcept;

		script_variant_t script_call(const script_variant_t *va_args, std::size_t num_args, ...) noexcept;

		static void closure_binding(ffi_cif *cif_, void *ret, void *args[], void *userptr) noexcept;

		inline void script_enable() noexcept
		{ enable(); }
		inline void script_disable() noexcept
		{ disable(); }
		inline void script_delete() noexcept
		{ delete this; }

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

		gsdk::HSCRIPT instance;

		gsdk::HSCRIPT new_func;

		ffi_cif cif_;

		std::vector<ffi_type *> arg_type_ptrs;
		ffi_type *ret_type_ptr;

		std::unique_ptr<unsigned char[]> ret_storage;
		std::vector<std::unique_ptr<unsigned char[]>> args_storage;
		std::vector<void *> args_storage_ptrs;

		ffi_closure *closure{nullptr};
		generic_func_t closure_func;

		union {
			generic_func_t old_func;
			generic_internal_mfp_t old_mfp;
		};

		unsigned char old_bytes[1 + sizeof(std::uintptr_t)];
	};
}
