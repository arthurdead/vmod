#pragma once

#include "plugin.hpp"
#include "vscript/variant.hpp"
#include "vscript/class_desc.hpp"
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <memory>

namespace vmod
{
	class main;

	class mod final
	{
		friend class main;

	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		inline mod(std::filesystem::path &&path_) noexcept
			: path{std::move(path_)}
		{ init(); }
		inline mod(const std::filesystem::path &path_) noexcept
			: path{path_}
		{ init(); }
		inline ~mod() noexcept
		{ unload(); }

		enum class load_status : unsigned char
		{
			error,
			disabled,
			success
		};

		load_status load() noexcept;
		load_status reload() noexcept;
		void unload() noexcept;

		inline operator bool() const noexcept
		{ return loaded; }
		inline bool operator!() const noexcept
		{ return !loaded; }

		inline vscript::handle_ref instance() noexcept
		{ return instance_; }

	private:
		static vscript::class_desc<mod> desc;

		void init() noexcept;

		void game_frame(bool simulating) noexcept;

		template <typename T, typename ...Args>
		void call_func_on_plugins(plugin::typed_function<T> plugin::*func, Args &&...args) noexcept
		{
			for(auto &it : plugins) {
				if(!*it.second) {
					continue;
				}

				auto ptr{it.second.get()};
				auto &var{ptr->*func};

				var(std::forward<Args>(args)...);
			}
		}

		bool loaded{false};

		std::filesystem::path path;

		std::vector<std::filesystem::path> paths;
		std::vector<std::filesystem::path> vpks;

		std::unordered_map<std::filesystem::path, std::unique_ptr<plugin>> plugins;

		vscript::handle_wrapper instance_{};

	private:
		mod() = delete;
		mod(const mod &) = delete;
		mod &operator=(const mod &) = delete;
		mod(mod &&other) = delete;
		mod &operator=(mod &&other) = delete;
	};
}
