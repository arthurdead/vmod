#pragma once

#include "../../vscript/vscript.hpp"
#include "../../vscript/singleton_class_desc.hpp"
#include "../singleton.hpp"
#include <string_view>

namespace vmod::bindings::cvar
{
	class singleton final : public singleton_base
	{
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		inline singleton() noexcept
			: singleton_base{"cvar"}
		{
		}

		~singleton() noexcept override;

		bool bindings() noexcept;
		void unbindings() noexcept;

		static singleton &instance() noexcept;

	private:
		static vscript::singleton_class_desc<singleton> desc;

		static gsdk::HSCRIPT script_create_cvar(std::string_view name, std::string_view value) noexcept;
		static gsdk::HSCRIPT script_find_cvar(std::string_view name) noexcept;

		gsdk::HSCRIPT flags_table{gsdk::INVALID_HSCRIPT};

	private:
		singleton(const singleton &) = delete;
		singleton &operator=(const singleton &) = delete;
		singleton(singleton &&) = delete;
		singleton &operator=(singleton &&) = delete;
	};
}
