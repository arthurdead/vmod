#pragma once

#include <filesystem>
#include <memory>

namespace vmod
{
	extern std::unique_ptr<unsigned char[]> read_file(const std::filesystem::path &path) noexcept;
	extern void write_file(const std::filesystem::path &path, const unsigned char *data, std::size_t size) noexcept;
}
