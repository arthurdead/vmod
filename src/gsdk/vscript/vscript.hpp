#pragma once

#include <cstdint>
#include <string_view>
#include "../tier1/interface.hpp"
#include "../mathlib/vector.hpp"
#include "../tier1/utlvector.hpp"
#include "../server/datamap.hpp"
#include <cstring>
#include <squirrel.h>

namespace gsdk
{
	class CUtlBuffer;
	class CUtlString;

	class IScriptVM;

	enum ScriptLanguage_t : int
	{
		SL_NONE,
		SL_GAMEMONKEY,
		SL_SQUIRREL,
		SL_LUA,
		SL_PYTHON,
		SL_DEFAULT = SL_SQUIRREL
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IScriptManager : public IAppSystem
	{
	public:
		static constexpr std::string_view interface_name{"VScriptManager010"};

		virtual IScriptVM *CreateVM(ScriptLanguage_t = SL_DEFAULT) = 0;
		virtual void DestroyVM(IScriptVM *) = 0;
	};
	#pragma GCC diagnostic pop

	enum ScriptStatus_t : int
	{
		SCRIPT_ERROR = -1,
		SCRIPT_DONE,
		SCRIPT_RUNNING,
	};

	using ScriptDataType_t = int;

	struct ScriptFuncDescriptor_t
	{
	public:
		inline ScriptFuncDescriptor_t() noexcept
		{
		}

		ScriptFuncDescriptor_t(const ScriptFuncDescriptor_t &) = delete;
		ScriptFuncDescriptor_t &operator=(const ScriptFuncDescriptor_t &) = delete;

		inline ScriptFuncDescriptor_t(ScriptFuncDescriptor_t &&other) noexcept
		{ operator=(std::move(other)); }

		inline ScriptFuncDescriptor_t &operator=(ScriptFuncDescriptor_t &&other) noexcept
		{
			m_pszScriptName = other.m_pszScriptName;
			other.m_pszScriptName = nullptr;
			m_pszFunction = other.m_pszFunction;
			other.m_pszFunction = nullptr;
			m_pszDescription = other.m_pszDescription;
			other.m_pszDescription = nullptr;
			m_ReturnType = other.m_ReturnType;
			m_Parameters = std::move(other.m_Parameters);
			return *this;
		}

		const char *m_pszScriptName;
		const char *m_pszFunction;
		const char *m_pszDescription;
		ScriptDataType_t m_ReturnType;
		CUtlVector<ScriptDataType_t> m_Parameters;
	};

	enum SVFlags_t : int
	{
		SV_FREE = 0x01,
	};

	enum ExtendedFieldType : int
	{
		FIELD_TYPEUNKNOWN = FIELD_TYPECOUNT,
		FIELD_CSTRING,
		FIELD_HSCRIPT,
		FIELD_VARIANT
	};

	using HSCRIPT = HSQOBJECT *;
	inline HSCRIPT INVALID_HSCRIPT{reinterpret_cast<HSCRIPT>(-1)};

	class CVariantDefaultAllocator;

	template <typename T>
	class CVariantBase
	{
		friend class IScriptVM;

	protected:
		inline CVariantBase() noexcept
		{
		}

		inline ~CVariantBase() noexcept
		{
		}

	public:
		CVariantBase(const CVariantBase &) = delete;
		CVariantBase &operator=(const CVariantBase &) = delete;

		inline CVariantBase(CVariantBase &&other) noexcept
		{ operator=(std::move(other)); }

		inline CVariantBase &operator=(CVariantBase &&other) noexcept
		{
			m_hScript = other.m_hScript;
			other.m_hScript = nullptr;
			std::memcpy(unk1, other.unk1, sizeof(unk1));
			std::memset(other.unk1, 0, sizeof(unk1));
			m_type = other.m_type;
			m_flags = other.m_flags;
			other.m_flags = 0;
			return *this;
		}

		union
		{
			int m_int;
			float m_float;
			const char *m_pszString;
			const Vector *m_pVector;
			char m_char;
			bool m_bool;
			HSCRIPT m_hScript;
		};

		char unk1[sizeof(int)];
		short m_type;
		short m_flags;
	};

	using ScriptVariant_t = CVariantBase<CVariantDefaultAllocator>;

	using ScriptFunctionBindingStorageType_t = void *;

	using ScriptBindingFunc_t = bool(*)(ScriptFunctionBindingStorageType_t, int, void *, const ScriptVariant_t *, int, ScriptVariant_t *);

	enum ScriptFuncBindingFlags_t : int
	{
		SF_MEMBER_FUNC = 0x01,
	};

	struct ScriptFunctionBinding_t
	{
	protected:
		inline ScriptFunctionBinding_t() noexcept
		{
		}

	public:
		ScriptFunctionBinding_t(const ScriptFunctionBinding_t &) = delete;
		ScriptFunctionBinding_t &operator=(const ScriptFunctionBinding_t &) = delete;

		inline ScriptFunctionBinding_t(ScriptFunctionBinding_t &&other) noexcept
		{ operator=(std::move(other)); }

