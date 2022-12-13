#include "type_traits.hpp"
#include <charconv>

namespace vmod
{
	template <typename T>
	void ConVar::initialize(std::string_view name, T &&value, int flags) noexcept
	{
		using namespace std::literals::string_literals;

		gsdk::ConVar::CreateBase(name.data(), nullptr, flags);

		using decay_t = std::decay_t<T>;

		if constexpr(std::is_same_v<decay_t, std::string_view>) {
			def_value_str = value.data();
			m_pszDefaultValue = value.data();
		} else if constexpr(std::is_same_v<decay_t, std::string>) {
			def_value_str = value.c_str();
			m_pszDefaultValue = def_value_str.c_str();
		} else if constexpr(std::is_same_v<decay_t, std::filesystem::path>) {
			def_value_str = value.c_str();
			m_pszDefaultValue = def_value_str.c_str();
		} else if constexpr(std::is_same_v<decay_t, bool>) {
			def_value_str = value ? "true"s : "false"s;
			m_pszDefaultValue = value ? "true" : "false";
		} else if constexpr(std::is_integral_v<decay_t> || std::is_floating_point_v<decay_t>) {
			gsdk::ConVar::SetValue(std::forward<T>(value));

			constexpr std::size_t len{6 + 6};

			def_value_str.resize(len);

			char *begin{def_value_str.data()};
			char *end{def_value_str.data() + len};

			std::to_chars_result tc_res{std::to_chars(begin, end, value)};
			tc_res.ptr[0] = '\0';

			m_pszDefaultValue = def_value_str.c_str();
		} else {
			static_assert(false_t<T>::value);
		}

		set(std::forward<T>(value));

		cvar->RegisterConCommand(static_cast<gsdk::ConCommandBase *>(this));
	}

	template <typename T>
	ConVar &ConVar::set(T &&value) noexcept
	{
		using decay_t = std::decay_t<T>;

		if constexpr(std::is_same_v<decay_t, std::string_view>) {
			gsdk::ConVar::SetValue(value.data());
		} else if constexpr(std::is_same_v<decay_t, std::string>) {
			gsdk::ConVar::SetValue(value.c_str());
		} else if constexpr(std::is_same_v<decay_t, std::filesystem::path>) {
			gsdk::ConVar::SetValue(value.c_str());
		} else if constexpr(std::is_same_v<decay_t, bool>) {
			gsdk::ConVar::SetValue(value ? 1 : 0);
		} else if constexpr(std::is_integral_v<decay_t> || std::is_floating_point_v<decay_t>) {
			gsdk::ConVar::SetValue(std::forward<T>(value));
		} else {
			static_assert(false_t<T>::value);
		}

		return *this;
	}

	template <typename T>
	T ConVar::get() const noexcept
	{
		using decay_t = std::decay_t<T>;

		if constexpr(std::is_same_v<decay_t, bool>) {
			return gsdk::ConVar::GetBool();
		} else if constexpr(std::is_integral_v<decay_t>) {
			return static_cast<T>(gsdk::ConVar::GetInt());
		} else if constexpr(std::is_floating_point_v<decay_t>) {
			return static_cast<T>(gsdk::ConVar::GetFloat());
		} else {
			static_assert(false_t<T>::value);
		}
	}
}
