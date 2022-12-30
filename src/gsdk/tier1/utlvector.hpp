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
		CUtlVector() noexcept = default;

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

		void erase(std::size_t i) noexcept;

		inline const T &operator[](std::size_t i) const noexcept
		{ return m_Memory[i]; }
		inline T &operator[](std::size_t i) noexcept
		{ return m_Memory[i]; }

		inline std::size_t size() const noexcept
		{ return static_cast<std::size_t>(m_Size); }

		CUtlMemory<T> m_Memory;
		int m_Size{0};
		T *m_pElements{nullptr};

	private:
		CUtlVector(const CUtlVector &) = delete;
		CUtlVector &operator=(const CUtlVector &) = delete;
	};
}

#include "utlvector.tpp"
