#pragma once

#include <filesystem>

namespace vmod::bindings::cvar
{
	extern bool bindings() noexcept;
	extern void unbindings() noexcept;

	extern void write_docs(const std::filesystem::path &dir) noexcept;
}
