#include "mysql.hpp"
#include "singleton.hpp"
#include "../mem/singleton.hpp"

namespace vmod::bindings::net
{
	vscript::class_desc<mysql> mysql::desc{"net::mysql"};
	vscript::class_desc<mysql::stmt> mysql::stmt::desc{"net::mysql::stmt"};

	mysql::mysql(std::string &&host_, std::string &&user_, std::string &&pass_, std::string &&db_, unsigned int port_, std::string &&sock_, unsigned int flags_, plugin::typed_function<void(vscript::instance_handle_ref)> &&callback_) noexcept
		: host(std::move(host_)), user(std::move(user_)), pass(std::move(pass_)), sock(std::move(sock_)), db(std::move(db_)), port(port_), flags(flags_), callback(std::move(callback_))
	{
		mysql_init(&obj);
		mysql_options(&obj, MYSQL_OPT_NONBLOCK, nullptr);
	}

	mysql::~mysql() noexcept
	{
	}

	void mysql::free() noexcept
	{ step_.store(step::close_start); }

	bool mysql::can_close() const noexcept
	{
		return stmts.empty();
	}

	void mysql::prepare_close() noexcept
	{
		for(auto &ptr : stmts) {
			switch(ptr->step_.load()) {
			case stmt::step::prepare_idle:
			ptr->step_.store(mysql::stmt::step::close_start);
			break;
			default: break;
			}
		}
	}

	void mysql::plugin_unloaded() noexcept
	{
		callback.free();

		plugin::owned_instance::plugin_unloaded();
	}

	void mysql::stmt::plugin_unloaded() noexcept
	{
		callback.free();

		plugin::owned_instance::plugin_unloaded();
	}

	mysql::stmt::stmt(mysql &owner_, std::string &&query_, plugin::typed_function<void(vscript::instance_handle_ref,vscript::instance_handle_ref)> &&callback_) noexcept
		: obj(mysql_stmt_init(&owner_.obj)), owner(&owner_), callback(std::move(callback_)), query(std::move(query_))
	{
	}

	mysql::stmt::~stmt() noexcept
	{
	}

	void mysql::stmt::free() noexcept
	{ step_.store(step::close_start); }

	bool mysql::bindings() noexcept
	{
		using namespace std::literals::string_view_literals;

		desc.func(&mysql::script_last_errmsg, "script_last_errmsg"sv, "errmsg"sv);

		desc.func(&mysql::script_last_errno, "script_last_errno"sv, "errno"sv);

		desc.func(&mysql::script_busy, "script_busy"sv, "busy"sv);

		desc.func(&mysql::script_prepare, "script_prepare"sv, "prepare"sv);

		desc.dtor();

		if(!plugin::owned_instance::register_class(&desc)) {
			error("vmod: failed to register net mysql script class\n"sv);
			return false;
		}

		stmt::desc.func(&mysql::stmt::script_last_errmsg, "script_last_errmsg"sv, "errmsg"sv);

		stmt::desc.func(&mysql::stmt::script_last_errno, "script_last_errno"sv, "errno"sv);

		stmt::desc.func(&mysql::stmt::script_busy, "script_busy"sv, "busy"sv);

		stmt::desc.func(&mysql::stmt::script_owner, "script_owner"sv, "owner"sv);

		stmt::desc.func(&mysql::stmt::script_param_count, "script_param_count"sv, "param_count"sv);

		stmt::desc.func(&mysql::stmt::script_bind, "script_bind"sv, "bind"sv);

		stmt::desc.dtor();

		if(!plugin::owned_instance::register_class(&stmt::desc)) {
			error("vmod: failed to register net mysql stmt script class\n"sv);
			return false;
		}

		return true;
	}

	void mysql::unbindings() noexcept
	{

	}

	std::string_view mysql::script_last_errmsg() noexcept
	{
		lock_guard VMOD_UNIQUE_NAME{in_callback, obj_mx};
		return mysql_error(&obj);
	}

	unsigned int mysql::stmt::script_last_errno() noexcept
	{
		lock_guard VMOD_UNIQUE_NAME{in_callback, obj_mx};
		return mysql_stmt_errno(obj);
	}

	std::string_view mysql::stmt::script_last_errmsg() noexcept
	{
		lock_guard VMOD_UNIQUE_NAME{in_callback, obj_mx};
		return mysql_stmt_error(obj);
	}

	unsigned int mysql::script_last_errno() noexcept
	{
		lock_guard VMOD_UNIQUE_NAME{in_callback, obj_mx};
		return mysql_errno(&obj);
	}

	std::size_t mysql::stmt::script_param_count() noexcept
	{
		lock_guard VMOD_UNIQUE_NAME{in_callback, obj_mx};
		return mysql_stmt_param_count(obj);
	}

	bool mysql::stmt::script_bind(vscript::array_handle_ref params) noexcept
	{
		lock_guard VMOD_UNIQUE_NAME{in_callback, obj_mx};

		gsdk::IScriptVM *vm{vscript::vm()};

		if(!is_available()) {
			vm->RaiseException("vmod: unavailable at the moment");
			return false;
		}

		int count{vm->GetArrayCount(*params)};

		if(count != mysql_stmt_param_count(obj)) {
			vm->RaiseException("vmod: invalid param count");
			return false;
		}

		params_data.resize(count);

		for(int i{0}, it{0}; i < count; ++i) {
			vscript::variant value;
			it = vm->GetArrayValue(*params, it, &value);

			vscript::table_handle_ref tbl{value.get<vscript::table_handle_ref>()};
			if(!tbl) {
				vm->RaiseException("vmod: invalid param");
				params_data.clear();
				return false;
			}

			if(!vm->GetValue(*tbl, "type", &value)) {
				vm->RaiseException("vmod: invalid param");
				params_data.clear();
				return false;
			}

			params_data[i].buffer_type = value.get<enum_field_types>();

			if(vm->GetValue(*tbl, "unsigned", &value)) {
				params_data[i].is_unsigned = value.get<bool>();
			}

			params_data[i].buffer = nullptr;

			vm->RaiseException("vmod: unimplemented");
			params_data.clear();
			return false;
		}

		return mysql_stmt_bind_param(obj, params_data.data());
	}

	vscript::instance_handle_ref mysql::script_prepare(std::string &&query, vscript::func_handle_ref stmt_callback) noexcept
	{
		using namespace std::literals::string_literals;

		gsdk::IScriptVM *vm{vscript::vm()};

		if(!is_available()) {
			vm->RaiseException("vmod: unavailable at the moment");
			return nullptr;
		}

		if(!stmt_callback) {
			vm->RaiseException("vmod: invalid callback");
			return nullptr;
		}

		plugin::typed_function<void(vscript::instance_handle_ref,vscript::instance_handle_ref)> callback_copy;
		if(!plugin::read_function(*stmt_callback, callback_copy)) {
			vm->RaiseException("vmod: invalid callback");
			return nullptr;
		}

		stmt *stmt_{nullptr};

		if(in_callback) {
			stmt_ = new stmt{*this, std::move(query), std::move(callback_copy)};
		} else {
			std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{obj_mx};
			stmt_ = new stmt{*this, std::move(query), std::move(callback_copy)};
		}

		{
			std::lock_guard<std::mutex> VMOD_UNIQUE_NAME{stmts_mx};
			stmts.emplace_back(stmt_);
		}

		if(!stmt_->initialize()) {
			stmt_->free();
			return nullptr;
		}

		return stmt_->instance_;
	}
}
