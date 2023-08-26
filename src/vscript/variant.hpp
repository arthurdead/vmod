#pragma once

#include "vscript.hpp"

namespace vmod::vscript
{
	inline void null(gsdk::ScriptVariant_t &var) noexcept
	{
		var.m_type = gsdk::FIELD_VOID;
		std::memset(var.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));
		var.m_object = gsdk::INVALID_HSCRIPT;
		var.m_flags = gsdk::SV_NOFLAGS;
	}

	inline gsdk::ScriptVariant_t null() noexcept
	{
		gsdk::ScriptVariant_t var;
		null(var);
		return var;
	}

	class alignas(gsdk::ScriptVariant_t) variant final : public gsdk::ScriptVariant_t
	{
	public:
		variant() noexcept = default;
		~variant() noexcept = default;
		variant(variant &&other) noexcept = default;
		variant &operator=(variant &&other) noexcept = default;
		variant(const variant &other) noexcept = default;
		variant &operator=(const variant &other) noexcept = default;

		inline variant(gsdk::ScriptVariant_t &&other) noexcept
			: gsdk::ScriptVariant_t{std::move(other)}
		{
		}

		inline variant &operator=(gsdk::ScriptVariant_t &&other) noexcept
		{
			gsdk::ScriptVariant_t::operator=(std::move(other));
			return *this;
		}

		inline variant(const gsdk::ScriptVariant_t &other) noexcept
			: gsdk::ScriptVariant_t{other}
		{
		}

		inline variant &operator=(const gsdk::ScriptVariant_t &other) noexcept
		{
			gsdk::ScriptVariant_t::operator=(other);
			return *this;
		}

		template <typename T>
		variant &operator=(T &&value) noexcept;

		template <typename T>
		inline variant(T &&value) noexcept
		{ operator=(std::forward<T>(value)); }

		template <typename T>
		inline variant &assign(T &&value) noexcept
		{
			operator=(std::forward<T>(value));
			return *this;
		}

		template <typename T>
		std::remove_reference_t<T> get() const noexcept;

		template <typename T>
		inline bool operator==(const T &value) const noexcept
		{ return get<T>() == value; }
		template <typename T>
		inline bool operator!=(const T &value) const noexcept
		{ return !operator==(value); }

		template <typename T>
		inline explicit operator std::remove_reference_t<T>() const noexcept
		{ return get<T>(); }
	};

	static_assert(sizeof(variant) == sizeof(gsdk::ScriptVariant_t));
	static_assert(alignof(variant) == alignof(gsdk::ScriptVariant_t));

	template <typename T>
	constexpr gsdk::ScriptDataType_t type_to_field_impl() noexcept = delete;

	template <typename T>
	std::remove_reference_t<T> to_value_impl(const gsdk::ScriptVariant_t &) noexcept = delete;
}

#include "variant.tpp"

