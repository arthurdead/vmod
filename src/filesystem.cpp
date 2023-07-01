#include "filesystem.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>

namespace vmod
{
	std::unique_ptr<unsigned char[]> read_file(const std::filesystem::path &path) noexcept
	{
		std::size_t size;
		return read_file(path, size);
	}

	std::unique_ptr<unsigned char[]> read_file(const std::filesystem::path &path, std::size_t &size) noexcept
	{
		using namespace std::literals::string_view_literals;

		int fd{open(path.c_str(), O_RDONLY)};
		if(fd < 0) {
			std::cout << "\033[0;31m"sv << "failed to open "sv << path << " for reading\n"sv << "\033[0m"sv;
			size = 0;
			return {};
		}

		struct stat stat;
		fstat(fd, &stat);
		size = static_cast<std::size_t>(stat.st_size);

		std::unique_ptr<unsigned char[]> data{new unsigned char[size+1]};
		read(fd, data.get(), size);
		data[size] = '\0';

		close(fd);

		return data;
	}

	void write_file(const std::filesystem::path &path, const unsigned char *data, std::size_t size) noexcept
	{
		using namespace std::literals::string_view_literals;

		mode_t mode{
			S_IRUSR|S_IWUSR|
			S_IRGRP|S_IWGRP
		};

		int fd{open(path.c_str(), O_WRONLY|O_CREAT|O_TRUNC, mode)};
		if(fd < 0) {
			std::cout << "\033[0;31m"sv << "failed to open "sv << path << " for writing\n"sv << "\033[0m"sv;
			return;
		}

		write(fd, data, size);

		close(fd);
	}
}
