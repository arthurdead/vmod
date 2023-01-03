#pragma once

#if !defined __VMOD_VSCRIPT_HEADER_INCLUDED && !defined __VMOD_COMPILING_GSDK
	#error "include vscript/vscript.hpp instead"
#endif

#include <cstdarg>
#include <cstdint>
#include <string_view>
#include "../tier1/interface.hpp"
#include "../mathlib/vector.hpp"
#include "../tier1/utlvector.hpp"
#include "../server/datamap.hpp"
#include "../string_t.hpp"
#include <cstring>

#include "../../hacking.hpp"

#include <squirrel.h>

#include <cassert>
#include <type_traits>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsuggest-override"
#pragma clang diagnostic ignored "-Wextra-semi-stmt"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wold-style-cast"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wsuggest-destructor-override"
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wshadow-field"
#pragma clang diagnostic ignored "-Wdeprecated-copy-with-user-provided-copy"
#pragma clang diagnostic ignored "-Wdocumentation"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsuggest-override"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#pragma GCC diagnostic ignored "-Wredundant-tags"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif
#include <sqvm.h>
#include <sqobject.h>
#include <sqstate.h>
#include <squserdata.h>
#include <sqtable.h>
#include <sqarray.h>
#include <sqclass.h>
#undef type
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

namespace gsdk
{
	class CUtlBuffer;
	class CUtlString;
	class CUtlStringToken;
	class CCommand;
	class CCommandContext;

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

	enum SVFlags_t : short
	{
		SV_NOFLAGS = 0,
		SV_FREE = 0x01,
	};

	enum ExtendedFieldType : int;

	using HSCRIPT = HSQOBJECT *;
	inline HSCRIPT INVALID_HSCRIPT{reinterpret_cast<HSCRIPT>(-1)};

	class CVariantDefaultAllocator;

	template <typename T>
	class CVariantBase
	{
		friend class IScriptVM;

	public:
		inline CVariantBase() noexcept
		{
			std::memset(m_data, 0, sizeof(m_data));
			m_object = INVALID_HSCRIPT;
		}

		void free() noexcept;

		inline ~CVariantBase() noexcept
		{ free(); }

		CVariantBase(const CVariantBase &other) noexcept
		{ operator=(other); }

		inline CVariantBase &operator=(const CVariantBase &other) noexcept
		{
			free();
			m_type = other.m_type;
			std::memcpy(m_data, other.m_data, sizeof(m_data));
			m_flags = other.m_flags & ~SV_FREE;
			return *this;
		}

		inline CVariantBase(CVariantBase &&other) noexcept
		{ operator=(std::move(other)); }

		inline CVariantBase &operator=(CVariantBase &&other) noexcept
		{
			free();
			m_type = other.m_type;
			other.m_type = FIELD_TYPEUNKNOWN;
			std::memmove(m_data, other.m_data, sizeof(m_data));
			std::memset(other.m_data, 0, sizeof(m_data));
			m_flags = other.m_flags;
			other.m_flags = SV_NOFLAGS;
			return *this;
		}

		union
		{
			int m_int;
			unsigned int m_uint;
			short m_short;
			unsigned short m_ushort;
			char m_char;
			signed char m_schar;
			unsigned char m_uchar;
			bool m_bool;
			long m_long;
			unsigned long m_ulong;
			long long m_longlong;
			unsigned long long m_ulonglong;
			float m_float;
			double m_double;
			//long double m_longdouble;
			vscript::string_t m_tstr;
			char *m_cstr;
			const char *m_ccstr;
			CUtlStringToken *m_utlstringtoken;
			Vector *m_vector;
			Quaternion *m_quaternion;
			Vector2D *m_vector2d;
			QAngle *m_qangle;
			void *m_ehandle;
			HSCRIPT m_object;
			void *m_ptr;
			unsigned char m_data[sizeof(unsigned long long)];
		};

		short m_type{FIELD_TYPEUNKNOWN};
		short m_flags{SV_NOFLAGS};
	};

