#pragma once

#include <string>
#include <filesystem>
#include <memory>
#include "convar.hpp"

namespace vmod
{
	class squirrel_preprocessor final
	{
		friend class main;

	public:
		squirrel_preprocessor() noexcept;
		~squirrel_preprocessor() noexcept;

		bool preprocess(std::string &str, const std::filesystem::path &path, std::vector<std::filesystem::path> &incs) noexcept;

	private:
		bool initialize() noexcept;
		void shutdown() noexcept;

		static void warn_func(const char *fmt, va_list args) __attribute__((__format__(__printf__, 2, 0)));
		static void msg_func(const char *fmt, va_list args) __attribute__((__format__(__printf__, 2, 0)));

		static squirrel_preprocessor *current;

		ConVar vmod_preproc_dump;

		enum class print_state : unsigned char
		{
			unknown,
			warning,
			error
		};

		print_state print_state{print_state::unknown};

		char path_buff[PATH_MAX];
		static char msg_buff[gsdk::MAXPRINTMSG];

		std::vector<std::filesystem::path> *curr_incs{nullptr};
		const std::filesystem::path *curr_path{nullptr};

		bool initialized{false};

		std::filesystem::path pp_dir;

	private:
		squirrel_preprocessor(const squirrel_preprocessor &) = delete;
		squirrel_preprocessor &operator=(const squirrel_preprocessor &) = delete;
		squirrel_preprocessor(squirrel_preprocessor &&) = delete;
		squirrel_preprocessor &operator=(squirrel_preprocessor &&) = delete;
	};
}
