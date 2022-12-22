#pragma once

#include <cstring>
#include <cstdint>
#include "vscript.hpp"
#include "hacking.hpp"
#include "plugin.hpp"

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

namespace vmod
{
	extern ffi_type ffi_type_vector;
	extern ffi_type ffi_type_qangle;
	extern ffi_type ffi_type_color32;
	extern ffi_type ffi_type_ehandle;
	extern ffi_type ffi_type_bool;
	extern ffi_type ffi_type_cstr;

	extern void script_var_to_ptr(ffi_type *type_ptr, void *arg_ptr, const script_variant_t &arg_var) noexcept;
	extern void ptr_to_script_var(ffi_type *type_ptr, void *arg_ptr, script_variant_t &arg_var) noexcept;
	extern ffi_type *ffi_type_id_to_ptr(int id) noexcept;

	extern bool ffi_bindings() noexcept;
	extern void ffi_unbindings() noexcept;

	class memory_singleton final : public gsdk::ISquirrelMetamethodDelegate
	{
	public:
		bool bindings() noexcept;

		static memory_singleton &instance() noexcept;

		void unbindings() noexcept;

	private:
		friend class vmod;

		static gsdk::HSCRIPT script_allocate(std::size_t size) noexcept;

		static gsdk::HSCRIPT script_allocate_aligned(std::size_t align, std::size_t size) noexcept;

		static gsdk::HSCRIPT script_allocate_type(gsdk::HSCRIPT type) noexcept;

		static gsdk::HSCRIPT script_allocate_zero(std::size_t num, std::size_t size) noexcept;

		static script_variant_t script_read(void *ptr, gsdk::HSCRIPT type) noexcept;

		static void script_write(void *ptr, gsdk::HSCRIPT type, script_variant_t arg_var) noexcept;

		static inline void *script_add(void *ptr, std::ptrdiff_t off) noexcept
		{
			unsigned char *temp{reinterpret_cast<unsigned char *>(ptr)};
			temp += off;
			return reinterpret_cast<void *>(temp);
		}

		static inline void *script_sub(void *ptr, std::ptrdiff_t off) noexcept
		{
			unsigned char *temp{reinterpret_cast<unsigned char *>(ptr)};
			temp -= off;
			return reinterpret_cast<void *>(temp);
		}

		static inline generic_vtable_t script_get_vtable(generic_object_t *obj) noexcept
		{ return vtable_from_object(obj); }

		static inline generic_plain_mfp_t script_get_vfunc(generic_object_t *obj, std::size_t index) noexcept
		{
			generic_plain_mfp_t *vtable{vtable_from_object<generic_object_t>(obj)};
			generic_plain_mfp_t vfunc{vtable[index]};
			return vfunc;
		}

		bool Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value) override;

		struct mem_type final
		{
			mem_type() noexcept = default;
			mem_type(const mem_type &) = delete;
			mem_type &operator=(const mem_type &) = delete;
			inline mem_type(mem_type &&other) noexcept
			{ operator=(std::move(other)); }
			inline mem_type &operator=(mem_type &&other) noexcept
			{
				type_ptr = other.type_ptr;
				other.type_ptr = nullptr;
				name = std::move(other.name);
				table = other.table;
				other.table = gsdk::INVALID_HSCRIPT;
				return *this;
			}

			~mem_type() noexcept;

			ffi_type *type_ptr;
			std::string name;
			gsdk::HSCRIPT table;
		};

		bool register_type(ffi_type *type_ptr, std::string_view name) noexcept;

