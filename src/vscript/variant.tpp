#include <charconv>
#include <string>
#include <string_view>
#include <filesystem>
#include "../gsdk/string_t.hpp"
#include "../gsdk/server/baseentity.hpp"

namespace vmod::vscript
{
	namespace detail
	{
		template <typename T>
		T to_float_impl(const gsdk::ScriptVariant_t &var) noexcept
		{
			static_assert(std::is_floating_point_v<T>);

			using namespace std::literals::string_view_literals;

			switch(var.m_type) {
				case gsdk::FIELD_TIME:
				case gsdk::FIELD_FLOAT:
				return static_cast<T>(var.m_float);
				case gsdk::FIELD_FLOAT64:
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				return static_cast<T>(var.m_double);
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				return static_cast<T>(var.m_float);
			#else
				#error
			#endif
				case gsdk::FIELD_STRING: {
					const char *ccstr{gsdk::vscript::STRING(var.m_tstr)};

					if(std::strcmp(ccstr, "true") == 0) {
						return static_cast<T>(1.0f);
					} else if(std::strcmp(ccstr, "false") == 0) {
						return static_cast<T>(0.0f);
					}

					const char *begin{ccstr};
					const char *end{begin + std::strlen(ccstr)};

					T ret;
					std::from_chars(begin, end, ret);
					return ret;
				}
				case gsdk::FIELD_CSTRING: {
					if(std::strcmp(var.m_ccstr, "true") == 0) {
						return static_cast<T>(1.0f);
					} else if(std::strcmp(var.m_ccstr, "false") == 0) {
						return static_cast<T>(0.0f);
					}

					const char *begin{var.m_ccstr};
					const char *end{begin + std::strlen(var.m_ccstr)};

					T ret;
					std::from_chars(begin, end, ret);
					return ret;
				}
				case gsdk::FIELD_CHARACTER:
				return static_cast<T>(var.m_char);
				case gsdk::FIELD_SHORT:
				return static_cast<T>(var.m_short);
				case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
				case gsdk::FIELD_TICK:
				case gsdk::FIELD_INTEGER:
				return static_cast<T>(var.m_int);
				case gsdk::FIELD_UINT32:
				return static_cast<T>(var.m_uint);
			#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				case gsdk::FIELD_INTEGER64:
				#if GSDK_ENGINE == GSDK_ENGINE_L4D2
				return static_cast<T>(var.m_longlong);
				#else
				return static_cast<T>(var.m_long);
				#endif
			#endif
				case gsdk::FIELD_UINT64:
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				return static_cast<T>(var.m_ulonglong);
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				return static_cast<T>(var.m_ulong);
			#else
				#error
			#endif
				case gsdk::FIELD_BOOLEAN:
				return var.m_bool ? static_cast<T>(1.0f) : static_cast<T>(0.0f);
				case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
				case gsdk::FIELD_HSCRIPT:
				return static_cast<T>(to_float(var.m_object));
				default:
				return {};
			}
		}

		template float to_float_impl<float>(const gsdk::ScriptVariant_t &var) noexcept;
		template double to_float_impl<double>(const gsdk::ScriptVariant_t &var) noexcept;
		template long double to_float_impl<long double>(const gsdk::ScriptVariant_t &var) noexcept;

