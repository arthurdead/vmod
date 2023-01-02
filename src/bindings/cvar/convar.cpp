#include "convar.hpp"
#include "../../main.hpp"
#include "../docs.hpp"

namespace vmod::bindings::cvar
{
	vscript::class_desc<convar> convar::desc{"cvar::convar"};

	bool convar::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&convar::script_set, "script_set"sv, "set"sv)
		.desc("(value)"sv);

		desc.func(&convar::script_set_string, "script_set_string"sv, "set_string"sv)
		.desc("(value)"sv);

		desc.func(&convar::script_set_float, "script_set_float"sv, "set_float"sv)
		.desc("(value)"sv);

		desc.func(&convar::script_set_int, "script_set_int"sv, "set_int"sv)
		.desc("(value)"sv);

		desc.func(&convar::script_set_bool, "script_set_bool"sv, "set_bool"sv)
		.desc("(value)"sv);

		desc.func(&convar::script_get, "script_get"sv, "get"sv);
		desc.func(&convar::script_get_string, "script_get_string"sv, "string"sv);
		desc.func(&convar::script_get_float, "script_get_float"sv, "float"sv);
		desc.func(&convar::script_get_int, "script_get_int"sv, "int"sv);
		desc.func(&convar::script_get_bool, "script_get_bool"sv, "bool"sv);

		if(!plugin::owned_instance::register_class(&desc)) {
			error("vmod: failed to register cvar convar class\n"sv);
			return false;
		}

		return true;
	}

	void convar::unbindings() noexcept
	{
		
	}

	convar::~convar() noexcept
	{
		if(free) {
			delete var;
		}
	}

	void convar::script_set(vscript::variant &&value) noexcept
	{
		switch(value.m_type) {
			case gsdk::FIELD_CSTRING:
			var->ConVar::SetValue(value.m_ccstr);
			break;
			case gsdk::FIELD_INTEGER:
			var->ConVar::SetValue(value.m_int);
			break;
			case gsdk::FIELD_FLOAT:
			var->ConVar::SetValue(value.m_float);
			break;
			case gsdk::FIELD_BOOLEAN:
			var->ConVar::SetValue(value.m_bool);
			break;
			default:
			main::instance().vm()->RaiseException("vmod: invalid type: '%s'", bindings::docs::type_name(value.m_type).data());
			break;
		}
	}

	vscript::variant convar::script_get() const noexcept
	{
		const char *str{var->ConVar::InternalGetString()};
		std::size_t len{var->ConVar::InternalGetStringLength()};

		if(std::strncmp(str, "true", len) == 0) {
			return true;
		} else if(std::strncmp(str, "false", len) == 0) {
			return false;
		} else {
			bool is_float{false};

			for(std::size_t i{0}; i < len; ++i) {
				if(str[i] == '.') {
					is_float = true;
					continue;
				}

				if(!std::isdigit(str[i])) {
					return std::string_view{str, len};
				}
			}

			if(is_float) {
				return var->ConVar::GetFloat();
			} else {
				return var->ConVar::GetInt();
			}
		}
	}
}
