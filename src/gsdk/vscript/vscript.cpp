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
	HSCRIPT IScriptVM::CreateArray() noexcept
	{
		ScriptVariant_t var;
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		(this->*CreateArray_ptr)(var);
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		CreateArray_impl(var);
	#else
		#error
	#endif
		HSCRIPT ret{var.m_object};
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
	#endif
		return ret;
	}
#endif

#if GSDK_ENGINE == GSDK_ENGINE_TF2
	void(IScriptVM::*IScriptVM::CreateArray_ptr)(ScriptVariant_t &) {nullptr};
	int(IScriptVM::*IScriptVM::GetArrayCount_ptr)(HSCRIPT) const {nullptr};
	bool(IScriptVM::*IScriptVM::IsArray_ptr)(HSCRIPT) const {nullptr};
	bool(IScriptVM::*IScriptVM::IsTable_ptr)(HSCRIPT) const {nullptr};

	int IScriptVM::GetArrayCount(HSCRIPT array) const noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!array || array == INVALID_HSCRIPT) {
			return 0;
		}
	#endif

		return (this->*GetArrayCount_ptr)(array);
	}

	bool IScriptVM::IsArray(HSCRIPT array) const noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!array || array == INVALID_HSCRIPT) {
			return false;
		}
	#endif

		return (this->*IsArray_ptr)(array);
	}

	bool IScriptVM::IsTable(HSCRIPT table) const noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!table || table == INVALID_HSCRIPT) {
			return false;
		}
	#endif

		return (this->*IsTable_ptr)(table);
	}
#endif

#if GSDK_ENGINE != GSDK_ENGINE_L4D2 && GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	bool IScriptVM::IsArray(HSCRIPT array) const noexcept
	{
		if(!array || array == INVALID_HSCRIPT) {
			return false;
		}

		return (array->_type == OT_ARRAY);
	}

	bool IScriptVM::IsTable(HSCRIPT table) const noexcept
	{
		if(!table || table == INVALID_HSCRIPT) {
			return false;
		}

		return (table->_type == OT_TABLE);
	}

	void IScriptVM::ArrayAddToTail(HSCRIPT array, const ScriptVariant_t &var)
	{
		if(!array || array == INVALID_HSCRIPT) {
			return;
		}

		if(array->_type != OT_ARRAY) {
			return;
		}

		//array->_unVal.pArray->Append();
	}

	HSCRIPT IScriptVM::CreateArray() noexcept
	{
		return INVALID_HSCRIPT;
	}

	int IScriptVM::GetArrayCount(HSCRIPT array) const noexcept
	{
		if(!array || array == INVALID_HSCRIPT) {
			return 0;
		}

		if(array->_type != OT_ARRAY) {
			return false;
		}

		return array->_unVal.pArray->Size();
	}

	bool IScriptVM::GetScalarValue(HSCRIPT object, ScriptVariant_t *var) noexcept
	{
		std::memset(var->m_data, 0, sizeof(gsdk::ScriptVariant_t::m_data));

		switch(object->_type) {
			case OT_NULL: {
				var->m_type = FIELD_VOID;
				var->m_object = INVALID_HSCRIPT;
				return true;
			}
			case OT_INTEGER: {
				var->m_type = FIELD_INTEGER;
				var->m_int = object->_unVal.nInteger;
				return true;
			}
			case OT_FLOAT: {
				var->m_type = FIELD_FLOAT;
				var->m_float = object->_unVal.fFloat;
				return true;
			}
			case OT_BOOL: {
				var->m_type = FIELD_BOOLEAN;
				var->m_bool = (object->_unVal.nInteger > 0 ? true : false);
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
#endif

#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
	HSCRIPT IScriptVM::ReferenceObject(HSCRIPT object) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!object || object == INVALID_HSCRIPT) {
			return INVALID_HSCRIPT;
		}
	#endif

		return ReferenceScope(object);
	}
#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	HSCRIPT IScriptVM::ReferenceObject(HSCRIPT object) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!object || object == INVALID_HSCRIPT) {
			return INVALID_HSCRIPT;
		}
	#endif

		return object;
	}
