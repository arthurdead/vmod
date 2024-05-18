#pragma once

#include "../../vscript/vscript.hpp"
#include "../../vscript/variant.hpp"
#include "../../vscript/singleton_class_desc.hpp"
#include "../singleton.hpp"

#include <thread>
#include <mutex>
#include <atomic>
#include "mysql.hpp"

namespace vmod::bindings::net
{
	class singleton final : public singleton_base
	{
		friend class vmod::main;
		friend class mysql;
		friend void write_docs(const std::filesystem::path &) noexcept;

	public:
		inline singleton() noexcept
			: singleton_base{"net"}
		{
		}

		~singleton() noexcept override;

		bool bindings() noexcept;
		void unbindings() noexcept;

		static singleton &instance() noexcept;

		bool initialize() noexcept;
		void shutdown() noexcept;

	private:
		static vscript::singleton_class_desc<singleton> desc;

	private:
		static void runner_thread() noexcept;
		void runner() noexcept;

		void handle_mysqls() noexcept;
		void handle_stmts(mysql &sql) noexcept;

		void run_callbacks() noexcept;
		void run_stmt_callbacks(mysql &sql) noexcept;

		vscript::instance_handle_ref script_mysql_conn(vscript::table_handle_ref info) noexcept;

		bool thread_safe{false};

		std::atomic_bool done{false};

		std::vector<std::unique_ptr<mysql>> mysqls;
		std::mutex mysqls_mx;

		std::jthread thr;
		bool thr_spawned{false};

		vscript::table_handle_wrapper types_table{};
	};
}
