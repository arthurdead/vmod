#include "serverclass.hpp"

namespace vmod::bindings::ent
{
	vscript::class_desc<serverclass> serverclass::desc{"ent::serverclass"};

	serverclass::~serverclass() noexcept {}

	bool serverclass::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		if(!vm->RegisterClass(&desc)) {
			error("vmod: failed to register serverclass script class\n"sv);
			return false;
		}

		return true;
	}

	void serverclass::unbindings() noexcept
	{
		
	}
}
