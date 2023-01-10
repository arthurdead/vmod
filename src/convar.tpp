#include "type_traits.hpp"
#include <charconv>

namespace vmod
{
	template <typename T>
	void ConCommand::initialize(std::string_view name, int flags, T &&func_) noexcept
	{
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		flags |= gsdk::FCVAR_RELEASE;
	#endif

		if(sv_engine->IsDedicatedServer()) {
			flags |= gsdk::FCVAR_GAMEDLL;
		}

		gsdk::ConCommand::Create(name.data(), nullptr, flags);

		func = std::move(func_);

		cvar->RegisterConCommand(this);
	}

	template <typename T>
	void ConVar::initialize(std::string_view name, T &&value, int flags) noexcept
	{
		using namespace std::literals::string_literals;

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		flags |= gsdk::FCVAR_RELEASE;
	#endif

		if(sv_engine->IsDedicatedServer()) {
			flags |= gsdk::FCVAR_GAMEDLL;
		}

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
			def_value_str = value ? "1"s : "0"s;
			m_pszDefaultValue = value ? "1" : "0";
		} else if constexpr(std::is_integral_v<decay_t> || std::is_floating_point_v<decay_t>) {
			gsdk::ConVar::SetValue(std::forward<T>(value));

			std::size_t len{6 + 6};

			def_value_str.resize(len);

			char *begin{def_value_str.data()};
			char *end{def_value_str.data() + len};

			std::to_chars_result tc_res{std::to_chars(begin, end, value)};
			tc_res.ptr[0] = '\0';

			len = std::strlen(begin);
			def_value_str.resize(len);

			m_pszDefaultValue = def_value_str.c_str();
		} else {
			static_assert(false_t<T>::value);
		}

	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		gsdk::ConVar::Create(name.data(), m_pszDefaultValue, flags, nullptr, false, 0.0f, false, 0.0f, nullptr);
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		gsdk::ConVar::Create(name.data(), nullptr, flags);
	#else
		#error
	#endif

		const char *cmdline_value{cvar->GetCommandLineValue(name.data())};
		if(cmdline_value) {
			set(cmdline_value);
		} else {
			set(std::forward<T>(value));
		}

		cvar->RegisterConCommand(this);
	}

	template <typename T>
	void ConVar::set(T &&value) noexcept
	{
		using decay_t = std::decay_t<T>;

		if constexpr(std::is_same_v<decay_t, std::string_view>) {
			gsdk::ConVar::SetValue(value.data());
		} else if constexpr(std::is_same_v<decay_t, const char *>) {
			gsdk::ConVar::SetValue(std::forward<T>(value));
		} else if constexpr(std::is_same_v<decay_t, std::string>) {
			gsdk::ConVar::SetValue(value.c_str());
		} else if constexpr(std::is_same_v<decay_t, std::filesystem::path>) {
			gsdk::ConVar::SetValue(value.c_str());
		} else if constexpr(std::is_same_v<decay_t, bool>) {
			gsdk::ConVar::SetValue(std::forward<T>(value));
		} else if constexpr(std::is_integral_v<decay_t> || std::is_floating_point_v<decay_t>) {
			gsdk::ConVar::SetValue(std::forward<T>(value));
		} else {
			static_assert(false_t<T>::value);
		}
	}

	template <typename T>
	std::remove_reference_t<T> ConVar::get() const noexcept
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
