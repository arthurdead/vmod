namespace vmod
{
	template <typename R, typename ...Args>
	R plugin::function::execute(Args &&...args) noexcept
	{
		if constexpr(std::is_void_v<R>) {
			if constexpr(sizeof...(Args) == 0) {
				execute_internal();
			} else {
				std::vector<script_variant_t> args_var;
				(args_var.emplace_back(std::forward<Args>(args)), ...);
				execute_internal(args_var);
			}
		} else {
			if constexpr(sizeof...(Args) == 0) {
				script_variant_t ret_var;
				execute_internal(ret_var);
				return variant_to_value<R>(ret_var);
			} else {
				std::vector<script_variant_t> args_var;
				(args_var.emplace_back(std::forward<Args>(args)), ...);
				script_variant_t ret_var;
				execute_internal(ret_var, args_var);
				return variant_to_value<R>(ret_var);
			}
		}
	}
}
