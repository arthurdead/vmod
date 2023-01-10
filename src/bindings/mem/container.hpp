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

		inline container(std::size_t size_, bool ent_) noexcept
			: ent{ent_}, size{size_}
		{
			if(ent_) {
				ptr = static_cast<unsigned char *>(sv_engine->PvAllocEntPrivateData(static_cast<long>(size_)));
			} else {
				ptr = static_cast<unsigned char *>(std::malloc(size_));
			}
		}

		inline container(std::align_val_t align, std::size_t size_) noexcept
			: ptr{static_cast<unsigned char *>(std::aligned_alloc(static_cast<std::size_t>(align), size_))}, size{size_}
		{
		}

		inline container(std::size_t num, std::size_t size_) noexcept
			: ptr{static_cast<unsigned char *>(std::calloc(num, size_))}, size{num * size_}
		{
		}

		void script_set_free_callback(gsdk::HSCRIPT func) noexcept;

		unsigned char *script_release() noexcept;

		inline unsigned char *script_ptr() noexcept
		{ return ptr; }

		inline std::size_t script_size() const noexcept
		{ return size; }

		bool ent{false};
		unsigned char *ptr;
		std::size_t size;
		gsdk::HSCRIPT free_callback{gsdk::INVALID_HSCRIPT};

	private:
		container() = delete;
		container(const container &) = delete;
		container &operator=(const container &) = delete;
		container(container &&) = delete;
		container &operator=(container &&) = delete;
	};
}
