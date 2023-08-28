#pragma once

#include <string_view>
#include <cstring>
#include "../tier0/memalloc.hpp"
#include "utlmemory.hpp"

namespace gsdk
{
	class CUtlBinaryBlock
	{
	public:
		~CUtlBinaryBlock() noexcept = default;

		inline unsigned char *data() noexcept
		{ return m_Memory.data(); }
		inline const unsigned char *data() const noexcept
		{ return m_Memory.data(); }

	private:
		CUtlMemory<unsigned char> m_Memory;
		int m_nActualLength;

	private:
		CUtlBinaryBlock() = delete;
		CUtlBinaryBlock(const CUtlBinaryBlock &) = delete;
		CUtlBinaryBlock &operator=(const CUtlBinaryBlock &) = delete;
		CUtlBinaryBlock(CUtlBinaryBlock &&) = delete;
		CUtlBinaryBlock &operator=(CUtlBinaryBlock &&) = delete;
	};

	class CUtlString
	{
	public:
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		inline const char *c_str() const noexcept
		{ return reinterpret_cast<const char *>(m_Storage.data()); }

		~CUtlString() noexcept = default;
	#else
		inline const char *c_str() const noexcept
		{ return m_pString; }

		~CUtlString() noexcept
		{
			if(m_pString) {
				free<char>(m_pString);
			}
		}
	#endif

		inline bool operator==(std::string_view other) const noexcept
		{ return std::strncmp(c_str(), other.data(), other.length()) == 0; }
		inline bool operator!=(std::string_view other) const noexcept
		{ return std::strncmp(c_str(), other.data(), other.length()) != 0; }

	private:
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		CUtlBinaryBlock m_Storage;
	#else
		char *m_pString{nullptr};
	#endif

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
