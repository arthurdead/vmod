#include "squirrel_vm.hpp"
#include "../gsdk.hpp"
#include "../main.hpp"
#include <string>
#include <string_view>
#include "../gsdk/mathlib/vector.hpp"
#include "../bindings/docs.hpp"
#include "../filesystem.hpp"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
#endif
#include <sqstdaux.h>
#include <sqstdstring.h>
#include <sqstdmath.h>
#include <sqstdblob.h>
#include <sqstddatetime.h>
#include <sqstdio.h>
#include <sqstdsystem.h>
#include <sqstddatetime.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#define __VMOD_VM_FORCE_DEBUG

namespace vmod::vm
{
	namespace detail
	{
	#if __has_include("vmod_init.nut.h")
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wmissing-variable-declarations"
	#endif
		#include "vmod_init.nut.h"
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		#define __VMOD_INIT_SCRIPT_HEADER_INCLUDED
	#endif
	}

#ifdef __VMOD_INIT_SCRIPT_HEADER_INCLUDED
	static std::string __vmod_vm_init_script{reinterpret_cast<const char *>(detail::__vmod_vm_init_script_data), sizeof(detail::__vmod_vm_init_script_data)};
#endif
}

namespace vmod::vm
{
	char squirrel_vm::err_buff[gsdk::MAXPRINTMSG];
	char squirrel_vm::print_buff[gsdk::MAXPRINTMSG];

	squirrel_vm::~squirrel_vm() noexcept {}

	static SQInteger developer(HSQUIRRELVM vm)
	{
		if(sq_gettop(vm) != 1) {
			return sq_throwerror(vm, "wrong number of parameters");
		}

	#ifndef __VMOD_VM_FORCE_DEBUG
		sq_pushinteger(vm, static_cast<SQInteger>(vmod::developer->GetInt()));
	#else
		sq_pushinteger(vm, 4);
	#endif
		return SQ_OK;
	}

	static SQInteger get_func_signature(HSQUIRRELVM vm)
	{
		using namespace std::literals::string_view_literals;

		SQInteger top{sq_gettop(vm)};
		if(top < 2 || top > 3) {
			return sq_throwerror(vm, "wrong number of parameters");
		}

		HSQOBJECT func;
		sq_resetobject(&func);
		if(SQ_FAILED(sq_getstackobj(vm, -2, &func))) {
			return sq_throwerror(vm, "failed to get func parameter");
		}

		const SQChar *name{nullptr};
		std::size_t num_params{0};
		std::vector<const SQChar *> params;

		auto handle_proto{
			[&name,&num_params,&params](SQFunctionProto *proto) noexcept -> void {
				if(sq_isstring(proto->_name)) {
					name = proto->_name._unVal.pString->_val;
				}

				for(std::size_t i{0}; i < num_params; ++i) {
					if(sq_isstring(proto->_parameters[i])) {
						const SQChar *param{proto->_parameters[i]._unVal.pString->_val};
						params.emplace_back(param);
					} else {
						params.emplace_back(_SC("<unknown>"));
					}
				}

				num_params = static_cast<std::size_t>(proto->_nparameters);

				if(num_params > params.size()) {
					params.resize(num_params, _SC("<unknown>"));
				}
			}
		};

		if(sq_isclosure(func)) {
			SQClosure *closure{func._unVal.pClosure};
			SQFunctionProto *proto{closure->_function};

			handle_proto(proto);
		} else if(sq_type(func) == OT_FUNCPROTO) {
			SQFunctionProto *proto{func._unVal.pFunctionProto};

			handle_proto(proto);
		} else if(sq_isnativeclosure(func)) {
			SQNativeClosure *closure{func._unVal.pNativeClosure};

			if(!closure->_typecheck.empty()) {
				for(SQUnsignedInteger i{0}; i < closure->_typecheck.size(); ++i) {
					SQObjectType type{static_cast<SQObjectType>(closure->_typecheck[i])};

					const SQChar *str{sq_objtypestr(type)};
					params.emplace_back(str);
				}
			}

			if(closure->_nparamscheck != 0) {
				if(closure->_nparamscheck < 0) {
					num_params = static_cast<std::size_t>(-closure->_nparamscheck);
				} else {
					num_params = static_cast<std::size_t>(closure->_nparamscheck);
				}
			}

			if(num_params > params.size()) {
				params.resize(num_params, _SC("<unknown>"));
			}
		} else {
			return sq_throwerror(vm, "invalid function parameter");
		}

		const SQChar *new_name{nullptr};

		if(top == 3) {
			if(SQ_FAILED(sq_getstring(vm, -3, &new_name))) {
				return sq_throwerror(vm, "failed to get name parameter");
			}
		} else {
			new_name = name;
		}

		std::string sig;

		sig += "function "sv;
		if(new_name) {
			sig += new_name;
		} else {
			sig += "<<unnamed>>"sv;
		}

		sig += '(';

		for(const SQChar *param : params) {
			sig += param;
			sig += ',';
		}
		if(!params.empty()) {
			sig.pop_back();
		}

		sig += ')';

		sq_pushstring(vm, sig.c_str(), static_cast<SQInteger>(sig.length()));

		return SQ_OK;
	}

	struct native_closure_info
	{
		std::string_view name;
		SQFUNCTION impl;
		ssize_t nargs;
		std::string_view typemask;
	};

	template <typename T>
	static SQInteger vector_release(SQUserPointer userptr, SQInteger size)
	{
		if(static_cast<std::size_t>(size) != sizeof(T)) {
			return SQ_ERROR;
		}

		T *vec{static_cast<T *>(userptr)};
		delete vec;

		return SQ_OK;
	}

	template <typename T>
	static SQInteger vector_ctor(HSQUIRRELVM vm)
	{
		using namespace std::literals::string_view_literals;

		SQInteger top{sq_gettop(vm)};
		if(top < 1 || top > 4) {
			return sq_throwerror(vm, "wrong number of parameters");
		}

		float x{0.0f};
		float y{0.0f};
		float z{0.0f};

		std::size_t num_args{static_cast<std::size_t>(top)};
		if(num_args >= 2) {
			if(SQ_FAILED(sq_getfloat(vm, -2, &x))) {
				return sq_throwerror(vm, "failed to get x parameter");
			}
		}
		if(num_args >= 3) {
			if(SQ_FAILED(sq_getfloat(vm, -3, &y))) {
				return sq_throwerror(vm, "failed to get y parameter");
			}
		}
		if(num_args == 4) {
			if(SQ_FAILED(sq_getfloat(vm, -4, &z))) {
				return sq_throwerror(vm, "failed to get z parameter");
			}
		}

		T *vec{new T{x, y, z}};

		if(SQ_FAILED(sq_setinstanceup(vm, 1, vec))) {
			delete vec;
			return sq_throwerror(vm, "failed to set instance");
		}

		sq_setreleasehook(vm, 1, &vector_release<T>);

		return SQ_OK;
	}

	void squirrel_vm::error_func(HSQUIRRELVM vm, const SQChar *fmt, ...)
	{
		squirrel_vm *actual_vm{static_cast<squirrel_vm *>(sq_getforeignptr(vm))};

		va_list vargs;
		va_start(vargs, fmt);
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wformat-nonliteral"
	#endif
		std::vsnprintf(err_buff, sizeof(err_buff), fmt, vargs);
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		va_end(vargs);

		error("%s", err_buff);

		if(actual_vm->err_callback) {
			(void)actual_vm->err_callback(gsdk::SCRIPT_LEVEL_ERROR, err_buff);
		}
	}

