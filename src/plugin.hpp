#pragma once

#include "vscript/vscript.hpp"
#include "vscript/class_desc.hpp"
#include "vscript/variant.hpp"
#include <filesystem>
#include <vector>
#include <utility>
#include <unordered_map>
#include "bindings/instance.hpp"

namespace vmod
{
	class main;
	class mod;

	class plugin final
	{
		friend class main;
		friend class mod;

	public:
		static bool bindings() noexcept;
		static void unbindings() noexcept;

		plugin(std::filesystem::path &&path__) noexcept;
		plugin(const std::filesystem::path &path__) noexcept;
		~plugin() noexcept;

		enum class load_status : unsigned char
		{
			error,
			disabled,
			success
		};

		load_status load() noexcept;
		load_status reload() noexcept;
		void unload() noexcept;

		inline vscript::instance_handle_ref instance() noexcept
		{ return instance_; }
		inline vscript::scope_handle_ref private_scope() noexcept
		{ return private_scope_; }

		inline std::filesystem::path path() const noexcept
		{ return path_; }

		inline operator bool() const noexcept
		{ return (running && script); }
		inline bool operator!() const noexcept
		{ return (!running || !script); }

		static plugin *assumed_currently_running() noexcept;

		class function
		{
			friend class plugin;

		public:
			~function() noexcept;

			inline operator bool() const noexcept
			{ return static_cast<bool>(func); }
			inline bool operator!() const noexcept
			{ return !func; }
			inline gsdk::HSCRIPT operator*() noexcept
			{ return *func; }

			template <typename R, typename ...Args>
			R execute(Args &&...args) noexcept;

			void free() noexcept;

		private:
			function() noexcept = default;
			function(function &&) noexcept = default;
			function &operator=(function &&) noexcept = default;

			gsdk::ScriptStatus_t execute_internal(gsdk::ScriptVariant_t &ret, const gsdk::ScriptVariant_t *args, std::size_t num_args) noexcept;
			gsdk::ScriptStatus_t execute_internal() noexcept;
			gsdk::ScriptStatus_t execute_internal(gsdk::ScriptVariant_t &ret) noexcept;
			gsdk::ScriptStatus_t execute_internal(const gsdk::ScriptVariant_t *args, std::size_t num_args) noexcept;

			inline bool valid() const noexcept
			{ return static_cast<bool>(func); }

			vscript::scope_handle_ref scope{};
			vscript::func_handle_wrapper func{};
			plugin *owner{nullptr};

		private:
			function(const function &) = delete;
			function &operator=(const function &) = delete;
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

			using function::function;
			using function::operator=;

		private:
			template <typename ER, typename ...EArgs>
			ER execute(EArgs &&...args) = delete;
		};

	public:
		bool lookup_function(std::string_view func_name, function &func) noexcept;
		static bool read_function(vscript::func_handle_ref func_hndl, function &func_obj) noexcept;

	public:
		function lookup_function(std::string_view func_name) noexcept = delete;

		class shared_instance : public bindings::instance_base
		{
			friend class plugin;
			friend class main;

		public:
			shared_instance() noexcept = default;
			~shared_instance() noexcept override;

			shared_instance(shared_instance &&other) noexcept = default;
			shared_instance &operator=(shared_instance &&other) noexcept = default;

			bool register_instance(gsdk::ScriptClassDesc_t *target_desc, void *pthis) noexcept override;
			bool register_instance(gsdk::ScriptClassDesc_t *) = delete;

			static bool bindings() noexcept;
			static void unbindings() noexcept;

			void add_plugin() noexcept;
			void remove_plugin() noexcept;

		public:
			static vscript::class_desc<shared_instance> desc;

		private:
			void plugin_unloaded(plugin *pl) noexcept;

			void script_delete() noexcept;

			std::vector<plugin *> owner_plugins;

		private:
			shared_instance(const shared_instance &) = delete;
			shared_instance &operator=(const shared_instance &) = delete;
		};