		inline ScriptFunctionBinding_t &operator=(ScriptFunctionBinding_t &&other) noexcept
		{
			m_desc = std::move(other.m_desc);
			m_pfnBinding = other.m_pfnBinding;
			other.m_pfnBinding = nullptr;
			m_pFunction = other.m_pFunction;
			other.m_pFunction = nullptr;
			m_adjustor = other.m_adjustor;
			other.m_adjustor = 0;
			m_flags = other.m_flags;
			return *this;
		}

		ScriptFuncDescriptor_t m_desc;
		ScriptBindingFunc_t m_pfnBinding;
		ScriptFunctionBindingStorageType_t m_pFunction;
		int m_adjustor;
		unsigned m_flags;
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	#pragma GCC diagnostic ignored "-Wweak-vtables"
	class IScriptInstanceHelper
	{
	public:
		virtual void *GetProxied(void *p) = 0;
		virtual bool ToString(void *, char *, int) = 0;
		virtual void *BindOnRead(HSCRIPT, void *, const char *) = 0;
	};
	#pragma GCC diagnostic pop

	struct ScriptClassDesc_t
	{
	protected:
		inline ScriptClassDesc_t() noexcept
		{
		}

	public:
		ScriptClassDesc_t(const ScriptClassDesc_t &) = delete;
		ScriptClassDesc_t &operator=(const ScriptClassDesc_t &) = delete;
		ScriptClassDesc_t(ScriptClassDesc_t &&) = delete;
		ScriptClassDesc_t &operator=(ScriptClassDesc_t &&) = delete;

		const char *m_pszScriptName;
		const char *m_pszClassname;
		const char *m_pszDescription;
		ScriptClassDesc_t *m_pBaseDesc;
		CUtlVector<ScriptFunctionBinding_t> m_FunctionBindings;
		void *(*m_pfnConstruct)();
		void (*m_pfnDestruct)(void *);
		IScriptInstanceHelper *pHelper;
		ScriptClassDesc_t *m_pNextDesc;
	};

	enum ScriptErrorLevel_t : int
	{
		SCRIPT_LEVEL_WARNING,
		SCRIPT_LEVEL_ERROR,
	};

	using ScriptOutputFunc_t = void(*)(const char *);
	using ScriptErrorFunc_t = bool(*)(ScriptErrorLevel_t, const char *);

	class ISquirrelMetamethodDelegate
	{
	public:
		virtual ~ISquirrelMetamethodDelegate();

		//TODO!!! change to CUtlConstString
		virtual bool Get(const CUtlString &, ScriptVariant_t &) = 0;
	};

