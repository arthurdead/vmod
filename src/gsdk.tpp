namespace vmod
{
	template <typename T>
	T *gsdk_library::addr(std::string_view name) noexcept
	{
		void *addr{find_addr(name)};
		if(!addr) {
			return nullptr;
		}

		return *static_cast<T **>(addr);
	}
}
