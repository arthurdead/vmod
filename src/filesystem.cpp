#include "filesystem.hpp"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace vmod
{
	std::unique_ptr<unsigned char[]> read_file(const std::filesystem::path &path) noexcept
	{
		int fd{open(path.c_str(), O_RDONLY)};
		if(fd < 0) {
			return {};
		}

		struct stat stat;
		fstat(fd, &stat);
		std::size_t size{static_cast<std::size_t>(stat.st_size)};

		std::unique_ptr<unsigned char[]> data{new unsigned char[size]};
		read(fd, data.get(), size);
		data[size] = '\0';

		close(fd);

		return data;
	}

	void write_file(const std::filesystem::path &path, const unsigned char *data, std::size_t size) noexcept
	{
		std::error_code err;
		std::filesystem::create_directories(path.parent_path(), err);

		mode_t mode{
			S_IRUSR|S_IWUSR|
			S_IRGRP|S_IWGRP
		};

		int fd{open(path.c_str(), O_WRONLY|O_CREAT|O_TRUNC, mode)};
		if(fd < 0) {
			return;
		}

		write(fd, data, size);

		close(fd);
	}
}
