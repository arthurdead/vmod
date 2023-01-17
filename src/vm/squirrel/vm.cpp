#include "vm.hpp"
#include "../../gsdk.hpp"
#include "../../main.hpp"
#include <string>
#include <string_view>
#include "../../gsdk/mathlib/vector.hpp"
#include "../../bindings/docs.hpp"
#include "../../filesystem.hpp"
#include "../../gsdk/tier1/utlstring.hpp"

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

#define USQTrue (1u)
#define USQFalse (0u)

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
		#define __VMOD_SQUIRREL_INIT_SCRIPT_HEADER_INCLUDED
	#endif
	}

#ifdef __VMOD_SQUIRREL_INIT_SCRIPT_HEADER_INCLUDED
	static std::string __squirrel_vmod_init_script{reinterpret_cast<const char *>(detail::__squirrel_vmod_init_script_data), sizeof(detail::__squirrel_vmod_init_script_data)};
#endif
}

namespace vmod::vm
{
	char squirrel::err_buff[gsdk::MAXPRINTMSG];
	char squirrel::print_buff[gsdk::MAXPRINTMSG];

#ifdef __VMOD_USING_QUIRREL
	std::underlying_type_t<SQLangFeature> squirrel::default_lang_feat{LF_STRICT_BOOL|LF_NO_PLUS_CONCAT|LF_EXPLICIT_ROOT_LOOKUP};
#endif

	squirrel::~squirrel() noexcept {}

	static SQInteger developer(HSQUIRRELVM vm)
	{
		if(sq_gettop(vm) != 1) {
			return sq_throwerror(vm, _SC("wrong number of parameters"));
		}

	#ifndef __VMOD_VM_FORCE_DEBUG
		sq_pushinteger(vm, static_cast<SQInteger>(vmod::developer->GetInt()));
	#else
		sq_pushinteger(vm, 4);
	#endif
		return 1;
	}

