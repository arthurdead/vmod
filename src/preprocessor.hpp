#pragma once

#include <string>
#include <filesystem>
#include <memory>
#include "convar.hpp"

namespace vmod
{
	class squirrel_preprocessor final
	{
	public:
		squirrel_preprocessor() noexcept;
		~squirrel_preprocessor() noexcept;

		bool preprocess(std::string &str, const std::filesystem::path &path, std::vector<std::filesystem::path> &incs) noexcept;

	private:
		friend class vmod;

		bool initialize() noexcept;

		static squirrel_preprocessor *current;

		ConVar vmod_preproc_dump;

		enum class print_state : unsigned char
		{
			unknown,
			warning,
			error
		};

		enum print_state print_state;

		std::vector<std::filesystem::path> *curr_incs;
		const std::filesystem::path *curr_path;
	};
}
