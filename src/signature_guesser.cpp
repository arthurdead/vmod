#include "gsdk_library.hpp"
#include "symbol_cache.hpp"
#include "filesystem.hpp"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <charconv>

int main(int argc, [[maybe_unused]] char *argv[], [[maybe_unused]] char *[])
{
	using namespace std::literals::string_view_literals;

	if(argc != 2) {
		std::cout << "vmod: usage: <dir>\n"sv;
		return EXIT_FAILURE;
	}

	std::filesystem::path game_root;
	std::filesystem::path game_dir;

	{
		char exe[PATH_MAX];
		ssize_t len{readlink("/proc/self/exe", exe, sizeof(exe))};
		exe[len] = '\0';

		game_root = exe;
		game_root = game_root.parent_path();
		game_root = game_root.parent_path();
		game_root = game_root.parent_path();
		game_root = game_root.parent_path();

		game_dir = game_root;

		game_root = game_root.parent_path();
	}

	std::filesystem::path dir{argv[1]};

#ifndef GSDK_NO_SYMBOLS
	if(!vmod::symbol_cache::initialize()) {
		return EXIT_FAILURE;
	}

	//TODO!!!

	return EXIT_SUCCESS;
#else
	std::cout << "vmod: no symbols available\n"sv;
	return EXIT_FAILURE;
#endif
}