namespace vmod::vscript
{
	namespace detail
	{
		template <typename T>
		concept initialize_specialized =
			requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, const std::decay_t<T> &)>(&initialize_impl); } ||
			requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, std::decay_t<T> &)>(&initialize_impl); } ||
			requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, std::decay_t<T>)>(&initialize_impl); } ||
			requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, std::decay_t<T> &&)>(&initialize_impl); }
		;

		template <typename T>
		concept to_value_specialized =
			requires () { static_cast<T(*)(const gsdk::ScriptVariant_t &)>(&to_value_impl<T>); }
		;

		template <typename T>
		concept type_to_field_specialized =
			requires () { static_cast<gsdk::ScriptDataType_t(*)()>(&type_to_field_impl<T>); }
		;
	}

	template <typename T>
	inline void initialize(gsdk::ScriptVariant_t &var, T &&value) noexcept
	{
		std::memset(var.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));

		using decayed_t = std::decay_t<T>;

		if constexpr(detail::initialize_specialized<T>) {
			initialize_impl(var, std::forward<T>(value));
		} else if constexpr(std::is_pointer_v<decayed_t>) {
			initialize_impl(var, static_cast<void *>(value));
		} else if constexpr(std::is_enum_v<decayed_t>) {
			initialize_impl(var, static_cast<std::underlying_type_t<T>>(value));
		} else if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, decayed_t>) {
			var = std::forward<T>(value);
		} else {
			static_assert(false_t<T>::value);
		}
	}

	template <typename T>
	std::remove_reference_t<T> to_value(const gsdk::ScriptVariant_t &var) noexcept
	{
		gsdk::ScriptVariant_t tmp_var;
		tmp_var.m_type = var.m_type;
		tmp_var.m_flags = var.m_flags & ~gsdk::SV_FREE;
		std::memcpy(tmp_var.m_data, var.m_data, sizeof(gsdk::ScriptVariant_t::m_data));

		if(var.m_type == gsdk::FIELD_HSCRIPT && var.m_object && var.m_object != gsdk::INVALID_HSCRIPT) {
			detail::get_scalar(var.m_object, &tmp_var);
		}

		using decayed_t = std::decay_t<T>;

		if constexpr(detail::to_value_specialized<T>) {
			return to_value_impl<T>(tmp_var);
		} else if constexpr(std::is_pointer_v<decayed_t>) {
			return static_cast<T>(to_value_impl<void *>(tmp_var));
		} else if constexpr(std::is_enum_v<decayed_t>) {
			return static_cast<T>(to_value_impl<std::underlying_type_t<T>>(tmp_var));
		} else if constexpr(is_optional<decayed_t>::value) {
			if(tmp_var.m_type == gsdk::FIELD_VOID) {
				return std::nullopt;
			}

			return to_value<typename is_optional<T>::type>(tmp_var);
		} else if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, decayed_t>) {
			return var;
		} else {
			static_assert(false_t<T>::value);
		}
	}

	template <typename T>
	constexpr inline gsdk::ScriptDataType_t type_to_field() noexcept
	{
		using decayed_t = std::decay_t<T>;

		if constexpr(detail::type_to_field_specialized<T>) {
			return type_to_field_impl<T>();
		} else if constexpr(std::is_pointer_v<decayed_t>) {
			return type_to_field_impl<void *>();
		} else if constexpr(std::is_enum_v<decayed_t>) {
			return type_to_field_impl<std::underlying_type_t<T>>();
		} else if constexpr(is_optional<decayed_t>::value) {
			return type_to_field<typename is_optional<T>::type>();
		} else if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, decayed_t>) {
			return gsdk::FIELD_VARIANT;
		} else {
			static_assert(false_t<T>::value);
		}
	}

#ifdef __VMOD_USING_CUSTOM_VM
	template <typename T>
	constexpr inline gsdk::ScriptDataTypeAndFlags_t type_to_field_and_flags() noexcept
	{
		using decayed_t = std::decay_t<T>;

		gsdk::ScriptDataTypeAndFlags_t param;
		param.type = type_to_field<T>();

		if constexpr(is_optional<decayed_t>::value) {
			param.flags |= gsdk::FIELD_FLAG_OPTIONAL;
		}

		return param;
	}
#endif

	template <typename T>
	inline void to_variant(gsdk::ScriptVariant_t &var, T &&value) noexcept
	{
		var.m_type = static_cast<short>(type_to_field<std::decay_t<T>>());
		var.m_flags = gsdk::SV_NOFLAGS;
		initialize(var, std::forward<T>(value));
	}

	template <typename T>
	variant &variant::operator=(T &&value) noexcept
	{
		if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, std::decay_t<T>>) {
			gsdk::ScriptVariant_t::operator=(std::forward<T>(value));
		} else {
			free();
			to_variant<T>(*this, std::forward<T>(value));
		}
		return *this;
	}

	template <typename T>
	std::remove_reference_t<T> variant::get() const noexcept
	{ return to_value<T>(*this); }
}
