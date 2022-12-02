#pragma once

#include "vscript.hpp"
#include <filesystem>
#include <vector>
#include <utility>

namespace vmod
{
	class plugin final
	{
		friend class vmod;

	public:
		plugin() = delete;
		plugin(const plugin &) = delete;
		plugin &operator=(const plugin &) = delete;

		inline plugin(plugin &&other) noexcept
		{ operator=(std::move(other)); }
		plugin &operator=(plugin &&other) noexcept;

		plugin(std::filesystem::path &&path_) noexcept;
		inline ~plugin() noexcept
		{ unload(); }

		bool load() noexcept;
		inline bool reload() noexcept {
			unload();
			return load();
		}
		void unload() noexcept;

		inline operator const std::filesystem::path &() const noexcept
		{ return path; }

		gsdk::HSCRIPT scope() noexcept
		{ return scope_; }
		gsdk::HSCRIPT instance() noexcept
		{ return instance_; }

		inline operator bool() const noexcept
		{ return script != gsdk::INVALID_HSCRIPT; }
		inline bool operator!() const noexcept
		{ return script == gsdk::INVALID_HSCRIPT; }

		class function
		{
			friend class plugin;

		public:
			inline function(function &&other) noexcept
			{ operator=(std::move(other)); }
			inline function &operator=(function &&other) noexcept
			{
				scope = other.scope;
				func = other.func;
				other.scope = gsdk::INVALID_HSCRIPT;
				other.func = gsdk::INVALID_HSCRIPT;
				return *this;
			}

			~function() noexcept;

			inline operator bool() const noexcept
			{ return func != gsdk::INVALID_HSCRIPT; }
			inline bool operator!() const noexcept
			{ return func == gsdk::INVALID_HSCRIPT; }

			template <typename R, typename ...Args>
			R execute(Args &&...args) noexcept;

		private:
			inline function() noexcept
				: scope{gsdk::INVALID_HSCRIPT}, func{gsdk::INVALID_HSCRIPT}
			{
			}
			function(const function &) = delete;
			function &operator=(const function &) = delete;

			function(plugin &pl, std::string_view func_name) noexcept;

			gsdk::ScriptStatus_t execute_internal(script_variant_t &ret, const std::vector<script_variant_t> &args) noexcept;
			gsdk::ScriptStatus_t execute_internal() noexcept;
			gsdk::ScriptStatus_t execute_internal(script_variant_t &ret) noexcept;
			gsdk::ScriptStatus_t execute_internal(const std::vector<script_variant_t> &args) noexcept;

			void unload() noexcept;

			inline bool valid() const noexcept
			{ return ((func != gsdk::INVALID_HSCRIPT) && (scope != gsdk::INVALID_HSCRIPT)); }

			gsdk::HSCRIPT scope;
			gsdk::HSCRIPT func;
		};

		template <typename T>
		class typed_function;

		template <typename R, typename ...Args>
		class typed_function<R(Args...)> : public function
		{
		public:
			using function::function;
			using function::operator=;

			inline R operator()(Args ...args) noexcept
			{ return function::execute<R, Args...>(std::forward<Args>(args)...); }

		private:
			template <typename ER, typename ...EArgs>
			ER execute(EArgs &&...args) = delete;
		};

		inline function lookup_function(std::string_view func_name) noexcept
		{ return {*this, func_name}; }

	private:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		gsdk::HSCRIPT script_lookup_function(std::string_view func_name) noexcept;
		script_variant_t script_lookup_value(std::string_view val_name) noexcept;

		std::filesystem::path path;
		std::string name;

		gsdk::HSCRIPT instance_{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT script{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT scope_{gsdk::INVALID_HSCRIPT};

		std::unordered_map<std::string, gsdk::HSCRIPT> function_cache;

		typed_function<void()> map_active;
		typed_function<void(std::string_view)> map_loaded;
		typed_function<void()> map_unloaded;
		typed_function<void()> plugin_loaded;
		typed_function<void()> plugin_unloaded;
		typed_function<void()> all_plugins_loaded;
	};

	extern class_desc_t<plugin> plugin_desc;
}

#include "plugin.tpp"
