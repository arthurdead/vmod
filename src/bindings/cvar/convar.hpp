#pragma once

#include "../../vscript/variant.hpp"
#include "../../plugin.hpp"
#include "../../convar.hpp"
#include "../../vscript/class_desc.hpp"

namespace vmod::bindings::cvar
{
	class singleton;

	class convar final : public plugin::owned_instance
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		~convar() noexcept override;

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	private:
		convar() = delete;
		convar(const convar &) = delete;
		convar &operator=(const convar &) = delete;
		convar(convar &&) = delete;
		convar &operator=(convar &&) = delete;

		static vscript::class_desc<convar> desc;

		inline convar(gsdk::ConVar *var_, bool free_) noexcept
			: var{var_}, free{free_}
		{
		}

		inline bool initialize() noexcept
		{ return register_instance(&desc); }

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

		gsdk::ConVar *var;
		bool free;
	};
}
