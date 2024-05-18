#pragma once

#include <string_view>
#include "../vscript/vscript.hpp"
#include "../vscript/variant.hpp"

namespace vmod::bindings
{
	class instance_base
	{
	public:
		instance_base() noexcept = default;
		virtual inline ~instance_base() noexcept
		{ instance_.free(); }

		instance_base(instance_base &&other) noexcept = default;
		instance_base &operator=(instance_base &&other) noexcept = default;

		inline vscript::instance_handle_ref instance() noexcept
		{ return instance_; }

	protected:
		virtual bool register_instance(gsdk::ScriptClassDesc_t *target_desc, void *pthis) noexcept;
		bool register_instance(gsdk::ScriptClassDesc_t *) = delete;

	public:
		vscript::instance_handle_wrapper instance_{};

	private:
		instance_base(const instance_base &) = delete;
		instance_base &operator=(const instance_base &) = delete;
	};
}
