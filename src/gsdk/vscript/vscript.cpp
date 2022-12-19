#include "vscript.hpp"

namespace gsdk
{
	ISquirrelMetamethodDelegate::~ISquirrelMetamethodDelegate() {}

	void *IScriptInstanceHelper::GetProxied(void *ptr)
	{ return ptr; }
	void *IScriptInstanceHelper::BindOnRead([[maybe_unused]] HSCRIPT instance, void *ptr, [[maybe_unused]] const char *)
	{ return ptr; }

	void(IScriptVM::*IScriptVM::CreateArray_ptr)(ScriptVariant_t &);
	int(IScriptVM::*IScriptVM::GetArrayCount_ptr)(HSCRIPT) const;
	bool(IScriptVM::*IScriptVM::IsArray_ptr)(HSCRIPT) const;
	bool(IScriptVM::*IScriptVM::IsTable_ptr)(HSCRIPT) const;

	int IScriptVM::GetArrayCount(HSCRIPT array) const noexcept
	{
		if(!array || array == INVALID_HSCRIPT) {
			return 0;
		}

		return (this->*GetArrayCount_ptr)(array);
	}

	HSCRIPT IScriptVM::CreateArray() noexcept
	{
		ScriptVariant_t var;
		(this->*CreateArray_ptr)(var);
		HSCRIPT ret{var.m_hScript};
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
		return ret;
	}

	bool IScriptVM::IsArray(HSCRIPT array) const noexcept
	{
		if(!array || array == INVALID_HSCRIPT) {
			return false;
		}

		return (this->*IsArray_ptr)(array);
	}

	bool IScriptVM::IsTable(HSCRIPT table) const noexcept
	{
		if(!table || table == INVALID_HSCRIPT) {
			return false;
		}

		return (this->*IsTable_ptr)(table);
	}

	HSCRIPT IScriptVM::ReferenceObject(HSCRIPT object) noexcept
	{
		if(!object || object == INVALID_HSCRIPT) {
			return INVALID_HSCRIPT;
		}

		return ReferenceScope(object);
	}

	HSCRIPT IScriptVM::LookupFunction(const char *name, HSCRIPT scope) noexcept
	{
		if(scope == INVALID_HSCRIPT) {
			return INVALID_HSCRIPT;
		}

		HSCRIPT ret{LookupFunction_impl(name, scope)};
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
		return ret;
	}

	HSCRIPT IScriptVM::RegisterInstance(ScriptClassDesc_t *desc, void *value) noexcept
	{
		HSCRIPT ret{RegisterInstance_impl(desc, value)};
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
		return ret;
	}

	bool IScriptVM::SetValue(HSCRIPT scope, const char *name, const ScriptVariant_t &var) noexcept
	{
		if(scope == INVALID_HSCRIPT) {
			return false;
		}

		ScriptVariant_t temp;
		temp.m_type = fixup_var_field(var.m_type);
		temp.m_flags = var.m_flags & ~SV_FREE;
		temp.m_ulonglong = var.m_ulonglong;
		return SetValue_impl(scope, name, temp);
	}

	bool IScriptVM::SetValue(HSCRIPT scope, const char *name, ScriptVariant_t &&var) noexcept
	{
		if(scope == INVALID_HSCRIPT) {
			return false;
		}

		bool ret{SetValue(scope, name, static_cast<const ScriptVariant_t &>(var))};
		var.m_flags &= ~SV_FREE;
		return ret;
	}

	bool IScriptVM::SetValue(HSCRIPT scope, const char *name, HSCRIPT object) noexcept
	{
		if(!object || object == INVALID_HSCRIPT) {
			return false;
		}

		ScriptVariant_t var;
		var.m_type = FIELD_HSCRIPT;
		var.m_flags = SV_NOFLAGS;
		var.m_hScript = object;
		return SetValue(scope, name, static_cast<const ScriptVariant_t &>(var));
	}

	HSCRIPT IScriptVM::CreateTable() noexcept
	{
		ScriptVariant_t var;
		CreateTable_impl(var);
		HSCRIPT ret{var.m_hScript};
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
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
		var.m_hScript = table;
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
		var.m_hScript = array;
		ReleaseValue(var);
	}

	HSCRIPT IScriptVM::CreateScope(const char *script, HSCRIPT parent) noexcept
	{
		if(parent == INVALID_HSCRIPT) {
			return INVALID_HSCRIPT;
		}

		HSCRIPT ret{CreateScope_impl(script, parent)};
		if(!ret) {
			ret = INVALID_HSCRIPT;
		}
		return ret;
	}

	int IScriptVM::GetArrayValue(HSCRIPT array, int it, ScriptVariant_t *value) noexcept
	{
		if(!array || array == INVALID_HSCRIPT) {
			return -1;
		}

		ScriptVariant_t tmp;
		tmp.m_type = FIELD_VOID;
		tmp.m_flags = SV_NOFLAGS;
		tmp.m_ulonglong = 0;
		return GetKeyValue(array, it, &tmp, value);
	}

	bool IScriptVM::GetValue(HSCRIPT scope, const char *name, HSCRIPT *object) noexcept
	{
		if(scope == INVALID_HSCRIPT) {
			return false;
		}

		ScriptVariant_t tmp;
		tmp.m_type = FIELD_VOID;
		tmp.m_flags = SV_NOFLAGS;
		tmp.m_ulonglong = 0;
		bool ret{GetValue(scope, name, &tmp)};
		if(ret && tmp.m_type == FIELD_HSCRIPT && tmp.m_hScript) {
			*object = tmp.m_hScript;
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
		var.m_hScript = object;
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
		var.m_hScript = object;
		ReleaseValue(var);
	}

	CSquirrelMetamethodDelegateImpl *IScriptVM::MakeSquirrelMetamethod_Get(HSCRIPT scope, const char *name, ISquirrelMetamethodDelegate *delegate, bool free) noexcept
	{
		if(scope == INVALID_HSCRIPT) {
			return nullptr;
		}

		if(!scope) {
			scope = GetRootTable();
		}

		return MakeSquirrelMetamethod_Get_impl(scope, name, delegate, free);
	}

	void IScriptVM::ArrayAddToTail(HSCRIPT array, ScriptVariant_t &&var) noexcept
	{
		if(!array || array == INVALID_HSCRIPT) {
			return;
		}

		ArrayAddToTail(array, static_cast<const ScriptVariant_t &>(var));
		var.m_flags &= ~SV_FREE;
	}

	short IScriptVM::fixup_var_field(short field) noexcept
	{
		switch(field) {
			case FIELD_CLASSPTR:
			case FIELD_FUNCTION:
			case FIELD_UINT:
			case FIELD_UINT64:
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
}
