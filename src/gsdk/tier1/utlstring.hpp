#pragma once

#include <string_view>
#include <cstring>

namespace gsdk
{
	class CUtlString
	{
	public:
		inline bool operator==(std::string_view other) const noexcept
		{ return std::strncmp(m_pString, other.data(), other.length()) == 0; }
		inline bool operator!=(std::string_view other) const noexcept
		{ return std::strncmp(m_pString, other.data(), other.length()) != 0; }

		inline const char *c_str() const noexcept
		{ return m_pString; }

		char *m_pString{nullptr};

	private:
		CUtlString() = delete;
		CUtlString(const CUtlString &) = delete;
		CUtlString &operator=(const CUtlString &) = delete;
		CUtlString(CUtlString &&) = delete;
		CUtlString &operator=(CUtlString &&) = delete;
	};

	class CUtlConstString
	{
	public:
		CUtlConstString(const CUtlConstString &) noexcept = default;
		CUtlConstString &operator=(const CUtlConstString &) noexcept = default;
		CUtlConstString(CUtlConstString &&) noexcept = default;
		CUtlConstString &operator=(CUtlConstString &&) noexcept = default;

		CUtlConstString(std::nullptr_t) = delete;
		inline CUtlConstString(const char *str) noexcept
			: m_pString{str}
		{
		}
		inline CUtlConstString(std::string_view str) noexcept
			: m_pString{str.data()}
		{
		}

		inline bool operator==(std::string_view other) const noexcept
		{ return std::strncmp(m_pString, other.data(), other.length()) == 0; }
		inline bool operator!=(std::string_view other) const noexcept
		{ return std::strncmp(m_pString, other.data(), other.length()) != 0; }

		inline const char *data() const noexcept
		{ return m_pString; }

		inline operator std::string_view() const noexcept
		{ return std::string_view{m_pString}; }

		const char *m_pString{nullptr};

	private:
		CUtlConstString() = delete;
	};
}
