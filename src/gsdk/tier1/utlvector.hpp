#pragma once

#include <cstddef>
#include <utility>
#include "utlmemory.hpp"

namespace gsdk
{
	template <typename T>
	class CUtlVector
	{
	public:
		inline CUtlVector() noexcept
			: m_Size{0}, m_pElements{nullptr}
		{
		}
		CUtlVector(const CUtlVector &) = delete;
		CUtlVector &operator=(const CUtlVector &) = delete;
		inline CUtlVector(CUtlVector &&other) noexcept
		{ operator=(std::move(other)); }
		inline CUtlVector &operator=(CUtlVector &&other) noexcept
		{
			m_Memory = std::move(other.m_Memory);
			m_Size = other.m_Size;
			other.m_Size = 0;
			m_pElements = other.m_pElements;
			other.m_pElements = nullptr;
			return *this;
		}

		template <typename ...Args>
		T &emplace_back(Args &&...args) noexcept;

		CUtlMemory<T> m_Memory;
		int m_Size;
		T *m_pElements;
	};
}

#include "utlvector.tpp"
