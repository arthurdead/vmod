#pragma once

//#define GSDK_NO_STRING_T
//#define GSDK_WEAK_STRING_T

namespace gsdk
{
#ifndef GSDK_NO_STRING_T
	#ifdef GSDK_WEAK_STRING_T
	using string_t = int;

	constexpr inline const char *STRING(const string_t &offset) noexcept
	{ return offset ? reinterpret_cast<const char *>(offset) : ""; }
	#else
	struct string_t
	{
	public:
		constexpr inline const char *ToCStr() const noexcept
		{ return pszValue ? pszValue : ""; }

		const char *pszValue{nullptr};

	private:
		string_t() = delete;
		string_t(const string_t &) = delete;
		string_t &operator=(const string_t &) = delete;
		string_t(string_t &&) = delete;
		string_t &operator=(string_t &&) = delete;
	};

	constexpr inline const char *STRING(const string_t &string_t_obj) noexcept
	{ return string_t_obj.ToCStr(); }
	#endif
#else
	using string_t = const char *;

	constexpr inline const char *STRING(const string_t &c_str) noexcept
	{ return c_str; }
#endif
}
