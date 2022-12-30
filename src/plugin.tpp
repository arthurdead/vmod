namespace vmod
{
	template <typename R, typename ...Args>
	R plugin::function::execute(Args &&...args) noexcept
	{
		if constexpr(std::is_void_v<R>) {
			if constexpr(sizeof...(Args) == 0) {
				execute_internal();
			} else {
				std::vector<vscript::variant> args_var;
				(args_var.emplace_back(std::forward<Args>(args)), ...);
				execute_internal(args_var.data(), args_var.size());
			}
		} else {
			if constexpr(sizeof...(Args) == 0) {
				gsdk::ScriptVariant_t ret_var;
				execute_internal(ret_var);
				return to_value<R>(ret_var);
			} else {
				std::vector<vscript::variant> args_var;
				(args_var.emplace_back(std::forward<Args>(args)), ...);
				gsdk::ScriptVariant_t ret_var;
				execute_internal(ret_var, args_var.data(), args_var.size());
				return to_value<R>(ret_var);
			}
		}
	}
}
