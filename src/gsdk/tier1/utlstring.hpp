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

	public:
		char *m_pString;
	};
}
