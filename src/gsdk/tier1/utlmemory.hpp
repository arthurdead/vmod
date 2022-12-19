#pragma once

#include <cstddef>
#include <utility>

namespace gsdk
{
	template <typename T, typename I = int>
	class CUtlMemory
	{
	public:
		inline CUtlMemory() noexcept
			: m_pMemory{nullptr},
			m_nAllocationCount{0},
			m_nGrowSize{0}
		{
		}
		CUtlMemory(const CUtlMemory &) = delete;
		CUtlMemory &operator=(const CUtlMemory &) = delete;
		inline CUtlMemory(CUtlMemory &&other) noexcept
		{ operator=(std::move(other)); }
		inline CUtlMemory &operator=(CUtlMemory &&other) noexcept
		{
			m_pMemory = other.m_pMemory;
			other.m_pMemory = nullptr;
			m_nAllocationCount = other.m_nAllocationCount;
			other.m_nAllocationCount = 0;
			m_nGrowSize = other.m_nGrowSize;
			other.m_nGrowSize = 0;
			return *this;
		}

		static constexpr std::size_t npos{static_cast<I>(-1)};

		inline const T &operator[](std::size_t i) const noexcept
		{ return m_pMemory[i]; }
		inline T &operator[](std::size_t i) noexcept
		{ return m_pMemory[i]; }

		void resize(std::size_t num) noexcept;

		~CUtlMemory() noexcept;

		struct iterator
		{
			I index;
		};

		T *m_pMemory;
		int m_nAllocationCount;
		int m_nGrowSize;
	};
}

#include "utlmemory.tpp"
