#pragma once

#include <filesystem>
#include <memory>

namespace vmod
{
	extern std::unique_ptr<unsigned char[]> read_file(const std::filesystem::path &path) noexcept;
}
