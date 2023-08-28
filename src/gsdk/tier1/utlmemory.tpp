#include "../tier0/memalloc.hpp"

namespace gsdk
{
	template <typename T, typename I>
	void CUtlMemory<T, I>::resize(std::size_t num) noexcept
	{
		m_nAllocationCount = static_cast<int>(num);

		if(num == 0) {
			free<void>(m_pMemory);
			m_pMemory = nullptr;
		} else {
			if(!m_pMemory) {
				m_pMemory = static_cast<T *>(aligned_alloc<void>(alignof(T), sizeof(T) * num));
			} else {
				m_pMemory = static_cast<T *>(realloc<void>(static_cast<void *>(m_pMemory), sizeof(T) * num));
			}
		}
	}

	template <typename T, typename I>
	CUtlMemory<T, I>::~CUtlMemory() noexcept
	{
		if(m_pMemory) {
			free<void>(m_pMemory);
		}
	}
}
