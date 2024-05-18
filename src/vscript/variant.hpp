#pragma once

#include "vscript.hpp"
#include <functional>

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

	class handle_ref;

	template <gsdk::ScriptHandleType_t>
	class typed_handle_ref;

	//TODO!!! replace all raw HSCRIPT with handle_wrapper
	class handle_wrapper
	{
	public:
		handle_wrapper() noexcept = default;
		inline ~handle_wrapper() noexcept
		{ free(); }

		inline handle_wrapper(std::nullptr_t) noexcept
			: handle_wrapper{}
		{
		}

		inline handle_wrapper &operator=(std::nullptr_t) noexcept
		{
			free();
			return *this;
		}

		handle_wrapper(gsdk::HSCRIPT) = delete;
		handle_wrapper &operator=(gsdk::HSCRIPT) = delete;

		handle_wrapper(handle_wrapper &&) noexcept;
		handle_wrapper &operator=(handle_wrapper &&other) noexcept;
		handle_wrapper(const handle_wrapper &) = delete;
		handle_wrapper &operator=(const handle_wrapper &) = delete;

		handle_wrapper(gsdk::ScriptVariant_t &&var) noexcept;
		handle_wrapper &operator=(gsdk::ScriptVariant_t &&var) noexcept;
		handle_wrapper(const gsdk::ScriptVariant_t &) = delete;
		handle_wrapper &operator=(const gsdk::ScriptVariant_t &) = delete;

		handle_wrapper(gsdk::ScriptHandleWrapper_t &&var) noexcept;
		handle_wrapper &operator=(gsdk::ScriptHandleWrapper_t &&var) noexcept;
		handle_wrapper(const gsdk::ScriptHandleWrapper_t &) = delete;
		handle_wrapper &operator=(const gsdk::ScriptHandleWrapper_t &) = delete;

		handle_wrapper(handle_ref &&) = delete;
		handle_wrapper &operator=(handle_ref &&) = delete;
		handle_wrapper(const handle_ref &) = delete;
		handle_wrapper &operator=(const handle_ref &) = delete;

		bool operator==(std::nullptr_t) const = delete;
		bool operator!=(std::nullptr_t) const = delete;

		bool operator==(gsdk::HSCRIPT other) const = delete;
		bool operator!=(gsdk::HSCRIPT other) const = delete;

		bool operator==(const handle_wrapper &other) const = delete;
		bool operator!=(const handle_wrapper &other) const = delete;

		inline void reset(handle_wrapper &&other) noexcept
		{ operator=(std::move(other)); }

		inline void reset(gsdk::ScriptVariant_t &&other) noexcept
		{ operator=(std::move(other)); }

		inline void reset(gsdk::ScriptHandleWrapper_t &&other) noexcept
		{ operator=(std::move(other)); }

		void free() noexcept;

		inline gsdk::HSCRIPT operator*() const noexcept
		{ return object; }

		inline explicit operator gsdk::HSCRIPT() const noexcept
		{ return object; }

		inline gsdk::HSCRIPT get() const noexcept
		{ return object; }

		inline bool operator!() const noexcept
		{ return (!object || object == gsdk::INVALID_HSCRIPT); }

		inline explicit operator bool() const noexcept
		{ return (object && object != gsdk::INVALID_HSCRIPT); }

		inline bool should_free() const noexcept
		{ return free_; }

		inline gsdk::ScriptHandleType_t type() const noexcept
		{ return type_; }

		gsdk::HSCRIPT release() noexcept;

		operator handle_ref() noexcept;

	protected:
		gsdk::HSCRIPT object{gsdk::INVALID_HSCRIPT};
		bool free_{false};
		gsdk::ScriptHandleType_t type_{gsdk::HANDLETYPE_UNKNOWN};
	};

	template <gsdk::ScriptHandleType_t T>
	class typed_handle_wrapper : public handle_wrapper
	{
	public:
		using handle_wrapper::handle_wrapper;
		using handle_wrapper::operator=;

		typed_handle_wrapper(typed_handle_ref<T> &&) = delete;
		typed_handle_wrapper &operator=(typed_handle_ref<T> &&) = delete;
		typed_handle_wrapper(const typed_handle_ref<T> &) = delete;
		typed_handle_wrapper &operator=(const typed_handle_ref<T> &) = delete;

		operator typed_handle_ref<T>() noexcept;
	};

	using unknown_handle_wrapper = typed_handle_wrapper<gsdk::HANDLETYPE_UNKNOWN>;
	using instance_handle_wrapper = typed_handle_wrapper<gsdk::HANDLETYPE_INSTANCE>;
	using table_handle_wrapper = typed_handle_wrapper<gsdk::HANDLETYPE_TABLE>;
	using array_handle_wrapper = typed_handle_wrapper<gsdk::HANDLETYPE_ARRAY>;
	using scope_handle_wrapper = typed_handle_wrapper<gsdk::HANDLETYPE_SCOPE>;
	using script_handle_wrapper = typed_handle_wrapper<gsdk::HANDLETYPE_SCRIPT>;
	using func_handle_wrapper = typed_handle_wrapper<gsdk::HANDLETYPE_FUNCTION>;

	class handle_ref
	{
		friend class handle_wrapper;

		template <gsdk::ScriptHandleType_t T>
		friend class typed_handle_wrapper;

	public:
		handle_ref() noexcept = default;
		~handle_ref() noexcept = default;

		inline handle_ref(std::nullptr_t) noexcept
			: handle_ref{}
		{
		}

		inline handle_ref &operator=(std::nullptr_t) noexcept
		{
			object = gsdk::INVALID_HSCRIPT;
			type_ = gsdk::HANDLETYPE_UNKNOWN;
			return *this;
		}

		inline handle_ref(gsdk::HSCRIPT other) noexcept
			: object{other}, type_{gsdk::HANDLETYPE_UNKNOWN}
		{
		}
		inline handle_ref &operator=(gsdk::HSCRIPT other) noexcept
		{
			object = other;
			type_ = gsdk::HANDLETYPE_UNKNOWN;
			return *this;
		}

		handle_ref(handle_ref &&) noexcept = default;
		handle_ref &operator=(handle_ref &&) noexcept = default;
		handle_ref(const handle_ref &) noexcept = default;
		handle_ref &operator=(const handle_ref &) noexcept = default;

		handle_ref(gsdk::ScriptVariant_t &&) = delete;
		handle_ref &operator=(gsdk::ScriptVariant_t &&) = delete;
		inline handle_ref(const gsdk::ScriptVariant_t &other) noexcept
		{ operator=(other); }
		handle_ref &operator=(const gsdk::ScriptVariant_t &other) noexcept
		{
			switch(other.m_type) {
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_EMBEDDED:
			case gsdk::FIELD_CUSTOM:
			case gsdk::FIELD_FUNCTION:
			case gsdk::FIELD_HSCRIPT:
			object = other.m_object;
			break;
			default:
			object = gsdk::INVALID_HSCRIPT;
			break;
			}
			type_ = gsdk::HANDLETYPE_UNKNOWN;
			return *this;
		}

		handle_ref(gsdk::ScriptHandleWrapper_t &&) = delete;
		handle_ref &operator=(gsdk::ScriptHandleWrapper_t &&) = delete;
		inline handle_ref(const gsdk::ScriptHandleWrapper_t &other) noexcept
			: object{other.object}, type_{other.type}
		{
		}
		inline handle_ref &operator=(const gsdk::ScriptHandleWrapper_t &other) noexcept
		{
			object = other.object;
			type_ = other.type;
			return *this;
		}

		handle_ref(handle_wrapper &&) = delete;
		handle_ref &operator=(handle_wrapper &&) = delete;
		inline handle_ref(const handle_wrapper &other) noexcept
			: object{*other}, type_{other.type()}
		{
		}
		inline handle_ref &operator=(const handle_wrapper &other) noexcept
		{
			object = *other;
			type_ = other.type();
			return *this;
		}

		inline bool operator==(std::nullptr_t) const noexcept
		{ return (!object || object == gsdk::INVALID_HSCRIPT); }
		inline bool operator!=(std::nullptr_t) const noexcept
		{ return (object && object != gsdk::INVALID_HSCRIPT); }

		bool operator==(gsdk::HSCRIPT other) const noexcept;
		bool operator!=(gsdk::HSCRIPT other) const noexcept;

		inline bool operator==(const handle_ref &other) const noexcept
		{ return operator==(other.object); }
		inline bool operator!=(const handle_ref &other) const noexcept
		{ return operator!=(other.object); }

		inline gsdk::HSCRIPT operator*() const noexcept
		{ return object; }

		inline explicit operator gsdk::HSCRIPT() const noexcept
		{ return object; }

		inline gsdk::HSCRIPT get() const noexcept
		{ return object; }

		inline bool operator!() const noexcept
		{ return (!object || object == gsdk::INVALID_HSCRIPT); }

		inline explicit operator bool() const noexcept
		{ return (object && object != gsdk::INVALID_HSCRIPT); }

	protected:
		gsdk::HSCRIPT object{gsdk::INVALID_HSCRIPT};
		gsdk::ScriptHandleType_t type_{gsdk::HANDLETYPE_UNKNOWN};
	};

	inline handle_wrapper::operator handle_ref() noexcept
	{
		handle_ref tmp;
		tmp.object = object;
		tmp.type_ = type_;
		return tmp;
	}

	template <gsdk::ScriptHandleType_t T>
	class typed_handle_ref : public handle_ref
	{
	public:
		using handle_ref::handle_ref;
		using handle_ref::operator=;

		typed_handle_ref(typed_handle_wrapper<T> &&) = delete;
		typed_handle_ref &operator=(typed_handle_wrapper<T> &&) = delete;
		inline typed_handle_ref(const typed_handle_wrapper<T> &other) noexcept
			: handle_ref{other}
		{
		}
		inline typed_handle_ref &operator=(const typed_handle_wrapper<T> &other) noexcept
		{
			object = *other;
			type_ = other.type();
			return *this;
		}
	};

	using unknown_handle_ref = typed_handle_ref<gsdk::HANDLETYPE_UNKNOWN>;
	using instance_handle_ref = typed_handle_ref<gsdk::HANDLETYPE_INSTANCE>;
	using table_handle_ref = typed_handle_ref<gsdk::HANDLETYPE_TABLE>;
	using array_handle_ref = typed_handle_ref<gsdk::HANDLETYPE_ARRAY>;
	using scope_handle_ref = typed_handle_ref<gsdk::HANDLETYPE_SCOPE>;
	using script_handle_ref = typed_handle_ref<gsdk::HANDLETYPE_SCRIPT>;
	using func_handle_ref = typed_handle_ref<gsdk::HANDLETYPE_FUNCTION>;

	template <gsdk::ScriptHandleType_t T>
	inline typed_handle_wrapper<T>::operator typed_handle_ref<T>() noexcept
	{
		typed_handle_ref<T> tmp;
		tmp.object = object;
		tmp.type_ = type_;
		return tmp;
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
		std::remove_reference_t<T> get() noexcept;

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
	std::remove_reference_t<T> to_value_impl(gsdk::ScriptVariant_t &) noexcept = delete;
	template <typename T>
	std::remove_reference_t<T> to_value_impl(const gsdk::ScriptVariant_t &) noexcept = delete;
	template <typename T>
	std::remove_reference_t<T> to_value_impl(gsdk::ScriptVariant_t &&) noexcept = delete;
}

