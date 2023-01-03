#pragma once

#include <string_view>
#include "../vscript/vscript.hpp"

namespace vmod::bindings
{
	class instance_base
	{
	public:
		instance_base() noexcept = default;
		virtual ~instance_base() noexcept;

		inline instance_base(instance_base &&other) noexcept
		{ operator=(std::move(other)); }
		inline instance_base &operator=(instance_base &&other) noexcept
		{
			instance = other.instance;
			other.instance = gsdk::INVALID_HSCRIPT;
			return *this;
		}

	protected:
		virtual bool register_instance(gsdk::ScriptClassDesc_t *target_desc, void *pthis) noexcept;
		bool register_instance(gsdk::ScriptClassDesc_t *) = delete;

	public:
		gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};

	private:
		instance_base(const instance_base &) = delete;
		instance_base &operator=(const instance_base &) = delete;
	};
}
