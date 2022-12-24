#include "type_traits.hpp"
#include <filesystem>
#include <charconv>
#include "gsdk/server/baseentity.hpp"

namespace vmod
{
	extern bool __vmod_to_bool(gsdk::HSCRIPT) noexcept;
	extern float __vmod_to_float(gsdk::HSCRIPT) noexcept;
	extern int __vmod_to_int(gsdk::HSCRIPT) noexcept;
	extern std::string_view __vmod_to_string(gsdk::HSCRIPT) noexcept;
	extern std::string_view __vmod_typeof(gsdk::HSCRIPT) noexcept;
	extern bool __vmod_get_scalar(gsdk::HSCRIPT, gsdk::ScriptVariant_t *) noexcept;

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<void>() noexcept
	{ return gsdk::FIELD_VOID; }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<bool>() noexcept
	{ return gsdk::FIELD_BOOLEAN; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, bool value) noexcept
	{ var.m_bool = value; }
	template <>
	bool variant_to_value<bool>(const gsdk::ScriptVariant_t &var) noexcept;

	template <typename T>
	T variant_to_float_value(const gsdk::ScriptVariant_t &var) noexcept
	{
		static_assert(std::is_floating_point_v<T>);

		using namespace std::literals::string_view_literals;

		switch(var.m_type) {
			case gsdk::FIELD_TIME:
			case gsdk::FIELD_INTERVAL:
			case gsdk::FIELD_FLOAT: {
				return static_cast<T>(var.m_float);
			}
			case gsdk::FIELD_DOUBLE: {
				return static_cast<T>(var.m_double);
			}
			case gsdk::FIELD_STRING: {
				const char *m_pszString{gsdk::STRING(var.m_tstring)};

				if(std::strcmp(m_pszString, "true") == 0) {
					return static_cast<T>(1.0f);
				} else if(std::strcmp(m_pszString, "false") == 0) {
					return static_cast<T>(0.0f);
				}

				const char *begin{m_pszString};
				const char *end{m_pszString + std::strlen(m_pszString)};

				T ret;
				std::from_chars(begin, end, ret);
				return ret;
			}
			case gsdk::FIELD_CSTRING: {
				if(std::strcmp(var.m_pszString, "true") == 0) {
					return static_cast<T>(1.0f);
				} else if(std::strcmp(var.m_pszString, "false") == 0) {
					return static_cast<T>(0.0f);
				}

				const char *begin{var.m_pszString};
				const char *end{var.m_pszString + std::strlen(var.m_pszString)};

				T ret;
				std::from_chars(begin, end, ret);
				return ret;
			}
			case gsdk::FIELD_CHARACTER: {
				return static_cast<T>(var.m_char);
			}
			case gsdk::FIELD_SHORT: {
				return static_cast<T>(var.m_short);
			}
			case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
			case gsdk::FIELD_TICK:
			case gsdk::FIELD_INTEGER: {
				return static_cast<T>(var.m_int);
			}
			case gsdk::FIELD_UINT: {
				return static_cast<T>(var.m_uint);
			}
			case gsdk::FIELD_INTEGER64: {
				return static_cast<T>(var.m_longlong);
			}
			case gsdk::FIELD_UINT64: {
				return static_cast<T>(var.m_ulonglong);
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool ? static_cast<T>(1.0f) : static_cast<T>(0.0f);
			}
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT: {
				return static_cast<T>(__vmod_to_float(var.m_hScript));
			}
			default: return {};
		}
	}

	template float variant_to_float_value<float>(const gsdk::ScriptVariant_t &var) noexcept;
	template double variant_to_float_value<double>(const gsdk::ScriptVariant_t &var) noexcept;
	template long double variant_to_float_value<long double>(const gsdk::ScriptVariant_t &var) noexcept;

