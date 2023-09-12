#include "variant.hpp"
#include "../gsdk/tier0/memalloc.hpp"

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
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			return var.m_double > 0.0;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			return var.m_float > 0.0f;
		#else
			#error
		#endif
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
			#if GSDK_ENGINE == GSDK_ENGINE_L4D2
			return var.m_longlong > 0;
			#else
			return var.m_long > 0;
			#endif
		#endif
			case gsdk::FIELD_UINT64:
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			return var.m_ulonglong > 0;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			return var.m_ulong > 0;
		#else
			#error
		#endif
			case gsdk::FIELD_BOOLEAN:
			return var.m_bool;
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT:
			return detail::to_bool(var.m_object);
			default:
			return {};
		}
	}

	void initialize_impl(gsdk::ScriptVariant_t &var, std::string &&value) noexcept
	{
		if(!value.empty()) {
			std::size_t len{value.length()};
			var.m_cstr = gsdk::alloc_arr<char>(len+1);
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
			var.m_cstr = gsdk::alloc_arr<char>(len+1);
			std::strncpy(var.m_cstr, value.c_str(), len);
			var.m_cstr[len] = '\0';
			var.m_flags |= gsdk::SV_FREE;
		} else {
			var.m_ccstr = "";
		}
	}

	handle_wrapper::handle_wrapper(handle_wrapper &&other) noexcept
		: object{other.object}, free_{other.free_}, type_{other.type_}
	{
		other.object = gsdk::INVALID_HSCRIPT;
		other.free_ = false;
		other.type_ = gsdk::HANDLETYPE_UNKNOWN;
	}

	handle_wrapper &handle_wrapper::operator=(handle_wrapper &&other) noexcept
	{
		free();
		object = other.object;
		free_ = other.free_;
		type_ = other.type_;
		other.object = gsdk::INVALID_HSCRIPT;
		other.free_ = false;
		other.type_ = gsdk::HANDLETYPE_UNKNOWN;
		return *this;
	}

	handle_wrapper::handle_wrapper(gsdk::ScriptVariant_t &&var) noexcept
	{
		free_ = var.should_free();
		object = var.release_object();
		type_ = gsdk::HANDLETYPE_UNKNOWN;
	}

	handle_wrapper &handle_wrapper::operator=(gsdk::ScriptVariant_t &&var) noexcept
	{
		free_ = var.should_free();
		object = var.release_object();
		type_ = gsdk::HANDLETYPE_UNKNOWN;
		return *this;
	}

	handle_wrapper::handle_wrapper(gsdk::ScriptHandleWrapper_t &&var) noexcept
	{
		free_ = var.should_free();
		type_ = var.type;
		object = var.release();
	}

	handle_wrapper &handle_wrapper::operator=(gsdk::ScriptHandleWrapper_t &&var) noexcept
	{
		free_ = var.should_free();
		type_ = var.type;
		object = var.release();
		return *this;
	}

	void handle_wrapper::free() noexcept
	{
		if(free_ && object && object != gsdk::INVALID_HSCRIPT) {
			switch(type_) {
		#ifndef __clang__
			default:
		#endif
			case gsdk::HANDLETYPE_UNKNOWN:
			vm()->ReleaseObject(object);
			break;
			case gsdk::HANDLETYPE_FUNCTION:
			vm()->ReleaseFunction(object);
			break;
			case gsdk::HANDLETYPE_TABLE:
			vm()->ReleaseTable(object);
			break;
			case gsdk::HANDLETYPE_ARRAY:
			vm()->ReleaseArray(object);
			break;
			case gsdk::HANDLETYPE_SCOPE:
			vm()->ReleaseScope(object);
			break;
			case gsdk::HANDLETYPE_INSTANCE:
			vm()->RemoveInstance(object);
			break;
			case gsdk::HANDLETYPE_SCRIPT:
			vm()->ReleaseScript(object);
			break;
			}
			type_ = gsdk::HANDLETYPE_UNKNOWN;
			object = gsdk::INVALID_HSCRIPT;
			free_ = false;
		}
	}

	gsdk::HSCRIPT handle_wrapper::release() noexcept
	{
		gsdk::HSCRIPT tmp{object};
		free_ = false;
		type_ = gsdk::HANDLETYPE_UNKNOWN;
		object = gsdk::INVALID_HSCRIPT;
		return tmp;
	}

	bool handle_ref::operator==(gsdk::HSCRIPT other) const noexcept
	{
		if(!other || other == gsdk::INVALID_HSCRIPT) {
			return (!object || object == gsdk::INVALID_HSCRIPT);
		} else if(!object || object == gsdk::INVALID_HSCRIPT) {
			return false;
		} else {
			return (object == other);
		}
	}
	
	bool handle_ref::operator!=(gsdk::HSCRIPT other) const noexcept
	{
		if(!other || other == gsdk::INVALID_HSCRIPT) {
			return (object && object != gsdk::INVALID_HSCRIPT);
		} else if(!object || object == gsdk::INVALID_HSCRIPT) {
			return true;
		} else {
			return (object != other);
		}
	}
}
