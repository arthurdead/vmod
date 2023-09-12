#include "../../mod.hpp"

namespace vmod
{
	vscript::class_desc<mod> mod::desc{"mod"};

	bool mod::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register mod script class\n"sv);
			return false;
		}

		return true;
	}

	void mod::unbindings() noexcept
	{
	}
}
