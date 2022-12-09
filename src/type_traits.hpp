#pragma once

namespace vmod
{
	template <typename T>
	struct false_t final
	{
		static constexpr bool value{false};
	};
}
