#pragma once

#include <string_view>
#include <cstring>

namespace gsdk
{
	class CUtlString
	{
	public:
		inline bool operator==(std::string_view other) const noexcept
		{ return strncmp(m_pString, other.data(), other.length()) == 0; }

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
}
