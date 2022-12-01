#include <memory>
#include <cstdlib>

namespace gsdk
{
	template <typename T>
	void CUtlMemory<T>::resize(std::size_t num) noexcept
	{
		if(static_cast<std::size_t>(m_nAllocationCount) >= num) {
			return;
		}

		m_nAllocationCount = static_cast<int>(num);

		if(!m_pMemory) {
			m_pMemory = static_cast<T *>(std::aligned_alloc(alignof(T), sizeof(T) * num));
		} else {
			m_pMemory = static_cast<T *>(std::realloc(static_cast<void *>(m_pMemory), sizeof(T) * num));
		}
	}

	template <typename T>
	CUtlMemory<T>::~CUtlMemory() noexcept
	{
		if(m_pMemory) {
			std::size_t num{static_cast<std::size_t>(m_nAllocationCount)};
			for(std::size_t i{0}; i < num; ++i) {
				m_pMemory[i].~T();
			}

			free(m_pMemory);
		}
	}

	template <typename T> template <typename ...Args>
	T &CUtlVector<T>::emplace_back(Args &&...args) noexcept
	{
		m_Memory.resize(static_cast<std::size_t>(++m_Size));
		T *elements{m_Memory.m_pMemory};
		m_pElements = elements;
		T &element{elements[m_Size-1]};
		new (&element) T{std::forward<Args>(args)...};
		return element;
	}
}