	using ScriptVariant_t = CVariantBase<CVariantDefaultAllocator>;

	static_assert(sizeof(ScriptVariant_t) == (sizeof(unsigned long long) + (sizeof(short) * 2)));

	struct ScriptFunctionBindingStorageType_t
	{
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		union {
			vmod::generic_func_t func;
			vmod::generic_mfp_t mfp;
		};
		char unk1[sizeof(unsigned long long)];
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		union {
			vmod::generic_func_t func;
			vmod::generic_plain_mfp_t plain;
		};
	#else
		#error
	#endif
	};

	static_assert(std::is_trivial_v<ScriptFunctionBindingStorageType_t>);
#if GSDK_ENGINE == GSDK_ENGINE_TF2
	static_assert(sizeof(ScriptFunctionBindingStorageType_t) == (sizeof(vmod::generic_mfp_t) + sizeof(unsigned long long)));
#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
	static_assert(sizeof(ScriptFunctionBindingStorageType_t) == sizeof(vmod::generic_plain_mfp_t));
#else
	#error
#endif

	using ScriptBindingFunc_t = bool(*)(ScriptFunctionBindingStorageType_t, void *, const ScriptVariant_t *, int, ScriptVariant_t *);

	struct alignas(ScriptFunctionBindingStorageType_t) CScriptFunctionBindingStorageType : public ScriptFunctionBindingStorageType_t
	{
		inline CScriptFunctionBindingStorageType() noexcept
		{
		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			mfp = nullptr;
			std::memset(unk1, 0, sizeof(unk1));
		#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
			plain = nullptr;
		#else
			#error
		#endif
		}

		CScriptFunctionBindingStorageType(CScriptFunctionBindingStorageType &&other) noexcept
		{ operator=(std::move(other)); }

		inline CScriptFunctionBindingStorageType &operator=(CScriptFunctionBindingStorageType &&other) noexcept
		{
		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			mfp = other.mfp;
			std::memmove(unk1, other.unk1, sizeof(unk1));
			std::memset(other.unk1, 0, sizeof(unk1));
		#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
			plain = other.plain;
		#else
			#error
		#endif
			return *this;
		}

	private:
		CScriptFunctionBindingStorageType(const CScriptFunctionBindingStorageType &) noexcept = delete;
		CScriptFunctionBindingStorageType &operator=(const CScriptFunctionBindingStorageType &) noexcept = delete;
	};

	static_assert(sizeof(CScriptFunctionBindingStorageType) == sizeof(ScriptFunctionBindingStorageType_t));
	static_assert(alignof(CScriptFunctionBindingStorageType) == alignof(ScriptFunctionBindingStorageType_t));

	enum ScriptFuncBindingFlags_t : unsigned int
	{
		SF_NOFLAGS =        0,
		SF_MEMBER_FUNC = 0x01,

		SF_LAST_FLAG =   SF_MEMBER_FUNC,
		SF_NUM_FLAGS =      1
	};

	struct ScriptFuncDescriptor_t
	{
	public:
		ScriptFuncDescriptor_t() noexcept = default;

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
			other.m_ReturnType = FIELD_TYPEUNKNOWN;
			m_Parameters = std::move(other.m_Parameters);
			return *this;
		}

		const char *m_pszScriptName{nullptr};
		const char *m_pszFunction{nullptr};
		const char *m_pszDescription{nullptr};
		ScriptDataType_t m_ReturnType{FIELD_TYPEUNKNOWN};
		CUtlVector<ScriptDataType_t> m_Parameters;

