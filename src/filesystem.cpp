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
}