		std::vector<mem_type> types;
		gsdk::HSCRIPT scope{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT types_table{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT vs_instance_{gsdk::INVALID_HSCRIPT};
		gsdk::CSquirrelMetamethodDelegateImpl *get_impl{nullptr};
	};

	extern class memory_singleton memory_singleton;

	extern singleton_class_desc_t<class memory_singleton> memory_desc;

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

		void *script_release() noexcept;

		inline void *script_ptr() noexcept
		{ return ptr; }

		inline std::size_t script_get_size() const noexcept
		{ return size; }

		unsigned char *ptr;
		std::size_t size;
		gsdk::HSCRIPT dtor_func{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
	};

	extern class_desc_t<memory_block> mem_block_desc;

	struct cif
	{
		inline cif(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: cif_{}, arg_type_ptrs{std::move(args)}, ret_type_ptr{ret}
		{
		}

		~cif() noexcept;

		bool initialize(ffi_abi abi) noexcept;

		inline ffi_cif *operator&() noexcept
		{ return &cif_; }

		ffi_cif cif_;

		std::vector<ffi_type *> arg_type_ptrs;
		ffi_type *ret_type_ptr;

		struct free_deleter
		{
			inline void operator()(unsigned char *ptr) noexcept
			{ std::free(ptr); }
		};

		std::unique_ptr<unsigned char[], free_deleter> ret_storage;
		std::vector<std::unique_ptr<unsigned char[], free_deleter>> args_storage;
		std::vector<void *> args_storage_ptrs;
	};

	class dynamic_detour final : public cif, public plugin::owned_instance
	{
	public:
		inline dynamic_detour(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: cif{ret, std::move(args)}
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

		ffi_closure *closure{nullptr};
		generic_func_t closure_func;

		union {
			generic_func_t old_func;
			generic_internal_mfp_t old_mfp;
		};

		unsigned char old_bytes[1 + sizeof(std::uintptr_t)];
	};

	extern class_desc_t<class dynamic_detour> detour_desc;

	class script_cif final : public cif, public plugin::owned_instance
	{
	public:
		~script_cif() noexcept override;

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	private:
		friend class ffi_singleton;

		inline script_cif(ffi_type *ret, std::vector<ffi_type *> &&args) noexcept
			: cif{ret, std::move(args)}
		{
		}

		bool initialize(ffi_abi abi) noexcept;

		script_variant_t script_call(const script_variant_t *va_args, std::size_t num_args, ...) noexcept;

		void script_set_func(generic_func_t func_) noexcept;
		void script_set_mfp(generic_mfp_t func_) noexcept;

		union {
			generic_func_t func;
			generic_internal_mfp_t mfp;
		};
		gsdk::HSCRIPT instance;
	};

	extern class_desc_t<script_cif> cif_desc;

	class ffi_singleton final : public gsdk::ISquirrelMetamethodDelegate
	{
	public:
		inline ~ffi_singleton() noexcept override
		{
		}

		bool bindings() noexcept;
		void unbindings() noexcept;

		static ffi_singleton &instance() noexcept;

	private:
		friend class vmod;

		static bool script_create_cif_shared(std::vector<ffi_type *> &args_ptrs, gsdk::HSCRIPT args) noexcept;
		static gsdk::HSCRIPT script_create_static_cif(ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept;
		static gsdk::HSCRIPT script_create_member_cif(ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept;

		static dynamic_detour *script_create_detour_shared(ffi_type *ret, gsdk::HSCRIPT args) noexcept;
		static gsdk::HSCRIPT script_create_detour_member(generic_mfp_t old_func, gsdk::HSCRIPT new_func, ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept;
		static gsdk::HSCRIPT script_create_detour_static(generic_func_t old_func, gsdk::HSCRIPT new_func, ffi_abi abi, ffi_type *ret, gsdk::HSCRIPT args) noexcept;

		bool Get(const gsdk::CUtlString &name, gsdk::ScriptVariant_t &value) override;

		gsdk::HSCRIPT scope{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT types_table{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT abi_table{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT vs_instance_{gsdk::INVALID_HSCRIPT};
		gsdk::CSquirrelMetamethodDelegateImpl *get_impl{nullptr};
	};

	extern class ffi_singleton ffi_singleton;

	extern singleton_class_desc_t<class ffi_singleton> ffi_singleton_desc;
}