	class CSquirrelMetamethodDelegateImpl;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IScriptVM
	{
	public:
		virtual bool Init() = 0;
		virtual void Shutdown() = 0;
		virtual bool ConnectDebugger() = 0;
		virtual void DisconnectDebugger() = 0;
		virtual ScriptLanguage_t GetLanguage() = 0;
		virtual const char *GetLanguageName() = 0;
		virtual void AddSearchPath(const char *) = 0;
		virtual bool Frame(float) = 0;
		virtual ScriptStatus_t Run(const char *, bool = true) = 0;
	 	virtual HSCRIPT CompileScript(const char *, const char * = nullptr) = 0;
		virtual void ReleaseScript(HSCRIPT) = 0;
		virtual ScriptStatus_t Run(HSCRIPT, HSCRIPT = nullptr, bool = true) = 0;
		virtual ScriptStatus_t Run(HSCRIPT, bool) = 0;
	private:
		virtual HSCRIPT CreateScope_impl(const char *, HSCRIPT = nullptr) = 0;
	public:
		inline HSCRIPT CreateScope(const char *script, HSCRIPT parent = nullptr) noexcept
		{
			HSCRIPT ret{CreateScope_impl(script, parent)};
			if(!ret) {
				ret = INVALID_HSCRIPT;
			}
			return ret;
		}
		virtual HSCRIPT ReferenceScope(HSCRIPT) = 0;
		inline HSCRIPT ReferenceObject(HSCRIPT object)
		{
			return ReferenceScope(object);
		}
		virtual void ReleaseScope(HSCRIPT) = 0;
	private:
		virtual HSCRIPT LookupFunction_impl(const char *, HSCRIPT = nullptr) = 0;
	public:
		inline HSCRIPT LookupFunction(const char *name, HSCRIPT scope = nullptr) noexcept
		{
			HSCRIPT ret{LookupFunction_impl(name, scope)};
			if(!ret) {
				ret = INVALID_HSCRIPT;
			}
			return ret;
		}
		virtual void ReleaseFunction(HSCRIPT) = 0;
		virtual ScriptStatus_t ExecuteFunction(HSCRIPT, const ScriptVariant_t *, int, ScriptVariant_t *, HSCRIPT, bool) = 0;
		virtual void RegisterFunction(ScriptFunctionBinding_t *) = 0;
		virtual bool RegisterClass(ScriptClassDesc_t *) = 0;
	private:
		virtual HSCRIPT RegisterInstance_impl(ScriptClassDesc_t *, void *) = 0;
	public:
		inline HSCRIPT RegisterInstance(ScriptClassDesc_t *desc, void *value) noexcept
		{
			HSCRIPT ret{RegisterInstance_impl(desc, value)};
			if(!ret) {
				ret = INVALID_HSCRIPT;
			}
			return ret;
		}
		virtual void SetInstanceUniqeId(HSCRIPT, const char *) = 0;
		virtual void RemoveInstance(HSCRIPT) = 0;
		virtual void *GetInstanceValue(HSCRIPT, ScriptClassDesc_t * = nullptr) = 0;
		virtual bool GenerateUniqueKey(const char *, char *, int) = 0;
		virtual bool ValueExists(HSCRIPT, const char *) = 0;
		virtual bool SetValue(HSCRIPT, const char *, const char *) = 0;
		virtual bool SetValue(HSCRIPT, const char *, const ScriptVariant_t &) = 0;
		inline bool SetValue(HSCRIPT scope, const char *name, HSCRIPT object) noexcept
		{
			ScriptVariant_t var;
			var.m_type = FIELD_HSCRIPT;
			var.m_flags = 0;
			var.m_hScript = object;
			return SetValue(scope, name, var);
		}
	private:
		virtual void CreateTable_impl(ScriptVariant_t &) = 0;
	public:
		HSCRIPT CreateArray() noexcept;
		inline HSCRIPT CreateTable() noexcept
		{
			ScriptVariant_t var;
			CreateTable_impl(var);
			return var.m_hScript;
		}
		inline void ReleaseTable(HSCRIPT table) noexcept
		{
			ScriptVariant_t var;
			var.m_type = FIELD_HSCRIPT;
			var.m_flags = 0;
			var.m_hScript = table;
			ReleaseValue(var);
		}
		inline void ReleaseArray(HSCRIPT array) noexcept
		{
			ScriptVariant_t var;
			var.m_type = FIELD_HSCRIPT;
			var.m_flags = 0;
			var.m_hScript = array;
			ReleaseValue(var);
		}
		virtual int GetNumTableEntries(HSCRIPT) = 0;
		int GetArrayCount(HSCRIPT);
		virtual int GetKeyValue(HSCRIPT, int, ScriptVariant_t *, ScriptVariant_t *) = 0;
		inline void GetArrayValue(HSCRIPT array, int i, ScriptVariant_t *value)
		{
			ScriptVariant_t tmp;
			tmp.m_type = FIELD_VOID;
			tmp.m_flags = 0;
			tmp.m_hScript = nullptr;
			GetKeyValue(array, i, &tmp, value);
		}
		virtual bool GetValue(HSCRIPT, const char *, ScriptVariant_t *) = 0;
		virtual void ReleaseValue(ScriptVariant_t &) = 0;
		inline void ReleaseValue(HSCRIPT object) noexcept
		{
			ScriptVariant_t var;
			var.m_type = FIELD_HSCRIPT;
			var.m_flags = 0;
			var.m_hScript = object;
			ReleaseValue(var);
		}
		inline void ReleaseObject(HSCRIPT object) noexcept
		{
			ScriptVariant_t var;
			var.m_type = FIELD_HSCRIPT;
			var.m_flags = 0;
			var.m_hScript = object;
			ReleaseValue(var);
		}
		virtual bool ClearValue(HSCRIPT, const char *) = 0;
		virtual void WriteState(CUtlBuffer *) = 0;
		virtual void ReadState(CUtlBuffer *) = 0;
		virtual void RemoveOrphanInstances() = 0;
		virtual void DumpState() = 0;
		virtual void SetOutputCallback(ScriptOutputFunc_t) = 0;
		virtual void SetErrorCallback(ScriptErrorFunc_t) = 0;
		virtual bool RaiseException(const char *) = 0;
	private:
		virtual CSquirrelMetamethodDelegateImpl *MakeSquirrelMetamethod_Get_impl(HSCRIPT &, const char *, ISquirrelMetamethodDelegate *, bool) = 0;
	public:
		inline CSquirrelMetamethodDelegateImpl *MakeSquirrelMetamethod_Get(HSCRIPT scope, const char *name, ISquirrelMetamethodDelegate *delegate, bool free)
		{
			if(!scope) {
				scope = GetRootTable();
			}

			return MakeSquirrelMetamethod_Get_impl(scope, name, delegate, free);
		}
		virtual void DestroySquirrelMetamethod_Get(CSquirrelMetamethodDelegateImpl *) = 0;
		virtual int GetKeyValue2(HSCRIPT, int, ScriptVariant_t *, ScriptVariant_t *) = 0;
		virtual HSQUIRRELVM GetInternalVM() = 0;
		virtual bool GetScalarValue(HSCRIPT, ScriptVariant_t *) = 0;
		virtual void ArrayAddToTail(HSCRIPT, const ScriptVariant_t &) = 0;
		virtual HSCRIPT GetRootTable() = 0;
		virtual HSCRIPT CopyHandle(HSCRIPT) = 0;
		virtual HSCRIPT GetIdentity(HSCRIPT) = 0;
		virtual void CollectGarbage(const char *, bool) = 0;
	};
	#pragma GCC diagnostic pop
}
