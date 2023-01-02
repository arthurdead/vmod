#pragma once

#include <cstddef>
#include <utility>

namespace gsdk
{
	template <typename T, typename I = int>
	class CUtlMemory
	{
	public:
		CUtlMemory() noexcept = default;
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

		using value_type = T;

		inline const value_type &operator[](std::size_t i) const noexcept
		{ return m_pMemory[i]; }
		inline value_type &operator[](std::size_t i) noexcept
		{ return m_pMemory[i]; }

		inline std::size_t size() const noexcept
		{ return static_cast<std::size_t>(m_nAllocationCount); }

		using iterator = value_type *;
		using const_iterator = const value_type *;

		inline iterator begin() noexcept
		{ return m_pMemory; }
		inline iterator end() noexcept
		{ return (m_pMemory + size()); }

		inline const_iterator begin() const noexcept
		{ return m_pMemory; }
		inline const_iterator end() const noexcept
		{ return (m_pMemory + size()); }

		inline const_iterator cbegin() const noexcept
		{ return m_pMemory; }
		inline const_iterator cend() const noexcept
		{ return (m_pMemory + size()); }

		inline value_type *data() noexcept
		{ return m_pMemory; }
		inline const value_type *data() const noexcept
		{ return m_pMemory; }

		void resize(std::size_t num) noexcept;

		~CUtlMemory() noexcept;

		struct iterator_t
		{
			I index;
		};

		T *m_pMemory{nullptr};
		int m_nAllocationCount{0};
		int m_nGrowSize{0};

	private:
		CUtlMemory(const CUtlMemory &) = delete;
		CUtlMemory &operator=(const CUtlMemory &) = delete;
	};
}

#include "utlmemory.tpp"