		class owned_instance : public bindings::instance_base
		{
			friend class plugin;
			friend class main;

		public:
			owned_instance() noexcept = default;
			~owned_instance() noexcept override;

			bool register_instance(gsdk::ScriptClassDesc_t *target_desc, void *pthis) noexcept override;
			bool register_instance(gsdk::ScriptClassDesc_t *) = delete;

			static bool bindings() noexcept;
			static void unbindings() noexcept;

			static bool register_class(gsdk::ScriptClassDesc_t *target_desc) noexcept;

			inline plugin *owner() noexcept
			{ return owner_plugin; }
			inline vscript::scope_handle_ref owner_scope() noexcept
			{
				if(owner_plugin) {
					return owner_plugin->private_scope_;
				} else {
					return nullptr;
				}
			}

			inline owned_instance(owned_instance &&other) noexcept
			{ operator=(std::move(other)); }
			inline owned_instance &operator=(owned_instance &&other) noexcept
			{
				instance_ = std::move(other.instance_);
				owner_plugin = other.owner_plugin;
				other.owner_plugin = nullptr;
				return *this;
			}

		public:
			static vscript::class_desc<owned_instance> desc;

		protected:
			virtual void free() noexcept
			{ delete this; }

			virtual void plugin_unloaded() noexcept;

		private:
			void set_plugin() noexcept;

			void script_delete() noexcept;

			plugin *owner_plugin{nullptr};

		private:
			owned_instance(const owned_instance &) = delete;
			owned_instance &operator=(const owned_instance &) = delete;
		};

		class callable;

		class callback_instance : public owned_instance
		{
			friend class callable;
			friend class plugin;
			friend class owned_instance;
			friend class main;

		public:
			~callback_instance() noexcept override;

		public:
			callback_instance(callable *caller_, vscript::func_handle_wrapper &&callback_, bool post_) noexcept;

		public:
			static vscript::class_desc<callback_instance> desc;

		public:
			inline bool initialize() noexcept
			{ return register_instance(&desc, this); }

		protected:
			virtual inline void script_toggle(std::optional<bool> value) noexcept
			{
				if(!value) {
					enabled = !enabled;
				} else {
					enabled = *value;
				}
			}
			virtual inline void script_enable() noexcept
			{ enabled = true; }
			virtual inline void script_disable() noexcept
			{ enabled = false; }

		private:
			inline bool script_enabled() noexcept
			{ return enabled; }

			void callable_destroyed() noexcept;

			vscript::func_handle_wrapper callback;
			bool post;
			callable *caller;
			bool enabled;

		private:
			callback_instance() = delete;
			callback_instance(const callback_instance &) = delete;
			callback_instance &operator=(const callback_instance &) = delete;
			callback_instance(callback_instance &&) noexcept = delete;
			callback_instance &operator=(callback_instance &&) = delete;
		};

		//#define __VMOD_PASS_EXTRA_INFO_TO_CALLABLES

		class callable
		{
			friend class callback_instance;

		public:
			callable() noexcept = default;
			virtual ~callable() noexcept;

			enum class return_flags : unsigned char
			{
				ignored =          0,
				error =      (1 << 1),
				halt =       (1 << 2),
				handled =    (1 << 3),
				changed =    (1 << 4)
			};
			friend constexpr inline bool operator&(return_flags lhs, return_flags rhs) noexcept
			{ return static_cast<bool>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); }
			friend constexpr inline return_flags operator|(return_flags lhs, return_flags rhs) noexcept
			{ return static_cast<return_flags>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); }

			static_assert(static_cast<unsigned char>(return_flags::ignored) == 0);

			inline bool empty() const noexcept
			{ return (callbacks_pre.empty() && callbacks_post.empty()); }

