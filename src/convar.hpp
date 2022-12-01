#pragma once

#include "gsdk/vstdlib/convar.hpp"
#include "gsdk.hpp"
#include "vmod.hpp"
#include <string_view>
#include <functional>

namespace vmod
{
	class ConCommand final : private gsdk::ConCommand
	{
	public:
		ConCommand(std::string_view name, int flags = gsdk::FCVAR_NONE) noexcept;
		inline ~ConCommand() noexcept override
		{ unregister(); }

		inline void initialize() noexcept
		{ Init(); }
		void unregister() noexcept;

		template <typename T>
		inline ConCommand &operator=(T &&fnc) noexcept
		{
			func = std::move(fnc);
			return *this;
		}

		inline void operator()(const gsdk::CCommand &args) noexcept
		{ func(args); }
		inline void operator()() noexcept
		{ func(gsdk::CCommand{}); }

	private:
		void Dispatch(const gsdk::CCommand &args) override;

		gsdk::CVarDLLIdentifier_t GetDLLIdentifier() const override;

		void Init() override;

		std::function<void(const gsdk::CCommand &)> func;
	};
}
