#pragma once

#include "config.hpp"

namespace gsdk::detail::string_t::weak
{
	using string_t = int;

	constexpr inline const char *STRING(const string_t &offset) noexcept
	{ return offset ? reinterpret_cast<const char *>(offset) : ""; }
}

namespace gsdk::detail::string_t::plain
{
	using string_t = const char *;

	constexpr inline const char *STRING(const string_t &c_str) noexcept
	{ return c_str; }
}

namespace gsdk::detail::string_t::object
{
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
}

namespace gsdk::vscript
{
	using string_t = detail::string_t::object::string_t;
	using detail::string_t::object::STRING;
}

namespace gsdk::engine
{
	using string_t = detail::string_t::object::string_t;
	using detail::string_t::object::STRING;
}

namespace gsdk::server
{
	using string_t = detail::string_t::object::string_t;
	using detail::string_t::object::STRING;
}

namespace gsdk::client
{
	using string_t = detail::string_t::plain::string_t;
	using detail::string_t::plain::STRING;
}
