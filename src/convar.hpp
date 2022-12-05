#pragma once

#include "gsdk/vstdlib/convar.hpp"
#include "gsdk.hpp"
#include <string_view>
#include <functional>

namespace vmod
{
	class ConCommand final : private gsdk::ConCommand
	{
	public:
		ConCommand() noexcept = default;
		inline ~ConCommand() noexcept override
		{ unregister(); }

		template <typename T>
		inline void initialize(std::string_view name, T &&func_) noexcept
		{ initialize<T>(name, gsdk::FCVAR_NONE, std::move(func_)); }

		template <typename T>
		inline void initialize(std::string_view name, int flags, T &&func_) noexcept
		{
			m_pszName = name.data();
			m_pszHelpString = "";
			m_nFlags = flags;

			func = std::move(func_);

			cvar->RegisterConCommand(static_cast<gsdk::ConCommandBase *>(this));
		}

		inline void unregister() noexcept
		{
			if(m_bRegistered) {
				cvar->UnregisterConCommand(static_cast<gsdk::ConCommandBase *>(this));
			}
		}

		inline operator bool() const noexcept
		{ return static_cast<bool>(func) && m_bRegistered; }
		inline bool operator!() const noexcept
		{ return !static_cast<bool>(func) || !m_bRegistered; }

		template <typename T>
		inline ConCommand &operator=(T &&fnc) noexcept
		{
			func = std::move(fnc);
			return *this;
		}

		inline void operator()(const gsdk::CCommand &args) noexcept
		{
			if(func) {
				func(args);
			}
		}
		inline void operator()() noexcept
		{
			if(func) {
				func(gsdk::CCommand{});
			}
		}

	private:
		void Dispatch(const gsdk::CCommand &args) override;

		gsdk::CVarDLLIdentifier_t GetDLLIdentifier() const override;

		void Init() override;

		std::function<void(const gsdk::CCommand &)> func;
	};
}
