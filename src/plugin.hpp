#pragma once

#include "vscript/vscript.hpp"
#include "vscript/class_desc.hpp"
#include "vscript/variant.hpp"
#include <filesystem>
#include <vector>
#include <utility>
#include <unordered_map>

namespace vmod
{
	class main;

	class plugin final
	{
		friend class main;

	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		plugin(std::filesystem::path &&path_) noexcept;
		inline plugin(const std::filesystem::path &path_) noexcept
			: plugin{std::filesystem::path{path_}}
		{
		}
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

		inline gsdk::HSCRIPT instance() noexcept
		{ return instance_; }
		inline gsdk::HSCRIPT private_scope() noexcept
		{ return private_scope_; }

		inline operator bool() const noexcept
		{ return running && (script && script != gsdk::INVALID_HSCRIPT); }
		inline bool operator!() const noexcept
		{ return !running || (!script || script == gsdk::INVALID_HSCRIPT); }

		class function
		{
			friend class plugin;

		public:
			~function() noexcept;

			inline operator bool() const noexcept
			{ return (func && func != gsdk::INVALID_HSCRIPT); }
			inline bool operator!() const noexcept
			{ return (!func || func == gsdk::INVALID_HSCRIPT); }

			template <typename R, typename ...Args>
			R execute(Args &&...args) noexcept;

		private:
			function() noexcept = default;

			gsdk::ScriptStatus_t execute_internal(gsdk::ScriptVariant_t &ret, const gsdk::ScriptVariant_t *args, std::size_t num_args) noexcept;
			gsdk::ScriptStatus_t execute_internal() noexcept;
			gsdk::ScriptStatus_t execute_internal(gsdk::ScriptVariant_t &ret) noexcept;
			gsdk::ScriptStatus_t execute_internal(const gsdk::ScriptVariant_t *args, std::size_t num_args) noexcept;

			void unload() noexcept;

			inline bool valid() const noexcept
			{ return ((func && func != gsdk::INVALID_HSCRIPT) && (scope && scope != gsdk::INVALID_HSCRIPT)); }

			gsdk::HSCRIPT scope{gsdk::INVALID_HSCRIPT};
			gsdk::HSCRIPT func{gsdk::INVALID_HSCRIPT};
			plugin *owner{nullptr};

		private:
			function(const function &) = delete;
			function &operator=(const function &) = delete;
			function(function &&) noexcept = delete;
			function &operator=(function &&) noexcept = delete;
		};

		template <typename T>
		class typed_function;

		template <typename R, typename ...Args>
		class typed_function<R(Args...)> : public function
		{
			friend class plugin;

		public:
			inline R operator()(Args ...args) noexcept
			{ return function::execute<R, Args...>(std::forward<Args>(args)...); }

		private:
			using function::function;

			template <typename ER, typename ...EArgs>
			ER execute(EArgs &&...args) = delete;
		};

	private:
		void lookup_function(std::string_view func_name, function &func) noexcept;

	public:
		function lookup_function(std::string_view func_name) noexcept = delete;

		class owned_instance
		{
			friend class plugin;
			friend class main;

		public:
			owned_instance() noexcept = default;
			virtual ~owned_instance() noexcept;

			bool register_instance(gsdk::ScriptClassDesc_t *target_desc) noexcept;

			static bool bindings() noexcept;
			static void unbindings() noexcept;

			static bool register_class(gsdk::ScriptClassDesc_t *target_desc) noexcept;

			inline plugin *owner() noexcept
			{ return owner_; }
			inline gsdk::HSCRIPT owner_scope() noexcept
			{ return owner_ ? owner_->private_scope_ : nullptr; }

		protected:
			gsdk::HSCRIPT instance{gsdk::INVALID_HSCRIPT};

		private:
			static vscript::class_desc<owned_instance> desc;

			void plugin_unloaded() noexcept;

			void set_plugin() noexcept;

			void script_delete() noexcept;

			plugin *owner_{nullptr};

		private:
			owned_instance(const owned_instance &) = delete;
			owned_instance &operator=(const owned_instance &) = delete;
			owned_instance(owned_instance &&) noexcept = delete;
			owned_instance &operator=(owned_instance &&) = delete;
		};

	#if 0
		class callable;

		class callback_instance : public owned_instance
		{
			~callback_instance() noexcept override;

			gsdk::HSCRIPT callback{gsdk::INVALID_HSCRIPT};
			bool post;

			class callable *callable;
		};
	#endif

		class callable
		{
		public:
			enum class return_flags : unsigned char
			{
				ignored =          0,
				halt =       (1 << 0),
				handled =    (1 << 1)
			};
			friend constexpr inline bool operator&(return_flags lhs, return_flags rhs) noexcept
			{ return static_cast<bool>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); }
			friend constexpr inline return_flags operator|(return_flags lhs, return_flags rhs) noexcept
			{ return static_cast<return_flags>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); }

			static_assert(static_cast<unsigned char>(return_flags::ignored) == 0);

		private:
			//std::unordered_map<gsdk::HSCRIPT, callback_instance *> callbacks_pre;
			//std::unordered_map<gsdk::HSCRIPT, callback_instance *> callbacks_post;
		};

	private:
		static vscript::class_desc<plugin> desc;

		struct scope_assume_current final
		{
			static plugin *old_running;

			inline scope_assume_current(plugin *pl_) noexcept
			{
				old_running = assumed_currently_running;
				if(pl_) {
					assumed_currently_running = pl_;
				}
			}
			inline ~scope_assume_current() noexcept
			{ assumed_currently_running = old_running; }
		};

		static plugin *assumed_currently_running;

		gsdk::HSCRIPT script_lookup_function(std::string_view func_name) noexcept;
		gsdk::ScriptVariant_t script_lookup_value(std::string_view val_name) noexcept;

		void game_frame(bool simulating) noexcept;

		void watch() noexcept;
		void unwatch() noexcept;

		std::filesystem::path path;
		std::vector<std::filesystem::path> incs;
		int inotify_fd{-1};
		std::vector<int> watch_fds;

		gsdk::HSCRIPT instance_{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT script{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT private_scope_{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT public_scope_{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT functions_table{gsdk::INVALID_HSCRIPT};
		gsdk::HSCRIPT values_table{gsdk::INVALID_HSCRIPT};

		bool running{false};

		std::unordered_map<std::string, gsdk::HSCRIPT> function_cache;

		std::vector<owned_instance *> owned_instances;
		bool clearing_instances{false};

		typed_function<void()> map_active;
		typed_function<void(std::string_view)> map_loaded;
		typed_function<void()> map_unloaded;
		typed_function<void()> plugin_loaded;
		typed_function<void()> plugin_unloaded;
		typed_function<void()> all_plugins_loaded;
		typed_function<void()> string_tables_created;
		typed_function<void(bool)> game_frame_;

	private:
		plugin() = delete;
		plugin(const plugin &) = delete;
		plugin &operator=(const plugin &) = delete;
		plugin(plugin &&other) = delete;
		plugin &operator=(plugin &&other) = delete;
	};
}

#include "plugin.tpp"