		template <typename T>
		T to_int_impl(const gsdk::ScriptVariant_t &var) noexcept
		{
			static_assert(std::is_integral_v<T>);

			using namespace std::literals::string_view_literals;

			switch(var.m_type) {
				case gsdk::FIELD_TIME:
				case gsdk::FIELD_FLOAT:
				return static_cast<T>(var.m_float);
				case gsdk::FIELD_FLOAT64:
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				return static_cast<T>(var.m_double);
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				return static_cast<T>(var.m_float);
			#else
				#error
			#endif
				case gsdk::FIELD_STRING: {
					const char *ccstr{gsdk::vscript::STRING(var.m_tstr)};

					if(std::strcmp(ccstr, "true") == 0) {
						return static_cast<T>(1);
					} else if(std::strcmp(ccstr, "false") == 0) {
						return static_cast<T>(0);
					}

					const char *begin{ccstr};
					const char *end{begin + std::strlen(ccstr)};

					T ret;
					std::from_chars(begin, end, ret);
					return ret;
				}
				case gsdk::FIELD_CSTRING: {
					if(std::strcmp(var.m_ccstr, "true") == 0) {
						return static_cast<T>(1);
					} else if(std::strcmp(var.m_ccstr, "false") == 0) {
						return static_cast<T>(0);
					}

					const char *begin{var.m_ccstr};
					const char *end{begin + std::strlen(var.m_ccstr)};

					T ret;
					std::from_chars(begin, end, ret);
					return ret;
				}
				case gsdk::FIELD_CHARACTER:
				return static_cast<T>(var.m_char);
				case gsdk::FIELD_SHORT:
				return static_cast<T>(var.m_short);
				case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
				case gsdk::FIELD_INTEGER:
				return static_cast<T>(var.m_int);
				case gsdk::FIELD_UINT32:
				return static_cast<T>(var.m_uint);
			#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				case gsdk::FIELD_INTEGER64:
				#if GSDK_ENGINE == GSDK_ENGINE_L4D2
				return static_cast<T>(var.m_longlong);
				#else
				return static_cast<T>(var.m_long);
				#endif
			#endif
				case gsdk::FIELD_UINT64:
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				return static_cast<T>(var.m_ulonglong);
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				return static_cast<T>(var.m_ulong);
			#else
				#error
			#endif
				case gsdk::FIELD_BOOLEAN:
				return var.m_bool ? static_cast<T>(1) : static_cast<T>(0);
				case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
				case gsdk::FIELD_HSCRIPT:
				return static_cast<T>(to_int(var.m_object));
				default:
				return {};
			}
		}

		template char to_int_impl<char>(const gsdk::ScriptVariant_t &var) noexcept;
		template signed char to_int_impl<signed char>(const gsdk::ScriptVariant_t &var) noexcept;
		template unsigned char to_int_impl<unsigned char>(const gsdk::ScriptVariant_t &var) noexcept;

		template short to_int_impl<short>(const gsdk::ScriptVariant_t &var) noexcept;
		template unsigned short to_int_impl<unsigned short>(const gsdk::ScriptVariant_t &var) noexcept;

		template int to_int_impl<int>(const gsdk::ScriptVariant_t &var) noexcept;
		template unsigned int to_int_impl<unsigned int>(const gsdk::ScriptVariant_t &var) noexcept;

		template long to_int_impl<long>(const gsdk::ScriptVariant_t &var) noexcept;
		template unsigned long to_int_impl<unsigned long>(const gsdk::ScriptVariant_t &var) noexcept;

		template long long to_int_impl<long long>(const gsdk::ScriptVariant_t &var) noexcept;
		template unsigned long long to_int_impl<unsigned long long>(const gsdk::ScriptVariant_t &var) noexcept;

		constexpr std::size_t variant_str_buffer_max{11 + ((2 + (6 + 6)) * 5) + 2};
		extern char variant_str_buffer[variant_str_buffer_max];

