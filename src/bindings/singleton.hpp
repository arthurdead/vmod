#pragma once

#include <string_view>
#include "../vscript/vscript.hpp"
#include "../vscript/singleton_class_desc.hpp"

namespace vmod::bindings
{
	class singleton_base : public gsdk::ISquirrelMetamethodDelegate
	{
	public:
		bool create_get() noexcept;

	protected:
		inline singleton_base(std::string_view name_, bool root_ = false) noexcept
			: name{name_}, root{root_}
		{
		}

		bool bindings(gsdk::ScriptClassDesc_t *desc) noexcept;
		void unbindings() noexcept;

	private:
		bool Get(const gsdk::CUtlConstString &name, gsdk::ScriptVariant_t &value) override final;

	protected:
		gsdk::HSCRIPT scope{gsdk::INVALID_HSCRIPT};

	private:
		gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};
		gsdk::CSquirrelMetamethodDelegateImpl *get_impl{nullptr};

		std::string_view name;
		bool root;

	private:
		singleton_base() = delete;
		singleton_base(const singleton_base &) = delete;
		singleton_base &operator=(const singleton_base &) = delete;
		singleton_base(singleton_base &&) = delete;
		singleton_base &operator=(singleton_base &&) = delete;
	};
}
