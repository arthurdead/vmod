#define __VMOD_COMPILING_GSDK
#include "vscript.hpp"
#include "../tier0/dbg.hpp"

namespace gsdk
{
	namespace detail
	{
		static char vscript_uniqueid_buffer[IScriptVM::unique_id_max];
		static char vscript_exception_buffer[MAXPRINTMSG];
	}

	ISquirrelMetamethodDelegate::~ISquirrelMetamethodDelegate() {}

	void *IScriptInstanceHelper::GetProxied(void *ptr)
	{ return ptr; }
	void *IScriptInstanceHelper::BindOnRead([[maybe_unused]] HSCRIPT, void *ptr, [[maybe_unused]] const char *)
	{ return ptr; }

	IScriptVM *g_pScriptVM{nullptr};

	__attribute__((__format__(__printf__, 2, 0))) bool IScriptVM::RaiseExceptionv(const char *fmt, va_list vargs) noexcept
	{
	#ifdef __clang__
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wformat-nonliteral"
	#endif
		std::vsnprintf(detail::vscript_exception_buffer, sizeof(detail::vscript_exception_buffer), fmt, vargs);
	#ifdef __clang__
		#pragma clang diagnostic pop
	#endif
		bool ret{RaiseException_impl(detail::vscript_exception_buffer)};
		return ret;
	}

	__attribute__((__format__(__printf__, 2, 3))) bool IScriptVM::RaiseException(const char *fmt, ...) noexcept
	{
		va_list vargs;
		va_start(vargs, fmt);
		bool ret{RaiseExceptionv(fmt, vargs)};
		va_end(vargs);
		return ret;
	}

#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
	ScriptHandleWrapper_t IScriptVM::CreateArray() noexcept
	{
		ScriptVariant_t var;
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		gsdk::ScriptLanguage_t lang{GetLanguage()};
		switch(lang) {
			case gsdk::SL_SQUIRREL:
			(this->*squirrel_CreateArray_ptr)(var);
			break;
			default:
			return ScriptHandleWrapper_t{};
		}
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		CreateArray_impl(var);
	#else
		#error
	#endif
		ScriptHandleWrapper_t tmp{std::move(var)};
		tmp.type = HANDLETYPE_ARRAY;
		return tmp;
	}
#endif

#if GSDK_ENGINE == GSDK_ENGINE_TF2
	void(IScriptVM::*IScriptVM::squirrel_CreateArray_ptr)(ScriptVariant_t &) {nullptr};
	int(IScriptVM::*IScriptVM::squirrel_GetArrayCount_ptr)(HSCRIPT) const {nullptr};
	bool(IScriptVM::*IScriptVM::squirrel_IsArray_ptr)(HSCRIPT) const {nullptr};
	bool(IScriptVM::*IScriptVM::squirrel_IsTable_ptr)(HSCRIPT) const {nullptr};

	int IScriptVM::GetArrayCount(HSCRIPT array) const noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!array || array == INVALID_HSCRIPT) {
			return 0;
		}
	#endif

		gsdk::ScriptLanguage_t lang{GetLanguage()};
		switch(lang) {
			case gsdk::SL_SQUIRREL:
			return (this->*squirrel_GetArrayCount_ptr)(array);
			default:
			return 0;
		}
	}

	bool IScriptVM::IsArray(HSCRIPT array) const noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!array || array == INVALID_HSCRIPT) {
			return false;
		}
	#endif

		gsdk::ScriptLanguage_t lang{GetLanguage()};
		switch(lang) {
			case gsdk::SL_SQUIRREL:
			return (this->*squirrel_IsArray_ptr)(array);
			default:
			return false;
		}
	}

	bool IScriptVM::IsTable(HSCRIPT table) const noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!table || table == INVALID_HSCRIPT) {
			return false;
		}
	#endif

		gsdk::ScriptLanguage_t lang{GetLanguage()};
		switch(lang) {
			case gsdk::SL_SQUIRREL:
			return (this->*squirrel_IsTable_ptr)(table);
			default:
			return false;
		}
	}
