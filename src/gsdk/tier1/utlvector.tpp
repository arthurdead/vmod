namespace gsdk
{
	template <typename T>
	void CUtlVector<T>::erase(std::size_t i) noexcept
	{
		m_Memory.m_pMemory[i].~T();

		for(int j{static_cast<int>(i)}; j < m_Size-1; ++j) {
			m_Memory.m_pMemory[i] = std::move(m_Memory.m_pMemory[i+1]);
		}

		m_Memory.resize(static_cast<std::size_t>(--m_Size));
		m_pElements = m_Memory.m_pMemory;
	}

	template <typename T> template <typename ...Args>
	T &CUtlVector<T>::emplace_back(Args &&...args) noexcept
	{
		m_Memory.resize(static_cast<std::size_t>(++m_Size));
		m_pElements = m_Memory.m_pMemory;
		T &element{m_Memory.m_pMemory[m_Size-1]};
		new (&element) T{std::forward<Args>(args)...};
		return element;
	}
}
