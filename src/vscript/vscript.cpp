#include "vscript.hpp"
#include "../main.hpp"

namespace vmod::vscript
{
	gsdk::IScriptVM *vm() noexcept
	{ return main::instance().vm(); }

	namespace detail
	{
		bool to_bool(gsdk::HSCRIPT object) noexcept
		{ return main::instance().to_bool(object); }
		float to_float(gsdk::HSCRIPT object) noexcept
		{ return main::instance().to_float(object); }
		int to_int(gsdk::HSCRIPT object) noexcept
		{ return main::instance().to_int(object); }
		std::string_view to_string(gsdk::HSCRIPT object) noexcept
		{ return main::instance().to_string(object); }
		std::string_view type_of(gsdk::HSCRIPT object) noexcept
		{ return main::instance().type_of(object); }
		bool get_scalar(gsdk::HSCRIPT object, gsdk::ScriptVariant_t *var) noexcept
		{ return vm()->GetScalarValue(object, var); }

		__attribute__((__format__(__printf__, 1, 2))) bool raise_exception(const char *fmt, ...) noexcept
		{
			va_list vargs;
			va_start(vargs, fmt);
			bool ret{vm()->RaiseExceptionv(fmt, vargs)};
			va_end(vargs);
			return ret;
		}
	}
}
