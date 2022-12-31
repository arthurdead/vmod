#include "vscript.hpp"
#include "../main.hpp"

namespace vmod::vscript::detail
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
	void raise_exception(const char *str) noexcept
	{ main::instance().vm()->RaiseException(str); }
	bool get_scalar(gsdk::HSCRIPT object, gsdk::ScriptVariant_t *var) noexcept
	{ return main::instance().vm()->GetScalarValue(object, var); }
}
