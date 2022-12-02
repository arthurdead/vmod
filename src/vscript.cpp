#include "vscript.hpp"
#include "vmod.hpp"

namespace vmod
{
	void free_variant_hscript(gsdk::ScriptVariant_t &value) noexcept
	{ vm->ReleaseValue(value); }
}
