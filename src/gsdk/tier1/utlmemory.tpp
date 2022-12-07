#include <memory>
#include <cstdlib>

namespace gsdk
{
	template <typename T>
	void CUtlMemory<T>::resize(std::size_t num) noexcept
	{
		m_nAllocationCount = static_cast<int>(num);
		m_nGrowSize = static_cast<int>(sizeof(T) * num);

		if(num == 0) {
			std::free(m_pMemory);
			m_pMemory = nullptr;
		} else {
			if(!m_pMemory) {
				m_pMemory = static_cast<T *>(std::aligned_alloc(alignof(T), sizeof(T) * num));
			} else {
				m_pMemory = static_cast<T *>(std::realloc(static_cast<void *>(m_pMemory), sizeof(T) * num));
			}
		}
	}

	template <typename T>
	CUtlMemory<T>::~CUtlMemory() noexcept
	{
		if(m_nGrowSize > 0) {
			std::size_t num{static_cast<std::size_t>(m_nAllocationCount)};
			for(std::size_t i{0}; i < num; ++i) {
				m_pMemory[i].~T();
			}

			std::free(m_pMemory);
		}
	}
}