	static SQInteger get_func_signature(HSQUIRRELVM vm)
	{
		using namespace std::literals::string_view_literals;

		SQInteger top{sq_gettop(vm)};
		if(top < 2 || top > 3) {
			return sq_throwerror(vm, _SC("wrong number of parameters"));
		}

		HSQOBJECT func;
		sq_resetobject(&func);
		if(SQ_FAILED(sq_getstackobj(vm, -2, &func))) {
			return sq_throwerror(vm, _SC("failed to get func parameter"));
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
			return sq_throwerror(vm, _SC("invalid function parameter"));
		}

		const SQChar *new_name{nullptr};

		if(top == 3) {
			if(SQ_FAILED(sq_getstring(vm, -3, &new_name))) {
				return sq_throwerror(vm, _SC("failed to get name parameter"));
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

		return 1;
	}

	static SQInteger is_weak_ref(HSQUIRRELVM vm)
	{
		return sq_throwerror(vm, _SC("not implemented yet"));
	}

	static SQInteger dump_object(HSQUIRRELVM vm)
	{
		return sq_throwerror(vm, _SC("not implemented yet"));
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

		return 0;
	}

	template <typename T>
	static SQInteger vector_ctor(HSQUIRRELVM vm)
	{
		using namespace std::literals::string_view_literals;

		SQInteger top{sq_gettop(vm)};
		if(top < 1 || top > 4) {
			return sq_throwerror(vm, _SC("wrong number of parameters"));
		}

		float x{0.0f};
		float y{0.0f};
		float z{0.0f};

		std::size_t num_args{static_cast<std::size_t>(top)};
		if(num_args >= 2) {
			if(SQ_FAILED(sq_getfloat(vm, 2, &x))) {
				return sq_throwerror(vm, _SC("failed to get x parameter"));
			}
		}
		if(num_args >= 3) {
			if(SQ_FAILED(sq_getfloat(vm, 3, &y))) {
				return sq_throwerror(vm, _SC("failed to get y parameter"));
			}
		}
		if(num_args == 4) {
			if(SQ_FAILED(sq_getfloat(vm, 4, &z))) {
				return sq_throwerror(vm, _SC("failed to get z parameter"));
			}
		}

		SQUserPointer userptr{nullptr};
		if(SQ_FAILED(sq_getinstanceup(vm, 1, &userptr, typeid_ptr<T>()))) {
			return sq_throwerror(vm, _SC("failed to get instance pointer"));
		}

		new (userptr) T{x, y, z};

		sq_setreleasehook(vm, 1, &vector_release<T>);

		return 0;
	}

	void squirrel::error_func(HSQUIRRELVM vm, const SQChar *fmt, ...)
	{
		using namespace std::literals::string_view_literals;

		squirrel *actual_vm{static_cast<squirrel *>(sq_getforeignptr(vm))};

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

		error("%s"sv, err_buff);

		if(actual_vm->err_callback) {
			(void)actual_vm->err_callback(gsdk::SCRIPT_LEVEL_ERROR, err_buff);
		}
	}

	void squirrel::print_func(HSQUIRRELVM vm, const SQChar *fmt, ...)
	{
		using namespace std::literals::string_view_literals;

		squirrel *actual_vm{static_cast<squirrel *>(sq_getforeignptr(vm))};

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

		info("%s"sv, print_buff);

		if(actual_vm->output_callback) {
			actual_vm->output_callback(print_buff);
		}
	}

	template <typename ...Args>
	static inline auto sq_compilebuffer_strict(HSQUIRRELVM vm, Args &&...args) noexcept -> decltype(sq_compilebuffer(vm, std::forward<Args>(args)...))
	{
	#ifdef __VMOD_USING_QUIRREL
		_ss(vm)->defaultLangFeatures = (LF_STRICT_BOOL|LF_EXPLICIT_ROOT_LOOKUP|LF_NO_FUNC_DECL_SUGAR|LF_NO_CLASS_DECL_SUGAR|LF_NO_PLUS_CONCAT|LF_EXPLICIT_THIS);
	#endif
		auto &&ret{sq_compilebuffer(vm, std::forward<Args>(args)...)};
	#ifdef __VMOD_USING_QUIRREL
		_ss(vm)->defaultLangFeatures = squirrel::default_lang_feat;
	#endif
		return ret;
	}

	static bool compile_internal_squirrel_script(HSQUIRRELVM vm, std::filesystem::path path, const unsigned char *data, bool &from_file) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::filesystem::path filename{path.filename()};

		if(std::filesystem::exists(path)) {
			std::size_t len{0};
			std::unique_ptr<unsigned char[]> script_data{read_file(path, len)};

			if(SQ_FAILED(sq_compilebuffer_strict(vm, reinterpret_cast<const char *>(script_data.get()), static_cast<SQInteger>(len), filename.c_str(), USQTrue))) {
				if(data) {
					error("vmod: failed to compile '%s' from file '%s' re-trying with embedded data\n"sv, filename.c_str(), path.c_str());
					if(SQ_FAILED(sq_compilebuffer_strict(vm, reinterpret_cast<const char *>(data), static_cast<SQInteger>(std::strlen(reinterpret_cast<const char *>(data))), filename.c_str(), USQTrue))) {
						error("vmod: failed to compile '%s' from embedded data\n"sv, data);
						return false;
					}
				} else {
					error("vmod: failed to compile '%s' from file '%s'\n"sv, filename.c_str(), path.c_str());
					return false;
				}
			} else {
				from_file = true;
			}
		} else {
			if(data) {
				warning("vmod: missing '%s' file trying embedded data\n"sv, filename.c_str());
				if(SQ_FAILED(sq_compilebuffer_strict(vm, reinterpret_cast<const char *>(data), static_cast<SQInteger>(std::strlen(reinterpret_cast<const char *>(data))), filename.c_str(), USQTrue))) {
					error("vmod: failed to compile '%s' from embedded data\n"sv, filename.c_str());
					return false;
				}
			} else {
				error("vmod: missing '%s' file '%s'\n"sv, filename.c_str(), path.c_str());
				return false;
			}
		}

		return true;
	}

	bool squirrel::Init()
	{
		using namespace std::literals::string_view_literals;

		impl = sq_open(1024);
		if(!impl) {
			error("vmod vm: failed to open\n"sv);
			return false;
		}

	#ifdef __VMOD_USING_QUIRREL
		_ss(impl)->defaultLangFeatures = default_lang_feat;
	#endif

		sq_setforeignptr(impl, this);

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
			sq_pushroottable(impl);

			sq_resetobject(&root_table);
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
				sq_pop(impl, 1);
				return false;
			}

			if(SQ_FAILED(sqstd_register_bloblib(impl))) {
				error("vmod vm: failed to register bloblib\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			if(SQ_FAILED(sqstd_register_systemlib(impl))) {
				error("vmod vm: failed to register systemlib\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			if(SQ_FAILED(sqstd_register_iolib(impl))) {
				error("vmod vm: failed to register iolib\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			if(SQ_FAILED(sqstd_register_datetimelib(impl))) {
				error("vmod vm: failed to register datetimelib\n"sv);
				sq_pop(impl, 1);
				return false;
			}

			sq_pop(impl, 1);
		}

	#ifdef __VMOD_USING_QUIRREL
		modules.reset(new SqModules{impl});
		modules->registerBaseLibs();
		modules->registerSystemLib();
		modules->registerIoLib();
		modules->registerDateTimeLib();
	#endif

		{
			sq_pushroottable(impl);

		#ifdef __VMOD_USING_QUIRREL
			sq_pushstring(impl, _SC("_quirrel_"), 9);
			sq_pushbool(impl, SQTrue);
			(void)sq_newslot(impl, -3, SQFalse);
		#else
			sq_pushstring(impl, _SC("_squirrel_"), 10);
			sq_pushbool(impl, SQTrue);
			(void)sq_newslot(impl, -3, SQFalse);
		#endif

		#ifdef __VMOD_USING_QUIRREL
			sq_pushstring(impl, _SC("_version_"), 9);
			sq_pushstring(impl, SQUIRREL_VERSION, -1);
			(void)sq_newslot(impl, -3, SQFalse);

			static_assert(SQUIRREL_VERSION_NUMBER_MINOR < 10);
			static_assert(SQUIRREL_VERSION_NUMBER_PATCH < 10);

			sq_pushstring(impl, _SC("_versionnumber_"), 15);
			sq_pushinteger(impl, ((SQUIRREL_VERSION_NUMBER_MAJOR * 100) + (SQUIRREL_VERSION_NUMBER_MINOR * 10) + SQUIRREL_VERSION_NUMBER_PATCH));
			(void)sq_newslot(impl, -3, SQFalse);

			sq_pushstring(impl, _SC("_versionnumber_patch_"), 21);
			sq_pushinteger(impl, SQUIRREL_VERSION_NUMBER_PATCH);
			(void)sq_newslot(impl, -3, SQFalse);

			sq_pushstring(impl, _SC("_versionnumber_minor_"), 21);
			sq_pushinteger(impl, SQUIRREL_VERSION_NUMBER_MINOR);
			(void)sq_newslot(impl, -3, SQFalse);

			sq_pushstring(impl, _SC("_versionnumber_major_"), 21);
			sq_pushinteger(impl, SQUIRREL_VERSION_NUMBER_MAJOR);
			(void)sq_newslot(impl, -3, SQFalse);
		#else
			static_assert(SQUIRREL_VERSION_NUMBER >= 100 && SQUIRREL_VERSION_NUMBER <= 999);

			sq_pushstring(impl, _SC("_versionnumber_patch_"), 21);
			sq_pushinteger(impl, (SQUIRREL_VERSION_NUMBER % 10));
			(void)sq_newslot(impl, -3, SQFalse);

			sq_pushstring(impl, _SC("_versionnumber_minor_"), 21);
			sq_pushinteger(impl, ((SQUIRREL_VERSION_NUMBER % 100) - (SQUIRREL_VERSION_NUMBER % 10)));
			(void)sq_newslot(impl, -3, SQFalse);

			sq_pushstring(impl, _SC("_versionnumber_major_"), 21);
			sq_pushinteger(impl, (SQUIRREL_VERSION_NUMBER / 100));
			(void)sq_newslot(impl, -3, SQFalse);
		#endif

		#ifdef __VMOD_USING_QUIRREL
			sq_pushstring(impl, _SC("_charsize_"), 10);
			sq_pushinteger(impl, sizeof(SQChar));
			(void)sq_newslot(impl, -3, SQFalse);
		#endif

			sq_pushstring(impl, _SC("_charalign_"), 11);
			sq_pushinteger(impl, alignof(SQChar));
			(void)sq_newslot(impl, -3, SQFalse);

		#ifdef __VMOD_USING_QUIRREL
			sq_pushstring(impl, _SC("_intsize_"), 9);
			sq_pushinteger(impl, sizeof(SQInteger));
			(void)sq_newslot(impl, -3, SQFalse);
		#endif

			sq_pushstring(impl, _SC("_intalign_"), 10);
			sq_pushinteger(impl, alignof(SQInteger));
			(void)sq_newslot(impl, -3, SQFalse);

		#ifdef __VMOD_USING_QUIRREL
			sq_pushstring(impl, _SC("_floatsize_"), 11);
			sq_pushinteger(impl, sizeof(SQFloat));
			(void)sq_newslot(impl, -3, SQFalse);
		#endif

			sq_pushstring(impl, _SC("_floatalign_"), 12);
			sq_pushinteger(impl, alignof(SQFloat));
			(void)sq_newslot(impl, -3, SQFalse);

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
						warning("vmod vm: failed to set '%s' name\n"sv, info.name.data());
					}
				}
				if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
					error("vmod vm: failed to create '%s'\n"sv, info.name.data());
					return false;
				}
				return true;
			}
		};

		{
			sq_pushroottable(impl);

			if(!register_func({
				"developer"sv, developer, 1, "."sv
			})) {
				sq_pop(impl, 1);
				return false;
			}

			if(!register_func({
				"GetFunctionSignature"sv, get_func_signature, -2, ".cs"sv
			})) {
				sq_pop(impl, 1);
				return false;
			}

			if(!register_func({
				"IsWeakref"sv, is_weak_ref, 3, "..."sv
			})) {
				sq_pop(impl, 1);
				return false;
			}

			if(!register_func({
				"DumpObject"sv, dump_object, 2, ".."sv
			})) {
				sq_pop(impl, 1);
				return false;
			}

			sq_pop(impl, 1);
		}

		auto register_class{
			[this,register_func]<std::size_t S>(HSQOBJECT &classobj, bool &got, std::string_view name, auto type, native_closure_info (&&funcs)[S]) noexcept -> bool {
				using type_t = typename decltype(type)::type;

				sq_pushstring(impl, name.data(), static_cast<SQInteger>(name.length()));
				if(SQ_FAILED(sq_newclass(impl, SQFalse))) {
					error("vmod vm: failed to create '%s' class\n"sv, name.data());
					sq_pop(impl, 1);
					return false;
				}
				if(SQ_FAILED(sq_settypetag(impl, -1, typeid_ptr<type_t>()))) {
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
				got = true;

				if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
					error("vmod vm: failed to create '%s'\n"sv, name.data());
					sq_release(impl, &classobj);
					sq_resetobject(&classobj);
					got = false;
					return false;
				}

				return true;
			}
		};

		{
			sq_pushroottable(impl);

			if(!register_class(
				vector_class, vector_registered, "Vector"sv, std::type_identity<gsdk::Vector>{}, {
					native_closure_info{"constructor"sv, vector_ctor<gsdk::Vector>, -1, ".nnn"sv}
				})) {
				sq_pop(impl, 1);
				return false;
			}

			if(!register_class(
				qangle_class, qangle_registered, "QAngle"sv, std::type_identity<gsdk::QAngle>{}, {
					native_closure_info{"constructor"sv, vector_ctor<gsdk::QAngle>, -1, ".nnn"sv}
				})) {
				sq_pop(impl, 1);
				return false;
			}

			sq_pop(impl, 1);
		}

		{
		#ifdef __VMOD_SQUIRREL_OVERRIDE_INIT_SCRIPT
			std::filesystem::path init_script_path{main::instance().root_dir()};
			init_script_path /= "base/squirrel/vmod_init.nut"sv;

			bool script_from_file{false};

			if(!compile_internal_squirrel_script(impl, init_script_path, 
		#ifdef __VMOD_SQUIRREL_INIT_SCRIPT_HEADER_INCLUDED
			reinterpret_cast<const unsigned char *>(__squirrel_vmod_init_script.c_str())
		#else
			nullptr
		#endif
			, script_from_file)) {
				return false;
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
			#ifdef __VMOD_SQUIRREL_OVERRIDE_INIT_SCRIPT
				if(script_from_file) {
					error("vmod vm: failed to execute init script file '%s'\n"sv, init_script_path.c_str());
					return false;
				} else {
					error("vmod vm: failed to execute embedded init script file\n"sv);
					return false;
				}
			#else
				error("vmod vm: failed to execute 'g_Script_init'\n"sv);
				return false;
			#endif
			}
		}

		auto get_req_func{
			[this](HSQOBJECT &funcobj, bool &got, std::string_view name) noexcept -> bool {
				sq_pushstring(impl, name.data(), static_cast<SQInteger>(name.length()));
				if(SQ_FAILED(sq_get(impl, -2))) {
					error("vmod vm: failed to get '%s'\n"sv, name.data());
					return false;
				}
				sq_resetobject(&funcobj);
				if(SQ_FAILED(sq_getstackobj(impl, -1, &funcobj))) {
					error("vmod vm: failed to get '%s' obj\n"sv, name.data());
					sq_pop(impl, 1);
					return false;
				}
				sq_addref(impl, &funcobj);
				got = true;
				sq_pop(impl, 1);
				return true;
			}
		};

		auto get_opt_func{
			[this](HSQOBJECT &funcobj, bool &got, std::string_view name) noexcept -> void {
				sq_pushstring(impl, name.data(), static_cast<SQInteger>(name.length()));
				if(SQ_SUCCEEDED(sq_get(impl, -2))) {
					sq_resetobject(&funcobj);
					if(SQ_SUCCEEDED(sq_getstackobj(impl, -1, &funcobj))) {
						sq_addref(impl, &funcobj);
						got = true;
					} else {
						warning("vmod vm: failed to get '%s' obj\n"sv, name.data());
					}
					sq_pop(impl, 1);
				} else {
					warning("vmod vm: failed to get '%s'\n"sv, name.data());
				}
			}
		};

		{
			sq_pushroottable(impl);

			if(!get_req_func(create_scope_func, got_create_scope, "VSquirrel_OnCreateScope"sv)) {
				sq_pop(impl, 1);
				return false;
			}

			if(!get_req_func(release_scope_func, got_release_scope, "VSquirrel_OnReleaseScope"sv)) {
				sq_pop(impl, 1);
				return false;
			}

			if(debug_vm) {
				get_opt_func(register_func_desc, got_reg_func_desc, "RegisterFunctionDocumentation"sv);
			}

			sq_pop(impl, 1);
		}

		return true;
	}

	void squirrel::Shutdown()
	{
		if(impl) {
			if(qangle_registered) {
				sq_release(impl, &qangle_class);
				sq_resetobject(&qangle_class);
				qangle_registered = false;
			}
			if(got_last_exception) {
				sq_release(impl, &last_exception);
				sq_resetobject(&last_exception);
				got_last_exception = false;
			}
			if(got_root_table) {
				sq_release(impl, &root_table);
				sq_resetobject(&root_table);
				got_root_table = false;
			}
			if(vector_registered) {
				sq_release(impl, &vector_class);
				sq_resetobject(&vector_class);
				vector_registered = false;
			}
			for(auto &&it : registered_classes) {
				sq_release(impl, &it.second->obj);
				sq_resetobject(&it.second->obj);
			}
			registered_classes.clear();
			registered_funcs.clear();
			if(got_create_scope) {
				sq_release(impl, &create_scope_func);
				sq_resetobject(&create_scope_func);
				got_create_scope = false;
			}
			if(got_release_scope) {
				sq_release(impl, &release_scope_func);
				sq_resetobject(&release_scope_func);
				got_release_scope = false;
			}
			if(got_reg_func_desc) {
				sq_release(impl, &register_func_desc);
				sq_resetobject(&register_func_desc);
				got_reg_func_desc = false;
			}
			modules.reset(nullptr);
			sq_collectgarbage(impl);
			sq_close(impl);
			impl = nullptr;
		}

		unique_ids = 0;
	}

	bool squirrel::ConnectDebugger()
	{
		if(debug_vm) {
			return true;
		} else {
			return false;
		}
	}

	void squirrel::DisconnectDebugger()
	{
	}

	gsdk::ScriptLanguage_t squirrel::GetLanguage()
	{
		return gsdk::SL_SQUIRREL;
	}

	const char *squirrel::GetLanguageName()
	{
		return "Squirrel";
	}

	HSQUIRRELVM squirrel::GetInternalVM()
	{
		return impl;
	}

	void squirrel::AddSearchPath([[maybe_unused]] const char *)
	{
	}

	bool squirrel::ForwardConsoleCommand([[maybe_unused]] const gsdk::CCommandContext &, [[maybe_unused]] const gsdk::CCommand &)
	{
		return false;
	}

	bool squirrel::Frame([[maybe_unused]] float)
	{
		return true;
	}

	gsdk::ScriptStatus_t squirrel::Run(const char *code, bool wait)
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

	gsdk::HSCRIPT squirrel::CompileScript_strict(const char *code, const char *name) noexcept
	{
		if(!name || name[0] == '\0') {
			name = "<<unnamed>>";
		}

		if(SQ_FAILED(sq_compilebuffer_strict(impl, code, static_cast<SQInteger>(std::strlen(code)), name, USQTrue))) {
			return gsdk::INVALID_HSCRIPT;
		}

		return compile_script();
	}

	gsdk::HSCRIPT squirrel::CompileScript(const char *code, const char *name)
	{
		if(!name || name[0] == '\0') {
			name = "<<unnamed>>";
		}

		if(SQ_FAILED(sq_compilebuffer(impl, code, static_cast<SQInteger>(std::strlen(code)), name, SQTrue))) {
			return gsdk::INVALID_HSCRIPT;
		}

		return compile_script();
	}

	gsdk::HSCRIPT squirrel::compile_script() noexcept
	{
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

	void squirrel::ReleaseScript(gsdk::HSCRIPT obj)
	{
		sq_release(impl, obj);
		delete obj;
	}

	gsdk::ScriptStatus_t squirrel::Run(gsdk::HSCRIPT obj, gsdk::HSCRIPT scope, bool wait)
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

	gsdk::ScriptStatus_t squirrel::Run(gsdk::HSCRIPT obj, bool wait)
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

	gsdk::HSCRIPT squirrel::CreateScope_impl(const char *name, gsdk::HSCRIPT parent)
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

	gsdk::HSCRIPT squirrel::ReferenceScope(gsdk::HSCRIPT obj)
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

	void squirrel::ReleaseScope(gsdk::HSCRIPT obj)
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

	gsdk::HSCRIPT squirrel::LookupFunction_impl(const char *name, gsdk::HSCRIPT scope)
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

			sq_pop(impl, 1);
		}

		sq_pop(impl, 1);

		return copy;
	}

	void squirrel::ReleaseFunction(gsdk::HSCRIPT obj)
	{
		sq_release(impl, obj);
		delete obj;
	}

	bool squirrel::push(const gsdk::ScriptVariant_t &var) noexcept
	{
		auto copy_vector{
			[this,&var]<typename T>(const HSQOBJECT &classobj, T *(gsdk::ScriptVariant_t::*member)) noexcept -> bool {
				sq_pushobject(impl, classobj);
				if(SQ_FAILED(sq_createinstance(impl, -1))) {
					sq_pop(impl, 1);
					return false;
				}

				SQUserPointer userptr{nullptr};
				if(SQ_FAILED(sq_getinstanceup(impl, -1, &userptr, typeid_ptr<T>()))) {
					sq_pop(impl, 2);
					return false;
				}

				new (userptr) T{*(var.*member)};

				sq_remove(impl, -2);
				return true;
			}
		};

		switch(var.m_type) {
			case gsdk::FIELD_VOID: {
				sq_pushnull(impl);
				return true;
			}
			case gsdk::FIELD_TIME:
			case gsdk::FIELD_FLOAT: {
				sq_pushfloat(impl, var.m_float);
				return true;
			}
			case gsdk::FIELD_FLOAT64: {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				sq_pushfloat(impl, static_cast<float>(var.m_double));
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				sq_pushfloat(impl, var.m_float);
			#else
				#error
			#endif
				return true;
			}
			case gsdk::FIELD_STRING: {
				const char *ccstr{gsdk::vscript::STRING(var.m_tstr)};

				sq_pushstring(impl, ccstr, static_cast<SQInteger>(std::strlen(ccstr)));
				return true;
			}
			case gsdk::FIELD_MODELNAME:
			case gsdk::FIELD_SOUNDNAME:
			case gsdk::FIELD_CSTRING: {
				sq_pushstring(impl, var.m_ccstr, static_cast<SQInteger>(std::strlen(var.m_ccstr)));
				return true;
			}
			case gsdk::FIELD_CHARACTER: {
				char buff[2]{var.m_char, '\0'};
				sq_pushstring(impl, buff, 1);
				return true;
			}
			case gsdk::FIELD_SHORT: {
				sq_pushinteger(impl, var.m_short);
				return true;
			}
			case gsdk::FIELD_POSITIVEINTEGER_OR_NULL: {
				if(var.m_int > 0) {
					sq_pushinteger(impl, var.m_int);
				} else {
					sq_pushnull(impl);
				}
				return true;
			}
			case gsdk::FIELD_MODELINDEX:
			case gsdk::FIELD_MATERIALINDEX:
			case gsdk::FIELD_TICK:
			case gsdk::FIELD_INTEGER: {
				sq_pushinteger(impl, var.m_int);
				return true;
			}
			case gsdk::FIELD_UINT32: {
				sq_pushinteger(impl, static_cast<SQInteger>(var.m_uint));
				return true;
			}
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			case gsdk::FIELD_INTEGER64: {
			#if GSDK_ENGINE == GSDK_ENGINE_L4D2
				sq_pushinteger(impl, static_cast<SQInteger>(var.m_longlong));
			#else
				sq_pushinteger(impl, static_cast<SQInteger>(var.m_long));
			#endif
				return true;
			}
		#endif
			case gsdk::FIELD_UINT64: {
			#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
				sq_pushinteger(impl, static_cast<SQInteger>(var.m_ulonglong));
			#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
				sq_pushinteger(impl, static_cast<SQInteger>(var.m_ulong));
			#else
				#error
			#endif
				return true;
			} 
			case gsdk::FIELD_BOOLEAN: {
				sq_pushbool(impl, var.m_bool);
				return true;
			}
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT: {
				if(var.m_object && var.m_object != gsdk::INVALID_HSCRIPT) {
					sq_pushobject(impl, *var.m_object);
				} else {
					sq_pushnull(impl);
				}
				return true;
			}
			case gsdk::FIELD_QANGLE: {
				return copy_vector(qangle_class, &gsdk::ScriptVariant_t::m_qangle);
			}
			case gsdk::FIELD_POSITION_VECTOR:
			case gsdk::FIELD_VECTOR: {
				return copy_vector(vector_class, &gsdk::ScriptVariant_t::m_vector);
			}
			case gsdk::FIELD_VARIANT: {
				return false;
			}
			case gsdk::FIELD_TYPEUNKNOWN: {
				return false;
			}
			default: {
				return false;
			}
		}
	}

	bool squirrel::get(SQInteger idx, gsdk::ScriptVariant_t &var) noexcept
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
			case OT_NATIVECLOSURE:
			case OT_TABLE: {
				var.m_object = new HSQOBJECT;
				sq_resetobject(var.m_object);
				if(SQ_SUCCEEDED(sq_getstackobj(impl, idx, var.m_object))) {
					sq_addref(impl, var.m_object);
					var.m_type = gsdk::FIELD_HSCRIPT;
					var.m_flags |= gsdk::SV_FREE;
					return true;
				} else {
					delete var.m_object;
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

	bool squirrel::get(const HSQOBJECT &obj, gsdk::ScriptVariant_t &var) noexcept
	{
		std::memset(var.m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));

		switch(sq_type(obj)) {
			case OT_NULL: {
				var.m_type = gsdk::FIELD_VOID;
				var.m_object = gsdk::INVALID_HSCRIPT;
				return true;
			}
			case OT_INTEGER: {
				var.m_type = gsdk::FIELD_INTEGER;
				var.m_int = sq_objtointeger(&obj);
				return true;
			}
			case OT_FLOAT: {
				var.m_type = gsdk::FIELD_FLOAT;
				var.m_float = sq_objtofloat(&obj);
				return true;
			}
			case OT_BOOL: {
				var.m_type = gsdk::FIELD_BOOLEAN;
				var.m_bool = sq_objtobool(&obj);
				return true;
			}
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
				return true;
			}
			case OT_NATIVECLOSURE:
			case OT_TABLE: {
				sq_pushobject(impl, obj);

				var.m_object = new HSQOBJECT;
				sq_resetobject(var.m_object);
				if(SQ_SUCCEEDED(sq_getstackobj(impl, -1, var.m_object))) {
					sq_addref(impl, var.m_object);
					sq_pop(impl, 1);
					var.m_type = gsdk::FIELD_HSCRIPT;
					var.m_flags |= gsdk::SV_FREE;
					return true;
				} else {
					sq_pop(impl, 1);
					delete var.m_object;
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

	gsdk::ScriptStatus_t squirrel::ExecuteFunction_impl(gsdk::HSCRIPT obj, const gsdk::ScriptVariant_t *args, int num_args, gsdk::ScriptVariant_t *ret_var, gsdk::HSCRIPT scope, bool wait)
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

		if(SQ_FAILED(sq_call(impl, 1+num_args, ret_var ? SQTrue : SQFalse, SQTrue))) {
			failed = true;
		} else if(ret_var) {
			HSQOBJECT ret_obj;
			sq_resetobject(&ret_obj);

			if(SQ_SUCCEEDED(sq_getstackobj(impl, -1, &ret_obj))) {
				get(ret_obj, *ret_var);
			} else {
				failed = true;
			}

			sq_pop(impl, 1);
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

	SQInteger squirrel::static_func_call(HSQUIRRELVM vm)
	{
		squirrel *actual_vm{static_cast<squirrel *>(sq_getforeignptr(vm))};

		SQInteger top{sq_gettop(vm)};
		if(top < 2) {
			return sq_throwerror(vm, _SC("wrong number of parameters"));
		}

		SQUserPointer userptr{nullptr};
		if(SQ_FAILED(sq_getuserpointer(vm, -1, &userptr))) {
			return sq_throwerror(vm, _SC("failed to get userptr"));
		}

		gsdk::ScriptFunctionBinding_t *info{static_cast<gsdk::ScriptFunctionBinding_t *>(userptr)};

		std::size_t num_params{info->m_desc.m_Parameters.size()};
		if(static_cast<std::size_t>(top-2) != num_params) {
			return sq_throwerror(vm, _SC("wrong number of parameters"));
		}

		std::vector<gsdk::ScriptVariant_t> args;

		if(num_params > 0) {
			args.resize(num_params);

			for(std::size_t i{0}; i < num_params; ++i) {
				if(!actual_vm->get(2+static_cast<SQInteger>(i), args[i])) {
					return sq_throwerror(vm, _SC("failed to get arg"));
				}
			}
		}

		return actual_vm->func_call(info, nullptr, args);
	}

	SQInteger squirrel::func_call(const gsdk::ScriptFunctionBinding_t *info, void *obj, const std::vector<gsdk::ScriptVariant_t> &args) noexcept
	{
		bool is_ret_void{info->m_desc.m_ReturnType == gsdk::FIELD_VOID};

		gsdk::ScriptVariant_t ret_var;
		bool success{info->m_pfnBinding(info->m_pFunction, obj, args.empty() ? nullptr : args.data(), static_cast<int>(args.size()), is_ret_void ? nullptr : &ret_var)};

		if(got_last_exception) {
			sq_pushobject(impl, last_exception);

			SQRESULT res{sq_throwobject(impl)};

			sq_release(impl, &last_exception);
			got_last_exception = false;

			sq_resetobject(&last_exception);

			return res;
		} else if(!success) {
			return sq_throwerror(impl, _SC("binding function failed"));
		}

		if(!is_ret_void) {
			if(!push(ret_var)) {
				return sq_throwerror(impl, _SC("failed to push return"));
			}
		}

		return is_ret_void ? 0 : 1;
	}

	SQInteger squirrel::member_func_call(HSQUIRRELVM vm)
	{
		squirrel *actual_vm{static_cast<squirrel *>(sq_getforeignptr(vm))};

		SQInteger top{sq_gettop(vm)};
		if(top < 2) {
			return sq_throwerror(vm, _SC("wrong number of parameters"));
		}

		SQUserPointer userptr1{nullptr};
		if(SQ_FAILED(sq_getuserpointer(vm, -1, &userptr1))) {
			return sq_throwerror(vm, _SC("failed to get userptr"));
		}

		SQUserPointer userptr2{nullptr};
		if(SQ_FAILED(sq_getuserpointer(vm, -2, &userptr2))) {
			return sq_throwerror(vm, _SC("failed to get userptr"));
		}

		gsdk::ScriptClassDesc_t *classinfo{static_cast<gsdk::ScriptClassDesc_t *>(userptr1)};
		gsdk::ScriptFunctionBinding_t *funcinfo{static_cast<gsdk::ScriptFunctionBinding_t *>(userptr2)};

		void *obj{nullptr};

		SQUserPointer userptr3{nullptr};
		if(SQ_FAILED(sq_getinstanceup(vm, -3, &userptr3, classinfo))) {
			return sq_throwerror(vm, _SC("failed to get userptr"));
		}

		instance_info_t *instinfo{static_cast<instance_info_t *>(userptr3)};
		if(!instinfo->ptr) {
			return sq_throwerror(vm, _SC("failed to get object ptr"));
		}

		obj = instinfo->ptr;

		if(classinfo->pHelper) {
			obj = classinfo->pHelper->GetProxied(obj);

			if(!obj) {
				return sq_throwerror(vm, _SC("failed to get object ptr"));
			}
		}

		std::size_t num_params{funcinfo->m_desc.m_Parameters.size()};
		if(static_cast<std::size_t>(top-3) != num_params) {
			return sq_throwerror(vm, _SC("wrong number of parameters"));
		}

		std::vector<gsdk::ScriptVariant_t> args;

		if(num_params > 0) {
			args.resize(num_params);

			for(std::size_t i{0}; i < num_params; ++i) {
				if(!actual_vm->get(3+static_cast<SQInteger>(i), args[i])) {
					return sq_throwerror(vm, _SC("failed to get arg"));
				}
			}
		}

		return actual_vm->func_call(funcinfo, obj, args);
	}

	SQInteger squirrel::external_ctor(HSQUIRRELVM vm)
	{
		SQInteger top{sq_gettop(vm)};
		if(top < 2) {
			return sq_throwerror(vm, _SC("wrong number of parameters"));
		}

		SQUserPointer userptr1{nullptr};
		if(SQ_FAILED(sq_getuserpointer(vm, -1, &userptr1))) {
			return sq_throwerror(vm, _SC("failed to get userptr"));
		}

		gsdk::ScriptClassDesc_t *classinfo{static_cast<gsdk::ScriptClassDesc_t *>(userptr1)};

		return sq_throwerror(vm, _SC("not implemented yet"));
	}

	SQInteger squirrel::generic_dtor(SQUserPointer userptr, SQInteger size)
	{
		instance_info_t *info{static_cast<instance_info_t *>(userptr)};

		delete info;

		return 0;
	}

	SQInteger squirrel::external_dtor(SQUserPointer userptr, SQInteger size)
	{
		instance_info_t *info{static_cast<instance_info_t *>(userptr)};

		info->classinfo->m_pfnDestruct(info->ptr);

		delete info;

		return 0;
	}

	bool squirrel::register_func(const gsdk::ScriptClassDesc_t *classinfo, const gsdk::ScriptFunctionBinding_t *info, std::string_view name_str) noexcept
	{
		using namespace std::literals::string_literals;
		using namespace std::literals::string_view_literals;

		sq_pushstring(impl, name_str.data(), static_cast<SQInteger>(name_str.length()));
		if(classinfo && (info->m_flags & gsdk::SF_MEMBER_FUNC)) {
			sq_pushuserpointer(impl, const_cast<gsdk::ScriptClassDesc_t *>(classinfo));
			sq_pushuserpointer(impl, const_cast<gsdk::ScriptFunctionBinding_t *>(info));
			sq_newclosure(impl, member_func_call, 2);
		} else {
			sq_pushuserpointer(impl, const_cast<gsdk::ScriptFunctionBinding_t *>(info));
			sq_newclosure(impl, static_func_call, 1);
		}

		if(debug_vm) {
			if(SQ_FAILED(sq_setnativeclosurename(impl, -1, name_str.data()))) {
				warning("vmod vm: failed to set '%s' name\n"sv, name_str.data());
			}
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

		if(SQ_FAILED(sq_setparamscheck(impl, 1+static_cast<SQInteger>(info->m_desc.m_Parameters.size()), typemask.empty() ? nullptr : typemask.c_str()))) {
			error("vmod vm: failed to set '%s' typemask '%s'\n"sv, name_str.data(), typemask.c_str());
			sq_pop(impl, 2);
			return false;
		}

		if(debug_vm && got_reg_func_desc) {
			if(info->m_desc.m_pszDescription && info->m_desc.m_pszDescription[0] != '@') {
				HSQOBJECT func_obj;
				if(SQ_SUCCEEDED(sq_getstackobj(impl, -1, &func_obj))) {
					std::string sig;

					sig += bindings::docs::type_name(info->m_desc.m_ReturnType, true);
					sig += ' ';
					if(classinfo) {
						sig += classinfo->m_pszScriptName;
						sig += "::"sv;
					}
					sig += name_str;
					sig += '(';
					for(gsdk::ScriptDataType_t param : info->m_desc.m_Parameters) {
						sig += bindings::docs::type_name(param, true);
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

		if(SQ_FAILED(sq_newslot(impl, -3, (classinfo && !(info->m_flags & gsdk::SF_MEMBER_FUNC)) ? SQTrue : SQFalse))) {
			error("vmod vm: failed to create '%s'\n"sv, name_str.data());
			return false;
		}

		return true;
	}

	void squirrel::RegisterFunction_nonvirtual(gsdk::ScriptFunctionBinding_t *info) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string name_str{info->m_desc.m_pszScriptName};

		if(registered_funcs.find(name_str) != registered_funcs.end()) {
			warning("vmod vm: duplicate '%s'\n"sv, name_str.c_str());
			return;
		}

		sq_pushroottable(impl);

		bool success{register_func(nullptr, info, name_str)};
		if(!success) {
			error("vmod vm: failed to register '%s'\n"sv, name_str.c_str());
		}

		sq_pop(impl, 1);

		if(success) {
			registered_funcs.emplace(std::move(name_str), func_info_t{info});
		}
	}

	void squirrel::RegisterFunction(gsdk::ScriptFunctionBinding_t *info)
	{
		RegisterFunction_nonvirtual(info);
	}

	SQInteger squirrel::instance_str(HSQUIRRELVM vm)
	{
		return sq_throwerror(vm, _SC("not implemented yet"));
	}

	SQInteger squirrel::instance_valid(HSQUIRRELVM vm)
	{
		return sq_throwerror(vm, _SC("not implemented yet"));
	}

	bool squirrel::register_class(const gsdk::ScriptClassDesc_t *info, std::string &&classname_str, HSQOBJECT **obj) noexcept
	{
		using namespace std::literals::string_view_literals;

		HSQOBJECT *base_obj{nullptr};

		bool has_base{info->m_pBaseDesc != nullptr};

		if(has_base) {
			std::string basename_str{info->m_pBaseDesc->m_pszScriptName};

			auto base_it{registered_classes.find(basename_str)};
			if(base_it == registered_classes.end()) {
				sq_pushroottable(impl);

				bool success{register_class(info->m_pBaseDesc, std::move(basename_str), &base_obj)};

				sq_pop(impl, 1);

				if(!success) {
					error("vmod vm: failed to register '%s' base '%s'\n"sv, classname_str.c_str(), basename_str.c_str());
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
			error("vmod vm: failed to create '%s'\n"sv, classname_str.c_str());
			return false;
		}

		if(SQ_FAILED(sq_settypetag(impl, -1, const_cast<gsdk::ScriptClassDesc_t *>(info)))) {
			sq_pop(impl, has_base ? 3 : 2);
			error("vmod vm: failed to set '%s' typetag\n"sv, classname_str.c_str());
			return false;
		}

		if(SQ_FAILED(sq_setclassudsize(impl, -1, sizeof(instance_info_t)))) {
			sq_pop(impl, has_base ? 3 : 2);
			error("vmod vm: failed to set '%s' size\n"sv, classname_str.c_str());
			return false;
		}

		std::unique_ptr<class_info_t> tmpinfo{new class_info_t{info}};

		if(info->m_pfnConstruct) {
			sq_pushstring(impl, _SC("constructor"), 11);
			sq_pushuserpointer(impl, const_cast<gsdk::ScriptClassDesc_t *>(info));
			sq_newclosure(impl, external_ctor, 1);
			if(SQ_FAILED(sq_setparamscheck(impl, 1, _SC("x")))) {
				error("vmod vm: failed to set '%s' constructor typemask\n"sv);
				sq_pop(impl, has_base ? 3 : 2);
				return false;
			}
			if(debug_vm) {
				if(SQ_FAILED(sq_setnativeclosurename(impl, -1, _SC("constructor")))) {
					warning("vmod vm: failed to set '%s' constructor name\n"sv);
				}
			}
			if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
				sq_pop(impl, has_base ? 3 : 2);
				error("vmod vm: failed to create '%s' constructor\n"sv, classname_str.c_str());
				return false;
			}
		}

		{
			{
				sq_pushstring(impl, _SC("_tostring"), 9);
				sq_pushuserpointer(impl, const_cast<gsdk::ScriptClassDesc_t *>(info));
				sq_newclosure(impl, instance_str, 1);
				if(SQ_FAILED(sq_setparamscheck(impl, 1, _SC("x")))) {
					warning("vmod vm: failed to set '%s' _tostring typemask\n"sv);
				}
				if(debug_vm) {
					if(SQ_FAILED(sq_setnativeclosurename(impl, -1, _SC("_tostring")))) {
						warning("vmod vm: failed to set '%s' _tostring name\n"sv);
					}
				}
				if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
					warning("vmod vm: failed to create '%s' _tostring\n"sv, classname_str.c_str());
				}
			}

			{
				sq_pushstring(impl, _SC("IsValid"), 7);
				sq_pushuserpointer(impl, const_cast<gsdk::ScriptClassDesc_t *>(info));
				sq_newclosure(impl, instance_valid, 1);
				if(SQ_FAILED(sq_setparamscheck(impl, 1, _SC("x")))) {
					error("vmod vm: failed to set '%s' IsValid typemask\n"sv);
					sq_pop(impl, has_base ? 3 : 2);
					return false;
				}
				if(debug_vm) {
					if(SQ_FAILED(sq_setnativeclosurename(impl, -1, _SC("IsValid")))) {
						warning("vmod vm: failed to set '%s' IsValid name\n"sv);
					}
				}
				if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
					sq_pop(impl, has_base ? 3 : 2);
					error("vmod vm: failed to create '%s' IsValid\n"sv, classname_str.c_str());
					return false;
				}
			}
		}

		for(const auto &func_info : info->m_FunctionBindings) {
			std::string funcname_str{func_info.m_desc.m_pszScriptName};

			if(tmpinfo->registered_funcs.find(funcname_str) != tmpinfo->registered_funcs.end()) {
				warning("vmod vm: duplicate '%s::%s'\n"sv, classname_str.c_str(), funcname_str.c_str());
				continue;
			}

			if(!register_func(info, &func_info, funcname_str)) {
				sq_pop(impl, has_base ? 3 : 2);
				error("vmod vm: failed to register '%s::%s'\n"sv, classname_str.c_str(), funcname_str.c_str());
				return false;
			}

			tmpinfo->registered_funcs.emplace(std::move(funcname_str), func_info_t{&func_info});
		}

		sq_resetobject(&tmpinfo->obj);

		if(SQ_FAILED(sq_getstackobj(impl, -1, &tmpinfo->obj))) {
			error("vmod vm: failed to get '%s' obj\n"sv, classname_str.c_str());
			return false;
		}

		sq_addref(impl, &tmpinfo->obj);

		if(SQ_FAILED(sq_newslot(impl, -3, SQFalse))) {
			sq_release(impl, &tmpinfo->obj);
			sq_resetobject(&tmpinfo->obj);
			error("vmod vm: failed to create '%s'\n"sv, classname_str.c_str());
			return false;
		}

		auto this_it{registered_classes.emplace(std::move(classname_str), std::move(tmpinfo)).first};

		if(obj) {
			*obj = &this_it->second->obj;
		}

		last_registered_class = this_it;

		return true;
	}

	bool squirrel::RegisterClass_nonvirtual(gsdk::ScriptClassDesc_t *info) noexcept
	{
		using namespace std::literals::string_view_literals;

		std::string classname_str{info->m_pszScriptName};

		if(registered_classes.find(classname_str) != registered_classes.end()) {
			warning("vmod vm: duplicate '%s'\n"sv, classname_str.c_str());
			return true;
		}

		sq_pushroottable(impl);

		bool success{register_class(info, std::move(classname_str), nullptr)};
		if(!success) {
			error("vmod vm: failed to register '%s'\n"sv, classname_str.c_str());
		}

		sq_pop(impl, 1);

		return success;
	}

	bool squirrel::RegisterClass(gsdk::ScriptClassDesc_t *info)
	{
		return RegisterClass_nonvirtual(info);
	}

	gsdk::HSCRIPT squirrel::RegisterInstance_impl_nonvirtual(gsdk::ScriptClassDesc_t *info, void *ptr) noexcept
	{
		std::string classname_str{info->m_pszScriptName};

		auto class_it{registered_classes.find(classname_str)};
		if(class_it == registered_classes.end()) {
			if(!squirrel::RegisterClass_nonvirtual(info)) {
				return gsdk::INVALID_HSCRIPT;
			}

			class_it = last_registered_class;
		}

		sq_pushobject(impl, class_it->second->obj);

		if(SQ_FAILED(sq_createinstance(impl, -1))) {
			sq_pop(impl, 1);
			return gsdk::INVALID_HSCRIPT;
		}

		SQUserPointer userptr{nullptr};
		if(SQ_FAILED(sq_getinstanceup(impl, -1, &userptr, info))) {
			sq_pop(impl, 2);
			return gsdk::INVALID_HSCRIPT;
		}

		new (userptr) instance_info_t{info, ptr};

		if(info->m_pfnDestruct) {
			sq_setreleasehook(impl, -1, external_dtor);
		} else {
			sq_setreleasehook(impl, -1, generic_dtor);
		}

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

	gsdk::HSCRIPT squirrel::RegisterInstance_impl(gsdk::ScriptClassDesc_t *info, void *ptr)
	{
		return RegisterInstance_impl_nonvirtual(info, ptr);
	}

	void squirrel::SetInstanceUniqeId(gsdk::HSCRIPT obj, const char *id)
	{
		sq_pushobject(impl, *obj);

		SQUserPointer userptr{nullptr};
		if(SQ_SUCCEEDED(sq_getinstanceup(impl, -1, &userptr, nullptr))) {
			instance_info_t *info{static_cast<instance_info_t *>(userptr)};
			info->id = id;
		}

		sq_pop(impl, 1);
	}

	void squirrel::RemoveInstance(gsdk::HSCRIPT obj)
	{
		sq_pushobject(impl, *obj);

		sq_setreleasehook(impl, -1, nullptr);

		SQUserPointer userptr{nullptr};
		if(SQ_SUCCEEDED(sq_getinstanceup(impl, -1, &userptr, nullptr))) {
			instance_info_t *info{static_cast<instance_info_t *>(userptr)};
			delete info;
		}

		sq_setinstanceup(impl, -1, nullptr);

		sq_pop(impl, 1);

		sq_release(impl, obj);
		delete obj;
	}

	void *squirrel::GetInstanceValue_impl(gsdk::HSCRIPT obj, gsdk::ScriptClassDesc_t *classinfo)
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

	bool squirrel::GenerateUniqueKey(const char *root, char *buff, int len)
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

	bool squirrel::ValueExists(gsdk::HSCRIPT scope, const char *name)
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

	bool squirrel::SetValue_nonvirtual(gsdk::HSCRIPT obj, const char *name, const char *value) noexcept
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushstring(impl, name, static_cast<SQInteger>(std::strlen(name)));

		if(value) {
			sq_pushstring(impl, value, static_cast<SQInteger>(std::strlen(value)));
		} else {
			sq_pushnull(impl);
		}

		bool success{SQ_SUCCEEDED(sq_newslot(impl, -3, SQFalse))};

		sq_pop(impl, 1);

		return success;
	}

	bool squirrel::SetValue(gsdk::HSCRIPT obj, const char *name, const char *value)
	{
		return SetValue_nonvirtual(obj, name, value);
	}

	bool squirrel::SetValue_impl_nonvirtual(gsdk::HSCRIPT obj, const char *name, const gsdk::ScriptVariant_t &value) noexcept
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushstring(impl, name, static_cast<SQInteger>(std::strlen(name)));
		if(!push(value)) {
			sq_pop(impl, 2);
			return false;
		}

		bool success{SQ_SUCCEEDED(sq_newslot(impl, -3, SQFalse))};

		sq_pop(impl, 1);

		return success;
	}

	bool squirrel::SetValue_impl(gsdk::HSCRIPT obj, const char *name, const gsdk::ScriptVariant_t &value)
	{
		return SetValue_impl_nonvirtual(obj, name, value);
	}

	bool squirrel::SetValue_impl(gsdk::HSCRIPT obj, int idx, const gsdk::ScriptVariant_t &value)
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushinteger(impl, idx);
		if(!push(value)) {
			sq_pop(impl, 2);
			return false;
		}

		bool success{SQ_SUCCEEDED(sq_newslot(impl, -3, SQFalse))};

		sq_pop(impl, 1);

		return success;
	}

	void squirrel::get_obj(gsdk::ScriptVariant_t &value) noexcept
	{
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

	void squirrel::CreateTable_impl(gsdk::ScriptVariant_t &value)
	{
		sq_newtable(impl);

		get_obj(value);
	}

	void squirrel::CreateArray_impl_nonvirtual(gsdk::ScriptVariant_t &value) noexcept
	{
		sq_newarray(impl, 0);

		get_obj(value);
	}

	void squirrel::CreateArray_impl(gsdk::ScriptVariant_t &value)
	{
		CreateArray_impl_nonvirtual(value);
	}

	bool squirrel::IsTable_nonvirtual(gsdk::HSCRIPT obj) noexcept
	{
		return sq_istable(*obj);
	}

	bool squirrel::IsTable(gsdk::HSCRIPT obj)
	{
		return IsTable_nonvirtual(obj);
	}

	int squirrel::GetNumTableEntries(gsdk::HSCRIPT obj) const
	{
		if(obj) {
			sq_pushobject(impl, *obj);
		} else {
			sq_pushroottable(impl);
		}

		int size{sq_getsize(impl, -1)};

		sq_pop(impl, 1);

		return size;
	}

	int squirrel::GetArrayCount_nonvirtual(gsdk::HSCRIPT obj) noexcept
	{
		sq_pushobject(impl, *obj);

		int size{sq_getsize(impl, -1)};

		sq_pop(impl, 1);

		return size;
	}

	int squirrel::GetKeyValue(gsdk::HSCRIPT obj, int it, gsdk::ScriptVariant_t *key, gsdk::ScriptVariant_t *value)
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

			sq_pop(impl, 2);

			(void)sq_getinteger(impl, -1, &next_it);
		}

		sq_pop(impl, 2);

		return next_it;
	}

	int squirrel::GetKeyValue2(gsdk::HSCRIPT obj, int it, gsdk::ScriptVariant_t *key, gsdk::ScriptVariant_t *value)
	{
		return squirrel::GetKeyValue(obj, it, key, value);
	}

	bool squirrel::GetValue_impl(gsdk::HSCRIPT obj, const char *name, gsdk::ScriptVariant_t *value)
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

			sq_pop(impl, 1);
		}

		sq_pop(impl, 1);

		return got;
	}

	bool squirrel::GetValue_impl(gsdk::HSCRIPT obj, int idx, gsdk::ScriptVariant_t *value)
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

			sq_pop(impl, 1);
		}

		sq_pop(impl, 1);

		return got;
	}

	bool squirrel::GetScalarValue(gsdk::HSCRIPT obj, gsdk::ScriptVariant_t *value)
	{
		return get(*obj, *value);
	}

	void squirrel::ReleaseValue(gsdk::ScriptVariant_t &value)
	{
		switch(value.m_type) {
			case gsdk::FIELD_HSCRIPT_NEW_INSTANCE:
			case gsdk::FIELD_HSCRIPT: {
				if(value.m_flags & gsdk::SV_FREE) {
					sq_release(impl, value.m_object);
					delete value.m_object;

					value.m_flags = gsdk::SV_NOFLAGS;
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

	bool squirrel::ClearValue(gsdk::HSCRIPT obj, const char *name)
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

	bool squirrel::IsArray_nonvirtual(gsdk::HSCRIPT obj) noexcept
	{
		return sq_isarray(*obj);
	}

	bool squirrel::IsArray(gsdk::HSCRIPT obj)
	{
		return IsArray_nonvirtual(obj);
	}

	int squirrel::GetArrayCount(gsdk::HSCRIPT obj)
	{
		return GetArrayCount_nonvirtual(obj);
	}

	void squirrel::ArrayAddToTail(gsdk::HSCRIPT obj, const gsdk::ScriptVariant_t &value)
	{
		sq_pushobject(impl, *obj);

		if(!push(value)) {
			sq_pop(impl, 1);
			return;
		}

		(void)sq_arrayappend(impl, -2);

		sq_pop(impl, 1);
	}

	void squirrel::WriteState(gsdk::CUtlBuffer *)
	{
		debugtrap();
	}

	void squirrel::ReadState(gsdk::CUtlBuffer *)
	{
		debugtrap();
	}

	void squirrel::CollectGarbage(const char *, bool)
	{
		sq_collectgarbage(impl);
	}

	void squirrel::RemoveOrphanInstances()
	{
		sq_collectgarbage(impl);
	}

	void squirrel::DumpState()
	{
		
	}

	void squirrel::SetOutputCallback(gsdk::ScriptOutputFunc_t func)
	{
		output_callback = func;
	}

	void squirrel::SetErrorCallback(gsdk::ScriptErrorFunc_t func)
	{
		err_callback = func;
	}

	bool squirrel::RaiseException_impl(const char *msg)
	{
		sq_pushstring(impl, msg, static_cast<SQInteger>(std::strlen(msg)));

		sq_resetobject(&last_exception);
		if(SQ_SUCCEEDED(sq_getstackobj(impl, -1, &last_exception))) {
			sq_addref(impl, &last_exception);
			got_last_exception = true;
		} else {
			got_last_exception = false;
		}

		sq_pop(impl, 1);

		return true;
	}

	gsdk::HSCRIPT squirrel::GetRootTable()
	{
		return &root_table;
	}

	gsdk::HSCRIPT squirrel::CopyHandle(gsdk::HSCRIPT)
	{
		return gsdk::INVALID_HSCRIPT;
	}

	gsdk::HSCRIPT squirrel::GetIdentity(gsdk::HSCRIPT)
	{
		return gsdk::INVALID_HSCRIPT;
	}

	class metamethod_delegate_impl final
	{
		friend class squirrel;

	public:
		inline metamethod_delegate_impl(gsdk::ISquirrelMetamethodDelegate *delegate, bool free) noexcept
			: interface{delegate}, should_free{free}
		{
		}

		inline ~metamethod_delegate_impl() noexcept
		{
			if(should_free) {
				delete interface;
			}
		}

	private:
		gsdk::ISquirrelMetamethodDelegate *interface{nullptr};
		bool should_free{true};

	private:
		metamethod_delegate_impl() = delete;
		metamethod_delegate_impl(const metamethod_delegate_impl &) = delete;
		metamethod_delegate_impl &operator=(const metamethod_delegate_impl &) = delete;
		metamethod_delegate_impl(metamethod_delegate_impl &&) = delete;
		metamethod_delegate_impl &operator=(metamethod_delegate_impl &&) = delete;
	};

	SQInteger squirrel::metamethod_call(HSQUIRRELVM vm)
	{
		squirrel *actual_vm{static_cast<squirrel *>(sq_getforeignptr(vm))};

		SQInteger top{sq_gettop(vm)};
		if(top != 3) {
			return sq_throwerror(vm, _SC("wrong number of parameters"));
		}

		SQUserPointer userptr{nullptr};
		if(SQ_FAILED(sq_getuserpointer(vm, -1, &userptr))) {
			return sq_throwerror(vm, _SC("failed to get userptr"));
		}

		metamethod_delegate_impl *delegate_impl{static_cast<metamethod_delegate_impl *>(userptr)};

		const SQChar *name{nullptr};
		SQInteger name_len{0};
		if(SQ_FAILED(sq_getstringandsize(vm, -2, &name, &name_len))) {
			return sq_throwerror(vm, _SC("failed to get name"));
		}

		gsdk::ScriptVariant_t value;
		if(delegate_impl->interface->Get(name, value)) {
			if(!actual_vm->push(value)) {
				return sq_throwerror(vm, _SC("failed to push value"));
			}
		} else {
			return sq_throwerror(vm, _SC("failed to get value"));
		}

		return 1;
	}

	gsdk::CSquirrelMetamethodDelegateImpl *squirrel::MakeSquirrelMetamethod_Get_impl(gsdk::HSCRIPT &scope, const char *name, gsdk::ISquirrelMetamethodDelegate *delegate, bool free)
	{
		if(scope) {
			sq_pushobject(impl, *scope);
		} else {
			sq_pushroottable(impl);
		}

		sq_pushstring(impl, name, static_cast<SQInteger>(std::strlen(name)));

		metamethod_delegate_impl *delegate_impl{nullptr};

		if(SQ_SUCCEEDED(sq_get(impl, -2))) {
			sq_newtable(impl);

			delegate_impl = new metamethod_delegate_impl{delegate, free};

			sq_pushstring(impl, _SC("_get"), 4);
			sq_pushuserpointer(impl, delegate_impl);
			sq_newclosure(impl, metamethod_call, 1);

			if(SQ_SUCCEEDED(sq_newslot(impl, -3, SQFalse))) {
				sq_pushstring(impl, _SC("cppdelegate"), 11);
				sq_pushuserpointer(impl, delegate_impl);

				if(SQ_SUCCEEDED(sq_newslot(impl, -3, SQFalse))) {
					sq_setdelegate(impl, -2);
				} else {
					sq_pop(impl, 1);

					delete delegate_impl;
					delegate_impl = nullptr;
				}
			} else {
				sq_pop(impl, 1);

				delete delegate_impl;
				delegate_impl = nullptr;
			}
		}

		sq_pop(impl, 1);

		return reinterpret_cast<gsdk::CSquirrelMetamethodDelegateImpl *>(delegate_impl);
	}

	void squirrel::DestroySquirrelMetamethod_Get(gsdk::CSquirrelMetamethodDelegateImpl *delegate_impl)
	{
		delete reinterpret_cast<metamethod_delegate_impl *>(delegate_impl);
	}
}