	template <typename T>
	T variant_to_int_value(const gsdk::ScriptVariant_t &var) noexcept
	{
		static_assert(std::is_integral_v<T>);

		using namespace std::literals::string_view_literals;

		switch(var.m_type) {
			case gsdk::FIELD_INTERVAL:
			case gsdk::FIELD_TIME:
			case gsdk::FIELD_FLOAT: {
				return static_cast<T>(var.m_float);
			}
			case gsdk::FIELD_DOUBLE: {
				return static_cast<T>(var.m_double);
			}
			case gsdk::FIELD_STRING: {
				const char *m_pszString{gsdk::STRING(var.m_tstring)};

				if(std::strcmp(m_pszString, "true") == 0) {
					return static_cast<T>(1);
				} else if(std::strcmp(m_pszString, "false") == 0) {
					return static_cast<T>(0);
				}

				const char *begin{m_pszString};
				const char *end{m_pszString + std::strlen(m_pszString)};

				T ret;
				std::from_chars(begin, end, ret);
				return ret;
			}
			case gsdk::FIELD_CSTRING: {
				if(std::strcmp(var.m_pszString, "true") == 0) {
					return static_cast<T>(1);
				} else if(std::strcmp(var.m_pszString, "false") == 0) {
					return static_cast<T>(0);
				}

				const char *begin{var.m_pszString};
				const char *end{var.m_pszString + std::strlen(var.m_pszString)};

				T ret;
				std::from_chars(begin, end, ret);
				return ret;
			}
			case gsdk::FIELD_CHARACTER: {
				return static_cast<T>(var.m_char);
			}
			case gsdk::FIELD_SHORT: {
				return static_cast<T>(var.m_short);
			}
			case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
			case gsdk::FIELD_INTEGER: {
				return static_cast<T>(var.m_int);
			}
			case gsdk::FIELD_UINT: {
				return static_cast<T>(var.m_uint);
			}
			case gsdk::FIELD_INTEGER64: {
				return static_cast<T>(var.m_longlong);
			}
			case gsdk::FIELD_UINT64: {
				return static_cast<T>(var.m_ulonglong);
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool ? static_cast<T>(1) : static_cast<T>(0);
			}
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT: {
				return static_cast<T>(__vmod_to_int(var.m_hScript));
			}
			default: return {};
		}
	}

	template char variant_to_int_value<char>(const gsdk::ScriptVariant_t &var) noexcept;
	template signed char variant_to_int_value<signed char>(const gsdk::ScriptVariant_t &var) noexcept;
	template unsigned char variant_to_int_value<unsigned char>(const gsdk::ScriptVariant_t &var) noexcept;

	template short variant_to_int_value<short>(const gsdk::ScriptVariant_t &var) noexcept;
	template unsigned short variant_to_int_value<unsigned short>(const gsdk::ScriptVariant_t &var) noexcept;

	template int variant_to_int_value<int>(const gsdk::ScriptVariant_t &var) noexcept;
	template unsigned int variant_to_int_value<unsigned int>(const gsdk::ScriptVariant_t &var) noexcept;

	template long variant_to_int_value<long>(const gsdk::ScriptVariant_t &var) noexcept;
	template unsigned long variant_to_int_value<unsigned long>(const gsdk::ScriptVariant_t &var) noexcept;

