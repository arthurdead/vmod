#pragma once

#include <cstdint>
#include <string_view>
#include "../tier1/interface.hpp"
#include "../mathlib/vector.hpp"
#include "../tier1/utlvector.hpp"
#include "../server/datamap.hpp"

namespace gsdk
{
	class CUtlBuffer;

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
		ScriptFuncDescriptor_t() noexcept = default;
		ScriptFuncDescriptor_t(const ScriptFuncDescriptor_t &) = delete;
		ScriptFuncDescriptor_t &operator=(const ScriptFuncDescriptor_t &) = delete;
		ScriptFuncDescriptor_t(ScriptFuncDescriptor_t &&) = default;
		ScriptFuncDescriptor_t &operator=(ScriptFuncDescriptor_t &&) = default;

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
		FIELD_VARIANT,
	};

	using HSCRIPT = void *;
	inline HSCRIPT INVALID_HSCRIPT{reinterpret_cast<HSCRIPT>(-1)};

	struct ScriptVariant_t
	{
		ScriptVariant_t() noexcept = default;
		ScriptVariant_t(const ScriptVariant_t &) = delete;
		ScriptVariant_t &operator=(const ScriptVariant_t &) = delete;
		ScriptVariant_t(ScriptVariant_t &&) = default;
		ScriptVariant_t &operator=(ScriptVariant_t &&) = default;

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

	using ScriptFunctionBindingStorageType_t = void *;

	using ScriptBindingFunc_t = bool(*)(ScriptFunctionBindingStorageType_t, void *, ScriptVariant_t *, int, ScriptVariant_t *);

	enum ScriptFuncBindingFlags_t : int
	{
		SF_MEMBER_FUNC = 0x01,
	};

	struct ScriptFunctionBinding_t
	{
		ScriptFunctionBinding_t() noexcept = default;
		ScriptFunctionBinding_t(const ScriptFunctionBinding_t &) = delete;
		ScriptFunctionBinding_t &operator=(const ScriptFunctionBinding_t &) = delete;
		ScriptFunctionBinding_t(ScriptFunctionBinding_t &&) = default;
		ScriptFunctionBinding_t &operator=(ScriptFunctionBinding_t &&) = default;

		ScriptFuncDescriptor_t m_desc;
		ScriptBindingFunc_t m_pfnBinding;
		ScriptFunctionBindingStorageType_t m_pFunction;
		unsigned m_flags;
		char unk1[sizeof(int)];
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IScriptInstanceHelper
	{
	public:
		virtual void *GetProxied(void *p) { return p; }
		virtual bool ToString(void *, char *, int) { return false; }
		virtual void *BindOnRead(HSCRIPT, void *, const char *) { return nullptr; }
	};
	#pragma GCC diagnostic pop

	struct ScriptClassDesc_t
	{
		ScriptClassDesc_t() noexcept = default;
		ScriptClassDesc_t(const ScriptClassDesc_t &) = delete;
		ScriptClassDesc_t &operator=(const ScriptClassDesc_t &) = delete;
		ScriptClassDesc_t(ScriptClassDesc_t &&) = default;
		ScriptClassDesc_t &operator=(ScriptClassDesc_t &&) = default;

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
		virtual HSCRIPT CreateScope(const char *, HSCRIPT = nullptr) = 0;
		virtual HSCRIPT ReferenceScope(HSCRIPT) = 0;
		virtual void ReleaseScope(HSCRIPT) = 0;
		virtual HSCRIPT LookupFunction(const char *, HSCRIPT = nullptr) = 0;
		virtual void ReleaseFunction(HSCRIPT) = 0;
		virtual ScriptStatus_t ExecuteFunction(HSCRIPT, const ScriptVariant_t *, int, ScriptVariant_t *, HSCRIPT, bool) = 0;
		virtual void RegisterFunction(ScriptFunctionBinding_t *) = 0;
		virtual bool RegisterClass(ScriptClassDesc_t *) = 0;
		virtual HSCRIPT RegisterInstance(ScriptClassDesc_t *, void *) = 0;
		virtual void SetInstanceUniqeId(HSCRIPT, const char *) = 0;
		virtual void RemoveInstance(HSCRIPT) = 0;
		virtual void *GetInstanceValue(HSCRIPT, ScriptClassDesc_t * = nullptr) = 0;
		virtual bool GenerateUniqueKey(const char *, char *, int) = 0;
		virtual bool ValueExists(HSCRIPT, const char *) = 0;
		virtual bool SetValue(HSCRIPT, const char *, const char *) = 0;
		virtual bool SetValue(HSCRIPT, const char *, const ScriptVariant_t &) = 0;
		virtual void CreateTable(ScriptVariant_t &) = 0;
		virtual int GetNumTableEntries(HSCRIPT) = 0;
		virtual int GetKeyValue(HSCRIPT, int, ScriptVariant_t *, ScriptVariant_t *) = 0;
		virtual bool GetValue(HSCRIPT, const char *, ScriptVariant_t *) = 0;
		virtual void ReleaseValue(ScriptVariant_t &) = 0;
		virtual bool ClearValue(HSCRIPT, const char *) = 0;
		virtual void WriteState(CUtlBuffer *) = 0;
		virtual void ReadState(CUtlBuffer *) = 0;
		virtual void RemoveOrphanInstances() = 0;
		virtual void DumpState() = 0;
		virtual void SetOutputCallback(ScriptOutputFunc_t) = 0;
		virtual void SetErrorCallback(ScriptErrorFunc_t) = 0;
		virtual bool RaiseException(const char *) = 0;
	};
	#pragma GCC diagnostic pop
}
