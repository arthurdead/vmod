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
	bool get_scalar(gsdk::HSCRIPT object, gsdk::ScriptVariant_t *var) noexcept
	{
	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		return main::instance().vm()->GetScalarValue(object, var);
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		switch(object->_type) {
			case OT_NULL: {
				var->m_type = gsdk::FIELD_VOID;
				std::memset(var->m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));
				var->m_object = gsdk::INVALID_HSCRIPT;
				return true;
			}
			case OT_INTEGER: {
				var->m_type = gsdk::FIELD_INTEGER;
				std::memset(var->m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));
				var->m_int = object->_unVal.nInteger;
				return true;
			}
			case OT_FLOAT: {
				var->m_type = gsdk::FIELD_FLOAT;
				std::memset(var->m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));
				var->m_float = object->_unVal.fFloat;
				return true;
			}
			case OT_BOOL: {
				var->m_type = gsdk::FIELD_BOOLEAN;
				std::memset(var->m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));
				var->m_bool = (object->_unVal.nInteger > 0 ? true : false);
				return true;
			}
			case OT_STRING: {
				//TODO!!!!
				return false;
			}
			default: {
				return false;
			}
		}
	#else
		#error
	#endif
	}

	__attribute__((__format__(__printf__, 1, 2))) bool raise_exception(const char *fmt, ...) noexcept
	{
		va_list vargs;
		va_start(vargs, fmt);
		bool ret{main::instance().vm()->RaiseExceptionv(fmt, vargs)};
		va_end(vargs);
		return ret;
	}
}
