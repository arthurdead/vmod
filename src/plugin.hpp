#pragma once

#include "vscript.hpp"
#include <filesystem>
#include <vector>
#include <utility>
#include <unordered_map>

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
		~plugin() noexcept;

		enum class load_status : unsigned char
		{
			error,
			disabled,
			success
		};

		load_status load() noexcept;
		inline load_status reload() noexcept {
			unload();
			return load();
		}
		void unload() noexcept;

		inline operator const std::filesystem::path &() const noexcept
		{ return path; }

		inline gsdk::HSCRIPT instance() noexcept
		{ return instance_; }
		inline gsdk::HSCRIPT private_scope() noexcept
		{ return private_scope_; }

		inline operator bool() const noexcept
		{ return running && script != gsdk::INVALID_HSCRIPT; }
		inline bool operator!() const noexcept
		{ return !running || script == gsdk::INVALID_HSCRIPT; }

		class function
		{
			friend class plugin;

		public:
			inline function(function &&other) noexcept
			{ operator=(std::move(other)); }
			inline function &operator=(function &&other) noexcept
			{
				scope = other.scope;
				other.scope = gsdk::INVALID_HSCRIPT;
				func = other.func;
				other.func = gsdk::INVALID_HSCRIPT;
				owner = other.owner;
				other.owner = nullptr;
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
				: scope{gsdk::INVALID_HSCRIPT},
				func{gsdk::INVALID_HSCRIPT},
				owner{nullptr}
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
			plugin *owner;
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

		class owned_instance
		{
		public:
			virtual ~owned_instance() noexcept;
			virtual void plugin_unloaded() noexcept;

			void set_plugin() noexcept;

		protected:
			inline plugin *owner() noexcept
			{ return owner_; }
			inline gsdk::HSCRIPT owner_scope() noexcept
			{ return owner_ ? owner_->private_scope_ : nullptr; }

		private:
			plugin *owner_;
		};

	private:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		struct scope_assume_current final
		{
			static plugin *old_running;

			inline scope_assume_current(plugin *pl_) noexcept
			{
				old_running = assumed_currently_running;
				assumed_currently_running = pl_;
			}
			inline ~scope_assume_current() noexcept
			{ assumed_currently_running = old_running; }
		};

		static plugin *assumed_currently_running;

		gsdk::HSCRIPT script_lookup_function(std::string_view func_name) noexcept;
		script_variant_t script_lookup_value(std::string_view val_name) noexcept;

		void game_frame(bool simulating) noexcept;

		void watch() noexcept;
		void unwatch() noexcept;

		std::filesystem::path path;
		std::vector<std::filesystem::path> incs;
		int inotify_fd;
		std::vector<int> watch_ds;

		std::string name;

		gsdk::HSCRIPT instance_;
		gsdk::HSCRIPT script;
		gsdk::HSCRIPT private_scope_;
		gsdk::HSCRIPT public_scope_;
		gsdk::HSCRIPT functions_table;
		gsdk::HSCRIPT values_table;

		bool running;

		std::unordered_map<std::string, gsdk::HSCRIPT> function_cache;

		std::vector<owned_instance *> owned_instances;
		bool deleting_instances;

		typed_function<void()> map_active;
		typed_function<void(std::string_view)> map_loaded;
		typed_function<void()> map_unloaded;
		typed_function<void()> plugin_loaded;
		typed_function<void()> plugin_unloaded;
		typed_function<void()> all_plugins_loaded;
		typed_function<void()> string_tables_created;
		typed_function<void(bool)> game_frame_;
	};

	extern class_desc_t<plugin> plugin_desc;
}

#include "plugin.tpp"
