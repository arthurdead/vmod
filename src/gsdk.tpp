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

	template <typename ...Args>
	void print(std::string_view fmt, Args &&...args)
	{
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wformat-nonliteral"
		#pragma clang diagnostic ignored "-Wformat-security"
	#endif
		if(dedicated) {
			char buff[gsdk::MAXPRINTMSG];
			std::snprintf(buff, sizeof(buff), fmt.data(), std::forward<Args>(args)...);
			dedicated->Sys_Printf(buff);
		} else {
			sv_engine->Con_NPrintf(0, fmt.data(), std::forward<Args>(args)...);
		}
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
	}

	template <typename ...Args>
	void success(std::string_view fmt, Args &&...args) noexcept
	{
		if(dedicated) {
			dedicated->Sys_Printf("\033[0;32m");
		}

		print(fmt, std::forward<Args>(args)...);

		if(dedicated) {
			dedicated->Sys_Printf("\033[0m");
		}
	}

	template <typename ...Args>
	void info(std::string_view fmt, Args &&...args) noexcept
	{
		if(dedicated) {
			dedicated->Sys_Printf("\033[0;34m");
		}

		print(fmt, std::forward<Args>(args)...);

		if(dedicated) {
			dedicated->Sys_Printf("\033[0m");
		}
	}

	template <typename ...Args>
	void warning(std::string_view fmt, Args &&...args) noexcept
	{
		if(dedicated) {
			dedicated->Sys_Printf("\033[0;33m");
		}

		print(fmt, std::forward<Args>(args)...);

		if(dedicated) {
			dedicated->Sys_Printf("\033[0m");
		}
	}

	template <typename ...Args>
	void error(std::string_view fmt, Args &&...args) noexcept
	{
		if(dedicated) {
			dedicated->Sys_Printf("\033[0;31m");
		}

		print(fmt, std::forward<Args>(args)...);

		if(dedicated) {
			dedicated->Sys_Printf("\033[0m");
		}
	}
}
