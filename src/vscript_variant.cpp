#include "vscript.hpp"

namespace vmod
{
	char __vscript_variant_to_value_buffer[__vscript_max_strsiz];

	template <>
	bool variant_to_value<bool>(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		switch(var.m_type) {
			case gsdk::FIELD_INTERVAL:
			case gsdk::FIELD_FLOAT: {
				return var.m_float > 0.0f;
			}
			case gsdk::FIELD_DOUBLE: {
				return var.m_double > 0.0;
			}
			case gsdk::FIELD_STRING: {
				const char *m_pszString{gsdk::STRING(var.m_tstring)};

				if(std::strcmp(m_pszString, "true") == 0) {
					return true;
				} else if(std::strcmp(m_pszString, "false") == 0) {
					return false;
				}

				const char *begin{m_pszString};
				const char *end{m_pszString + std::strlen(m_pszString)};

				unsigned short ret;
				std::from_chars(begin, end, ret);
				return ret > 0;
			}
			case gsdk::FIELD_CSTRING: {
				if(std::strcmp(var.m_pszString, "true") == 0) {
					return true;
				} else if(std::strcmp(var.m_pszString, "false") == 0) {
					return false;
				}

				const char *begin{var.m_pszString};
				const char *end{var.m_pszString + std::strlen(var.m_pszString)};

				unsigned short ret;
				std::from_chars(begin, end, ret);
				return ret > 0;
			}
			case gsdk::FIELD_CHARACTER: {
				return var.m_char > 0;
			}
			case gsdk::FIELD_SHORT: {
				return var.m_short > 0.0f;
			}
			case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
			case gsdk::FIELD_INTEGER: {
				return var.m_int > 0;
			}
			case gsdk::FIELD_UINT: {
				return var.m_uint > 0;
			}
			case gsdk::FIELD_INTEGER64: {
				return var.m_longlong > 0;
			}
			case gsdk::FIELD_UINT64: {
				return var.m_ulonglong > 0;
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool;
			}
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT: {
				return __vmod_to_bool(var.m_hScript);
			}
			default: return {};
		}
	}

	void initialize_variant_value(gsdk::ScriptVariant_t &var, std::string &&value) noexcept
	{
		if(!value.empty()) {
			std::size_t len{value.length()};
			var.m_pszString = new char[len+1];
			std::strncpy(const_cast<char *>(var.m_pszString), value.c_str(), len);
			const_cast<char *>(var.m_pszString)[len] = '\0';
			var.m_flags |= gsdk::SV_FREE;
		} else {
			var.m_pszString = "";
		}
	}

	void initialize_variant_value(gsdk::ScriptVariant_t &var, std::filesystem::path &&value) noexcept
	{
		if(!value.empty()) {
			std::size_t len{value.native().length()};
			var.m_pszString = new char[len+1];
			std::strncpy(const_cast<char *>(var.m_pszString), value.c_str(), len);
			const_cast<char *>(var.m_pszString)[len] = '\0';
			var.m_flags |= gsdk::SV_FREE;
		} else {
			var.m_pszString = "";
		}
	}
}
