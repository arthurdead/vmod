#pragma once

#define __VMOD_VSCRIPT_HEADER_INCLUDED

#include "../gsdk/vscript/vscript.hpp"

namespace vmod::vscript
{
	namespace detail
	{
		extern bool to_bool(gsdk::HSCRIPT object) noexcept;
		extern float to_float(gsdk::HSCRIPT object) noexcept;
		extern int to_int(gsdk::HSCRIPT object) noexcept;
		extern std::string_view to_string(gsdk::HSCRIPT object) noexcept;
		extern std::string_view type_of(gsdk::HSCRIPT object) noexcept;
		extern bool get_scalar(gsdk::HSCRIPT object, gsdk::ScriptVariant_t *var) noexcept;
		extern __attribute__((__format__(__printf__, 1, 2))) bool raise_exception(const char *fmt, ...) noexcept;
	}

	extern gsdk::IScriptVM *vm() noexcept;
}
