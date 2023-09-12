#pragma once

#include "../../hacking.hpp"
#include "../../ffi.hpp"
#include "../../vscript/vscript.hpp"
#include "../../vscript/variant.hpp"
#include "../../vscript/singleton_class_desc.hpp"
#include "../singleton.hpp"
#include <cstddef>
#include <ffi.h>
#include <unordered_map>
#include <string>
#include <string_view>

namespace vmod::bindings::mem
{
	class singleton final : public singleton_base
	{
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		inline singleton() noexcept
			: singleton_base{"mem"}
		{
		}

		~singleton() noexcept override;

		bool bindings() noexcept;
		void unbindings() noexcept;

		static singleton &instance() noexcept;

		struct type final
		{
			friend class singleton;

		public:
			~type() noexcept;

			inline type(type &&other) noexcept
			{ operator=(std::move(other)); }

			inline type &operator=(type &&other) noexcept
			{
				ptr = other.ptr;
				other.ptr = nullptr;
				name_ = std::move(other.name_);
				table_ = std::move(other.table_);
				return *this;
			}

			inline const std::string &name() const noexcept
			{ return name_; }

			inline vscript::handle_ref table() noexcept
			{ return table_; }

		private:
			inline type(std::string_view name__, ffi_type *type_, vscript::handle_wrapper &&table__) noexcept
				: ptr{type_}, name_{name__}, table_{std::move(table__)}
			{
			}

			ffi_type *ptr;
			std::string name_;
			vscript::handle_wrapper table_{};

		private:
			type() noexcept = delete;
			type(const type &) = delete;
			type &operator=(const type &) = delete;
		};

		type *find_type(ffi_type *ptr) noexcept;

		static ffi_type *read_type(vscript::handle_ref type_table) noexcept;

	private:
		static vscript::singleton_class_desc<singleton> desc;

		static vscript::handle_ref script_allocate(std::size_t size) noexcept;
		static vscript::handle_ref script_allocate_ent(std::size_t size) noexcept;
		static vscript::handle_ref script_allocate_aligned(std::align_val_t align, std::size_t size) noexcept;
		static vscript::handle_ref script_allocate_type(vscript::handle_wrapper type) noexcept;
		static vscript::handle_ref script_allocate_zero(std::size_t num, std::size_t size) noexcept;

		static vscript::variant script_read(unsigned char *ptr, vscript::handle_wrapper type_table) noexcept;
		static void script_write(unsigned char *ptr, vscript::handle_wrapper type_table, const vscript::variant &var) noexcept;

		static unsigned char *script_add(unsigned char *ptr, std::ptrdiff_t off) noexcept;
		static unsigned char *script_sub(unsigned char *ptr, std::ptrdiff_t off) noexcept;

		static generic_vtable_t script_get_vtable(generic_object_t *obj) noexcept;
		static generic_plain_mfp_t script_get_vfunc(generic_object_t *obj, std::size_t index) noexcept;

		bool register_type(ffi_type *ptr, std::string_view type_name) noexcept;

		std::vector<std::unique_ptr<type>> types;
		vscript::handle_wrapper types_table{};

	private:
		singleton(const singleton &) = delete;
		singleton &operator=(const singleton &) = delete;
		singleton(singleton &&) = delete;
		singleton &operator=(singleton &&) = delete;
	};
}
