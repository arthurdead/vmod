namespace gsdk
{
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
