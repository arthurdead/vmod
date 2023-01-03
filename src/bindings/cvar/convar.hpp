#pragma once

#include "../../vscript/variant.hpp"
#include "../../plugin.hpp"
#include "../../convar.hpp"
#include "../../vscript/class_desc.hpp"
#include "../instance.hpp"

namespace vmod::bindings::cvar
{
	class singleton;

	class convar_base
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		virtual ~convar_base() noexcept;

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	protected:
		static vscript::class_desc<convar_base> desc;

		inline convar_base(gsdk::ConVar *var_) noexcept
			: var{var_}
		{
		}

	private:
		inline int script_get_int() const noexcept
		{ return var->ConVar::GetInt(); }
		inline float script_get_float() const noexcept
		{ return var->ConVar::GetFloat(); }
		inline std::string_view script_get_string() const noexcept
		{ return var->ConVar::GetString(); }
		inline bool script_get_bool() const noexcept
		{ return var->ConVar::GetBool(); }

		inline void script_set_int(int value) noexcept
		{ var->ConVar::SetValue(value); }
		inline void script_set_float(float value) noexcept
		{ var->ConVar::SetValue(value); }
		inline void script_set_string(std::string_view value) noexcept
		{ var->ConVar::SetValue(value.data()); }
		inline void script_set_bool(bool value) noexcept
		{ var->ConVar::SetValue(value); }

		void script_set(vscript::variant &&value) noexcept;

		vscript::variant script_get() const noexcept;

	protected:
		gsdk::ConVar *var;

	private:
		convar_base() = delete;
		convar_base(const convar_base &) = delete;
		convar_base &operator=(const convar_base &) = delete;
		convar_base(convar_base &&) = delete;
		convar_base &operator=(convar_base &&) = delete;
	};

	class convar_ref final : public convar_base, public instance_base
	{
		friend class singleton;
		friend class convar_base;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		using convar_base::convar_base;

		~convar_ref() noexcept override;

	protected:
		static vscript::class_desc<convar_ref> desc;

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }
	};

	class convar final : public convar_base, public plugin::owned_instance
	{
		friend class singleton;
		friend class convar_base;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		using convar_base::convar_base;

		~convar() noexcept override;

	protected:
		static vscript::class_desc<convar> desc;

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }
	};
}
