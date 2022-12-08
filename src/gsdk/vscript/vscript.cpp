#include "vscript.hpp"

namespace gsdk
{
	ISquirrelMetamethodDelegate::~ISquirrelMetamethodDelegate() {}

	void *IScriptInstanceHelper::GetProxied(void *ptr)
	{ return ptr; }
	void *IScriptInstanceHelper::BindOnRead([[maybe_unused]] HSCRIPT instance, void *ptr, [[maybe_unused]] const char *)
	{ return ptr; }
}