	void squirrel_vm::print_func(HSQUIRRELVM vm, const SQChar *fmt, ...)
	{
		squirrel_vm *actual_vm{static_cast<squirrel_vm *>(sq_getforeignptr(vm))};

		va_list vargs;
		va_start(vargs, fmt);
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wformat-nonliteral"
	#endif
		std::vsnprintf(print_buff, sizeof(print_buff), fmt, vargs);
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		va_end(vargs);

		info("%s", print_buff);

		if(actual_vm->output_callback) {
			actual_vm->output_callback(print_buff);
		}
	}

	bool squirrel_vm::Init()
	{
		using namespace std::literals::string_view_literals;

		impl = sq_open(1024);
		if(!impl) {
			error("vmod vm: failed to open\n"sv);
			return false;
		}

	#ifdef __VMOD_USING_QUIRREL
		_ss(impl)->defaultLangFeatures = LF_STRICT_BOOL|LF_EXPLICIT_ROOT_LOOKUP|LF_NO_PLUS_CONCAT;
	#endif

		sq_setforeignptr(impl, this);

		sq_resetobject(&last_exception);

		sq_setprintfunc(impl, print_func, error_func);

	#ifndef __VMOD_VM_FORCE_DEBUG
		debug_vm = (vmod::developer->GetInt() > 0);
	#else
		debug_vm = true;
	#endif

		if(debug_vm) {
			sq_enabledebuginfo(impl, SQTrue);
			sq_notifyallexceptions(impl, SQTrue);
		}

		{
			sq_resetobject(&root_table);

			sq_pushroottable(impl);

			if(SQ_FAILED(sq_getstackobj(impl, -1, &root_table))) {
				error("vmod vm: failed to get root table obj\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			sq_addref(impl, &root_table);
			got_root_table = true;

			sq_pop(impl, 1);
		}

		{
			sq_pushroottable(impl);

			sqstd_seterrorhandlers(impl);

			if(SQ_FAILED(sqstd_register_mathlib(impl))) {
				error("vmod vm: failed to register mathlib\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			if(SQ_FAILED(sqstd_register_stringlib(impl))) {
				error("vmod vm: failed to register stringlib\n"sv);
				return false;
			}

			if(SQ_FAILED(sqstd_register_datetimelib(impl))) {
				error("vmod vm: failed to register datetimelib\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			if(SQ_FAILED(sqstd_register_systemlib(impl))) {
				error("vmod vm: failed to register systemlib\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			if(SQ_FAILED(sqstd_register_bloblib(impl))) {
				error("vmod vm: failed to register bloblib\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			if(SQ_FAILED(sqstd_register_iolib(impl))) {
				error("vmod vm: failed to register iolib\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			sq_pop(impl, 1);
		}

		auto register_func{
			[this](native_closure_info &&info) noexcept -> bool {
				sq_pushstring(impl, info.name.data(), static_cast<SQInteger>(info.name.length()));
				sq_newclosure(impl, info.impl, 0);
				if(SQ_FAILED(sq_setparamscheck(impl, info.nargs, info.typemask.empty() ? nullptr : info.typemask.data()))) {
					error("vmod vm: failed to set '%s' typemask '%s'\n"sv, info.name.data(), info.typemask.data());
					sq_pop(impl, 2);
					return false;
				}
				if(debug_vm) {
					if(SQ_FAILED(sq_setnativeclosurename(impl, -1, info.name.data()))) {
						error("vmod vm: failed to set '%s' name\n"sv, info.name.data());
						sq_pop(impl, 2);
						return false;
					}
				}
				if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
					error("vmod vm: failed to create '%s'\n"sv, info.name.data());
					sq_pop(impl, 2);
					return false;
				}
				return true;
			}
		};

		{
			sq_pushroottable(impl);

			if(!register_func({
				"developer"sv, developer, -1, "."sv
			})) {
				error("vmod vm: failed to register 'developer'\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			if(!register_func({
				"GetFunctionSignature"sv, get_func_signature, -2, ".cs"sv
			})) {
				error("vmod vm: failed to register 'GetFunctionSignature'\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			sq_pop(impl, 1);
		}

		auto register_class{
			[this,register_func]<std::size_t S>(HSQOBJECT &classobj, std::string_view name, auto type, native_closure_info (&&funcs)[S]) noexcept -> bool {
				using type_t = typename decltype(type)::type;

				sq_pushstring(impl, name.data(), static_cast<SQInteger>(name.length()));
				if(SQ_FAILED(sq_newclass(impl, SQFalse))) {
					error("vmod vm: failed to create '%s' class\n"sv, name.data());
					sq_pop(impl, 1);
					return false;
				}
				if(SQ_FAILED(sq_settypetag(impl, -1, const_cast<std::type_info *>(&typeid(type_t))))) {
					error("vmod vm: failed to set '%s' typetag\n"sv, name.data());
					sq_pop(impl, 2);
					return false;
				}
				if(SQ_FAILED(sq_setclassudsize(impl, -1, sizeof(type_t)))) {
					error("vmod vm: failed to set '%s' size\n"sv, name.data());
					sq_pop(impl, 2);
					return false;
				}

				for(native_closure_info &info : funcs) {
					if(!register_func(std::move(info))) {
						error("vmod vm: failed to register '%s::%s'\n"sv, name.data(), info.name.data());
						sq_pop(impl, 2);
						return false;
					}
				}

				sq_resetobject(&classobj);
				if(SQ_FAILED(sq_getstackobj(impl, -1, &classobj))) {
					error("vmod vm: failed to get '%s' obj\n"sv, name.data());
					sq_pop(impl, 2);
					return false;
				}
				sq_addref(impl, &classobj);
				if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
					error("vmod vm: failed to create '%s'\n"sv, name.data());
					sq_release(impl, &classobj);
					sq_pop(impl, 2);
					return false;
				}

				return true;
			}
		};

		{
			sq_pushroottable(impl);

			if(!register_class(
				vector_class, "Vector"sv, std::type_identity<gsdk::Vector>{}, {
					native_closure_info{"constructor"sv, vector_ctor<gsdk::Vector>, -1, ".nnn"sv}
				})) {
				error("vmod vm: failed to register 'Vector'\n"sv);
				sq_pop(impl, 1);
				return false;
			} else {
				vector_registered = true;
			}

			if(!register_class(
				qangle_class, "QAngle"sv, std::type_identity<gsdk::QAngle>{}, {
					native_closure_info{"constructor"sv, vector_ctor<gsdk::QAngle>, -1, ".nnn"sv}
				})) {
				error("vmod vm: failed to register 'QAngle'\n"sv);
				sq_pop(impl, 1);
				return false;
			} else {
				qangle_registered = true;
			}

			sq_pop(impl, 1);
		}

		{
		#ifdef __VMOD_USING_QUIRREL
			std::filesystem::path init_script_path{main::instance().root_dir()};

			init_script_path /= "base"sv;
			init_script_path /= "vmod_init.nut"sv;

			if(std::filesystem::exists(init_script_path)) {
				{
					std::size_t len{0};
					std::unique_ptr<unsigned char[]> script_data{read_file(init_script_path, len)};

					if(SQ_FAILED(sq_compilebuffer(impl, reinterpret_cast<const char *>(script_data.get()), static_cast<SQInteger>(len), _SC("init.nut"), SQTrue))) {
					#ifndef __VMOD_INIT_SCRIPT_HEADER_INCLUDED
						error("vmod vm: failed to compile init script from file '%s'\n"sv, init_script_path.c_str());
						return false;
					#else
						error("vmod vm: failed to compile init script from file '%s' re-trying with embedded\n"sv, init_script_path.c_str());
						if(SQ_FAILED(sq_compilebuffer(impl, __vmod_vm_init_script.c_str(), static_cast<SQInteger>(__vmod_vm_init_script.length()), _SC("init.nut"), SQTrue))) {
							error("vmod vm: failed to compile embedded init script\n"sv);
							return false;
						}
					#endif
					}
				}
			} else {
			#ifdef __VMOD_INIT_SCRIPT_HEADER_INCLUDED
				if(SQ_FAILED(sq_compilebuffer(impl, __vmod_vm_init_script.c_str(), static_cast<SQInteger>(__vmod_vm_init_script.length()), _SC("init.nut"), SQTrue))) {
					error("vmod vm: failed to compile embedded init script\n"sv);
					return false;
				}
			#else
				error("vmod vm: missing init script file '%s'\n"sv, init_script_path.c_str());
				return false;
			#endif
			}
		#else
			if(!g_Script_init) {
				error("vmod vm: missing 'g_Script_init' address\n"sv);
				return false;
			}

			if(SQ_FAILED(sq_compilebuffer(impl, reinterpret_cast<const char *>(g_Script_init), static_cast<SQInteger>(std::strlen(reinterpret_cast<const char *>(g_Script_init))), _SC("init.nut"), SQTrue))) {
				error("vmod vm: failed to compile 'g_Script_init'\n"sv);
				return false;
			}
		#endif

			sq_pushroottable(impl);

			bool failed{SQ_FAILED(sq_call(impl, 1, SQFalse, SQTrue))};

			sq_pop(impl, 1);

			if(failed) {
				error("vmod vm: failed to execute 'g_Script_init'\n"sv);
				return false;
			}
		}

		{
			sq_pushroottable(impl);

			{
				sq_pushstring(impl, _SC("VSquirrel_OnCreateScope"), 23);
				if(SQ_FAILED(sq_get(impl, -2))) {
					error("vmod vm: failed to get 'VSquirrel_OnCreateScope'\n"sv);
					sq_pop(impl, 1);
					return false;
				}
				sq_resetobject(&create_scope_func);
				if(SQ_FAILED(sq_getstackobj(impl, -1, &create_scope_func))) {
					error("vmod vm: failed to get 'VSquirrel_OnCreateScope' obj\n"sv);
					sq_pop(impl, 1);
					return false;
				}
				sq_addref(impl, &create_scope_func);
				got_create_scope = true;
				sq_pop(impl, 1);
			}

			{
				sq_pushstring(impl, _SC("VSquirrel_OnReleaseScope"), 24);
				if(SQ_FAILED(sq_get(impl, -2))) {
					error("vmod vm: failed to get 'VSquirrel_OnReleaseScope'\n"sv);
					sq_pop(impl, 1);
					return false;
				}
				sq_resetobject(&release_scope_func);
				if(SQ_FAILED(sq_getstackobj(impl, -1, &release_scope_func))) {
					error("vmod vm: failed to get 'VSquirrel_OnReleaseScope' obj\n"sv);
					sq_pop(impl, 1);
					return false;
				}
				sq_addref(impl, &release_scope_func);
				got_release_scope = true;
				sq_pop(impl, 1);
			}

			if(debug_vm) {
				{
					sq_pushstring(impl, _SC("RegisterFunctionDocumentation"), 29);
					if(SQ_SUCCEEDED(sq_get(impl, -2))) {
						sq_resetobject(&register_func_desc);
						if(SQ_SUCCEEDED(sq_getstackobj(impl, -1, &register_func_desc))) {
							sq_addref(impl, &register_func_desc);
							got_reg_func_desc = true;
						} else {
							warning("vmod vm: failed to get 'RegisterFunctionDocumentation' obj\n"sv);
						}
						sq_pop(impl, 1);
					} else {
						warning("vmod vm: failed to get 'RegisterFunctionDocumentation'\n"sv);
					}
				}
			}

			sq_pop(impl, 1);
		}

		return true;
	}

	void squirrel_vm::Shutdown()
	{
		if(impl) {
			if(qangle_registered) {
				sq_release(impl, &qangle_class);
			}
			if(got_last_exception) {
				sq_release(impl, &last_exception);
			}
			if(got_root_table) {
				sq_release(impl, &root_table);
			}
			if(vector_registered) {
				sq_release(impl, &vector_class);
			}
			for(auto &&it : registered_classes) {
				sq_release(impl, &it.second->obj);
			}
			if(got_create_scope) {
				sq_release(impl, &create_scope_func);
			}
			if(got_release_scope) {
				sq_release(impl, &release_scope_func);
			}
			if(got_reg_func_desc) {
				sq_release(impl, &register_func_desc);
			}
			sq_collectgarbage(impl);
			sq_close(impl);
			impl = nullptr;
		}
	}

	bool squirrel_vm::ConnectDebugger()
	{
		if(debug_vm) {
			return true;
		} else {
			return false;
		}
	}

	void squirrel_vm::DisconnectDebugger()
	{
	}

	gsdk::ScriptLanguage_t squirrel_vm::GetLanguage()
	{
		return gsdk::SL_SQUIRREL;
	}

	const char *squirrel_vm::GetLanguageName()
	{
		return "Squirrel";
	}

	HSQUIRRELVM squirrel_vm::GetInternalVM()
	{
		return impl;
	}

	void squirrel_vm::AddSearchPath([[maybe_unused]] const char *)
	{
	}

	bool squirrel_vm::ForwardConsoleCommand([[maybe_unused]] const gsdk::CCommandContext &, [[maybe_unused]] const gsdk::CCommand &)
	{
		return false;
	}

	bool squirrel_vm::Frame([[maybe_unused]] float)
	{
		return true;
	}

	gsdk::ScriptStatus_t squirrel_vm::Run(const char *code, bool wait)
	{
		if(SQ_FAILED(sq_compilebuffer(impl, code, static_cast<SQInteger>(std::strlen(code)), "<<unnamed>>", SQTrue))) {
			return gsdk::SCRIPT_ERROR;
		}

		sq_pushroottable(impl);

		bool failed{SQ_FAILED(sq_call(impl, 1, SQFalse, SQTrue))};

		sq_pop(impl, 1);

		if(failed) {
			return gsdk::SCRIPT_ERROR;
		} else if(wait) {
			return gsdk::SCRIPT_DONE;
		} else {
			return gsdk::SCRIPT_RUNNING;
		}
	}

	gsdk::HSCRIPT squirrel_vm::CompileScript(const char *code, const char *name)
	{
		if(!name || name[0] == '\0') {
			name = "<<unnamed>>";
		}

		if(SQ_FAILED(sq_compilebuffer(impl, code, static_cast<SQInteger>(std::strlen(code)), name, SQTrue))) {
			return gsdk::INVALID_HSCRIPT;
		}

		HSQOBJECT *script_obj{new HSQOBJECT};
		sq_resetobject(script_obj);

		if(SQ_FAILED(sq_getstackobj(impl, -1, script_obj))) {
			sq_release(impl, script_obj);
			delete script_obj;
			script_obj = gsdk::INVALID_HSCRIPT;
		} else {
			sq_addref(impl, script_obj);
		}

		sq_pop(impl, 1);

		return script_obj;
	}

	void squirrel_vm::ReleaseScript(gsdk::HSCRIPT obj)
	{
		sq_release(impl, obj);
		delete obj;
	}

	gsdk::ScriptStatus_t squirrel_vm::Run(gsdk::HSCRIPT obj, gsdk::HSCRIPT scope, bool wait)
	{
		sq_pushobject(impl, *obj);

		if(scope) {
			sq_pushobject(impl, *scope);
		} else {
			sq_pushroottable(impl);
		}

		bool failed{SQ_FAILED(sq_call(impl, 1, SQFalse, SQTrue))};

		sq_pop(impl, 1);

		if(failed) {
			return gsdk::SCRIPT_ERROR;
		} else if(wait) {
			return gsdk::SCRIPT_DONE;
		} else {
			return gsdk::SCRIPT_RUNNING;
		}
	}

	gsdk::ScriptStatus_t squirrel_vm::Run(gsdk::HSCRIPT obj, bool wait)
	{
		sq_pushobject(impl, *obj);

		sq_pushroottable(impl);

		bool failed{SQ_FAILED(sq_call(impl, 1, SQFalse, SQTrue))};

		sq_pop(impl, 1);

		if(failed) {
			return gsdk::SCRIPT_ERROR;
		} else if(wait) {
			return gsdk::SCRIPT_DONE;
		} else {
			return gsdk::SCRIPT_RUNNING;
		}
	}

	gsdk::HSCRIPT squirrel_vm::CreateScope_impl(const char *name, gsdk::HSCRIPT parent)
	{
		sq_pushobject(impl, create_scope_func);

		sq_pushroottable(impl);
		sq_pushstring(impl, name, static_cast<SQInteger>(std::strlen(name)));
		if(parent) {
			sq_pushobject(impl, *parent);
		} else {
			sq_pushroottable(impl);
		}

		HSQOBJECT *scope{gsdk::INVALID_HSCRIPT};

		if(SQ_SUCCEEDED(sq_call(impl, 3, SQTrue, SQTrue))) {
			scope = new HSQOBJECT;
			sq_resetobject(scope);

			bool got{SQ_SUCCEEDED(sq_getstackobj(impl, -1, scope))};

			if(!got || sq_isnull(*scope)) {
				sq_release(impl, scope);
				delete scope;
				scope = gsdk::INVALID_HSCRIPT;
			} else {
				sq_addref(impl, scope);
			}

			sq_pop(impl, 1);
		}

		sq_pop(impl, 1);

		return scope;
	}

	gsdk::HSCRIPT squirrel_vm::ReferenceScope(gsdk::HSCRIPT obj)
	{
		sq_pushobject(impl, *obj);

		HSQOBJECT *copy{new HSQOBJECT};
		sq_resetobject(copy);
		if(SQ_FAILED(sq_getstackobj(impl, -1, copy))) {
			sq_release(impl, copy);
			delete copy;
			copy = gsdk::INVALID_HSCRIPT;
		} else {
			sq_addref(impl, copy);
		}

		sq_pop(impl, 1);
		return copy;
	}

	void squirrel_vm::ReleaseScope(gsdk::HSCRIPT obj)
	{
		sq_pushobject(impl, release_scope_func);

		sq_pushroottable(impl);
		sq_pushobject(impl, *obj);

		if(SQ_SUCCEEDED(sq_call(impl, 2, SQFalse, SQTrue))) {
			sq_release(impl, obj);
			delete obj;
		}

		sq_pop(impl, 1);
	}

	gsdk::HSCRIPT squirrel_vm::LookupFunction_impl(const char *name, gsdk::HSCRIPT scope)
	{
		if(scope) {
			sq_pushobject(impl, *scope);
		} else {
			sq_pushroottable(impl);
		}

		HSQOBJECT *copy{gsdk::INVALID_HSCRIPT};

		sq_pushstring(impl, name, static_cast<SQInteger>(std::strlen(name)));
		if(SQ_SUCCEEDED(sq_get(impl, -2))) {
			copy = new HSQOBJECT;
			sq_resetobject(copy);

			if(SQ_FAILED(sq_getstackobj(impl, -1, copy))) {
				sq_release(impl, copy);
				delete copy;
				copy = gsdk::INVALID_HSCRIPT;
			} else {
				sq_addref(impl, copy);
			}
		}

		sq_pop(impl, 1);

		return copy;
	}

	void squirrel_vm::ReleaseFunction(gsdk::HSCRIPT obj)
	{
		sq_release(impl, obj);
		delete obj;
	}

	void squirrel_vm::push(const gsdk::ScriptVariant_t &var) noexcept
	{
		switch(var.m_type) {
			case gsdk::FIELD_VOID: {
				sq_pushnull(impl);
			} break;
			case gsdk::FIELD_TIME:
			case gsdk::FIELD_FLOAT: {
				sq_pushfloat(impl, var.m_float);
			} break;
			case gsdk::FIELD_FLOAT64: {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				sq_pushfloat(impl, static_cast<float>(var.m_double));
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				sq_pushfloat(impl, var.m_float);
			#else
				#error
			#endif
			} break;
			case gsdk::FIELD_STRING: {
				const char *ccstr{gsdk::vscript::STRING(var.m_tstr)};

				sq_pushstring(impl, ccstr, static_cast<SQInteger>(std::strlen(ccstr)));
			} break;
			case gsdk::FIELD_MODELNAME:
			case gsdk::FIELD_SOUNDNAME:
			case gsdk::FIELD_CSTRING: {
				sq_pushstring(impl, var.m_ccstr, static_cast<SQInteger>(std::strlen(var.m_ccstr)));
			} break;
			case gsdk::FIELD_CHARACTER: {
				char buff[2]{var.m_char, '\0'};
				sq_pushstring(impl, buff, 1);
			} break;
			case gsdk::FIELD_SHORT: {
				sq_pushinteger(impl, var.m_short);
			} break;
			case gsdk::FIELD_POSITIVEINTEGER_OR_NULL: {
				if(var.m_int > 0) {
					sq_pushinteger(impl, var.m_int);
				} else {
					sq_pushnull(impl);
				}
			} break;
			case gsdk::FIELD_MODELINDEX:
			case gsdk::FIELD_MATERIALINDEX:
			case gsdk::FIELD_TICK:
			case gsdk::FIELD_INTEGER: {
				sq_pushinteger(impl, var.m_int);
			} break;
			case gsdk::FIELD_UINT32: {
				sq_pushinteger(impl, static_cast<SQInteger>(var.m_uint));
			} break;
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			case gsdk::FIELD_INTEGER64: {
			#if GSDK_ENGINE == GSDK_ENGINE_L4D2
				sq_pushinteger(impl, static_cast<SQInteger>(var.m_longlong));
			#else
				sq_pushinteger(impl, static_cast<SQInteger>(var.m_long));
			#endif
			} break;
		#endif
			case gsdk::FIELD_UINT64: {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				sq_pushinteger(impl, static_cast<SQInteger>(var.m_ulonglong));
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				sq_pushinteger(impl, static_cast<SQInteger>(var.m_ulong));
			#else
				#error
			#endif
			} break;
			case gsdk::FIELD_BOOLEAN: {
				sq_pushbool(impl, var.m_bool);
			} break;
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE: {
				if(var.m_object && var.m_object != gsdk::INVALID_HSCRIPT) {
					sq_addref(impl, var.m_object);
				}
				[[fallthrough]];
			}
			case gsdk::FIELD_HSCRIPT: {
				if(var.m_object && var.m_object != gsdk::INVALID_HSCRIPT) {
					sq_pushobject(impl, *var.m_object);
				} else {
					sq_pushnull(impl);
				}
			} break;
			case gsdk::FIELD_QANGLE: {
				sq_pushobject(impl, qangle_class);
				sq_createinstance(impl, -1);
				sq_setinstanceup(impl, -1, var.m_vector);
				sq_remove(impl, -2);
			} break;
			case gsdk::FIELD_POSITION_VECTOR:
			case gsdk::FIELD_VECTOR: {
				sq_pushobject(impl, vector_class);
				sq_createinstance(impl, -1);
				sq_setinstanceup(impl, -1, var.m_vector);
				sq_remove(impl, -2);
			} break;
			case gsdk::FIELD_VARIANT: {
				sq_pushnull(impl);
			} break;
			case gsdk::FIELD_TYPEUNKNOWN: {
				sq_pushnull(impl);
			} break;
			default: {
				sq_pushnull(impl);
			} break;
		}
	}

	bool squirrel_vm::get(SQInteger idx, gsdk::ScriptVariant_t &var) noexcept
	{
		std::memset(var.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));

		switch(sq_gettype(impl, idx)) {
			case OT_NULL: {
				var.m_type = gsdk::FIELD_VOID;
				var.m_object = gsdk::INVALID_HSCRIPT;
				return true;
			}
			case OT_INTEGER: {
				SQInteger value{0};
				if(SQ_SUCCEEDED(sq_getinteger(impl, idx, &value))) {
					var.m_type = gsdk::FIELD_INTEGER;
					var.m_int = value;
					return true;
				} else {
					var.m_type = gsdk::FIELD_VOID;
					var.m_object = gsdk::INVALID_HSCRIPT;
					return false;
				}
			}
			case OT_FLOAT: {
				SQFloat value{0.0f};
				if(SQ_SUCCEEDED(sq_getfloat(impl, idx, &value))) {
					var.m_type = gsdk::FIELD_FLOAT;
					var.m_float = value;
					return true;
				} else {
					var.m_type = gsdk::FIELD_VOID;
					var.m_object = gsdk::INVALID_HSCRIPT;
					return false;
				}
			}
			case OT_BOOL: {
				SQBool value{false};
				if(SQ_SUCCEEDED(sq_getbool(impl, idx, &value))) {
					var.m_type = gsdk::FIELD_BOOLEAN;
					var.m_bool = value;
					return true;
				} else {
					var.m_type = gsdk::FIELD_VOID;
					var.m_object = gsdk::INVALID_HSCRIPT;
					return false;
				}
			}
			case OT_STRING: {
				const SQChar *value{nullptr};
				SQInteger len{0};
				if(SQ_SUCCEEDED(sq_getstringandsize(impl, idx, &value, &len))) {
					var.m_type = gsdk::FIELD_CSTRING;
					std::size_t len_siz{static_cast<std::size_t>(len)};
				#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
					var.m_cstr = static_cast<char *>(std::malloc(len_siz+1));
				#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
					var.m_cstr = new char[len_siz+1];
				#else
					#error
				#endif
					std::strncpy(var.m_cstr, value, len_siz);
					var.m_cstr[len_siz] = '\0';
					var.m_flags |= gsdk::SV_FREE;
					return true;
				} else {
					var.m_type = gsdk::FIELD_VOID;
					var.m_object = gsdk::INVALID_HSCRIPT;
					return false;
				}
			}
			default: {
				var.m_type = gsdk::FIELD_TYPEUNKNOWN;
				var.m_object = gsdk::INVALID_HSCRIPT;
				return false;
			}
		}
	}

	void squirrel_vm::get(const HSQOBJECT &obj, gsdk::ScriptVariant_t &var) noexcept
	{
		std::memset(var.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));

		switch(sq_type(obj)) {
			case OT_NULL: {
				var.m_type = gsdk::FIELD_VOID;
				var.m_object = gsdk::INVALID_HSCRIPT;
			} break;
			case OT_INTEGER: {
				var.m_type = gsdk::FIELD_INTEGER;
				var.m_int = sq_objtointeger(&obj);
			} break;
			case OT_FLOAT: {
				var.m_type = gsdk::FIELD_FLOAT;
				var.m_float = sq_objtofloat(&obj);
			} break;
			case OT_BOOL: {
				var.m_type = gsdk::FIELD_BOOLEAN;
				var.m_bool = sq_objtobool(&obj);
			} break;
			case OT_STRING: {
				const SQChar *str{sq_objtostring(&obj)};
				if(str) {
					var.m_type = gsdk::FIELD_CSTRING;
					std::size_t len{std::strlen(str)};
				#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
					var.m_cstr = static_cast<char *>(std::malloc(len+1));
				#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
					var.m_cstr = new char[len+1];
				#else
					#error
				#endif
					std::strncpy(var.m_cstr, str, len);
					var.m_cstr[len] = '\0';
					var.m_flags |= gsdk::SV_FREE;
				} else {
					var.m_type = gsdk::FIELD_VOID;
					var.m_object = gsdk::INVALID_HSCRIPT;
				}
			} break;
			default: {
				var.m_type = gsdk::FIELD_TYPEUNKNOWN;
				var.m_object = gsdk::INVALID_HSCRIPT;
			} break;
		}
	}

	gsdk::ScriptStatus_t squirrel_vm::ExecuteFunction_impl(gsdk::HSCRIPT obj, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret_var, gsdk::HSCRIPT scope, bool wait)
	{
		sq_pushobject(impl, *obj);

		if(scope) {
			sq_pushobject(impl, *scope);
		} else {
			sq_pushroottable(impl);
		}

		std::size_t num_args_siz{static_cast<std::size_t>(num_args)};
		for(std::size_t i{0}; i < num_args_siz; ++i) {
			push(args[i]);
		}

		bool failed{false};

		if(SQ_SUCCEEDED(sq_call(impl, 1+num_args, ret_var ? SQTrue : SQFalse , SQTrue))) {
			HSQOBJECT ret_obj;
			sq_resetobject(&ret_obj);

			if(SQ_SUCCEEDED(sq_getstackobj(impl, -1, &ret_obj))) {
				get(ret_obj, *ret_var);
			} else {
				failed = true;
			}

			sq_pop(impl, 1);
		} else {
			failed = true;
		}

		sq_pop(impl, 1);

		if(failed) {
			return gsdk::SCRIPT_ERROR;
		} else if(wait) {
			return gsdk::SCRIPT_DONE;
		} else {
			return gsdk::SCRIPT_RUNNING;
		}
	}

	SQInteger squirrel_vm::static_func_call(HSQUIRRELVM vm)
	{
		return SQ_ERROR;
	}

	SQInteger squirrel_vm::member_func_call(HSQUIRRELVM vm)
	{
		return SQ_ERROR;
	}

	SQInteger squirrel_vm::generic_ctor(HSQUIRRELVM vm)
	{
		return SQ_ERROR;
	}

	SQInteger squirrel_vm::generic_dtor(SQUserPointer userptr, SQInteger size)
	{
		return SQ_ERROR;
	}

	SQInteger squirrel_vm::external_dtor(SQUserPointer userptr, SQInteger size)
	{
		return SQ_ERROR;
	}

	bool squirrel_vm::register_func(const gsdk::ScriptClassDesc_t *classinfo, const gsdk::ScriptFunctionBinding_t *info, std::string_view name_str) noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		sq_pushstring(impl, name_str.data(), static_cast<SQInteger>(name_str.length()));
		if(classinfo && (info->m_flags & gsdk::SF_MEMBER_FUNC)) {
			sq_pushuserpointer(impl, const_cast<gsdk::ScriptFunctionBinding_t *>(info));
			sq_newclosure(impl, member_func_call, 1);
		} else {
			sq_pushuserpointer(impl, const_cast<gsdk::ScriptFunctionBinding_t *>(info));
			sq_newclosure(impl, static_func_call, 1);
		}

		if(SQ_FAILED(sq_setnativeclosurename(impl, -1, name_str.data()))) {
			sq_pop(impl, 2);
			return false;
		}

		std::string typemask{"."s};

		for(gsdk::ScriptDataType_t param : info->m_desc.m_Parameters) {
			switch(param) {
				case gsdk::FIELD_VOID: {
					typemask += 'o';
				} break;
				case gsdk::FIELD_TIME: {
					typemask += 'f';
				} break;
				case gsdk::FIELD_FLOAT64:
				case gsdk::FIELD_FLOAT: {
					typemask += 'n';
				} break;
				case gsdk::FIELD_STRING:
				case gsdk::FIELD_MODELNAME:
				case gsdk::FIELD_SOUNDNAME:
				case gsdk::FIELD_CSTRING: {
					typemask += 's';
				} break;
				case gsdk::FIELD_CHARACTER: {
					typemask += 's';
				} break;
				case gsdk::FIELD_POSITIVEINTEGER_OR_NULL: {
					typemask += "i|o"sv;
				} break;
				case gsdk::FIELD_MODELINDEX:
				case gsdk::FIELD_MATERIALINDEX:
				case gsdk::FIELD_TICK:{
					typemask += 'i';
				} break;
			#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				case gsdk::FIELD_INTEGER64:
			#endif
				case gsdk::FIELD_SHORT:
				case gsdk::FIELD_UINT64:
				case gsdk::FIELD_UINT32:
				case gsdk::FIELD_INTEGER: {
					typemask += 'n';
				} break;
				case gsdk::FIELD_BOOLEAN: {
					typemask += 'b';
				} break;
				case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
				case gsdk::FIELD_HSCRIPT: {
					typemask += 'x';
				} break;
				case gsdk::FIELD_QANGLE:
				case gsdk::FIELD_POSITION_VECTOR:
				case gsdk::FIELD_VECTOR: {
					typemask += 'x';
				} break;
				case gsdk::FIELD_VARIANT: {
					typemask += '.';
				} break;
				case gsdk::FIELD_TYPEUNKNOWN: {
					typemask += '.';
				} break;
				default: {
					typemask += '.';
				} break;
			}
		}

		if(SQ_FAILED(sq_setparamscheck(impl, static_cast<SQInteger>(info->m_desc.m_Parameters.size()), typemask.empty() ? nullptr : typemask.c_str()))) {
			sq_pop(impl, 2);
			return false;
		}

		if(SQ_FAILED(sq_newslot(impl, -3, (classinfo && !(info->m_flags & gsdk::SF_MEMBER_FUNC)) ? SQTrue : SQFalse))) {
			return false;
		}

		if(debug_vm && got_reg_func_desc) {
			if(info->m_desc.m_pszDescription && info->m_desc.m_pszDescription[0] != '@') {
				HSQOBJECT func_obj;
				if(SQ_SUCCEEDED(sq_getstackobj(impl, -1, &func_obj))) {
					std::string sig;

					sig += bindings::docs::type_name(info->m_desc.m_ReturnType);
					sig += ' ';
					if(classinfo) {
						sig += classinfo->m_pszScriptName;
						sig += "::"sv;
					}
					sig += name_str;
					sig += '(';
					for(gsdk::ScriptDataType_t param : info->m_desc.m_Parameters) {
						sig += bindings::docs::type_name(param);
						sig += ", "sv;
					}
					if(!info->m_desc.m_Parameters.empty()) {
						sig.erase(sig.end()-2, sig.end());
					}
					sig += ')';

					sq_pushobject(impl, register_func_desc);
					sq_pushroottable(impl);
					sq_pushobject(impl, func_obj);
					sq_pushstring(impl, name_str.data(), static_cast<SQInteger>(name_str.length()));
					sq_pushstring(impl, sig.c_str(), static_cast<SQInteger>(sig.length()));
					sq_pushstring(impl, info->m_desc.m_pszDescription, static_cast<SQInteger>(std::strlen(info->m_desc.m_pszDescription)));
					(void)sq_call(impl, 5, SQFalse, SQTrue);
					sq_pop(impl, 1);
				}
			}
		}

		return true;
	}

	void squirrel_vm::RegisterFunction_nonvirtual(gsdk::ScriptFunctionBinding_t *info) noexcept
	{
		std::string name_str{info->m_desc.m_pszScriptName};

		if(registered_funcs.find(name_str) != registered_funcs.end()) {
			return;
		}

		sq_pushroottable(impl);

		(void)register_func(nullptr, info, name_str);

		sq_pop(impl, 1);

		registered_funcs.emplace(std::move(name_str), func_info_t{info});
	}

	void squirrel_vm::RegisterFunction(gsdk::ScriptFunctionBinding_t *info)
	{
		RegisterFunction_nonvirtual(info);
	}

	bool squirrel_vm::register_class(const gsdk::ScriptClassDesc_t *info, HSQOBJECT **obj) noexcept
	{
		std::string classname_str{info->m_pszScriptName};

		HSQOBJECT *base_obj{nullptr};

		bool has_base{info->m_pBaseDesc != nullptr};

		if(has_base) {
			auto base_it{registered_classes.find(std::string{info->m_pBaseDesc->m_pszScriptName})};
			if(base_it == registered_classes.end()) {
				sq_pushroottable(impl);

				bool success{register_class(info->m_pBaseDesc, &base_obj)};

				sq_pop(impl, 1);

				if(!success) {
					return false;
				}
			} else {
				base_obj = &base_it->second->obj;
			}
		}

		sq_pushstring(impl, classname_str.c_str(), static_cast<SQInteger>(classname_str.length()));
		if(has_base) {
			sq_pushobject(impl, *base_obj);
		}
		if(SQ_FAILED(sq_newclass(impl, has_base ? SQTrue : SQFalse))) {
			return false;
		}

		if(SQ_FAILED(sq_settypetag(impl, -1, const_cast<gsdk::ScriptClassDesc_t *>(info)))) {
			return false;
		}

		if(SQ_FAILED(sq_setclassudsize(impl, -1, sizeof(instance_info_t)))) {
			return false;
		}

		std::unique_ptr<class_info_t> tmpinfo{new class_info_t{info}};

		if(info->m_pfnConstruct) {
			sq_pushstring(impl, _SC("constructor"), 11);
			auto udata{reinterpret_cast<const gsdk::ScriptClassDesc_t **>(sq_newuserdata(impl, sizeof(gsdk::ScriptClassDesc_t *)))};
			*udata = info;
			sq_newclosure(impl, generic_ctor, 1);
			if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
				return false;
			}
		}

		for(const auto &func_info : info->m_FunctionBindings) {
			std::string funcname_str{func_info.m_desc.m_pszScriptName};

			if(tmpinfo->registered_funcs.find(funcname_str) != tmpinfo->registered_funcs.end()) {
				continue;
			}

			if(!register_func(info, &func_info, funcname_str)) {
				return false;
			}

			tmpinfo->registered_funcs.emplace(std::move(funcname_str), func_info_t{&func_info});
		}

		if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
			return false;
		}

		sq_resetobject(&tmpinfo->obj);

		if(SQ_FAILED(sq_getstackobj(impl, -1, &tmpinfo->obj))) {
			sq_pop(impl, 1);
			return false;
		}

		sq_addref(impl, &tmpinfo->obj);

		auto this_it{registered_classes.emplace(std::move(classname_str), std::move(tmpinfo)).first};

		if(obj) {
			*obj = &this_it->second->obj;
		}

		return true;
	}

	bool squirrel_vm::RegisterClass_nonvirtual(gsdk::ScriptClassDesc_t *info) noexcept
	{
		if(registered_classes.find(std::string{info->m_pszScriptName}) != registered_classes.end()) {
			return true;
		}

		sq_pushroottable(impl);

		bool success{register_class(info, nullptr)};

		sq_pop(impl, 1);

		return success;
	}

	bool squirrel_vm::RegisterClass(gsdk::ScriptClassDesc_t *info)
	{
		return RegisterClass_nonvirtual(info);
	}

	gsdk::HSCRIPT squirrel_vm::RegisterInstance_impl_nonvirtual(gsdk::ScriptClassDesc_t *info, void *ptr) noexcept
	{
		if(!squirrel_vm::RegisterClass_nonvirtual(info)) {
			return gsdk::INVALID_HSCRIPT;
		}

		auto class_it{registered_classes.find(info->m_pszScriptName)};
		if(class_it == registered_classes.end()) {
			return gsdk::INVALID_HSCRIPT;
		}

		sq_pushobject(impl, class_it->second->obj);

		if(SQ_FAILED(sq_createinstance(impl, -1))) {
			sq_pop(impl, 1);
			return gsdk::INVALID_HSCRIPT;
		}

		if(info->m_pfnDestruct) {
			sq_setreleasehook(impl, -1, external_dtor);
		} else {
			sq_setreleasehook(impl, -1, generic_dtor);
		}

		SQUserPointer userptr{nullptr};
		if(SQ_FAILED(sq_getinstanceup(impl, -1, &userptr, info))) {
			sq_pop(impl, 2);
			return gsdk::INVALID_HSCRIPT;
		}

		new (userptr) instance_info_t{info, ptr};

		HSQOBJECT *copy{new HSQOBJECT};
		sq_resetobject(copy);

		if(SQ_FAILED(sq_getstackobj(impl, -1, copy))) {
			sq_release(impl, copy);
			delete copy;
			copy = gsdk::INVALID_HSCRIPT;
		} else {
			sq_addref(impl, copy);
		}

		return copy;
	}

	gsdk::HSCRIPT squirrel_vm::RegisterInstance_impl(gsdk::ScriptClassDesc_t *info, void *ptr)
	{
		return RegisterInstance_impl_nonvirtual(info, ptr);
	}

	void squirrel_vm::SetInstanceUniqeId(gsdk::HSCRIPT obj, const char *id)
	{
		sq_pushobject(impl, *obj);

		SQUserPointer userptr{nullptr};
		if(SQ_SUCCEEDED(sq_getinstanceup(impl, -1, &userptr, nullptr))) {
			instance_info_t *info{static_cast<instance_info_t *>(userptr)};
			info->id = id;
		}

		sq_pop(impl, 1);
	}

	void squirrel_vm::RemoveInstance(gsdk::HSCRIPT obj)
	{
		sq_release(impl, obj);
		delete obj;
	}

	void *squirrel_vm::GetInstanceValue_impl(gsdk::HSCRIPT obj, gsdk::ScriptClassDesc_t *classinfo)
	{
		sq_pushobject(impl, *obj);

		if(classinfo) {
			auto class_it{registered_classes.find(classinfo->m_pszScriptName)};
			if(class_it == registered_classes.end()) {
				sq_pop(impl, 1);
				return nullptr;
			}

			sq_pushobject(impl, class_it->second->obj);

			if(sq_instanceof(impl) != SQTrue) {
				sq_pop(impl, 2);
				return nullptr;
			}

			sq_pop(impl, 1);
		}

		SQUserPointer userptr{nullptr};
		if(SQ_FAILED(sq_getinstanceup(impl, -1, &userptr, classinfo ? classinfo : nullptr))) {
			sq_pop(impl, 1);
			return nullptr;
		}

		instance_info_t *info{static_cast<instance_info_t *>(userptr)};

		void *ptr{info->ptr};

		sq_pop(impl, 1);

		return ptr;
	}

	bool squirrel_vm::GenerateUniqueKey(const char *root, char *buff, int len)
	{
		std::size_t len_siz{static_cast<std::size_t>(len)};

		std::size_t root_len{std::strlen(root)};
		if(len_siz <= root_len) {
			return false;
		}

		std::size_t total_len{root_len + std::numeric_limits<std::size_t>::digits10 + 2};
		if(len_siz < total_len) {
			return false;
		}

		std::snprintf(buff, static_cast<std::size_t>(len), "%zu_%s", unique_ids, root);
		++unique_ids;
		return true;
	}

	bool squirrel_vm::ValueExists(gsdk::HSCRIPT scope, const char *name)
	{
		if(scope) {
			sq_pushobject(impl, *scope);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushstring(impl, name, static_cast<SQInteger>(std::strlen(name)));

		bool got{SQ_SUCCEEDED(sq_get(impl, -2))};

		sq_pop(impl, 1);

		return got;
	}

	bool squirrel_vm::SetValue_nonvirtual(gsdk::HSCRIPT obj, const char *name, const char *value) noexcept
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushstring(impl, name, static_cast<SQInteger>(std::strlen(name)));

		if(value) {
			sq_pushstring(impl, value,static_cast<SQInteger>(std::strlen(value)));
		} else {
			sq_pushnull(impl);
		}

		if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
			return false;
		}

		sq_pop(impl, 1);

		return true;
	}

	bool squirrel_vm::SetValue(gsdk::HSCRIPT obj, const char *name, const char *value)
	{
		return SetValue_nonvirtual(obj, name, value);
	}

	bool squirrel_vm::SetValue_impl_nonvirtual(gsdk::HSCRIPT obj, const char *name, const gsdk::ScriptVariant_t &value) noexcept
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushstring(impl, name, static_cast<SQInteger>(std::strlen(name)));
		push(value);

		if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
			return false;
		}

