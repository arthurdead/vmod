#pragma once

#include "../../hacking.hpp"
#include "../../ffi.hpp"
#include "../../vscript/vscript.hpp"
#include "../../vscript/variant.hpp"
#include "../../vscript/singleton_class_desc.hpp"
#include "../singleton.hpp"
#include <cstddef>
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
				table_ = other.table_;
				other.table_ = gsdk::INVALID_HSCRIPT;
				return *this;
			}

			inline const std::string &name() const noexcept
			{ return name_; }

			inline gsdk::HSCRIPT table() noexcept
			{ return table_; }

		private:
			inline type(std::string_view name__, ffi_type *type_, gsdk::HSCRIPT table__) noexcept
				: ptr{type_}, name_{name__}, table_{table__}
			{
			}

			ffi_type *ptr;
			std::string name_;
			gsdk::HSCRIPT table_;

		private:
			type() noexcept = delete;
			type(const type &) = delete;
			type &operator=(const type &) = delete;
		};

		type *find_type(ffi_type *ptr) noexcept;

	private:
		static vscript::singleton_class_desc<singleton> desc;

		static gsdk::HSCRIPT script_allocate(std::size_t size) noexcept;
		static gsdk::HSCRIPT script_allocate_aligned(std::align_val_t align, std::size_t size) noexcept;
		static gsdk::HSCRIPT script_allocate_type(gsdk::HSCRIPT type) noexcept;
		static gsdk::HSCRIPT script_allocate_zero(std::size_t num, std::size_t size) noexcept;

		static gsdk::ScriptVariant_t script_read(unsigned char *ptr, gsdk::HSCRIPT type_table) noexcept;
		static void script_write(unsigned char *ptr, gsdk::HSCRIPT type_table, const vscript::variant &var) noexcept;

		static unsigned char *script_add(unsigned char *ptr, std::ptrdiff_t off) noexcept;
		static unsigned char *script_sub(unsigned char *ptr, std::ptrdiff_t off) noexcept;

		static generic_vtable_t script_get_vtable(generic_object_t *obj) noexcept;
		static generic_plain_mfp_t script_get_vfunc(generic_object_t *obj, std::size_t index) noexcept;

		bool register_type(ffi_type *ptr, std::string_view type_name) noexcept;

		std::unordered_map<ffi_type *, std::unique_ptr<type>> types;
		gsdk::HSCRIPT types_table{gsdk::INVALID_HSCRIPT};

	private:
		singleton(const singleton &) = delete;
		singleton &operator=(const singleton &) = delete;
		singleton(singleton &&) = delete;
		singleton &operator=(singleton &&) = delete;
	};
}
