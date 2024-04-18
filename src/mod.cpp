#include "mod.hpp"
#include "main.hpp"
#include "filesystem.hpp"

namespace vmod
{
	void mod::init() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(!std::filesystem::exists(path)) {
			return;
		}

		std::filesystem::path plugins_dir{path/"plugins"sv};

		if(std::filesystem::exists(plugins_dir)) {
			std::error_code ec;
			for(const auto &file : std::filesystem::directory_iterator{plugins_dir, ec}) {
				std::filesystem::path filepath{file.path()};
				std::filesystem::path filename{filepath.filename()};

				if(filename.native()[0] == '.') {
					continue;
				} else if(!file.is_regular_file()) {
					remark("vmod: '%s' not a file"sv, filepath.c_str());
					continue;
				}

				auto required_ext{main::instance().scripts_extension()};

				auto ext{filename.extension()};
				if(ext != required_ext) {
					if(ext != ".disabled"sv) {
						remark("vmod: '%s' invalid extension expected '%s'\n"sv, filepath.c_str(), required_ext.data());
					}
					continue;
				}

				std::unique_ptr<plugin> pl{new plugin{filepath}};
				plugins.emplace(std::move(filepath), std::move(pl));
			}
		}

		std::filesystem::path assets_dir{path/"assets"sv};

		if(std::filesystem::exists(assets_dir)) {
			paths.emplace_back(assets_dir);

			std::error_code ec;
			for(const auto &file : std::filesystem::directory_iterator{assets_dir, ec}) {
				std::filesystem::path filepath{file.path()};
				std::filesystem::path filename{filepath.filename()};

				if(filename.native()[0] == '.') {
					continue;
				} else if(!file.is_regular_file()) {
					continue;
				}

				auto ext{filename.extension()};
				if(ext != ".vpk"sv) {
					continue;
				}

				vpks.emplace_back(std::move(filepath));
			}
		}

		std::filesystem::path workshop_file{path/"workshop.txt"sv};

		if(std::filesystem::exists(workshop_file)) {
			auto &main{main::instance()};

			size_t size{0};
			auto bytes{read_file(workshop_file, size)};
			char *begin{reinterpret_cast<char *>(bytes.get())};
			char *end{begin + size};
			while(begin != end) {
				char *line{begin};
				while(line != end && *line != '\n' && *line != '\0') {
					++line;
				}
				*line = '\0';
				auto id{static_cast<PublishedFileId_t>(std::strtoul(begin, nullptr, 10))};
				if(main.track_workshop_item(id, false)) {
					workshop_items.emplace_back(id);
				}
				begin = line;
			}
		}
	}

	void mod::unload() noexcept
	{
		using namespace std::literals::string_view_literals;

		for(auto &it : plugins) {
			it.second->unload();
		}

		instance_.free();

		auto &main{main::instance()};

		for(const auto &it : paths) {
			main.remove_search_path(it, "GAME"sv);
			main.remove_search_path(it, "mod"sv);
		}

		for(const auto &it : vpks) {
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			filesystem->RemoveVPKFile(it.c_str());
		#else
			filesystem->RemoveVPKFile(it.c_str(), "GAME");
			filesystem->RemoveVPKFile(it.c_str(), "mod");
		#endif
		}

		for(auto it : workshop_items) {
			main.untrack_workshop_item(it, false);
		}

		loaded = false;
	}

	mod::load_status mod::reload() noexcept
	{
		unload();

		plugins.clear();
		paths.clear();
		vpks.clear();
		workshop_items.clear();

		init();

		return load();
	}

	mod::load_status mod::load() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(loaded) {
			return mod::load_status::success;
		}

		if(!std::filesystem::exists(path)) {
			error("vmod: mod '%s' path doenst exist\n"sv, path.c_str());
			return mod::load_status::error;
		}

		gsdk::IScriptVM *vm{vscript::vm()};

		instance_ = vm->RegisterInstance(&desc, this);
		if(!instance_) {
			error("vmod: mod '%s' failed to register its own instance\n"sv, path.c_str());
			return load_status::error;
		}

		vm->SetInstanceUniqeId(*instance_, path.c_str());

		auto &main{main::instance()};

		for(const auto &it : paths) {
			main.add_search_path(it, "GAME"sv);
			//main.add_search_path(it, "mod"sv);
		}

		for(const auto &it : vpks) {
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			filesystem->AddVPKFile(it.c_str());
		#else
			filesystem->AddVPKFile(it.c_str(), "GAME");
			//filesystem->AddVPKFile(it.c_str(), "mod");
		#endif
		}

		bool any_loaded{false};

		{
			auto it{plugins.begin()};
			while(it != plugins.end()) {
				auto load_ret{it->second->load()};
				if(load_ret == plugin::load_status::disabled) {
					plugins.erase(it);
					continue;
				} else if(load_ret == plugin::load_status::error) {
					error("vmod: mod '%s' failed to load\n"sv, path.c_str());
					return mod::load_status::error;
				}
				any_loaded = true;
				++it;
			}
		}

		if(!any_loaded && paths.empty() && vpks.empty()) {
			return mod::load_status::disabled;
		}

		loaded = true;
		return mod::load_status::success;
	}

	void mod::game_frame(bool simulating) noexcept
	{
		if(!loaded) {
			return;
		}

		for(auto &it : plugins) {
			it.second->game_frame(simulating);
		}
	}
}