#endif

	HSCRIPT IScriptVM::LookupFunction(const char *name, HSCRIPT scope) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(scope == INVALID_HSCRIPT) {
			return INVALID_HSCRIPT;
		}
	#endif

		HSCRIPT ret{LookupFunction_impl(name, scope)};
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
	#endif
		return ret;
	}

	HSCRIPT IScriptVM::RegisterInstance(ScriptClassDesc_t *desc, void *value) noexcept
	{
		HSCRIPT ret{RegisterInstance_impl(desc, value)};
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
	#endif
		return ret;
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

		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(!scope) {
			scope = GetRootTable();
		}
		#endif
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

		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(!scope) {
			scope = GetRootTable();
		}
		#endif

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

		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(!scope) {
			scope = GetRootTable();
		}
		#endif
	#endif

		bool ret{SetValue(scope, name, static_cast<const ScriptVariant_t &>(var))};
		var.m_flags &= ~SV_FREE;
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

		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(!scope) {
			scope = GetRootTable();
		}
		#endif
	#endif

		ScriptVariant_t var;
		var.m_type = FIELD_HSCRIPT;
		var.m_flags = SV_NOFLAGS;
		var.m_object = object;
		return SetValue(scope, name, static_cast<const ScriptVariant_t &>(var));
	}

	HSCRIPT IScriptVM::CreateTable() noexcept
	{
		ScriptVariant_t var;
		CreateTable_impl(var);
		HSCRIPT ret{var.m_object};
	#ifndef __VMOD_USING_CUSTOM_VM
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
	#endif
		return ret;
	}

	void IScriptVM::ReleaseTable(HSCRIPT table) noexcept
	{
		if(!table || table == INVALID_HSCRIPT) {
			return;
		}

		ScriptVariant_t var;
		var.m_type = FIELD_HSCRIPT;
		var.m_flags = SV_NOFLAGS;
		var.m_object = table;
		ReleaseValue(var);
	}

	void IScriptVM::ReleaseArray(HSCRIPT array) noexcept
	{
		if(!array || array == INVALID_HSCRIPT) {
			return;
		}

		ScriptVariant_t var;
		var.m_type = FIELD_HSCRIPT;
		var.m_flags = SV_NOFLAGS;
		var.m_object = array;
		ReleaseValue(var);
	}

	HSCRIPT IScriptVM::CreateScope(const char *script, HSCRIPT parent) noexcept
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
		return ret;
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

		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(!scope) {
			scope = GetRootTable();
		}
		#endif
	#endif

		return GetValue_impl(scope, name, var);
	}

	bool IScriptVM::GetValue(HSCRIPT scope, const char *name, HSCRIPT *object) noexcept
	{
	#ifndef __VMOD_USING_CUSTOM_VM
		if(scope == INVALID_HSCRIPT) {
			return false;
		}

		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(!scope) {
			scope = GetRootTable();
		}
		#endif
	#endif

		ScriptVariant_t tmp;
		bool ret{GetValue(scope, name, &tmp)};
		if(ret && tmp.m_type == FIELD_HSCRIPT && (tmp.m_object && tmp.m_object != INVALID_HSCRIPT)) {
			*object = tmp.m_object;
		} else {
			*object = INVALID_HSCRIPT;
		}

		return ret;
	}

	void IScriptVM::ReleaseValue(HSCRIPT object) noexcept
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

		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		if(!scope) {
			scope = GetRootTable();
		}
		#endif
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
		var.m_flags &= ~SV_FREE;
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

	ScriptFunctionBinding_t::~ScriptFunctionBinding_t() noexcept
	{
		if(m_flags & SF_FREE_SCRIPT_NAME) {
			std::free(const_cast<char *>(m_desc.m_pszScriptName));
		}

		if(m_flags & SF_FREE_NAME) {
			std::free(const_cast<char *>(m_desc.m_pszFunction));
		}

		if(m_flags & SF_FREE_DESCRIPTION) {
			std::free(const_cast<char *>(m_desc.m_pszDescription));
		}
	}
}