			enum class return_value : unsigned char
			{
				none =            0,
				changed =   (1 << 0),
				call_orig = (1 << 1)
			};
			friend constexpr inline bool operator&(return_value lhs, return_value rhs) noexcept
			{ return static_cast<bool>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); }
			friend constexpr inline return_value operator~(return_value lhs) noexcept
			{ return static_cast<return_value>(~static_cast<unsigned char>(lhs)); }
			friend constexpr inline return_value &operator&=(return_value &lhs, return_value rhs) noexcept
			{ lhs = static_cast<return_value>(static_cast<unsigned char>(lhs) & static_cast<unsigned char>(rhs)); return lhs; }
			friend constexpr inline return_value operator|(return_value lhs, return_value rhs) noexcept
			{ return static_cast<return_value>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); }
			friend constexpr inline return_value &operator|=(return_value &lhs, return_value rhs) noexcept
			{ lhs = static_cast<return_value>(static_cast<unsigned char>(lhs) | static_cast<unsigned char>(rhs)); return lhs; }

			inline return_value call_pre(gsdk::ScriptVariant_t *args, std::size_t num_args, bool copyback = false) noexcept
			{ return call(callbacks_pre, args, num_args, false, copyback); }
			inline return_value call_post(gsdk::ScriptVariant_t *args, std::size_t num_args, bool copyback = false) noexcept
			{ return call(callbacks_post, args, num_args, true, copyback); }

		protected:
			virtual void on_wake() noexcept;
			virtual void on_sleep() noexcept;

		private:
			using callbacks_t = std::unordered_map<vscript::func_handle_ref, callback_instance *>;

			return_value call(callbacks_t &callbacks, gsdk::ScriptVariant_t *args, std::size_t num_args, bool post, bool copyback) noexcept;

			callbacks_t callbacks_pre;
			callbacks_t callbacks_post;

			bool clearing_callbacks{false};

		#ifdef __VMOD_PASS_EXTRA_INFO_TO_CALLABLES
			gsdk::ScriptVariant_t *argsblock{nullptr};
			std::size_t argsblock_size{0};
		#endif

		private:
			callable(const callable &) = delete;
			callable &operator=(const callable &) = delete;
			callable(callable &&) noexcept = delete;
			callable &operator=(callable &&) = delete;
		};

	private:
		static vscript::class_desc<plugin> desc;

		struct scope_assume_current final
		{
			static plugin *old_running;

			inline scope_assume_current(plugin *pl_) noexcept
			{
				old_running = assumed_currently_running();
				if(pl_) {
					assumed_currently_running_ = pl_;
				}
			}
			inline ~scope_assume_current() noexcept
			{ assumed_currently_running_ = old_running; }
		};

		static plugin *assumed_currently_running_;
		static std::unordered_map<std::filesystem::path, plugin *> path_plugin_map;

		vscript::func_handle_ref script_lookup_function(std::string_view func_name) noexcept;
		vscript::variant script_lookup_value(std::string_view val_name) noexcept;

		void game_frame(bool simulating) noexcept;

		void watch() noexcept;
		void unwatch() noexcept;

		std::filesystem::path path_;
		std::vector<std::filesystem::path> incs;
		int inotify_fd{-1};
		std::vector<int> watch_fds;

		vscript::instance_handle_wrapper instance_{};
		vscript::script_handle_wrapper script{};
		vscript::scope_handle_wrapper private_scope_{};
		vscript::scope_handle_wrapper public_scope_{};
		vscript::table_handle_wrapper functions_table{};
		vscript::table_handle_wrapper values_table{};

		bool running{false};

		std::unordered_map<std::string, vscript::func_handle_wrapper> function_cache;

		std::vector<owned_instance *> owned_instances;
		std::vector<shared_instance *> shared_instances;
		bool clearing_instances{false};

		typed_function<void()> map_active;
		typed_function<void(std::string_view)> map_loaded;
		typed_function<void()> map_unloaded;
		typed_function<void()> plugin_loaded;
		typed_function<void()> plugin_unloaded;
		typed_function<void()> all_mods_loaded;
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