#endif

#if GSDK_ENGINE != GSDK_ENGINE_L4D2 && GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	bool IScriptVM::IsArray(HSCRIPT array) const noexcept
	{
		if(!array || array == INVALID_HSCRIPT) {
			return false;
		}

		gsdk::ScriptLanguage_t lang{GetLanguage()};
		switch(lang) {
			case gsdk::SL_SQUIRREL:
			return (reinterpret_cast<HSQOBJECT *>(array)->_type == OT_ARRAY);
			default:
			return false;
		}
	}

	bool IScriptVM::IsTable(HSCRIPT table) const noexcept
	{
		if(!table || table == INVALID_HSCRIPT) {
			return false;
		}

		gsdk::ScriptLanguage_t lang{GetLanguage()};
		switch(lang) {
			case gsdk::SL_SQUIRREL:
			return (reinterpret_cast<HSQOBJECT *>(table)->_type == OT_TABLE);
			default:
			return false;
		}
	}

	void IScriptVM::ArrayAddToTail(HSCRIPT array, const ScriptVariant_t &var)
	{
		if(!array || array == INVALID_HSCRIPT) {
			return;
		}

		gsdk::ScriptLanguage_t lang{GetLanguage()};
		switch(lang) {
			case gsdk::SL_SQUIRREL:
			//array->_unVal.pArray->Append();
			break;
			default:
			break;
		}
	}

	ScriptHandleWrapper_t IScriptVM::CreateArray() noexcept
	{
		ScriptHandleWrapper_t tmp{};
		tmp.type = HANDLETYPE_ARRAY;
		return tmp;
	}

	int IScriptVM::GetArrayCount(HSCRIPT array) const noexcept
	{
		if(!array || array == INVALID_HSCRIPT) {
			return 0;
		}

		gsdk::ScriptLanguage_t lang{GetLanguage()};
		switch(lang) {
			case gsdk::SL_SQUIRREL:
			return reinterpret_cast<HSQOBJECT *>(array)->_unVal.pArray->Size();
			default:
			return 0;
		}
	}

	bool IScriptVM::GetScalarValue(HSCRIPT object, ScriptVariant_t *var) noexcept
	{
		std::memset(var->m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));

		gsdk::ScriptLanguage_t lang{GetLanguage()};
		switch(lang) {
			case gsdk::SL_SQUIRREL: {
				HSQOBJECT *sqobj{reinterpret_cast<HSQOBJECT *>(object)};

				switch(sqobj->_type) {
					case OT_NULL: {
						var->m_type = FIELD_VOID;
						var->m_object = INVALID_HSCRIPT;
						return true;
					}
					case OT_INTEGER: {
						var->m_type = FIELD_INTEGER;
						var->m_int = sqobj->_unVal.nInteger;
						return true;
					}
					case OT_FLOAT: {
						var->m_type = FIELD_FLOAT;
						var->m_float = sqobj->_unVal.fFloat;
						return true;
					}
					case OT_BOOL: {
						var->m_type = FIELD_BOOLEAN;
						var->m_bool = (sqobj->_unVal.nInteger > 0 ? true : false);
						return true;
					}
					case OT_STRING: {
						//TODO!!!!
						var->m_type = FIELD_CSTRING;
						var->m_ccstr = nullptr;
						return false;
					}
					default: {
						var->m_type = FIELD_TYPEUNKNOWN;
						var->m_object = INVALID_HSCRIPT;
						return false;
					}
				}
			}
			default: {
				var->m_type = FIELD_TYPEUNKNOWN;
				var->m_object = INVALID_HSCRIPT;
				return false;
			}
		}
	}
#endif