#include "variant.tpp"

namespace vmod::vscript
{
	namespace detail
	{
		template <typename T>
		concept initialize_data_specialized =
			requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, const std::remove_cvref_t<std::decay_t<T>> &)>(initialize_data_impl); } ||
			requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, std::remove_cvref_t<std::decay_t<T>> &)>(initialize_data_impl); } ||
			requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, std::remove_cvref_t<std::decay_t<T>>)>(initialize_data_impl); } ||
			requires () { static_cast<void(*)(gsdk::ScriptVariant_t &, std::remove_cvref_t<std::decay_t<T>> &&)>(initialize_data_impl); }
		;

		template <typename T>
		concept cref_to_value_specialized =
			requires () { static_cast<T(*)(const gsdk::ScriptVariant_t &)>(to_value_impl<T>); }
		;

		template <typename T>
		concept ref_to_value_specialized =
			requires () { static_cast<T(*)(gsdk::ScriptVariant_t &)>(to_value_impl<T>); }
		;

		template <typename T>
		concept rref_to_value_specialized =
			requires () { static_cast<T(*)(gsdk::ScriptVariant_t &&)>(to_value_impl<T>); }
		;

		template <typename T>
		concept any_to_value_specialized = (
			cref_to_value_specialized<T> ||
			ref_to_value_specialized<T> ||
			rref_to_value_specialized<T>
		);

		template <typename T>
		concept type_to_field_specialized =
			requires () { static_cast<gsdk::ScriptDataType_t(*)()>(type_to_field_impl<T>); }
		;

		template <typename T>
		concept is_vscript_variant = std::is_base_of_v<gsdk::ScriptVariant_t, std::decay_t<T>>;

		template <typename T>
		inline void initialize_data(gsdk::ScriptVariant_t &var, T &&value) noexcept
		{
			using decayed_t = std::decay_t<T>;

			if constexpr(!std::is_base_of_v<gsdk::ScriptVariant_t, decayed_t>) {
				std::memset(var.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));
			}

			if constexpr(detail::initialize_data_specialized<T>) {
				initialize_data_impl(var, std::forward<T>(value));
			} else if constexpr(!std::is_same_v<decayed_t, gsdk::HSCRIPT> && std::is_pointer_v<decayed_t>) {
				initialize_data_impl(var, static_cast<void *>(value));
			} else if constexpr(std::is_enum_v<decayed_t>) {
				initialize_data_impl(var, static_cast<std::underlying_type_t<T>>(value));
			} else if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, decayed_t>) {
				std::memcpy(var.m_data, value.m_data, sizeof(gsdk::ScriptVariant_t::m_data));
			} else {
				static_assert(false_t<T>::value);
			}
		}
	}

	template <typename T>
	constexpr inline gsdk::ScriptDataType_t type_to_field() noexcept;

	template <typename T, detail::is_vscript_variant VT>
	std::remove_reference_t<T> to_value(VT &&var) noexcept;

	namespace detail
	{
		template <std::size_t i, typename T, detail::is_vscript_variant VT>
		static void to_value_impl(VT &&vscript_var, T &std_var) noexcept
		{
			using type = std::variant_alternative_t<i, T>;

			if(vscript::type_to_field<type>() == vscript_var.m_type) {
				std_var.template emplace<type>(vscript::to_value<type>(std::forward<VT>(vscript_var)));
			} else if constexpr(i < (std::variant_size_v<T>-1)) {
				to_value_impl<i+1>(std::forward<VT>(vscript_var), std_var);
			}
		}
	}

	template <typename T, detail::is_vscript_variant VT>
	std::remove_reference_t<T> to_value(VT &&var) noexcept
	{
		constexpr bool isconst{std::is_const_v<std::remove_reference_t<VT>>};
		//constexpr bool isrref{std::is_rvalue_reference_v<VT>};

		using BVT = std::conditional_t<isconst, const gsdk::ScriptVariant_t, gsdk::ScriptVariant_t>;
		BVT *target_var{&var};

		using decayed_t = std::decay_t<T>;

		if constexpr(detail::any_to_value_specialized<decayed_t>) {
			if constexpr(detail::rref_to_value_specialized<decayed_t> && /*isrref &&*/ !isconst) {
				return to_value_impl<decayed_t>(std::move(*target_var));
			} else if constexpr(detail::ref_to_value_specialized<decayed_t> && !isconst) {
				return to_value_impl<decayed_t>(*target_var);
			} else if constexpr(detail::cref_to_value_specialized<decayed_t>) {
				return to_value_impl<decayed_t>(static_cast<const gsdk::ScriptVariant_t &>(*target_var));
			} else {
				static_assert(false_t<decayed_t>::value);
			}
		} else if constexpr(!std::is_same_v<decayed_t, gsdk::HSCRIPT> && std::is_pointer_v<decayed_t>) {
			return static_cast<T>(to_value_impl<void *>(static_cast<const gsdk::ScriptVariant_t &>(*target_var)));
		} else if constexpr(std::is_enum_v<decayed_t>) {
			return static_cast<T>(to_value_impl<std::underlying_type_t<T>>(static_cast<const gsdk::ScriptVariant_t &>(*target_var)));
		} else if constexpr(is_optional<decayed_t>::value) {
			if(target_var->m_type == gsdk::FIELD_VOID) {
				return std::nullopt;
			}

			return to_value<typename T::value_type>(std::forward<VT>(*target_var));
		} else if constexpr(is_std_variant<decayed_t>::value) {
			T tmp;
			detail::to_value_impl<0>(std::forward<VT>(*target_var), tmp);
			return tmp;
		} else if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, decayed_t>) {
			return var;
		} else {
			static_assert(false_t<T>::value);
		}
	}

	template <typename T>
	std::remove_reference_t<T> missing_value() noexcept
	{
		using decayed_t = std::decay_t<T>;

		if constexpr(is_optional<decayed_t>::value) {
			return std::nullopt;
		} else if constexpr(std::is_same_v<decayed_t, gsdk::HSCRIPT>) {
			return gsdk::INVALID_HSCRIPT;
		} else {
			return std::remove_cvref_t<decayed_t>{};
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
			return type_to_field<typename T::value_type>();
		} else if constexpr(is_std_variant<decayed_t>::value) {
			return type_to_field<std::variant_alternative_t<0, T>>();
		} else if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, decayed_t>) {
			return gsdk::FIELD_VARIANT;
		} else {
			static_assert(false_t<T>::value);
		}
	}

