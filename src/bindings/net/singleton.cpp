#include "singleton.hpp"
#include "../../main.hpp"

namespace vmod::bindings::net
{
	vscript::singleton_class_desc<singleton> singleton::desc{"net"};

	static singleton net_;

	singleton &singleton::instance() noexcept
	{ return net_; }

	singleton::~singleton() noexcept {}

	bool singleton::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		desc.func(&singleton::script_mysql_conn, "script_mysql_conn"sv, "mysql_conn"sv);

		if(!singleton_base::bindings(&desc)) {
			return false;
		}

		types_table = vm->CreateTable();
		if(!types_table) {
			error("vmod: failed to create net mysql types table\n"sv);
			return false;
		}

		{
			if(!vm->SetValue(*types_table, "decimal", vscript::variant{MYSQL_TYPE_DECIMAL})) {
				error("vmod: failed to register net mysql decimal type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "tiny", vscript::variant{MYSQL_TYPE_TINY})) {
				error("vmod: failed to register net mysql tiny type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "short", vscript::variant{MYSQL_TYPE_SHORT})) {
				error("vmod: failed to register net mysql short type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "long", vscript::variant{MYSQL_TYPE_LONG})) {
				error("vmod: failed to register net mysql long type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "float", vscript::variant{MYSQL_TYPE_FLOAT})) {
				error("vmod: failed to register net mysql float type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "double", vscript::variant{MYSQL_TYPE_DOUBLE})) {
				error("vmod: failed to register net mysql double type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "null", vscript::variant{MYSQL_TYPE_NULL})) {
				error("vmod: failed to register net mysql null type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "timestamp", vscript::variant{MYSQL_TYPE_TIMESTAMP})) {
				error("vmod: failed to register net mysql timestamp type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "long_long", vscript::variant{MYSQL_TYPE_LONGLONG})) {
				error("vmod: failed to register net mysql long_long type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "int24", vscript::variant{MYSQL_TYPE_INT24})) {
				error("vmod: failed to register net mysql int24 type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "date", vscript::variant{MYSQL_TYPE_DATE})) {
				error("vmod: failed to register net mysql date type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "time", vscript::variant{MYSQL_TYPE_TIME})) {
				error("vmod: failed to register net mysql time type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "datetime", vscript::variant{MYSQL_TYPE_DATETIME})) {
				error("vmod: failed to register net mysql datetime type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "year", vscript::variant{MYSQL_TYPE_YEAR})) {
				error("vmod: failed to register net mysql year type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "newdate", vscript::variant{MYSQL_TYPE_NEWDATE})) {
				error("vmod: failed to register net mysql newdate type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "varchar", vscript::variant{MYSQL_TYPE_VARCHAR})) {
				error("vmod: failed to register net mysql varchar type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "bit", vscript::variant{MYSQL_TYPE_BIT})) {
				error("vmod: failed to register net mysql bit type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "json", vscript::variant{MYSQL_TYPE_JSON})) {
				error("vmod: failed to register net mysql json type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "newdecimal", vscript::variant{MYSQL_TYPE_NEWDECIMAL})) {
				error("vmod: failed to register net mysql newdecimal type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "enum", vscript::variant{MYSQL_TYPE_ENUM})) {
				error("vmod: failed to register net mysql enum type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "set", vscript::variant{MYSQL_TYPE_SET})) {
				error("vmod: failed to register net mysql set type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "tiny_blob", vscript::variant{MYSQL_TYPE_TINY_BLOB})) {
				error("vmod: failed to register net mysql tiny_blob type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "medium_blob", vscript::variant{MYSQL_TYPE_MEDIUM_BLOB})) {
				error("vmod: failed to register net mysql medium_blob type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "long_blob", vscript::variant{MYSQL_TYPE_LONG_BLOB})) {
				error("vmod: failed to register net mysql long_blob type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "blob", vscript::variant{MYSQL_TYPE_BLOB})) {
				error("vmod: failed to register net mysql blob type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "varstring", vscript::variant{MYSQL_TYPE_VAR_STRING})) {
				error("vmod: failed to register net mysql varstring type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "string", vscript::variant{MYSQL_TYPE_STRING})) {
				error("vmod: failed to register net mysql string type\n"sv);
				return false;
			}

			if(!vm->SetValue(*types_table, "geometry", vscript::variant{MYSQL_TYPE_GEOMETRY})) {
				error("vmod: failed to register net mysql geometry type\n"sv);
				return false;
			}
		}

		if(!vm->SetValue(*scope, "mysql_types", *types_table)) {
			error("vmod: failed to set net mysql types table value\n"sv);
			return false;
		}

		return true;
	}

	void singleton::unbindings() noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		types_table.free();

		if(scope) {
			if(vm->ValueExists(*scope, "types")) {
				vm->ClearValue(*scope, "types");
			}
		}

		singleton_base::unbindings();
	}

	void singleton::handle_mysqls() noexcept
	{
		std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{mysqls_mx};

		MYSQL *mysql_ret{nullptr};

		for(auto it{mysqls.begin()}; it != mysqls.end();) {
			auto &ptr{*it};

			bool skip{false};

			switch(ptr->step_.load()) {
			case mysql::step::connect_start: {
				{
					std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{ptr->obj_mx};

					ptr->wait_status = mysql_real_connect_start(&mysql_ret, &ptr->obj, ptr->host.c_str(), ptr->user.c_str(), ptr->pass.c_str(), ptr->db.c_str(), ptr->port, ptr->sock.c_str(), ptr->flags);
				}

				if(ptr->wait_status == 0) {
					if(!mysql_ret) {
						ptr->step_.store(mysql::step::connect_fail);
					} else {
						ptr->step_.store(mysql::step::connect_done);
					}
				} else {
					ptr->step_.store(mysql::step::connect_continue);
				}
			} break;
			case mysql::step::connect_continue: {
				{
					std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{ptr->obj_mx};

					ptr->wait_status = mysql_real_connect_cont(&mysql_ret, &ptr->obj, ptr->wait_status);
				}

				if(ptr->wait_status == 0) {
					if(!mysql_ret) {
						ptr->step_.store(mysql::step::connect_fail);
					} else {
						ptr->step_.store(mysql::step::connect_done);
					}
				}
			} break;
			case mysql::step::connect_idle: {
				handle_stmts(*ptr);
			} break;
			case mysql::step::close_start: {
				if(ptr->can_close()) {
					{
						std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{ptr->obj_mx};

						ptr->wait_status = mysql_close_start(&ptr->obj);
					}

					if(ptr->wait_status == 0) {
						ptr->step_.store(mysql::step::close_done);
					}
				} else {
					ptr->step_.store(mysql::step::close_wait);
				}
			} break;
			case mysql::step::close_wait: {
				ptr->prepare_close();

				if(ptr->can_close()) {
					ptr->step_.store(mysql::step::close_start);
				}
			} break;
			case mysql::step::close_continue: {
				{
					std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{ptr->obj_mx};

					ptr->wait_status = mysql_close_cont(&ptr->obj, ptr->wait_status);
				}

				if(ptr->wait_status == 0) {
					ptr->step_.store(mysql::step::close_done);
				}
			} break;
			case mysql::step::close_done: {
				ptr.reset(nullptr);
				it = mysqls.erase(it);
				skip = true;
			} break;
			default: break;
			}

			if(!skip) {
				++it;
			}
		}
	}

	void singleton::handle_stmts(mysql &sql) noexcept
	{
		int stmt_err{0};
		my_bool stmt_ret{false};

		std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{sql.stmts_mx};

		for(auto it{sql.stmts.begin()}; it != sql.stmts.end();) {
			auto &ptr{*it};

			bool skip{false};

			switch(ptr->step_.load()) {
			case mysql::stmt::step::prepare_start: {
				{
					std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{ptr->obj_mx};

					ptr->wait_status = mysql_stmt_prepare_start(&stmt_err, ptr->obj, ptr->query.c_str(), ptr->query.length());
				}

				if(ptr->wait_status == 0) {
					if(stmt_err) {
						ptr->step_.store(mysql::stmt::step::prepare_fail);
					} else {
						ptr->step_.store(mysql::stmt::step::prepare_done);
					}
				} else {
					ptr->step_.store(mysql::stmt::step::prepare_continue);
				}
			} break;
			case mysql::stmt::step::prepare_continue: {
				{
					std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{ptr->obj_mx};

					ptr->wait_status = mysql_stmt_prepare_cont(&stmt_err, ptr->obj, ptr->wait_status);
				}

				if(ptr->wait_status == 0) {
					if(stmt_err) {
						ptr->step_.store(mysql::stmt::step::prepare_fail);
					} else {
						ptr->step_.store(mysql::stmt::step::prepare_done);
					}
				}
			} break;
			case mysql::stmt::step::close_start: {
				{
					std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{ptr->obj_mx};

					ptr->wait_status = mysql_stmt_close_start(&stmt_ret, ptr->obj);
				}

				if(ptr->wait_status == 0) {
					ptr->step_.store(mysql::stmt::step::close_done);
				}
			} break;
			case mysql::stmt::step::close_continue: {
				{
					std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{ptr->obj_mx};

					ptr->wait_status = mysql_stmt_close_cont(&stmt_ret, ptr->obj, ptr->wait_status);
				}

				if(ptr->wait_status == 0) {
					ptr->step_.store(mysql::stmt::step::close_done);
				}
			} break;
			case mysql::stmt::step::close_done: {
				ptr.reset(nullptr);
				it = sql.stmts.erase(it);
				skip = true;
			} break;
			default: break;
			}

			if(!skip) {
				++it;
			}
		}
	}

	void singleton::runner_thread() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(mysql_thread_init()) {
			error("vmod: failed to init mysql thread data\n"sv);
			return;
		}

		for(;!net_.done.load();) {
			net_.handle_mysqls();
		}

		mysql_thread_end();
	}

	void singleton::run_callbacks() noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		if(!mysqls.empty()) {
			std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{mysqls_mx};

			for(auto &ptr : mysqls) {
				auto step{ptr->step_.load()};
				switch(step) {
				case mysql::step::connect_fail:
				case mysql::step::connect_done: {
					if(ptr->callback) {
						std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{ptr->obj_mx};

						ptr->in_callback = true;
						ptr->callback(ptr->instance_);
						ptr->in_callback = false;
					}

					if(step == mysql::step::connect_done) {
						ptr->step_.store(mysql::step::connect_idle);
					} else {
						ptr->step_.store(mysql::step::close_start);
					}
				} break;
				case mysql::step::connect_idle: {
					run_stmt_callbacks(*ptr);
				} break;
				default: break;
				}
			}
		}
	}

	void singleton::run_stmt_callbacks(mysql &sql) noexcept
	{
		gsdk::IScriptVM *vm{vscript::vm()};

		std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{sql.stmts_mx};

		for(auto &ptr : sql.stmts) {
			auto step{ptr->step_.load()};
			switch(step) {
			case mysql::stmt::step::prepare_fail:
			case mysql::stmt::step::prepare_done: {
				if(ptr->callback) {
					std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{ptr->obj_mx};

					ptr->in_callback = true;
					ptr->callback(ptr->owner->instance_, ptr->instance_);
					ptr->in_callback = false;
				}

				if(step == mysql::stmt::step::prepare_done) {
					ptr->step_.store(mysql::stmt::step::prepare_idle);
				} else {
					ptr->step_.store(mysql::stmt::step::close_start);
				}
			} break;
			default: break;
			}
		}
	}

	void singleton::runner() noexcept
	{
		run_callbacks();
	}

	bool singleton::initialize() noexcept
	{
		using namespace std::literals::string_view_literals;

		if(mysql_server_init(0, nullptr, nullptr)) {
			error("vmod: failed to init mysql\n"sv);
			return false;
		}

		if(!mysql_thread_safe()) {
			error("vmod: mysql is not thread safe\n"sv);
			return false;
		}

		if(mysql_thread_init()) {
			error("vmod: failed to init mysql thread data\n"sv);
			return false;
		}

		thr = std::jthread{singleton::runner_thread};
		thr_spawned = true;

		return true;
	}

	void singleton::shutdown() noexcept
	{
		done.store(true);

		if(thr_spawned) {
			thr.join();
		}

		mysql_server_end();

		mysql_thread_end();
	}

	vscript::instance_handle_ref singleton::script_mysql_conn(vscript::table_handle_ref info) noexcept
	{
		using namespace std::literals::string_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		vscript::variant value;

		std::string host{"localhost"s};
		if(vm->GetValue(*info, "host", &value)) {
			host = value.get<std::string>();
		}

		std::string user{"root"s};
		if(vm->GetValue(*info, "user", &value)) {
			user = value.get<std::string>();
		}

		if(!vm->GetValue(*info, "passwd", &value)) {
			vm->RaiseException("vmod: table missing passwd key");
			return nullptr;
		}

		std::string pass{value.get<std::string>()};

		if(!vm->GetValue(*info, "database", &value)) {
			vm->RaiseException("vmod: table missing database key");
			return nullptr;
		}

		std::string db{value.get<std::string>()};

		std::string sock{"/run/mysqld/mysqld.sock"s};
		if(vm->GetValue(*info, "socket", &value)) {
			sock = value.get<std::string>();
		}

		unsigned int port{3306};
		if(vm->GetValue(*info, "port", &value)) {
			port = value.get<unsigned int>();
		}

		unsigned int flags{0};
		if(vm->GetValue(*info, "flags", &value)) {
			flags = value.get<unsigned int>();
		}

		if(!vm->GetValue(*info, "callback", &value)) {
			vm->RaiseException("vmod: table missing callback key");
			return nullptr;
		}

		vscript::func_handle_ref callback_ref{value};
		if(!callback_ref) {
			vm->RaiseException("vmod: invalid callback");
			return nullptr;
		}

		plugin::typed_function<void(vscript::instance_handle_ref)> callback;
		if(!plugin::read_function(*callback_ref, callback)) {
			vm->RaiseException("vmod: invalid callback");
			return nullptr;
		}

		mysql *sql{new mysql{std::move(host), std::move(user), std::move(pass), std::move(db), port, std::move(sock), flags, std::move(callback)}};

		{
			std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{mysqls_mx};
			mysqls.emplace_back(sql);
		}

		if(!sql->initialize()) {
			sql->free();
			return nullptr;
		}

		return sql->instance_;
	}
}