	private:
		ScriptFuncDescriptor_t(const ScriptFuncDescriptor_t &) = delete;
		ScriptFuncDescriptor_t &operator=(const ScriptFuncDescriptor_t &) = delete;
	};

	struct ScriptFunctionBinding_t
	{
	public:
		ScriptFunctionBinding_t() noexcept = default;

		inline ScriptFunctionBinding_t(ScriptFunctionBinding_t &&other) noexcept
		{ operator=(std::move(other)); }

		inline ScriptFunctionBinding_t &operator=(ScriptFunctionBinding_t &&other) noexcept
		{
			m_desc = std::move(other.m_desc);
			m_pfnBinding = other.m_pfnBinding;
			other.m_pfnBinding = nullptr;
			m_pFunction = std::move(other.m_pFunction);
			m_flags = other.m_flags;
			other.m_flags = SF_NOFLAGS;
			return *this;
		}

		ScriptFuncDescriptor_t m_desc;
		ScriptBindingFunc_t m_pfnBinding{nullptr};
		CScriptFunctionBindingStorageType m_pFunction;
		unsigned m_flags{SF_NOFLAGS};

	private:
		ScriptFunctionBinding_t(const ScriptFunctionBinding_t &) = delete;
		ScriptFunctionBinding_t &operator=(const ScriptFunctionBinding_t &) = delete;
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IScriptInstanceHelper
	{
	public:
		virtual void *GetProxied(void *);
		virtual bool ToString(void *, char *, int) = 0;
		virtual void *BindOnRead(HSCRIPT, void *, const char *);
	};
	#pragma GCC diagnostic pop

	struct ScriptClassDesc_t
	{
	public:
		ScriptClassDesc_t() noexcept = default;

		const char *m_pszScriptName{nullptr};
		const char *m_pszClassname{nullptr};
		const char *m_pszDescription{nullptr};
		ScriptClassDesc_t *m_pBaseDesc{nullptr};
		CUtlVector<ScriptFunctionBinding_t> m_FunctionBindings;
		void *(*m_pfnConstruct)() {nullptr};
		void (*m_pfnDestruct)(void *) {nullptr};
		IScriptInstanceHelper *pHelper{nullptr};
		ScriptClassDesc_t *m_pNextDesc{nullptr};

	private:
		ScriptClassDesc_t(const ScriptClassDesc_t &) = delete;
		ScriptClassDesc_t &operator=(const ScriptClassDesc_t &) = delete;
		ScriptClassDesc_t(ScriptClassDesc_t &&) = delete;
		ScriptClassDesc_t &operator=(ScriptClassDesc_t &&) = delete;
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

	struct InstanceContext_t
	{
		void *pInstance;
		ScriptClassDesc_t *pClassDesc;
		SQObjectPtr name;
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IScriptVM
	{
	public:
		static short fixup_var_field(short field) noexcept;
		static ScriptVariant_t &fixup_var(ScriptVariant_t &var) noexcept;

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		static void(IScriptVM::*CreateArray_ptr)(ScriptVariant_t &);
		static int(IScriptVM::*GetArrayCount_ptr)(HSCRIPT) const;
		static bool(IScriptVM::*IsArray_ptr)(HSCRIPT) const;
		static bool(IScriptVM::*IsTable_ptr)(HSCRIPT) const;
	#endif

		static constexpr std::size_t unique_id_max{4095 + 6 + 64};

		virtual bool Init() = 0;
		virtual void Shutdown() = 0;
		virtual bool ConnectDebugger() = 0;
		virtual void DisconnectDebugger() = 0;
		virtual ScriptLanguage_t GetLanguage() = 0;
		virtual const char *GetLanguageName() = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual HSQUIRRELVM GetInternalVM() = 0;
	#endif
		virtual void AddSearchPath(const char *) = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual bool ForwardConsoleCommand(const CCommandContext &, const CCommand &) = 0;
	#endif
		virtual bool Frame(float) = 0;
		virtual ScriptStatus_t Run(const char *, bool = true) = 0;
	 	virtual HSCRIPT CompileScript(const char *, const char * = nullptr) = 0;
		virtual void ReleaseScript(HSCRIPT) = 0;
		virtual ScriptStatus_t Run(HSCRIPT, HSCRIPT = nullptr, bool = true) = 0;
		virtual ScriptStatus_t Run(HSCRIPT, bool) = 0;
	private:
		virtual HSCRIPT CreateScope_impl(const char *, HSCRIPT = nullptr) = 0;
	public:
		HSCRIPT CreateScope(const char *script, HSCRIPT parent = nullptr) noexcept;
		virtual HSCRIPT ReferenceScope(HSCRIPT) = 0;
		HSCRIPT ReferenceObject(HSCRIPT object) noexcept;
		virtual void ReleaseScope(HSCRIPT) = 0;
	private:
		virtual HSCRIPT LookupFunction_impl(const char *, HSCRIPT = nullptr) = 0;
	public:
		HSCRIPT LookupFunction(const char *name, HSCRIPT scope = nullptr) noexcept;
		virtual void ReleaseFunction(HSCRIPT) = 0;
	private:
		virtual ScriptStatus_t ExecuteFunction_impl(HSCRIPT, const ScriptVariant_t *, int, ScriptVariant_t *, HSCRIPT, bool) = 0;
	public:
		ScriptStatus_t ExecuteFunction(HSCRIPT func, const ScriptVariant_t *args, int num_args, ScriptVariant_t *ret, HSCRIPT scope, bool wait) noexcept;
		virtual void RegisterFunction(ScriptFunctionBinding_t *) = 0;
		virtual bool RegisterClass(ScriptClassDesc_t *) = 0;
	public:
		virtual HSCRIPT RegisterInstance_impl(ScriptClassDesc_t *, void *) = 0;
	public:
		HSCRIPT RegisterInstance(ScriptClassDesc_t *desc, void *value) noexcept;
		virtual void SetInstanceUniqeId(HSCRIPT, const char *) = 0;
		bool SetInstanceUniqeId2(HSCRIPT instance, const char *root) noexcept;
		virtual void RemoveInstance(HSCRIPT) = 0;
	private:
		virtual void *GetInstanceValue_impl(HSCRIPT, ScriptClassDesc_t * = nullptr) = 0;
	public:
		template <typename T>
		inline T *GetInstanceValue(HSCRIPT instance, ScriptClassDesc_t *desc) noexcept
		{ return reinterpret_cast<T *>(GetInstanceValue_impl(instance, desc)); }
		template <typename T>
		inline T *GetInstanceValue(HSCRIPT instance) noexcept
		{
			using desc_t = decltype(T::desc);

			static_assert(std::is_base_of_v<ScriptClassDesc_t, desc_t>);

			if constexpr(std::is_pointer_v<desc_t>) {
				return static_cast<T *>(GetInstanceValue_impl(instance, T::desc));
			} else {
				return static_cast<T *>(GetInstanceValue_impl(instance, &T::desc));
			}
		}
		virtual bool GenerateUniqueKey(const char *, char *, int) = 0;
		template <std::size_t S>
		bool GenerateUniqueKey(const char *root, char (&buffer)[S]) noexcept
		{ return GenerateUniqueKey(root, buffer, static_cast<int>(S)); }
		virtual bool ValueExists(HSCRIPT, const char *) = 0;
		virtual bool SetValue(HSCRIPT, const char *, const char *) = 0;
	public:
		virtual bool SetValue_impl(HSCRIPT, const char *, const ScriptVariant_t &) = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual bool SetValue_impl(HSCRIPT, int, const ScriptVariant_t &) = 0;
	#endif
	public:
		bool SetValue(HSCRIPT scope, const char *name, const ScriptVariant_t &var) noexcept;
		bool SetValue(HSCRIPT scope, const char *name, ScriptVariant_t &&var) noexcept;
		bool SetValue(HSCRIPT scope, const char *name, HSCRIPT object) noexcept;
	private:
		virtual void CreateTable_impl(ScriptVariant_t &) = 0;
	public:
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		bool IsTable(HSCRIPT table) const noexcept;
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual bool IsTable(HSCRIPT) = 0;
	#else
		#error
	#endif
		HSCRIPT CreateTable() noexcept;
		void ReleaseTable(HSCRIPT table) noexcept;
		void ReleaseArray(HSCRIPT array) noexcept;
		virtual int GetNumTableEntries(HSCRIPT) = 0;
		virtual int GetKeyValue(HSCRIPT, int, ScriptVariant_t *, ScriptVariant_t *) = 0;
		int GetArrayValue(HSCRIPT array, int it, ScriptVariant_t *value) noexcept;
		virtual bool GetValue(HSCRIPT, const char *, ScriptVariant_t *) = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual bool GetValue(HSCRIPT, int, ScriptVariant_t *) = 0;
		virtual bool GetScalarValue(HSCRIPT, ScriptVariant_t *) = 0;
	#endif
		bool GetValue(HSCRIPT scope, const char *name, HSCRIPT *object) noexcept;
		virtual void ReleaseValue(ScriptVariant_t &) = 0;
		void ReleaseValue(HSCRIPT object) noexcept;
		void ReleaseObject(HSCRIPT object) noexcept;
		virtual bool ClearValue(HSCRIPT, const char *) = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		HSCRIPT CreateArray() noexcept;
		bool IsArray(HSCRIPT array) const noexcept;
		int GetArrayCount(HSCRIPT array) const noexcept;
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual HSCRIPT CreateArray() = 0;
		virtual bool IsArray(HSCRIPT) = 0;
		virtual int GetArrayCount(HSCRIPT) = 0;
		virtual void ArrayAddToTail(HSCRIPT, const ScriptVariant_t &) = 0;
	#else
		#error
	#endif
		void ArrayAddToTail(HSCRIPT array, ScriptVariant_t &&var) noexcept;
		virtual void WriteState(CUtlBuffer *) = 0;
		virtual void ReadState(CUtlBuffer *) = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual void CollectGarbage(const char *, bool) = 0;
	#endif
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		virtual void RemoveOrphanInstances() = 0;
	#endif
		virtual void DumpState() = 0;
		virtual void SetOutputCallback(ScriptOutputFunc_t) = 0;
		virtual void SetErrorCallback(ScriptErrorFunc_t) = 0;
	private:
		virtual bool RaiseException_impl(const char *) = 0;
	public:
		__attribute__((__format__(__printf__, 2, 3))) bool RaiseException(const char *fmt, ...) noexcept;
		__attribute__((__format__(__printf__, 2, 0))) bool RaiseExceptionv(const char *fmt, va_list vargs) noexcept;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual HSCRIPT GetRootTable() = 0;
		virtual HSCRIPT CopyHandle(HSCRIPT) = 0;
		virtual HSCRIPT GetIdentity(HSCRIPT) = 0;
	#endif
	private:
		virtual CSquirrelMetamethodDelegateImpl *MakeSquirrelMetamethod_Get_impl(HSCRIPT &, const char *, ISquirrelMetamethodDelegate *, bool) = 0;
	public:
		CSquirrelMetamethodDelegateImpl *MakeSquirrelMetamethod_Get(HSCRIPT scope, const char *name, ISquirrelMetamethodDelegate *delegate, bool free) noexcept;
		virtual void DestroySquirrelMetamethod_Get(CSquirrelMetamethodDelegateImpl *) = 0;
		virtual int GetKeyValue2(HSCRIPT, int, ScriptVariant_t *, ScriptVariant_t *) = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		virtual HSQUIRRELVM GetInternalVM() = 0;
		virtual bool GetScalarValue(HSCRIPT, ScriptVariant_t *) = 0;
		virtual void ArrayAddToTail(HSCRIPT, const ScriptVariant_t &) = 0;
		virtual HSCRIPT GetRootTable() = 0;
		virtual HSCRIPT CopyHandle(HSCRIPT) = 0;
		virtual HSCRIPT GetIdentity(HSCRIPT) = 0;
		virtual void CollectGarbage(const char *, bool) = 0;
	#endif
	};
	#pragma GCC diagnostic pop

	extern IScriptVM *g_pScriptVM;

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class CSquirrelVM : public IScriptVM
	{
	public:
		HSQUIRRELVM m_hVM;
	};
	#pragma GCC diagnostic pop
}

#include "vscript.tpp"