		template <typename T>
		T to_string_impl(const gsdk::ScriptVariant_t &var) noexcept
		{
			using namespace std::literals::string_view_literals;
			using namespace std::literals::string_literals;

			constexpr std::size_t buffers_size{variant_str_buffer_max / 5};

			switch(var.m_type) {
				case gsdk::FIELD_TIME:
				case gsdk::FIELD_FLOAT: {
					char *begin{variant_str_buffer};
					char *end{begin + buffers_size};

					std::to_chars_result tc_res{std::to_chars(begin, end, var.m_float)};
					tc_res.ptr[0] = '\0';
					return begin;
				}
				case gsdk::FIELD_FLOAT64: {
					char *begin{variant_str_buffer};
					char *end{begin + buffers_size};

					std::to_chars_result tc_res{std::to_chars(begin, end,
					#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
						var.m_double
					#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
						var.m_float
					#else
						#error
					#endif
					)};
					tc_res.ptr[0] = '\0';
					return begin;
				}
				case gsdk::FIELD_STRING: {
					return gsdk::vscript::STRING(var.m_tstr);
				}
				case gsdk::FIELD_MODELNAME:
				case gsdk::FIELD_SOUNDNAME:
				case gsdk::FIELD_CSTRING:
				return var.m_ccstr;
				case gsdk::FIELD_POSITIVEINTEGER_OR_NULL:
				case gsdk::FIELD_MODELINDEX:
				case gsdk::FIELD_MATERIALINDEX:
				case gsdk::FIELD_TICK:
				case gsdk::FIELD_INTEGER: {
					char *begin{variant_str_buffer};
					char *end{begin + buffers_size};

					std::to_chars_result tc_res{std::to_chars(begin, end, var.m_int)};
					tc_res.ptr[0] = '\0';
					return begin;
				}
				case gsdk::FIELD_UINT32: {
					char *begin{variant_str_buffer};
					char *end{begin + buffers_size};

					std::to_chars_result tc_res{std::to_chars(begin, end, var.m_uint)};
					tc_res.ptr[0] = '\0';
					return begin;
				}
				case gsdk::FIELD_CHARACTER: {
					char *begin{variant_str_buffer};
					char *end{begin + buffers_size};

					std::to_chars_result tc_res{std::to_chars(begin, end, static_cast<short>(var.m_char))};
					tc_res.ptr[0] = '\0';
					return begin;
				}
				case gsdk::FIELD_SHORT: {
					char *begin{variant_str_buffer};
					char *end{begin + buffers_size};

					std::to_chars_result tc_res{std::to_chars(begin, end, var.m_short)};
					tc_res.ptr[0] = '\0';
					return begin;
				}
			#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				case gsdk::FIELD_INTEGER64: {
					char *begin{variant_str_buffer};
					char *end{begin + buffers_size};

					std::to_chars_result tc_res{std::to_chars(begin, end,
					#if GSDK_ENGINE == GSDK_ENGINE_L4D2
						var.m_longlong
					#else
						var.m_long
					#endif
					)};
					tc_res.ptr[0] = '\0';
					return begin;
				}
			#endif
				case gsdk::FIELD_UINT64: {
					char *begin{variant_str_buffer};
					char *end{begin + buffers_size};

					std::to_chars_result tc_res{std::to_chars(begin, end,
					#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
						var.m_ulonglong
					#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
						var.m_ulong
					#else
						#error
					#endif
					)};
					tc_res.ptr[0] = '\0';
					return begin;
				}
				case gsdk::FIELD_BOOLEAN: {
					if constexpr(std::is_same_v<T, std::string_view>) {
						return var.m_bool ? "true"sv : "false"sv;
					} else  if constexpr(std::is_same_v<T, std::string>) {
						return var.m_bool ? "true"s : "false"s;
					} else  if constexpr(std::is_same_v<T, const char *>) {
						return var.m_bool ? "true" : "false";
					} else {
						static_assert(false_t<T>::value);
					}
				}
				case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
				case gsdk::FIELD_HSCRIPT: {
					if constexpr(std::is_same_v<T, std::string_view>) {
						return to_string(var.m_object);
					} else if constexpr(std::is_same_v<T, std::string>) {
						return std::string{to_string(var.m_object)};
					} else if constexpr(std::is_same_v<T, const char *>) {
						return to_string(var.m_object).data();
					} else {
						static_assert(false_t<T>::value);
					}
				}
				default: {
					if constexpr(std::is_same_v<T, std::string_view> ||
									std::is_same_v<T, std::string>) {
						return {};
					} else if constexpr(std::is_same_v<T, const char *>) {
						return "";
					} else {
						static_assert(false_t<T>::value);
					}
				}
			}
		}

