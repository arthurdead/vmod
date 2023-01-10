#pragma once

#include <filesystem>

namespace vmod::bindings::ent
{
	extern bool bindings() noexcept;
	extern bool create_get() noexcept;
	extern void unbindings() noexcept;

	extern bool detours() noexcept;

	extern void write_docs(const std::filesystem::path &dir) noexcept;
}
