#pragma once

#include "gsdk/vstdlib/convar.hpp"
#include "gsdk.hpp"
#include <string_view>
#include <string>
#include <functional>

namespace vmod
{
	class ConCommand final : private gsdk::ConCommand
	{
	public:
		ConCommand() noexcept = default;
		~ConCommand() noexcept override;

		template <typename T>
		inline void initialize(std::string_view name, T &&func_) noexcept
		{ initialize<T>(name, gsdk::FCVAR_NONE, std::move(func_)); }

		template <typename T>
		inline void initialize(std::string_view name, int flags, T &&func_) noexcept
		{
			gsdk::ConCommand::CreateBase(name.data(), nullptr, flags);

			func = std::move(func_);

			cvar->RegisterConCommand(static_cast<gsdk::ConCommandBase *>(this));
		}

		inline void unregister() noexcept
		{
			if(gsdk::ConCommandBase::IsRegistered()) {
				cvar->UnregisterConCommand(static_cast<gsdk::ConCommandBase *>(this));
			}
		}

		inline operator bool() const noexcept
		{ return static_cast<bool>(func) && gsdk::ConCommandBase::IsRegistered(); }
		inline bool operator!() const noexcept
		{ return !static_cast<bool>(func) || !gsdk::ConCommandBase::IsRegistered(); }

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

	class ConVar final : public gsdk::ConVar
	{
	public:
		ConVar() noexcept = default;
		~ConVar() noexcept override;

		template <typename T>
		inline void initialize(std::string_view name, T &&value) noexcept
		{ initialize(name, std::forward<T>(value), gsdk::FCVAR_NONE); }

		template <typename T>
		void initialize(std::string_view name, T &&value, int flags) noexcept;

		inline void unregister() noexcept
		{
			if(gsdk::ConCommandBase::IsRegistered()) {
				cvar->UnregisterConCommand(static_cast<gsdk::ConCommandBase *>(this));
			}
		}

		template <typename T>
		ConVar &set(T &&value) noexcept;

		template <typename T>
		T get() const noexcept;

		template <typename T>
		inline ConVar &operator=(T &&value) noexcept
		{ return set(std::forward<T>(value)); }

		operator bool() const noexcept = delete;
		bool operator!() const noexcept = delete;

		template <typename T>
		inline explicit operator T() const noexcept
		{ return get<T>(); }

	private:
		gsdk::CVarDLLIdentifier_t GetDLLIdentifier() const override;

		void Init() override;

		std::string def_value_str;
	};
}

#include "convar.tpp"
