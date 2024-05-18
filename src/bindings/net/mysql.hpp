#pragma once

#include <mysql/mysql.h>
#include "../../plugin.hpp"
#include <mutex>
#include <atomic>
#include <string>
#include "../../ffi.hpp"

namespace vmod::bindings::net
{
	class mysql final : public plugin::owned_instance
	{
		friend class singleton;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		mysql(std::string &&host_, std::string &&user_, std::string &&pass_, std::string &&db_, unsigned int port_, std::string &&sock_, unsigned int flags_, plugin::typed_function<void(vscript::instance_handle_ref)> &&callback_) noexcept;
		~mysql() noexcept override;

		enum class step : unsigned char
		{
			connect_start,
			connect_continue,
			connect_done,
			connect_idle,
			connect_fail,
			close_start,
			close_wait,
			close_continue,
			close_done,
		};

		static bool bindings() noexcept;
		static void unbindings() noexcept;

	private:
		static vscript::class_desc<mysql> desc;

		void free() noexcept override final;

		void plugin_unloaded() noexcept override final;

		inline bool initialize() noexcept
		{ return register_instance(&desc, this); }

		std::string_view script_last_errmsg() noexcept;
		unsigned int script_last_errno() noexcept;

		vscript::instance_handle_ref script_prepare(std::string &&query, vscript::func_handle_ref stmt_callback) noexcept;

		inline bool script_busy() const noexcept
		{ return is_busy(); }

		inline bool is_busy() const noexcept
		{ return step_.load() != step::connect_idle; }

		inline bool is_closing() const noexcept
		{ return step_.load() >= step::close_start; }

		inline bool is_available() const noexcept
		{ return !is_busy(); }

		bool can_close() const noexcept;
		void prepare_close() noexcept;

		struct lock_guard final
		{
			lock_guard(bool in_cb_, std::mutex &mtx) noexcept
				: in_cb(in_cb_), mtx_(mtx)
			{
				if(!in_cb) {
					mtx_.lock();
				}
			}

			~lock_guard() noexcept
			{
				if(!in_cb) {
					mtx_.unlock();
				}
			}

			bool in_cb{false};
			std::mutex &mtx_;
		};

		MYSQL obj{};

		std::string host;
		std::string user;
		std::string pass;
		std::string sock;
		std::string db;
		unsigned int port{0};
		unsigned int flags{0};
		int wait_status{0};

		plugin::typed_function<void(vscript::instance_handle_ref)> callback;

		std::atomic<step> step_{step::connect_start};

		std::mutex obj_mx;
		std::mutex stmts_mx;
		bool in_callback{false};

		class stmt final : public plugin::owned_instance
		{
			friend class singleton;
			friend class mysql;
			friend void write_docs(const std::filesystem::path &) noexcept;

		public:
			stmt(mysql &owner_, std::string &&query_, plugin::typed_function<void(vscript::instance_handle_ref,vscript::instance_handle_ref)> &&callback_) noexcept;
			~stmt() noexcept override;

			enum class step : unsigned char
			{
				prepare_start,
				prepare_continue,
				prepare_done,
				prepare_idle,
				prepare_fail,
				close_start,
				close_continue,
				close_done,
			};

		private:
			static vscript::class_desc<stmt> desc;

			void free() noexcept override final;

			void plugin_unloaded() noexcept override final;

			inline bool initialize() noexcept
			{ return register_instance(&desc, this); }

			std::string_view script_last_errmsg() noexcept;
			unsigned int script_last_errno() noexcept;

			bool script_bind(vscript::array_handle_ref params) noexcept;

			std::size_t script_param_count() noexcept;

			inline bool script_busy() const noexcept
			{ return is_busy(); }

			inline bool is_busy() const noexcept
			{ return step_.load() != step::prepare_idle; }

			inline bool is_closing() const noexcept
			{ return step_.load() >= step::close_start; }

			inline bool is_available() const noexcept
			{ return !is_busy(); }

			inline vscript::instance_handle_ref script_owner() const noexcept
			{ return owner->instance_; }

			MYSQL_STMT *obj{nullptr};

			mysql *owner{nullptr};

			std::mutex obj_mx;
			bool in_callback{false};

			plugin::typed_function<void(vscript::instance_handle_ref,vscript::instance_handle_ref)> callback;

			int wait_status{0};

			std::atomic<step> step_{step::prepare_start};

			std::string query;

			static enum_field_types map_type(ffi_type *type) noexcept;

			struct bind : public MYSQL_BIND
			{
				~bind() noexcept
				{

				}
			};

			std::vector<bind> params_data;
		};

		std::vector<std::unique_ptr<stmt>> stmts;
	};
}
