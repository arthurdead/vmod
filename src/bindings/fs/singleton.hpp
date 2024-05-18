#pragma once

#include <cstddef>
#include "../../vscript/vscript.hpp"
#include "../../vscript/variant.hpp"
#include "../../vscript/singleton_class_desc.hpp"
#include "../singleton.hpp"
#include <filesystem>

namespace vmod::bindings::fs
{
	class singleton final : public singleton_base
	{
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		inline singleton() noexcept
			: singleton_base{"fs"}
		{
		}

		~singleton() noexcept override;

		bool bindings() noexcept;
		void unbindings() noexcept;

		static singleton &instance() noexcept;

	private:
		static vscript::singleton_class_desc<singleton> desc;

		static int script_globerr(const char *epath, int eerrno) noexcept;

		static vscript::array_handle_wrapper script_glob(const std::filesystem::path &pattern) noexcept;
		static std::filesystem::path script_join_paths(const vscript::variant *args, std::size_t num_args, ...) noexcept;
	};
}