#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
	ScriptHandleWrapper_t IScriptVM::ReferenceObject(HSCRIPT object) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!object || object == INVALID_HSCRIPT) {
			return INVALID_HSCRIPT;
		}
	#endif

		HSCRIPT ret{ReferenceScope_impl(object)};
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
	#endif

		ScriptHandleWrapper_t tmp;
		tmp.should_free_ = true;
		tmp.object = ret;
		tmp.type = HANDLETYPE_UNKNOWN;

		return tmp;
	}
#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	ScriptHandleWrapper_t IScriptVM::ReferenceObject(HSCRIPT object) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!object || object == INVALID_HSCRIPT) {
			return ScriptHandleWrapper_t{};
		}
	#endif

		ScriptHandleWrapper_t tmp;
		tmp.should_free = false;
		tmp.object = object;
		tmp.type = HANDLETYPE_UNKNOWN;

		return tmp;
	}
#endif

	ScriptHandleWrapper_t IScriptVM::ReferenceFunction(HSCRIPT object) noexcept
	{
		ScriptHandleWrapper_t tmp{ReferenceObject(object)};
		tmp.type = HANDLETYPE_FUNCTION;
		return tmp;
	}

	ScriptHandleWrapper_t IScriptVM::LookupFunction(const char *name, HSCRIPT scope) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(scope == INVALID_HSCRIPT) {
			ScriptHandleWrapper_t tmp;
			tmp.type = HANDLETYPE_FUNCTION;
			return tmp;
		}
	#endif

		HSCRIPT ret{LookupFunction_impl(name, scope)};
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
	#endif

		ScriptHandleWrapper_t tmp;
		tmp.should_free_ = true;
		tmp.object = ret;
		tmp.type = HANDLETYPE_FUNCTION;

		return tmp;
	}

	ScriptHandleWrapper_t IScriptVM::RegisterInstance(ScriptClassDesc_t *desc, void *value) noexcept
	{
		HSCRIPT ret{RegisterInstance_impl(desc, value)};
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
	#endif

		ScriptHandleWrapper_t tmp;
		tmp.should_free_ = true;
		tmp.object = ret;
		tmp.type = HANDLETYPE_INSTANCE;

		return tmp;
	}

	ScriptStatus_t IScriptVM::ExecuteFunction(HSCRIPT func, const ScriptVariant_t *args, int num_args, ScriptVariant_t *ret, HSCRIPT scope, bool wait) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!func || func == INVALID_HSCRIPT) {
			return SCRIPT_ERROR;
		}

		if(scope == INVALID_HSCRIPT) {
			return SCRIPT_ERROR;
		}
	#endif

	#ifndef __VMOD_USING_CUSTOM_VM
		std::size_t num_args_siz{static_cast<std::size_t>(num_args)};
		for(std::size_t i{0}; i < num_args_siz; ++i) {
			fixup_var(const_cast<ScriptVariant_t &>(args[i]));
		}
	#endif

		return ExecuteFunction_impl(func, args, num_args, ret, scope, wait);
	}

	bool IScriptVM::SetValue(HSCRIPT scope, const char *name, const ScriptVariant_t &var) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(scope == INVALID_HSCRIPT) {
			return false;
		}

		ScriptVariant_t temp;
		temp.m_type = fixup_var_field(var.m_type);
		temp.m_flags = var.m_flags & ~SV_FREE;
		std::memcpy(temp.m_data, var.m_data, sizeof(ScriptVariant_t::m_data));
		return SetValue_impl(scope, name, temp);
	#else
		return SetValue_impl(scope, name, var);
	#endif
	}

	bool IScriptVM::SetValue(HSCRIPT scope, const char *name, ScriptVariant_t &&var) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(scope == INVALID_HSCRIPT) {
			return false;
		}
	#endif

		bool ret{SetValue(scope, name, static_cast<const ScriptVariant_t &>(var))};
		var.free();
		return ret;
	}

	bool IScriptVM::SetValue(HSCRIPT scope, const char *name, HSCRIPT object) noexcept
	{
		if(!object || object == INVALID_HSCRIPT) {
			return false;
		}

	#ifndef __VMOD_USING_CUSTOM_VM
		if(scope == INVALID_HSCRIPT) {
			return false;
		}
	#endif

		ScriptVariant_t var;
		var.m_type = FIELD_HSCRIPT;
		var.m_flags = SV_NOFLAGS;
		var.m_object = object;
		return SetValue(scope, name, static_cast<const ScriptVariant_t &>(var));
	}

	ScriptHandleWrapper_t IScriptVM::CreateTable() noexcept
	{
		ScriptVariant_t var;
		CreateTable_impl(var);
		ScriptHandleWrapper_t tmp{std::move(var)};
		tmp.type = HANDLETYPE_TABLE;
		return tmp;
	}

	void IScriptVM::ReleaseTable(HSCRIPT table) noexcept
	{
		ReleaseObject(table);
	}

	void IScriptVM::ReleaseArray(HSCRIPT array) noexcept
	{
		ReleaseObject(array);
	}

	ScriptHandleWrapper_t IScriptVM::CreateScope(const char *script, HSCRIPT parent) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(parent == INVALID_HSCRIPT) {
			return INVALID_HSCRIPT;
		}
	#endif

		HSCRIPT ret{CreateScope_impl(script, parent)};
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
	#endif

		ScriptHandleWrapper_t tmp;
		tmp.should_free_ = true;
		tmp.object = ret;
		tmp.type = HANDLETYPE_SCOPE;

		return tmp;
	}

	int IScriptVM::GetArrayValue(HSCRIPT array, int it, ScriptVariant_t *value) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!array || array == INVALID_HSCRIPT) {
			return -1;
		}
	#endif

		ScriptVariant_t tmp;
		return GetKeyValue(array, it, &tmp, value);
	}

	bool IScriptVM::GetValue(HSCRIPT scope, const char *name, ScriptVariant_t *var) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(scope == INVALID_HSCRIPT) {
			return false;
		}
	#endif

		return GetValue_impl(scope, name, var);
	}

	void IScriptVM::ReleaseValue(HSCRIPT object) noexcept
	{
		ReleaseObject(object);
	}

	void IScriptVM::ReleaseObject(HSCRIPT object) noexcept
	{
		if(!object || object == INVALID_HSCRIPT) {
			return;
		}

		ScriptVariant_t var;
		var.m_type = FIELD_HSCRIPT;
		var.m_flags = SV_NOFLAGS;
		var.m_object = object;
		ReleaseValue(var);
	}

