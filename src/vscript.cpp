#include "vscript.hpp"
#include "vmod.hpp"

namespace vmod
{
	void free_variant_hscript(gsdk::ScriptVariant_t &value) noexcept
	{ vmod.vm()->ReleaseObject(value.m_hScript); }
}