#ifdef __VMOD_USING_CUSTOM_VM
	namespace detail
	{
		template <std::size_t i, typename T>
		static void add_variant_types(gsdk::ScriptDataTypeAndFlags_t &param) noexcept
		{
			param.set_extra_type(type_to_field<std::variant_alternative_t<i, T>>());

			if constexpr(i < (std::variant_size_v<T>-1)) {
				add_variant_types<i+1, T>(param);
			}
		}
	}

	template <typename T>
	constexpr inline gsdk::ScriptDataTypeAndFlags_t type_to_field_and_flags() noexcept
	{
		using decayed_t = std::decay_t<T>;

		gsdk::ScriptDataTypeAndFlags_t param;
		param.set_main_type(type_to_field<T>());

		if constexpr(is_optional<decayed_t>::value) {
			param.add_flags(gsdk::FIELD_FLAG_OPTIONAL);
		} else if constexpr(is_std_variant<decayed_t>::value) {
			detail::add_variant_types<1, T>(param);
		}

		return param;
	}
#endif

	template <typename T>
	inline void to_variant(gsdk::ScriptVariant_t &var, T &&value) noexcept
	{
		using decayed_t = std::decay_t<T>;

		var.m_type = static_cast<short>(type_to_field<decayed_t>());
		var.m_flags = gsdk::SV_NOFLAGS;
		detail::initialize_data(var, std::forward<T>(value));

		if constexpr(std::is_base_of_v<gsdk::ScriptVariant_t, decayed_t> && std::is_rvalue_reference_v<T>) {
			value.m_flags &= ~gsdk::SV_FREE;
			value.reset();
		}
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
	std::remove_reference_t<T> variant::get() noexcept
	{ return to_value<T>(*this); }

	template <typename T>
	std::remove_reference_t<T> variant::get() const noexcept
	{ return to_value<T>(*this); }
}

namespace std
{
	template <>
	struct hash<vmod::vscript::handle_ref>
	{
		inline size_t operator()(vmod::vscript::handle_ref ptr) const noexcept
		{ return hash<gsdk::HSCRIPT>{}(*ptr); }
	};

	template <gsdk::ScriptHandleType_t T>
	struct hash<vmod::vscript::typed_handle_ref<T>>
	{
		inline size_t operator()(vmod::vscript::typed_handle_ref<T> ptr) const noexcept
		{ return hash<gsdk::HSCRIPT>{}(*ptr); }
	};
}