#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
	CSquirrelMetamethodDelegateImpl *IScriptVM::MakeSquirrelMetamethod_Get(HSCRIPT scope, const char *name, ISquirrelMetamethodDelegate *delegate, bool free) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(scope == INVALID_HSCRIPT) {
			return nullptr;
		}
	#endif

		return MakeSquirrelMetamethod_Get_impl(scope, name, delegate, free);
	}
#endif

	void IScriptVM::ArrayAddToTail(HSCRIPT array, ScriptVariant_t &&var) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!array || array == INVALID_HSCRIPT) {
			return;
		}
	#endif

		ArrayAddToTail(array, static_cast<const ScriptVariant_t &>(var));
		var.free();
	}

	bool IScriptVM::SetInstanceUniqeId2(HSCRIPT instance, const char *root) noexcept
	{
		if(!GenerateUniqueKey(root, detail::vscript_uniqueid_buffer)) {
			return false;
		}

		SetInstanceUniqeId(instance, detail::vscript_uniqueid_buffer);
		return true;
	}

#ifndef __VMOD_USING_CUSTOM_VM
	short IScriptVM::fixup_var_field(short field) noexcept
	{
		switch(field) {
			case FIELD_CLASSPTR:
			case FIELD_FUNCTION:
			case FIELD_UINT32:
			case FIELD_UINT64:
		#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			case FIELD_INTEGER64:
		#endif
			case FIELD_SHORT:
			return FIELD_INTEGER;
			default:
			return field;
		}
	}

	ScriptVariant_t &IScriptVM::fixup_var(ScriptVariant_t &var) noexcept
	{
		var.m_type = fixup_var_field(var.m_type);
		return var;
	}
