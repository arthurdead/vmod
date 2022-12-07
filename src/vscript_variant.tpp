#include <filesystem>
#include "vmod.hpp"
#include <charconv>

namespace vmod
{
	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<void>() noexcept
	{ return gsdk::FIELD_VOID; }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<bool>() noexcept
	{ return gsdk::FIELD_BOOLEAN; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, bool value) noexcept
	{ var.m_bool = value; }
	template <>
	inline bool variant_to_value<bool>(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		switch(var.m_type) {
			case gsdk::FIELD_FLOAT: {
				return var.m_float > 0.0f;
			}
			case gsdk::FIELD_CSTRING: {
				const char *begin{var.m_pszString};
				const char *end{var.m_pszString + strlen(var.m_pszString)};

				unsigned short ret;
				std::from_chars(begin, end, ret);
				return ret > 0;
			}
			case gsdk::FIELD_VECTOR: {
				return {};
			}
			case gsdk::FIELD_INTEGER: {
				return var.m_int > 0;
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool;
			}
			case gsdk::FIELD_HSCRIPT: {
				return vmod.to_bool(var.m_hScript);
			}
		}

		return {};
	}

	template <typename T>
	inline T variant_to_float_value(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		switch(var.m_type) {
			case gsdk::FIELD_FLOAT: {
				return static_cast<T>(var.m_float);
			}
			case gsdk::FIELD_CSTRING: {
				const char *begin{var.m_pszString};
				const char *end{var.m_pszString + strlen(var.m_pszString)};

				T ret;
				std::from_chars(begin, end, ret);
				return ret;
			}
			case gsdk::FIELD_VECTOR: {
				return {};
			}
			case gsdk::FIELD_INTEGER: {
				return static_cast<T>(var.m_int);
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool ? static_cast<T>(1.0f) : static_cast<T>(0.0f);
			}
			case gsdk::FIELD_HSCRIPT: {
				return static_cast<T>(vmod.to_float(var.m_hScript));
			}
		}

		return {};
	}