		sq_pop(impl, 1);

		return true;
	}

	bool squirrel_vm::SetValue_impl(gsdk::HSCRIPT obj, const char *name, const gsdk::ScriptVariant_t &value)
	{
		return SetValue_impl_nonvirtual(obj, name, value);
	}

	bool squirrel_vm::SetValue_impl(gsdk::HSCRIPT obj, int idx, const gsdk::ScriptVariant_t &value)
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushinteger(impl, idx);
		push(value);

		if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
			return false;
		}

		sq_pop(impl, 1);

		return true;
	}

	void squirrel_vm::CreateTable_impl(gsdk::ScriptVariant_t &value)
	{
		sq_newtable(impl);

		std::memset(value.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));

		HSQOBJECT *copy{new HSQOBJECT};
		sq_resetobject(copy);
		if(SQ_FAILED(sq_getstackobj(impl, -1, copy))) {
			sq_release(impl, copy);
			delete copy;
			value.m_type = gsdk::FIELD_TYPEUNKNOWN;
			value.m_object = gsdk::INVALID_HSCRIPT;
		} else {
			sq_addref(impl, copy);
			value.m_type = gsdk::FIELD_HSCRIPT;
			value.m_object = copy;
			value.m_flags |= gsdk::SV_FREE;
		}

		sq_pop(impl, 1);
	}

	void squirrel_vm::CreateArray_impl_nonvirtual(gsdk::ScriptVariant_t &value) noexcept
	{
		sq_newarray(impl, 0);

		std::memset(value.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));

		HSQOBJECT *copy{new HSQOBJECT};
		sq_resetobject(copy);
		if(SQ_FAILED(sq_getstackobj(impl, -1, copy))) {
			sq_release(impl, copy);
			delete copy;
			value.m_type = gsdk::FIELD_TYPEUNKNOWN;
			value.m_object = gsdk::INVALID_HSCRIPT;
		} else {
			sq_addref(impl, copy);
			value.m_type = gsdk::FIELD_HSCRIPT;
			value.m_object = copy;
			value.m_flags |= gsdk::SV_FREE;
		}

		sq_pop(impl, 1);
	}

	void squirrel_vm::CreateArray_impl(gsdk::ScriptVariant_t &value)
	{
		CreateArray_impl_nonvirtual(value);
	}

	bool squirrel_vm::IsTable_nonvirtual(gsdk::HSCRIPT obj) noexcept
	{
		return sq_istable(*obj);
	}

	bool squirrel_vm::IsTable(gsdk::HSCRIPT obj)
	{
		return IsTable_nonvirtual(obj);
	}

	int squirrel_vm::GetNumTableEntries(gsdk::HSCRIPT obj) const
	{
		sq_pushobject(impl, *obj);

		int size{sq_getsize(impl, -1)};

		sq_pop(impl, 1);

		return size;
	}

	int squirrel_vm::GetKeyValue(gsdk::HSCRIPT obj, int it, gsdk::ScriptVariant_t *key, gsdk::ScriptVariant_t *value)
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		if(it == -1) {
			sq_pushnull(impl);
		} else {
			sq_pushinteger(impl, it);
		}

		int next_it{-1};

		if(SQ_SUCCEEDED(sq_next(impl, -2))) {
			if(key) {
				(void)get(-2, *key);
			}

			if(value) {
				(void)get(-1, *value);
			}

			if(SQ_FAILED(sq_getinteger(impl, -3, &next_it))) {
				next_it = -1;
			}

			sq_pop(impl, 2);
		}

		sq_pop(impl, 2);

		return next_it;
	}

	int squirrel_vm::GetKeyValue2(gsdk::HSCRIPT obj, int it, gsdk::ScriptVariant_t *key, gsdk::ScriptVariant_t *value)
	{
		return squirrel_vm::GetKeyValue(obj, it, key, value);
	}

	bool squirrel_vm::GetValue_impl(gsdk::HSCRIPT obj, const char *name, gsdk::ScriptVariant_t *value)
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushstring(impl, name, static_cast<SQInteger>(std::strlen(name)));

		bool got{SQ_SUCCEEDED(sq_get(impl, -2))};

		if(got) {
			got = get(-1, *value);
		}

		sq_pop(impl, 1);

		return got;
	}

	bool squirrel_vm::GetValue_impl(gsdk::HSCRIPT obj, int idx, gsdk::ScriptVariant_t *value)
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushinteger(impl, idx);

		bool got{SQ_SUCCEEDED(sq_get(impl, -2))};

		if(got) {
			got = get(-1, *value);
		}

		sq_pop(impl, 1);

		return got;
	}

	bool squirrel_vm::GetScalarValue(gsdk::HSCRIPT obj, gsdk::ScriptVariant_t *value)
	{
		get(*obj, *value);
		return true;
	}

	void squirrel_vm::ReleaseValue(gsdk::ScriptVariant_t &value)
	{
		switch(value.m_type) {
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT: {
				if(value.m_flags & gsdk::SV_FREE) {
					sq_release(impl, value.m_object);
					delete value.m_object;

					value.m_flags &= ~gsdk::SV_FREE;
					std::memset(value.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));
					value.m_object = gsdk::INVALID_HSCRIPT;
					value.m_type = gsdk::FIELD_TYPEUNKNOWN;
				}
			} break;
			default: {
				value.free();
			} break;
		}
	}

	bool squirrel_vm::ClearValue(gsdk::HSCRIPT obj, const char *name)
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushstring(impl, name, static_cast<SQInteger>(std::strlen(name)));

		bool removed{SQ_SUCCEEDED(sq_deleteslot(impl, -2, SQFalse))};

		sq_pop(impl, 1);

		return removed;
	}

	bool squirrel_vm::IsArray_nonvirtual(gsdk::HSCRIPT obj) noexcept
	{
		return sq_isarray(*obj);
	}

	bool squirrel_vm::IsArray(gsdk::HSCRIPT obj)
	{
		return IsArray_nonvirtual(obj);
	}

	int squirrel_vm::GetArrayCount_nonvirtual(gsdk::HSCRIPT obj) noexcept
	{
		sq_pushobject(impl, *obj);

		int size{sq_getsize(impl, -1)};

		sq_pop(impl, 1);

		return size;
	}

	int squirrel_vm::GetArrayCount(gsdk::HSCRIPT obj)
	{
		return GetArrayCount_nonvirtual(obj);
	}

	void squirrel_vm::ArrayAddToTail(gsdk::HSCRIPT obj, const gsdk::ScriptVariant_t &value)
	{
		sq_pushobject(impl, *obj);

		push(value);
		(void)sq_arrayappend(impl, -2);

		sq_pop(impl, 2);
	}

	void squirrel_vm::WriteState(gsdk::CUtlBuffer *)
	{
		debugtrap();
	}

	void squirrel_vm::ReadState(gsdk::CUtlBuffer *)
	{
		debugtrap();
	}

	void squirrel_vm::CollectGarbage(const char *, bool)
	{
		sq_collectgarbage(impl);
	}

	void squirrel_vm::RemoveOrphanInstances()
	{
		sq_collectgarbage(impl);
	}

	void squirrel_vm::DumpState()
	{
		
	}

	void squirrel_vm::SetOutputCallback(gsdk::ScriptOutputFunc_t func)
	{
		output_callback = func;
	}

	void squirrel_vm::SetErrorCallback(gsdk::ScriptErrorFunc_t func)
	{
		err_callback = func;
	}

	bool squirrel_vm::RaiseException_impl(const char *msg)
	{
		sq_resetobject(&last_exception);
		got_last_exception = false;

		sq_pushstring(impl, msg, static_cast<SQInteger>(std::strlen(msg)));

		if(SQ_SUCCEEDED(sq_getstackobj(impl, -1, &last_exception))) {
			sq_addref(impl, &last_exception);
			got_last_exception = true;
		}

		sq_pop(impl, 1);

		return true;
	}

	gsdk::HSCRIPT squirrel_vm::GetRootTable()
	{
		return &root_table;
	}

	gsdk::HSCRIPT squirrel_vm::CopyHandle(gsdk::HSCRIPT)
	{
		return gsdk::INVALID_HSCRIPT;
	}

	gsdk::HSCRIPT squirrel_vm::GetIdentity(gsdk::HSCRIPT)
	{
		return gsdk::INVALID_HSCRIPT;
	}

	gsdk::CSquirrelMetamethodDelegateImpl *squirrel_vm::MakeSquirrelMetamethod_Get_impl(gsdk::HSCRIPT &, const char *, gsdk::ISquirrelMetamethodDelegate *, bool)
	{
		return nullptr;
	}

	void squirrel_vm::DestroySquirrelMetamethod_Get(gsdk::CSquirrelMetamethodDelegateImpl *)
	{
		
	}
}
