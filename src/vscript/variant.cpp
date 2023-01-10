#include "variant.hpp"

namespace vmod::vscript
{
	namespace detail
	{
		char variant_str_buffer[variant_str_buffer_max];
	}

	template <>
	bool to_value_impl<bool>(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		switch(var.m_type) {
			case gsdk::FIELD_FLOAT:
			return var.m_float > 0.0f;
			case gsdk::FIELD_FLOAT64:
			return var.m_double > 0.0;
			case gsdk::FIELD_STRING: {
				const char *ccstr{gsdk::vscript::STRING(var.m_tstr)};

				if(std::strcmp(ccstr, "true") == 0) {
					return true;
				} else if(std::strcmp(ccstr, "false") == 0) {
					return false;
				}

				const char *begin{ccstr};
				const char *end{ccstr + std::strlen(ccstr)};

				unsigned short ret;
				std::from_chars(begin, end, ret);
				return ret > 0;
			}
			case gsdk::FIELD_CSTRING: {
				if(std::strcmp(var.m_ccstr, "true") == 0) {
					return true;
				} else if(std::strcmp(var.m_ccstr, "false") == 0) {
					return false;
				}

				const char *begin{var.m_ccstr};
				const char *end{var.m_ccstr + std::strlen(var.m_ccstr)};

				unsigned short ret;
				std::from_chars(begin, end, ret);
				return ret > 0;
			}
			case gsdk::FIELD_CHARACTER:
			return var.m_char > 0;
			case gsdk::FIELD_SHORT:
			return var.m_short > 0.0f;
			case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
			case gsdk::FIELD_INTEGER:
			return var.m_int > 0;
			case gsdk::FIELD_UINT32:
			return var.m_uint > 0;
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			case gsdk::FIELD_INTEGER64:
			return var.m_longlong > 0;
		#endif
			case gsdk::FIELD_UINT64:
			return var.m_ulonglong > 0;
			case gsdk::FIELD_BOOLEAN:
			return var.m_bool;
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT:
			return detail::to_bool(var.m_object);
			default: return {};
		}
	}

	void initialize_impl(gsdk::ScriptVariant_t &var, std::string &&value) noexcept
	{
		if(!value.empty()) {
			std::size_t len{value.length()};
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
			var.m_cstr = static_cast<char *>(std::malloc(len+1));
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			var.m_cstr = new char[len+1];
		#else
			#error
		#endif
			std::strncpy(var.m_cstr, value.c_str(), len);
			var.m_cstr[len] = '\0';
			var.m_flags |= gsdk::SV_FREE;
		} else {
			var.m_ccstr = "";
		}
	}

	void initialize_impl(gsdk::ScriptVariant_t &var, std::filesystem::path &&value) noexcept
	{
		if(!value.empty()) {
			std::size_t len{value.native().length()};
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
			var.m_cstr = static_cast<char *>(std::malloc(len+1));
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			var.m_cstr = new char[len+1];
		#else
			#error
		#endif
			std::strncpy(var.m_cstr, value.c_str(), len);
			var.m_cstr[len] = '\0';
			var.m_flags |= gsdk::SV_FREE;
		} else {
			var.m_ccstr = "";
		}
	}
}
