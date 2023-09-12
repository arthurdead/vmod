#pragma once

#include "../../vscript/vscript.hpp"
#include "../../vscript/class_desc.hpp"
#include "../mem/singleton.hpp"
#include "../instance.hpp"

namespace vmod::bindings::ent
{
	class serverclass final : public instance_base
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		~serverclass() noexcept override;

	private:
		static vscript::class_desc<serverclass> desc;

		inline serverclass(gsdk::ServerClass *cls_) noexcept
			: cls{cls_}
		{
		}

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }

		gsdk::ServerClass *cls;

	private:
		serverclass() = delete;
		serverclass(const serverclass &) = delete;
		serverclass &operator=(const serverclass &) = delete;
		serverclass(serverclass &&) = delete;
		serverclass &operator=(serverclass &&) = delete;
	};
}