#endif

	ScriptHandleWrapper_t IScriptVM::CompileScript(const char *src, const char *name) noexcept
	{
		HSCRIPT ret{CompileScript_impl(src, name)};

		ScriptHandleWrapper_t tmp;
		tmp.should_free_ = true;
		tmp.object = ret;
		tmp.type = HANDLETYPE_SCRIPT;

		return tmp;
	}

	ScriptHandleWrapper_t IScriptVM::ReferenceScope(gsdk::HSCRIPT obj) noexcept
	{
		HSCRIPT ret{ReferenceScope_impl(obj)};

		ScriptHandleWrapper_t tmp;
		tmp.should_free_ = true;
		tmp.object = ret;
		tmp.type = HANDLETYPE_SCOPE;

		return tmp;
	}

	ScriptFunctionBinding_t::~ScriptFunctionBinding_t() noexcept
	{
		if(m_flags & SF_FREE_SCRIPT_NAME) {
			delete[] const_cast<char *>(m_desc.m_pszScriptName);
		}

		if(m_flags & SF_FREE_NAME) {
			delete[] const_cast<char *>(m_desc.m_pszFunction);
		}

		if(m_flags & SF_FREE_DESCRIPTION) {
			delete[] const_cast<char *>(m_desc.m_pszDescription);
		}
	}

	HSCRIPT ScriptHandleWrapper_t::release() noexcept
	{
		HSCRIPT tmp{object};
		if(!tmp) {
			tmp = INVALID_HSCRIPT;
		}
		should_free_ = false;
		object = INVALID_HSCRIPT;
		type = HANDLETYPE_UNKNOWN;
		return tmp;
	}

	void ScriptHandleWrapper_t::free() noexcept
	{
		if(should_free_ && object && object != INVALID_HSCRIPT) {
			switch(type) {
		#ifndef __clang__
			default:
		#endif
			case HANDLETYPE_UNKNOWN:
			g_pScriptVM->ReleaseObject(object);
			break;
			case HANDLETYPE_FUNCTION:
			g_pScriptVM->ReleaseFunction(object);
			break;
			case HANDLETYPE_TABLE:
			g_pScriptVM->ReleaseTable(object);
			break;
			case HANDLETYPE_ARRAY:
			g_pScriptVM->ReleaseArray(object);
			break;
			case HANDLETYPE_SCOPE:
			g_pScriptVM->ReleaseScope(object);
			break;
			case HANDLETYPE_INSTANCE:
			g_pScriptVM->RemoveInstance(object);
			break;
			case HANDLETYPE_SCRIPT:
			g_pScriptVM->ReleaseScript(object);
			break;
			}
			should_free_ = false;
			object = INVALID_HSCRIPT;
			type = HANDLETYPE_UNKNOWN;
		}
	}

	ScriptHandleWrapper_t::ScriptHandleWrapper_t(ScriptHandleWrapper_t &&other) noexcept
		: object{other.object}, should_free_{other.should_free_}, type{other.type}
	{
		other.should_free_ = false;
		other.object = INVALID_HSCRIPT;
		other.type = HANDLETYPE_UNKNOWN;
	}

	ScriptHandleWrapper_t &ScriptHandleWrapper_t::operator=(ScriptHandleWrapper_t &&other) noexcept
	{
		free();
		should_free_ = other.should_free_;
		object = other.object;
		type = other.type;
		other.should_free_ = false;
		other.object = INVALID_HSCRIPT;
		other.type = HANDLETYPE_UNKNOWN;
		return *this;
	}
}
