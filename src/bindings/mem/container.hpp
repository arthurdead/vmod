#pragma once

#include <cstddef>
#include <cstdlib>
#include "../../vscript/vscript.hpp"
#include "../../vscript/class_desc.hpp"
#include "../../plugin.hpp"

namespace vmod::bindings::mem
{
	class singleton;

	class container final : public plugin::owned_instance
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		~container() noexcept override;

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	private:
		static vscript::class_desc<container> desc;

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }

		enum class type : unsigned char
		{
			normal,
			entity,
		#ifndef GSDK_NO_ALLOC_OVERRIDE
			game,
		#endif
		};

		container(std::size_t size_, type type_) noexcept;
		container(std::align_val_t align, std::size_t size_, bool game) noexcept;
		container(std::size_t num, std::size_t size_, bool game) noexcept;

		void script_set_free_callback(gsdk::HSCRIPT func) noexcept;

		unsigned char *script_release() noexcept;

		inline unsigned char *script_ptr() noexcept
		{ return ptr; }

		inline std::size_t script_size() const noexcept
		{ return size; }

		type type{type::normal};
		unsigned char *ptr{nullptr};
		std::size_t size{0};
		bool aligned{false};
		gsdk::HSCRIPT free_callback{gsdk::INVALID_HSCRIPT};

	private:
		container() = delete;
		container(const container &) = delete;
		container &operator=(const container &) = delete;
		container(container &&) = delete;
		container &operator=(container &&) = delete;
	};
}