	template long long variant_to_int_value<long long>(const gsdk::ScriptVariant_t &var) noexcept;
	template unsigned long long variant_to_int_value<unsigned long long>(const gsdk::ScriptVariant_t &var) noexcept;

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned char>() noexcept
	{ return gsdk::FIELD_SHORT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, unsigned char value) noexcept
	{ var.m_uchar = value; }
	template <>
	inline unsigned char variant_to_value<unsigned char>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned char>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<signed char>() noexcept
	{ return gsdk::FIELD_SHORT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, signed char value) noexcept
	{ var.m_schar = value; }
	template <>
	inline signed char variant_to_value<signed char>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<signed char>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<char>() noexcept
	{ return gsdk::FIELD_CHARACTER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, char value) noexcept
	{ var.m_char = value; }
	template <>
	inline char variant_to_value<char>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<char>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned int>() noexcept
	{ return gsdk::FIELD_UINT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, unsigned int value) noexcept
	{ var.m_uint = value; }
	template <>
	inline unsigned int variant_to_value<unsigned int>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned int>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<int>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, int value) noexcept
	{ var.m_int = value; }
	template <>
	inline int variant_to_value<int>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<int>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned short>() noexcept
	{ return gsdk::FIELD_SHORT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, unsigned short value) noexcept
	{ var.m_ushort = value; }
	template <>
	inline unsigned short variant_to_value<unsigned short>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned short>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<short>() noexcept
	{ return gsdk::FIELD_SHORT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, short value) noexcept
	{ var.m_short = value; }
	template <>
	inline short variant_to_value<short>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<short>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned long>() noexcept
	{
	#if __SIZEOF_LONG__ == __SIZEOF_INT__
		return gsdk::FIELD_UINT;
	#elif __SIZEOF_LONG__ == __SIZEOF_LONG_LONG__
		return gsdk::FIELD_UINT64;
	#else
		#error
	#endif
	}
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, unsigned long value) noexcept
	{ var.m_ulong = value; }
	template <>
	inline unsigned long variant_to_value<unsigned long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long>() noexcept
	{
	#if __SIZEOF_LONG__ == __SIZEOF_INT__
		return gsdk::FIELD_INTEGER;
	#elif __SIZEOF_LONG__ == __SIZEOF_LONG_LONG__
		return gsdk::FIELD_INTEGER64;
	#else
		#error
	#endif
	}
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, long value) noexcept
	{ var.m_long = value; }
	template <>
	inline long variant_to_value<long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned long long>() noexcept
	{ return gsdk::FIELD_UINT64; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, unsigned long long value) noexcept
	{ var.m_ulonglong = value; }
	template <>
	inline unsigned long long variant_to_value<unsigned long long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned long long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long long>() noexcept
	{ return gsdk::FIELD_INTEGER64; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, long long value) noexcept
	{ var.m_longlong = value; }
	template <>
	inline long long variant_to_value<long long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<long long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<float>() noexcept
	{ return gsdk::FIELD_FLOAT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, float value) noexcept
	{ var.m_float = value; }
	template <>
	inline float variant_to_value<float>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_float_value<float>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<double>() noexcept
	{ return gsdk::FIELD_DOUBLE; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, double value) noexcept
	{ var.m_double = value; }
	template <>
	inline double variant_to_value<double>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_float_value<double>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long double>() noexcept
	{ return gsdk::FIELD_DOUBLE; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, long double value) noexcept
	{ var.m_double = static_cast<double>(value); }
	template <>
	inline long double variant_to_value<long double>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_float_value<long double>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<gsdk::HSCRIPT>() noexcept
	{ return gsdk::FIELD_HSCRIPT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, gsdk::HSCRIPT value) noexcept
	{
		if(value && value != gsdk::INVALID_HSCRIPT) {
			var.m_hScript = value;
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_hScript = gsdk::INVALID_HSCRIPT;
		}
	}
	template <>
	inline gsdk::HSCRIPT variant_to_value<gsdk::HSCRIPT>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT: {
				return var.m_hScript;
			}
			default: return {};
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<std::string_view>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, std::string_view value) noexcept
	{
		if(!value.empty()) {
			var.m_pszString = value.data();
		} else {
			var.m_pszString = "";
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<const char *>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, const char *value) noexcept
	{
		if(value) {
			var.m_pszString = value;
		} else {
			var.m_pszString = "";
		}
	}

	constexpr std::size_t __vscript_max_strsiz{11 + ((2 + (6 + 6)) * 5) + 2};
	extern char __vscript_variant_to_value_buffer[__vscript_max_strsiz];

	template <typename T>
	T variant_to_value_string_view(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		constexpr std::size_t buffers_size{sizeof(__vscript_variant_to_value_buffer) / 5};

		switch(var.m_type) {
			case gsdk::FIELD_INTERVAL:
			case gsdk::FIELD_TIME:
			case gsdk::FIELD_FLOAT: {
				char *begin{__vscript_variant_to_value_buffer};
				char *end{__vscript_variant_to_value_buffer + buffers_size};

				std::to_chars_result tc_res{std::to_chars(begin, end, var.m_float)};
				tc_res.ptr[0] = '\0';
				return begin;
			}
			case gsdk::FIELD_DOUBLE: {
				char *begin{__vscript_variant_to_value_buffer};
				char *end{__vscript_variant_to_value_buffer + buffers_size};

				std::to_chars_result tc_res{std::to_chars(begin, end, var.m_double)};
				tc_res.ptr[0] = '\0';
				return begin;
			}
			case gsdk::FIELD_STRING: {
				return gsdk::STRING(var.m_tstring);
			}
			case gsdk::FIELD_MODELNAME:
			case gsdk::FIELD_SOUNDNAME:
			case gsdk::FIELD_CSTRING: {
				return var.m_pszString;
			}
			case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
			case gsdk::FIELD_MODELINDEX:
			case gsdk::FIELD_MATERIALINDEX:
			case gsdk::FIELD_TICK:
			case gsdk::FIELD_INTEGER: {
				char *begin{__vscript_variant_to_value_buffer};
				char *end{__vscript_variant_to_value_buffer + buffers_size};

				std::to_chars_result tc_res{std::to_chars(begin, end, var.m_int)};
				tc_res.ptr[0] = '\0';
				return begin;
			}
			case gsdk::FIELD_UINT: {
				char *begin{__vscript_variant_to_value_buffer};
				char *end{__vscript_variant_to_value_buffer + buffers_size};

				std::to_chars_result tc_res{std::to_chars(begin, end, var.m_uint)};
				tc_res.ptr[0] = '\0';
				return begin;
			}
			case gsdk::FIELD_CHARACTER: {
				char *begin{__vscript_variant_to_value_buffer};
				char *end{__vscript_variant_to_value_buffer + buffers_size};

				std::to_chars_result tc_res{std::to_chars(begin, end, static_cast<short>(var.m_char))};
				tc_res.ptr[0] = '\0';
				return begin;
			}
			case gsdk::FIELD_SHORT: {
				char *begin{__vscript_variant_to_value_buffer};
				char *end{__vscript_variant_to_value_buffer + buffers_size};

				std::to_chars_result tc_res{std::to_chars(begin, end, var.m_short)};
				tc_res.ptr[0] = '\0';
				return begin;
			}
			case gsdk::FIELD_INTEGER64: {
				char *begin{__vscript_variant_to_value_buffer};
				char *end{__vscript_variant_to_value_buffer + buffers_size};

				std::to_chars_result tc_res{std::to_chars(begin, end, var.m_longlong)};
				tc_res.ptr[0] = '\0';
				return begin;
			}
			case gsdk::FIELD_UINT64: {
				char *begin{__vscript_variant_to_value_buffer};
				char *end{__vscript_variant_to_value_buffer + buffers_size};

				std::to_chars_result tc_res{std::to_chars(begin, end, var.m_ulonglong)};
				tc_res.ptr[0] = '\0';
				return begin;
			}
			case gsdk::FIELD_BOOLEAN: {
				if constexpr(std::is_same_v<T, std::string_view>) {
					return var.m_bool ? "true"sv : "false"sv;
				} else {
					return var.m_bool ? "true" : "false";
				}
			}
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT: {
				if constexpr(std::is_same_v<T, std::string_view>) {
					return __vmod_to_string(var.m_hScript);
				} else {
					return __vmod_to_string(var.m_hScript).data();
				}
			}
			default: {
				if constexpr(std::is_same_v<T, std::string_view>) {
					return {};
				} else {
					return "";
				}
			}
		}
	}

	template std::string_view variant_to_value_string_view<std::string_view>(const gsdk::ScriptVariant_t &var) noexcept;
	template const char *variant_to_value_string_view<const char *>(const gsdk::ScriptVariant_t &var) noexcept;

	template <>
	inline const char *variant_to_value<const char *>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_value_string_view<const char *>(var); }

	template <>
	inline std::string_view variant_to_value<std::string_view>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_value_string_view<std::string_view>(var); }

	template <>
	inline std::string variant_to_value<std::string>(const gsdk::ScriptVariant_t &var) noexcept
	{ return std::string{variant_to_value_string_view<std::string_view>(var)}; }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<std::string>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	extern void initialize_variant_value(gsdk::ScriptVariant_t &var, std::string &&value) noexcept;

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<std::filesystem::path>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, const std::filesystem::path &value) noexcept
	{ var.m_pszString = value.c_str(); }
	extern void initialize_variant_value(gsdk::ScriptVariant_t &var, std::filesystem::path &&value) noexcept;
	template <>
	inline std::filesystem::path variant_to_value<std::filesystem::path>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_STRING: {
				return gsdk::STRING(var.m_tstring);
			}
			case gsdk::FIELD_MODELNAME:
			case gsdk::FIELD_SOUNDNAME:
			case gsdk::FIELD_CSTRING: {
				return var.m_pszString;
			}
			default: return {};
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<std::nullptr_t>() noexcept
	{ return gsdk::FIELD_VOID; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, std::nullptr_t) noexcept
	{ var.m_ptr = nullptr; }
	template <>
	std::nullptr_t variant_to_value<std::nullptr_t>(const gsdk::ScriptVariant_t &var) noexcept = delete;

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<void *>() noexcept
	{
	#if __SIZEOF_POINTER__ == __SIZEOF_INT__
		return gsdk::FIELD_UINT;
	#elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
		return gsdk::FIELD_UINT64;
	#else
		#error
	#endif
	}
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, void *value) noexcept
	{
		if(value) {
			var.m_ptr = value;
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ptr = nullptr;
		}
	}
	template <>
	inline void *variant_to_value<void *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
		#if __SIZEOF_POINTER__ == __SIZEOF_INT__
			case gsdk::FIELD_UINT:
		#elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
			case gsdk::FIELD_UINT64:
		#else
			#error
		#endif
			return var.m_ptr;
			default: return {};
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<const void *>() noexcept
	{ return type_to_field<void *>(); }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, const void *value) noexcept
	{ initialize_variant_value(var, const_cast<void *>(value)); }
	template <>
	inline const void *variant_to_value<const void *>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_value<void *>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<generic_func_t>() noexcept
	{ return gsdk::FIELD_FUNCTION; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, generic_func_t value) noexcept
	{
		if(value) {
			var.m_ptr = reinterpret_cast<void *>(value);
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ptr = nullptr;
		}
	}
	template <>
	inline generic_func_t variant_to_value<generic_func_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_FUNCTION:
			return reinterpret_cast<generic_func_t>(var.m_ptr);
			default: return {};
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<generic_mfp_t>() noexcept
	{ return gsdk::FIELD_FUNCTION; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, generic_mfp_t value) noexcept
	{
		if(value) {
			generic_internal_mfp_t internal{value};
			var.m_ulonglong = static_cast<unsigned long long>(internal.value);
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ulonglong = 0;
		}
	}
	template <>
	inline generic_mfp_t variant_to_value<generic_mfp_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_FUNCTION: {
				generic_internal_mfp_t internal{static_cast<std::uint64_t>(var.m_ulonglong)};
				return internal.func;
			}
			default: return {};
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<generic_vtable_t>() noexcept
	{
	#if __SIZEOF_POINTER__ == __SIZEOF_INT__
		return gsdk::FIELD_UINT;
	#elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
		return gsdk::FIELD_UINT64;
	#else
		#error
	#endif
	}
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, generic_vtable_t value) noexcept
	{
		if(value) {
			var.m_ptr = static_cast<void *>(value);
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ptr = nullptr;
		}
	}
	template <>
	inline generic_vtable_t variant_to_value<generic_vtable_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
		#if __SIZEOF_POINTER__ == __SIZEOF_INT__
			case gsdk::FIELD_UINT:
		#elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
			case gsdk::FIELD_UINT64:
		#else
			#error
		#endif
			return static_cast<generic_vtable_t>(var.m_ptr);
			default: return {};
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<generic_plain_mfp_t>() noexcept
	{ return gsdk::FIELD_FUNCTION; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, generic_plain_mfp_t value) noexcept
	{
		if(value) {
			var.m_ptr = reinterpret_cast<void *>(value);
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ptr = nullptr;
		}
	}
	template <>
	inline generic_plain_mfp_t variant_to_value<generic_plain_mfp_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_FUNCTION:
			return reinterpret_cast<generic_plain_mfp_t>(var.m_ptr);
			default: return {};
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<generic_object_t *>() noexcept
	{ return gsdk::FIELD_CLASSPTR; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, generic_object_t *value) noexcept
	{
		if(value) {
			var.m_ptr = static_cast<void *>(value);
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ptr = nullptr;
		}
	}
	template <>
	inline generic_object_t *variant_to_value<generic_object_t *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_CLASSPTR:
			return static_cast<generic_object_t *>(var.m_ptr);
			default: return {};
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<gsdk::CBaseEntity *>() noexcept
	{ return gsdk::FIELD_HSCRIPT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, gsdk::CBaseEntity *value) noexcept
	{
		if(value) {
			var.m_hScript = value->GetScriptInstance();
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_hScript = gsdk::INVALID_HSCRIPT;
		}
	}
	template <>
	inline gsdk::CBaseEntity *variant_to_value<gsdk::CBaseEntity *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			//TODO!!!!
			case gsdk::FIELD_EHANDLE:
			return {};
			case gsdk::FIELD_EDICT:
			return {};
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_CLASSPTR:
			return static_cast<gsdk::CBaseEntity *>(var.m_ptr);
			case gsdk::FIELD_HSCRIPT:
			return gsdk::CBaseEntity::from_instance(var.m_hScript);
			default: return {};
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<gsdk::IServerNetworkable *>() noexcept
	{ return type_to_field<gsdk::CBaseEntity *>(); }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, gsdk::IServerNetworkable *value) noexcept
	{ initialize_variant_value(var, value ? value->GetBaseEntity() : static_cast<gsdk::CBaseEntity *>(nullptr)); }
	template <>
	inline gsdk::IServerNetworkable *variant_to_value<gsdk::IServerNetworkable *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		gsdk::CBaseEntity *ent{variant_to_value<gsdk::CBaseEntity *>(var)};
		if(ent) {
			return ent->GetNetworkable();
		}

		return nullptr;
	}

	template <typename T>
	concept type_to_field_specialized =
		requires () { static_cast<gsdk::ScriptDataType_t(*)()>(&type_to_field<T>); }
	;

	template <typename T>
	concept variant_to_value_specialized =
		requires () { static_cast<T(*)(const gsdk::ScriptVariant_t &)>(&variant_to_value<T>); }
	;

	template <typename T>
	concept initialize_variant_value_specialized =
		requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, const std::decay_t<T> &)>(&initialize_variant_value); } ||
		requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, std::decay_t<T> &)>(&initialize_variant_value); } ||
		requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, std::decay_t<T>)>(&initialize_variant_value); } ||
		requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, std::decay_t<T> &&)>(&initialize_variant_value); }
	;

	template <typename T>
	struct is_optional : std::false_type
	{
	};

	template <typename T>
	struct is_optional<std::optional<T>> : std::true_type
	{
		using type = T;
	};

	template <typename T>
	inline void __initialize_variant_value_impl(gsdk::ScriptVariant_t &var, T &&value) noexcept
	{
		std::memset(var.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));

		if constexpr(initialize_variant_value_specialized<T>) {
			initialize_variant_value(var, std::forward<T>(value));
		} else if constexpr(std::is_pointer_v<std::decay_t<T>>) {
			initialize_variant_value(var, static_cast<void *>(value));
		} else if constexpr(std::is_enum_v<T>) {
			initialize_variant_value(var, static_cast<std::underlying_type_t<T>>(value));
		} else {
			static_assert(false_t<T>::value);
		}
	}

	template <typename T>
	std::remove_reference_t<T> __variant_to_value_impl(const gsdk::ScriptVariant_t &var) noexcept
	{
		gsdk::ScriptVariant_t temp_var;
		temp_var.m_type = var.m_type;
		temp_var.m_flags = var.m_flags & ~gsdk::SV_FREE;
		std::memcpy(temp_var.m_data, var.m_data, sizeof(gsdk::ScriptVariant_t::m_data));

		if(var.m_type == gsdk::FIELD_HSCRIPT && var.m_hScript && var.m_hScript != gsdk::INVALID_HSCRIPT) {
			__vmod_get_scalar(var.m_hScript, &temp_var);
		}

		if constexpr(variant_to_value_specialized<T>) {
			return variant_to_value<T>(temp_var);
		} else if constexpr(std::is_pointer_v<std::decay_t<T>>) {
			return static_cast<T>(variant_to_value<void *>(temp_var));
		} else if constexpr(std::is_enum_v<T>) {
			return static_cast<T>(variant_to_value<std::underlying_type_t<T>>(temp_var));
		} else if constexpr(is_optional<T>::value) {
			if(temp_var.m_type == gsdk::FIELD_VOID) {
				return std::nullopt;
			}

			return __variant_to_value_impl<typename is_optional<T>::type>(temp_var);
		} else {
			static_assert(false_t<T>::value);
		}
	}

	template <typename T>
	constexpr inline gsdk::ScriptDataType_t __type_to_field_impl() noexcept
	{
		if constexpr(type_to_field_specialized<T>) {
			return type_to_field<T>();
		} else if constexpr(std::is_pointer_v<std::decay_t<T>>) {
			return type_to_field<void *>();
		} else if constexpr(std::is_enum_v<T>) {
			return type_to_field<std::underlying_type_t<T>>();
		} else if constexpr(is_optional<T>::value) {
			return __type_to_field_impl<typename is_optional<T>::type>();
		} else {
			static_assert(false_t<T>::value);
		}
	}
}
