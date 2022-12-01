#pragma once

namespace gsdk
{
	template <typename T>
	class CUtlMemory
	{
	public:
		inline CUtlMemory() noexcept
			: m_pMemory{nullptr}, m_nAllocationCount{0}, m_nGrowSize{0}
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
			m_nGrowSize = other.m_nGrowSize;
			return *this;
		}

		void resize(std::size_t num) noexcept;

		~CUtlMemory() noexcept;

		T *m_pMemory;
		int m_nAllocationCount;
		int m_nGrowSize;
	};

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