	template <typename T>
	inline T variant_to_int_value(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		switch(var.m_type) {
			case gsdk::FIELD_FLOAT: {
				return static_cast<T>(var.m_float);
			}
			case gsdk::FIELD_CSTRING: {
				const char *begin{var.m_pszString};
				const char *end{var.m_pszString + strlen(var.m_pszString)};

				T ret;
				std::from_chars(begin, end, ret);
				return ret;
			}
			case gsdk::FIELD_VECTOR: {
				return {};
			}
			case gsdk::FIELD_INTEGER: {
				return static_cast<T>(var.m_int);
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool ? static_cast<T>(1) : static_cast<T>(0);
			}
			case gsdk::FIELD_HSCRIPT: {
				return static_cast<T>(vmod.to_int(var.m_hScript));
			}
		}

		return {};
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned int>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, unsigned int value) noexcept
	{ var.m_int = static_cast<int>(value); }
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
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, unsigned short value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline unsigned short variant_to_value<unsigned short>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned short>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<short>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, short value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline short variant_to_value<short>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<short>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned long>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, unsigned long value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline unsigned long variant_to_value<unsigned long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, long value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline long variant_to_value<long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<unsigned long long>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, unsigned long long value) noexcept
	{ var.m_int = static_cast<int>(value); }
	template <>
	inline unsigned long long variant_to_value<unsigned long long>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_int_value<unsigned long long>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long long>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, long long value) noexcept
	{ var.m_int = static_cast<int>(value); }
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
	{ return gsdk::FIELD_FLOAT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, double value) noexcept
	{ var.m_float = static_cast<float>(value); }
	template <>
	inline double variant_to_value<double>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_float_value<double>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<long double>() noexcept
	{ return gsdk::FIELD_FLOAT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, long double value) noexcept
	{ var.m_float = static_cast<float>(value); }
	template <>
	inline long double variant_to_value<long double>(const gsdk::ScriptVariant_t &var) noexcept
	{ return variant_to_float_value<long double>(var); }

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<gsdk::HSCRIPT>() noexcept
	{ return gsdk::FIELD_HSCRIPT; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, gsdk::HSCRIPT value) noexcept
	{ var.m_hScript = value; }
	template <>
	inline gsdk::HSCRIPT variant_to_value<gsdk::HSCRIPT>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_HSCRIPT: {
				return var.m_hScript;
			}
		}

		return {};
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<std::string_view>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, std::string_view value) noexcept
	{ var.m_pszString = value.data(); }

	static char __vscript_variable_to_value_buffer[11 + ((2 + (6 + 6)) * 4) + 2];

	template <>
	inline std::string_view variant_to_value<std::string_view>(const gsdk::ScriptVariant_t &var) noexcept
	{
		using namespace std::literals::string_view_literals;

		constexpr std::size_t buffers_size{sizeof(__vscript_variable_to_value_buffer) / 4};

		switch(var.m_type) {
			case gsdk::FIELD_FLOAT: {
				char *begin{__vscript_variable_to_value_buffer};
				char *end{__vscript_variable_to_value_buffer + buffers_size};

				std::to_chars(begin, end, var.m_float);
				return begin;
			}
			case gsdk::FIELD_CSTRING: {
				return var.m_pszString;
			}
			case gsdk::FIELD_VECTOR: {
				std::strncpy(__vscript_variable_to_value_buffer, "(vector : (", buffers_size);

				char *begin{__vscript_variable_to_value_buffer + 11};
				char *end{begin + buffers_size};
				std::to_chars_result tc_res{std::to_chars(begin, end, var.m_pVector->x)};

				std::strncat(tc_res.ptr, ", ", buffers_size);

				begin = (tc_res.ptr + 2);
				end = (begin + buffers_size);
				tc_res = std::to_chars(begin, end, var.m_pVector->y);

				std::strncat(tc_res.ptr, ", ", buffers_size);

				begin = (tc_res.ptr + 2);
				end = (begin + buffers_size);
				std::to_chars(begin, end, var.m_pVector->z);

				std::strncat(tc_res.ptr, "))", buffers_size);

				return __vscript_variable_to_value_buffer;
			}
			case gsdk::FIELD_INTEGER: {
				char *begin{__vscript_variable_to_value_buffer};
				char *end{__vscript_variable_to_value_buffer + buffers_size};

				std::to_chars(begin, end, var.m_int);
				return begin;
			}
			case gsdk::FIELD_BOOLEAN: {
				return var.m_bool ? "true"sv : "false"sv;
			}
			case gsdk::FIELD_HSCRIPT: {
				return vmod.to_string(var.m_hScript);
			}
		}

		return {};
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<std::filesystem::path>() noexcept
	{ return gsdk::FIELD_CSTRING; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, const std::filesystem::path &value) noexcept
	{ var.m_pszString = value.c_str(); }
	template <>
	inline std::filesystem::path variant_to_value<std::filesystem::path>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_CSTRING: {
				return var.m_pszString;
			}
		}

		return {};
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<void *>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, void *value) noexcept
	{ var.m_hScript = reinterpret_cast<gsdk::HSCRIPT>(value); }
	template <>
	inline void *variant_to_value<void *>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			return reinterpret_cast<void *>(var.m_hScript);
		}

		return nullptr;
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<generic_func_t>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, generic_func_t value) noexcept
	{ var.m_int = reinterpret_cast<int>(value); }
	template <>
	inline generic_func_t variant_to_value<generic_func_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			return reinterpret_cast<generic_func_t>(var.m_int);
		}

		return nullptr;
	}

	template <>
	constexpr inline gsdk::ScriptDataType_t type_to_field<generic_mfp_t>() noexcept
	{ return gsdk::FIELD_INTEGER; }
	inline void initialize_variant_value(gsdk::ScriptVariant_t &var, generic_mfp_t value) noexcept
	{ var.m_int = reinterpret_cast<int>(mfp_to_func(value).first); }
	template <>
	inline generic_mfp_t variant_to_value<generic_mfp_t>(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_INTEGER:
			return mfp_from_func(reinterpret_cast<generic_plain_mfp_t>(var.m_int));
		}

		return nullptr;
	}
}