		template std::string_view to_string_impl<std::string_view>(const gsdk::ScriptVariant_t &var) noexcept;
		template const char *to_string_impl<const char *>(const gsdk::ScriptVariant_t &var) noexcept;
		template std::string to_string_impl<std::string>(const gsdk::ScriptVariant_t &var) noexcept;
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<void>() noexcept
	{ return gsdk::FIELD_VOID; }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<bool>() noexcept
	{ return gsdk::FIELD_BOOLEAN; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, bool value) noexcept
	{ var.m_bool = value; }
	template <>
	bool to_value_impl<bool>(const gsdk::ScriptVariant_t &var) noexcept;

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<unsigned char>() noexcept
	{ return gsdk::FIELD_SHORT; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, unsigned char value) noexcept
	{ var.m_uchar = value; }
	template <>
	inline unsigned char to_value_impl<unsigned char>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<unsigned char>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<signed char>() noexcept
	{ return gsdk::FIELD_SHORT; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, signed char value) noexcept
	{ var.m_schar = value; }
	template <>
	inline signed char to_value_impl<signed char>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<signed char>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<char>() noexcept
	{ return gsdk::FIELD_CHARACTER; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, char value) noexcept
	{ var.m_char = value; }
	template <>
	inline char to_value_impl<char>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<char>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<unsigned int>() noexcept
	{ return gsdk::FIELD_UINT32; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, unsigned int value) noexcept
	{ var.m_uint = value; }
	template <>
	inline unsigned int to_value_impl<unsigned int>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<unsigned int>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<int>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, int value) noexcept
	{ var.m_int = value; }
	template <>
	inline int to_value_impl<int>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<int>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<unsigned short>() noexcept
	{ return gsdk::FIELD_SHORT; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, unsigned short value) noexcept
	{ var.m_ushort = value; }
	template <>
	inline unsigned short to_value_impl<unsigned short>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<unsigned short>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<short>() noexcept
	{ return gsdk::FIELD_SHORT; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, short value) noexcept
	{ var.m_short = value; }
	template <>
	inline short to_value_impl<short>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<short>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<unsigned long>() noexcept
	{
	#if __SIZEOF_LONG__ == __SIZEOF_INT__
		return gsdk::FIELD_UINT32;
	#elif __SIZEOF_LONG__ == __SIZEOF_LONG_LONG__
		return gsdk::FIELD_UINT64;
	#else
		#error
	#endif
	}
	inline void initialize_impl(gsdk::ScriptVariant_t &var, unsigned long value) noexcept
	{ var.m_ulong = value; }
	template <>
	inline unsigned long to_value_impl<unsigned long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<unsigned long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<long>() noexcept
	{
	#if __SIZEOF_LONG__ == __SIZEOF_INT__
		return gsdk::FIELD_INTEGER;
	#elif __SIZEOF_LONG__ == __SIZEOF_LONG_LONG__
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		return gsdk::FIELD_INTEGER64;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		return gsdk::FIELD_INTEGER;
		#else
			#error
		#endif
	#else
		#error
	#endif
	}
	inline void initialize_impl(gsdk::ScriptVariant_t &var, long value) noexcept
	{ var.m_long = value; }
	template <>
	inline long to_value_impl<long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<unsigned long long>() noexcept
	{ return gsdk::FIELD_UINT64; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, unsigned long long value) noexcept
	{
	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		var.m_ulonglong = value;
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		var.m_ulong = static_cast<unsigned long>(value);
	#else
		#error
	#endif
	}
	template <>
	inline unsigned long long to_value_impl<unsigned long long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<unsigned long long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<long long>() noexcept
	{
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		return gsdk::FIELD_INTEGER64;
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		return gsdk::FIELD_INTEGER;
	#else
		#error
	#endif
	}
	inline void initialize_impl(gsdk::ScriptVariant_t &var, long long value) noexcept
	{
	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		var.m_longlong = value;
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		var.m_long = static_cast<long>(value);
	#else
		#error
	#endif
	}
	template <>
	inline long long to_value_impl<long long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_int_impl<long long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<float>() noexcept
	{ return gsdk::FIELD_FLOAT; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, float value) noexcept
	{ var.m_float = value; }
	template <>
	inline float to_value_impl<float>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_float_impl<float>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<double>() noexcept
	{ return gsdk::FIELD_FLOAT64; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, double value) noexcept
	{
	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		var.m_double = value;
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		var.m_float = static_cast<float>(value);
	#else
		#error
	#endif
	}
	template <>
	inline double to_value_impl<double>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_float_impl<double>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<long double>() noexcept
	{ return gsdk::FIELD_FLOAT64; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, long double value) noexcept
	{
	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		var.m_double = static_cast<double>(value);
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		var.m_float = static_cast<float>(value);
	#else
		#error
	#endif
	}
	template <>
	inline long double to_value_impl<long double>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_float_impl<long double>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<gsdk::HSCRIPT>() noexcept
	{ return gsdk::FIELD_HSCRIPT; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, gsdk::HSCRIPT value) noexcept
	{
		if(value && value != gsdk::INVALID_HSCRIPT) {
			var.m_object = value;
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_object = gsdk::INVALID_HSCRIPT;
		}
	}
	template <>
	inline gsdk::HSCRIPT to_value_impl<gsdk::HSCRIPT>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT:
			return var.m_object;
			default:
			return gsdk::INVALID_HSCRIPT;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<std::string_view>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, std::string_view value) noexcept
	{
		if(!value.empty()) {
			var.m_ccstr = value.data();
		} else {
			var.m_ccstr = "";
		}
	}
	template <>
	inline std::string_view to_value_impl<std::string_view>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_string_impl<std::string_view>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<const char *>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, const char *value) noexcept
	{
		if(value && value[0] != '\0') {
			var.m_ccstr = value;
		} else {
			var.m_ccstr = "";
		}
	}
	template <>
	inline const char *to_value_impl<const char *>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_string_impl<const char *>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<std::string>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	extern void initialize_impl(gsdk::ScriptVariant_t &var, std::string &&value) noexcept;
	template <>
	inline std::string to_value_impl<std::string>(const gsdk::ScriptVariant_t &var) noexcept
	{ return detail::to_string_impl<std::string>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<std::filesystem::path>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, const std::filesystem::path &value) noexcept
	{ var.m_ccstr = value.c_str(); }
	extern void initialize_impl(gsdk::ScriptVariant_t &var, std::filesystem::path &&value) noexcept;
	template <>
	inline std::filesystem::path to_value_impl<std::filesystem::path>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_STRING:
			return gsdk::vscript::STRING(var.m_tstr);
			case gsdk::FIELD_MODELNAME:
			case gsdk::FIELD_SOUNDNAME:
			case gsdk::FIELD_CSTRING:
			return var.m_ccstr;
			default:
			return {};
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<std::nullptr_t>() noexcept
	{ return gsdk::FIELD_VOID; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, std::nullptr_t) noexcept
	{ var.m_ptr = nullptr; }
	template <>
	std::nullptr_t to_value_impl<std::nullptr_t>(const gsdk::ScriptVariant_t &var) noexcept = delete;

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<void *>() noexcept
	{
	#if __SIZEOF_POINTER__ == __SIZEOF_INT__
		return gsdk::FIELD_UINT32;
	#elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
		return gsdk::FIELD_UINT64;
	#else
		#error
	#endif
	}
	inline void initialize_impl(gsdk::ScriptVariant_t &var, void *value) noexcept
	{
		if(value) {
			var.m_ptr = value;
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ptr = nullptr;
		}
	}
	template <>
	inline void *to_value_impl<void *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
		#if __SIZEOF_POINTER__ == __SIZEOF_INT__
			case gsdk::FIELD_UINT32:
		#elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
			case gsdk::FIELD_UINT64:
		#else
			#error
		#endif
			return var.m_ptr;
			default:
			return nullptr;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<const void *>() noexcept
	{ return type_to_field_impl<void *>(); }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, const void *value) noexcept
	{ initialize_impl(var, const_cast<void *>(value)); }
	template <>
	inline const void *to_value_impl<const void *>(const gsdk::ScriptVariant_t &var) noexcept
	{ return to_value_impl<void *>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<generic_func_t>() noexcept
	{ return gsdk::FIELD_FUNCTION; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, generic_func_t value) noexcept
	{
		if(value) {
		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wconditionally-supported"
		#endif
			var.m_ptr = reinterpret_cast<void *>(value);
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ptr = nullptr;
		}
	}
	template <>
	inline generic_func_t to_value_impl<generic_func_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_FUNCTION:
		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wconditionally-supported"
		#endif
			return reinterpret_cast<generic_func_t>(var.m_ptr);
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif
			default:
			return nullptr;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<generic_plain_mfp_t>() noexcept
	{ return gsdk::FIELD_FUNCTION; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, generic_plain_mfp_t value) noexcept
	{
		if(value) {
		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wconditionally-supported"
		#endif
			var.m_ptr = reinterpret_cast<void *>(value);
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ptr = nullptr;
		}
	}
	template <>
	inline generic_plain_mfp_t to_value_impl<generic_plain_mfp_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_FUNCTION:
		#ifndef __clang__
			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Wconditionally-supported"
		#endif
			return reinterpret_cast<generic_plain_mfp_t>(var.m_ptr);
		#ifndef __clang__
			#pragma GCC diagnostic pop
		#endif
			default:
			return nullptr;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<generic_mfp_t>() noexcept
	{ return gsdk::FIELD_FUNCTION; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, generic_mfp_t value) noexcept
	{
		if(value) {
			generic_internal_mfp_t internal{value};
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			var.m_ulonglong = internal.value;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			if(internal.adjustor == 0) {
				var.m_ptr = reinterpret_cast<void *>(internal.addr);
			} else {
				var.m_type = gsdk::FIELD_VOID;
				var.m_ulong = 0;
			}
		#else
			#error
		#endif
		} else {
			var.m_type = gsdk::FIELD_VOID;
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			var.m_ulonglong = 0;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			var.m_ulong = 0;
		#else
			#error
		#endif
		}
	}
	template <>
	inline generic_mfp_t to_value_impl<generic_mfp_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_FUNCTION: {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				generic_internal_mfp_t internal{var.m_ulonglong};
				return internal.func;
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				return reinterpret_cast<generic_mfp_t>(mfp_from_func(reinterpret_cast<generic_plain_mfp_t>(var.m_ptr), 0));
			#else
				#error
			#endif
			}
			default:
			return nullptr;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<generic_internal_mfp_t>() noexcept
	{ return gsdk::FIELD_FUNCTION; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, generic_internal_mfp_t value) noexcept
	{
		if(value) {
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			var.m_ulonglong = value.value;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			if(value.adjustor == 0) {
				var.m_ptr = reinterpret_cast<void *>(value.addr);
			} else {
				var.m_type = gsdk::FIELD_VOID;
				var.m_ulong = 0;
			}
		#else
			#error
		#endif
		} else {
			var.m_type = gsdk::FIELD_VOID;
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			var.m_ulonglong = 0;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			var.m_ulong = 0;
		#else
			#error
		#endif
		}
	}
	template <>
	inline generic_internal_mfp_t to_value_impl<generic_internal_mfp_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_FUNCTION: {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				generic_internal_mfp_t internal{var.m_ulonglong};
				return internal;
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				return get_internal_mfp(reinterpret_cast<generic_mfp_t>(mfp_from_func(reinterpret_cast<generic_plain_mfp_t>(var.m_ptr), 0)));
			#else
				#error
			#endif
			}
			default:
			return nullptr;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<generic_internal_mfp_va_t>() noexcept
	{ return gsdk::FIELD_FUNCTION; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, generic_internal_mfp_va_t value) noexcept
	{
		if(value) {
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			var.m_ulonglong = value.value;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			if(value.adjustor == 0) {
				var.m_ptr = reinterpret_cast<void *>(value.addr);
			} else {
				var.m_type = gsdk::FIELD_VOID;
				var.m_ulong = 0;
			}
		#else
			#error
		#endif
		} else {
			var.m_type = gsdk::FIELD_VOID;
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			var.m_ulonglong = 0;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			var.m_ulong = 0;
		#else
			#error
		#endif
		}
	}
	template <>
	inline generic_internal_mfp_va_t to_value_impl<generic_internal_mfp_va_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_FUNCTION: {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				generic_internal_mfp_va_t internal{var.m_ulonglong};
				return internal;
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				return get_internal_mfp(reinterpret_cast<generic_mfp_va_t>(mfp_from_func(reinterpret_cast<generic_plain_mfp_va_t>(var.m_ptr), 0)));
			#else
				#error
			#endif
			}
			default:
			return nullptr;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<mfp_or_func_t>() noexcept
	{ return gsdk::FIELD_FUNCTION; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, mfp_or_func_t value) noexcept
	{
		if(value) {
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			var.m_ulonglong = value.mfp.value;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			if(value.mfp.adjustor == 0) {
				var.m_ptr = reinterpret_cast<void *>(value.mfp.addr);
			} else {
				var.m_type = gsdk::FIELD_VOID;
				var.m_ulong = 0;
			}
		#else
			#error
		#endif
		} else {
			var.m_type = gsdk::FIELD_VOID;
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			var.m_ulonglong = 0;
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			var.m_ulong = 0;
		#else
			#error
		#endif
		}
	}
	template <>
	inline mfp_or_func_t to_value_impl<mfp_or_func_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_FUNCTION: {
				mfp_or_func_t internal;
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				internal.mfp = var.m_ulonglong;
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				internal.mfp.addr = reinterpret_cast<generic_plain_mfp_t>(var.m_ptr);
			#else
				#error
			#endif
				return internal;
			}
			default:
			return nullptr;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<generic_vtable_t>() noexcept
	{
	#if __SIZEOF_POINTER__ == __SIZEOF_INT__
		return gsdk::FIELD_UINT32;
	#elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
		return gsdk::FIELD_UINT64;
	#else
		#error
	#endif
	}
	inline void initialize_impl(gsdk::ScriptVariant_t &var, generic_vtable_t value) noexcept
	{
		if(value) {
			var.m_ptr = static_cast<void *>(value);
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ptr = nullptr;
		}
	}
	template <>
	inline generic_vtable_t to_value_impl<generic_vtable_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
		#if __SIZEOF_POINTER__ == __SIZEOF_INT__
			case gsdk::FIELD_UINT32:
		#elif __SIZEOF_POINTER__ == __SIZEOF_LONG_LONG__
			case gsdk::FIELD_UINT64:
		#else
			#error
		#endif
			return static_cast<generic_vtable_t>(var.m_ptr);
			default:
			return nullptr;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<generic_object_t *>() noexcept
	{ return gsdk::FIELD_CLASSPTR; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, generic_object_t *value) noexcept
	{
		if(value) {
			var.m_ptr = static_cast<void *>(value);
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_ptr = nullptr;
		}
	}
	template <>
	inline generic_object_t *to_value_impl<generic_object_t *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_CLASSPTR:
			return static_cast<generic_object_t *>(var.m_ptr);
			default:
			return nullptr;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<gsdk::CBaseEntity *>() noexcept
	{ return gsdk::FIELD_HSCRIPT; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, gsdk::CBaseEntity *value) noexcept
	{
		if(value) {
			var.m_object = value->GetScriptInstance();
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_object = gsdk::INVALID_HSCRIPT;
		}
	}
	template <>
	inline gsdk::CBaseEntity *to_value_impl<gsdk::CBaseEntity *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			//TODO!!!!
			case gsdk::FIELD_EHANDLE:
			return nullptr;
			case gsdk::FIELD_EDICT:
			return nullptr;
			case gsdk::FIELD_INTEGER:
			case gsdk::FIELD_CLASSPTR:
			return static_cast<gsdk::CBaseEntity *>(var.m_ptr);
			case gsdk::FIELD_HSCRIPT:
			return gsdk::CBaseEntity::from_instance(var.m_object);
			default:
			return nullptr;
		}
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<gsdk::IServerNetworkable *>() noexcept
	{ return type_to_field_impl<gsdk::CBaseEntity *>(); }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, gsdk::IServerNetworkable *value) noexcept
	{ initialize_impl(var, value ? value->GetBaseEntity() : static_cast<gsdk::CBaseEntity *>(nullptr)); }
	template <>
	inline gsdk::IServerNetworkable *to_value_impl<gsdk::IServerNetworkable *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		gsdk::CBaseEntity *ent{to_value_impl<gsdk::CBaseEntity *>(var)};
		if(ent) {
			return ent->GetNetworkable();
		}

		return nullptr;
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<gsdk::Vector *>() noexcept
	{ return gsdk::FIELD_VECTOR; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<gsdk::Vector>() noexcept
	{ return gsdk::FIELD_VECTOR; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, const gsdk::Vector &value) noexcept
	{
		var.m_vector = new gsdk::Vector{value};
		var.m_flags |= gsdk::SV_FREE;
	}
	inline void initialize_impl(gsdk::ScriptVariant_t &var, gsdk::Vector *value) noexcept
	{
		if(value) {
			initialize_impl(var, *value);
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_object = gsdk::INVALID_HSCRIPT;
		}
	}
	template <>
	inline gsdk::Vector *to_value_impl<gsdk::Vector *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_VECTOR:
			return var.m_vector;
			default:
			return nullptr;
		}
	}
	template <>
	inline gsdk::Vector to_value_impl<gsdk::Vector>(const gsdk::ScriptVariant_t &var) noexcept
	{
		auto ptr{to_value_impl<gsdk::Vector *>(var)};
		if(ptr) {
			return *ptr;
		}

		return gsdk::Vector{};
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<gsdk::QAngle *>() noexcept
	{ return gsdk::FIELD_QANGLE; }
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<gsdk::QAngle>() noexcept
	{ return gsdk::FIELD_QANGLE; }
	inline void initialize_impl(gsdk::ScriptVariant_t &var, const gsdk::QAngle &value) noexcept
	{
		var.m_qangle = new gsdk::QAngle{value};
		var.m_flags |= gsdk::SV_FREE;
	}
	inline void initialize_impl(gsdk::ScriptVariant_t &var, gsdk::QAngle *value) noexcept
	{
		if(value) {
			initialize_impl(var, *value);
		} else {
			var.m_type = gsdk::FIELD_VOID;
			var.m_object = gsdk::INVALID_HSCRIPT;
		}
	}
	template <>
	inline gsdk::QAngle *to_value_impl<gsdk::QAngle *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_QANGLE:
			return var.m_qangle;
			default:
			return nullptr;
		}
	}
	template <>
	inline gsdk::QAngle to_value_impl<gsdk::QAngle>(const gsdk::ScriptVariant_t &var) noexcept
	{
		auto ptr{to_value_impl<gsdk::QAngle *>(var)};
		if(ptr) {
			return *ptr;
		}

		return gsdk::QAngle{};
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field_impl<gsdk::ScriptVariant_t>() noexcept
	{ return gsdk::FIELD_VARIANT; }
	template <>
	gsdk::ScriptDataType_t type_to_field_impl<const gsdk::ScriptVariant_t *>() noexcept = delete;

	template <>
	gsdk::ScriptVariant_t to_value_impl<gsdk::ScriptVariant_t>(const gsdk::ScriptVariant_t &var) noexcept = delete;
}
