#pragma once

#if !defined __VMOD_VSCRIPT_HEADER_INCLUDED && !defined __VMOD_COMPILING_GSDK
	#error "include vscript/vscript.hpp instead"
#endif

#include <cstdarg>
#include <cstdint>
#include <string_view>
#include "../tier1/interface.hpp"
#include "../tier1/appframework.hpp"
#include "../mathlib/vector.hpp"
#include "../tier1/utlvector.hpp"
#include "../server/datamap.hpp"
#include "../string_t.hpp"
#include <cstring>

#include "../../hacking.hpp"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundef"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wnoexcept"
#endif
#include <squirrel.h>
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

//TODO!! how to detect
#if !defined SQUIRREL_VERSION_NUMBER
	#define __VMOD_USING_QUIRREL
#endif

#if defined __VMOD_USING_QUIRREL && !defined __VMOD_USING_CUSTOM_VM
	#error
#endif

#if !defined __VMOD_USING_QUIRREL && !defined SQUIRREL_VERSION_NUMBER
	#error
#endif

#if defined __VMOD_USING_QUIRREL || !(defined SQUIRREL_VERSION_NUMBER && SQUIRREL_VERSION_NUMBER >= 303)
SQUIRREL_API SQInteger sq_getversion();
#endif

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
#pragma clang diagnostic ignored "-Wreorder-ctor"
#pragma clang diagnostic ignored "-Wstring-conversion"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wcast-align"
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
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wnoexcept"
#pragma GCC diagnostic ignored "-Wcast-align"
#endif
#include <sqstate.h>
#include <squtils.h>
#include <sqvm.h>
#include <sqobject.h>
#include <squserdata.h>
#include <sqtable.h>
#include <sqarray.h>
#include <sqclass.h>
#include <sqfuncproto.h>
#include <sqclosure.h>
#undef type
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif

#if defined __VMOD_USING_CUSTOM_VM && defined __VMOD_ENABLE_SOURCEPAWN
namespace vmod::vm
{
	struct sp_object;

	enum class sp_object_type : int;
}

namespace SourcePawn
{
	class ISourcePawnEnvironment;
}
#endif

namespace gsdk
{
	class CUtlBuffer;
	class CUtlString;
	class CUtlConstString;
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
	#if defined __VMOD_USING_CUSTOM_VM
		#ifdef __VMOD_ENABLE_SOURCEPAWN
		SL_SOURCEPAWN,
		#endif
		#ifdef __VMOD_ENABLE_JS
		SL_JAVASCRIPT,
		#endif
	#endif
		SL_DEFAULT = SL_SQUIRREL,
	};

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	class IScriptManager : public IAppSystem
	{
	public:
	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
		static constexpr std::string_view interface_name{"VScriptManager010"};
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		static constexpr std::string_view interface_name{"VScriptManager009"};
	#else
		#error
	#endif

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

#ifdef __VMOD_USING_CUSTOM_VM
	enum ScriptParamFlags_t : unsigned char
	{
		FIELD_FLAG_NONE     = 0,
		FIELD_FLAG_OPTIONAL = (1 << 0),
		FIELD_FLAG_LAST = FIELD_FLAG_OPTIONAL,
	};

	enum class SlimScriptDataType_t : int
	{
		FIELD_FLOAT,
		FIELD_STRING,
		FIELD_VECTOR,
		FIELD_QUATERNION,
		FIELD_INTEGER,
		FIELD_BOOLEAN,
		FIELD_SHORT,
		FIELD_CHARACTER,
		FIELD_COLOR32,
		FIELD_CLASSPTR,
		FIELD_EHANDLE,
		FIELD_EDICT,
		FIELD_POSITION_VECTOR,
		FIELD_VMATRIX,
		FIELD_MATRIX3X4_WORLDSPACE,
		FIELD_VECTOR2D,
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		FIELD_INTEGER64,
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		FIELD_VECTOR4D,
	#endif
		FIELD_CSTRING,
		FIELD_HSCRIPT,
		FIELD_VARIANT,
		FIELD_UINT64,
		FIELD_FLOAT64,
		FIELD_UINT32,
		FIELD_QANGLE,
		SLIMFIELD_TYPECOUNT,
		FIELD_TYPEUNKNOWN = SLIMFIELD_TYPECOUNT
	};

	constexpr SlimScriptDataType_t slim_datatype(ScriptDataType_t type) noexcept
	{
		switch(type) {
		case FIELD_FLOAT: return SlimScriptDataType_t::FIELD_FLOAT;
		case FIELD_STRING: return SlimScriptDataType_t::FIELD_STRING;
		case FIELD_VECTOR: return SlimScriptDataType_t::FIELD_VECTOR;
		case FIELD_QUATERNION: return SlimScriptDataType_t::FIELD_QUATERNION;
		case FIELD_INTEGER: return SlimScriptDataType_t::FIELD_INTEGER;
		case FIELD_BOOLEAN: return SlimScriptDataType_t::FIELD_BOOLEAN;
		case FIELD_SHORT: return SlimScriptDataType_t::FIELD_SHORT;
		case FIELD_CHARACTER: return SlimScriptDataType_t::FIELD_CHARACTER;
		case FIELD_COLOR32: return SlimScriptDataType_t::FIELD_COLOR32;
		case FIELD_CLASSPTR: return SlimScriptDataType_t::FIELD_CLASSPTR;
		case FIELD_EHANDLE: return SlimScriptDataType_t::FIELD_EHANDLE;
		case FIELD_EDICT: return SlimScriptDataType_t::FIELD_EDICT;
		case FIELD_POSITION_VECTOR: return SlimScriptDataType_t::FIELD_POSITION_VECTOR;
		case FIELD_VMATRIX: return SlimScriptDataType_t::FIELD_VMATRIX;
		case FIELD_MATRIX3X4_WORLDSPACE: return SlimScriptDataType_t::FIELD_MATRIX3X4_WORLDSPACE;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		case FIELD_INTEGER64: return SlimScriptDataType_t::FIELD_INTEGER64;
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		case FIELD_VECTOR4D: return SlimScriptDataType_t::FIELD_VECTOR4D;
	#endif
		case FIELD_CSTRING: return SlimScriptDataType_t::FIELD_CSTRING;
		case FIELD_HSCRIPT_NEW_INSTANCE:
		case FIELD_CUSTOM:
		case FIELD_EMBEDDED:
		case FIELD_FUNCTION:
		case FIELD_HSCRIPT: return SlimScriptDataType_t::FIELD_HSCRIPT;
		case FIELD_VARIANT: return SlimScriptDataType_t::FIELD_VARIANT;
		case FIELD_UINT64: return SlimScriptDataType_t::FIELD_UINT64;
		case FIELD_FLOAT64: return SlimScriptDataType_t::FIELD_FLOAT64;
		case FIELD_UINT32: return SlimScriptDataType_t::FIELD_UINT32;
		case FIELD_QANGLE: return SlimScriptDataType_t::FIELD_QANGLE;
		default: return SlimScriptDataType_t::FIELD_TYPEUNKNOWN;
		}
	}

	constexpr ScriptDataType_t fat_datatype(SlimScriptDataType_t type) noexcept
	{
		switch(type) {
		case SlimScriptDataType_t::FIELD_FLOAT: return FIELD_FLOAT;
		case SlimScriptDataType_t::FIELD_STRING: return FIELD_STRING;
		case SlimScriptDataType_t::FIELD_VECTOR: return FIELD_VECTOR;
		case SlimScriptDataType_t::FIELD_QUATERNION: return FIELD_QUATERNION;
		case SlimScriptDataType_t::FIELD_INTEGER: return FIELD_INTEGER;
		case SlimScriptDataType_t::FIELD_BOOLEAN: return FIELD_BOOLEAN;
		case SlimScriptDataType_t::FIELD_SHORT: return FIELD_SHORT;
		case SlimScriptDataType_t::FIELD_CHARACTER: return FIELD_CHARACTER;
		case SlimScriptDataType_t::FIELD_COLOR32: return FIELD_COLOR32;
		case SlimScriptDataType_t::FIELD_CLASSPTR: return FIELD_CLASSPTR;
		case SlimScriptDataType_t::FIELD_EHANDLE: return FIELD_EHANDLE;
		case SlimScriptDataType_t::FIELD_EDICT: return FIELD_EDICT;
		case SlimScriptDataType_t::FIELD_POSITION_VECTOR: return FIELD_POSITION_VECTOR;
		case SlimScriptDataType_t::FIELD_VMATRIX: return FIELD_VMATRIX;
		case SlimScriptDataType_t::FIELD_MATRIX3X4_WORLDSPACE: return FIELD_MATRIX3X4_WORLDSPACE;
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		case SlimScriptDataType_t::FIELD_INTEGER64: return FIELD_INTEGER64;
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		case SlimScriptDataType_t::FIELD_VECTOR4D: return FIELD_VECTOR4D;
	#endif
		case SlimScriptDataType_t::FIELD_CSTRING: return FIELD_CSTRING;
		case SlimScriptDataType_t::FIELD_HSCRIPT: return FIELD_HSCRIPT;
		case SlimScriptDataType_t::FIELD_VARIANT: return FIELD_VARIANT;
		case SlimScriptDataType_t::FIELD_UINT64: return FIELD_UINT64;
		case SlimScriptDataType_t::FIELD_FLOAT64: return FIELD_FLOAT64;
		case SlimScriptDataType_t::FIELD_UINT32: return FIELD_UINT32;
		case SlimScriptDataType_t::FIELD_QANGLE: return FIELD_QANGLE;
		default: return FIELD_TYPEUNKNOWN;
		}
	}

	struct alignas(ScriptDataType_t) ScriptDataTypeAndFlags_t final
	{
		constexpr ScriptDataTypeAndFlags_t() noexcept = default;

		constexpr ScriptDataTypeAndFlags_t(const ScriptDataTypeAndFlags_t &) noexcept = default;
		constexpr ScriptDataTypeAndFlags_t &operator=(const ScriptDataTypeAndFlags_t &) noexcept = default;

		constexpr ScriptDataTypeAndFlags_t(ScriptDataTypeAndFlags_t &&) noexcept = default;
		constexpr ScriptDataTypeAndFlags_t &operator=(ScriptDataTypeAndFlags_t &&) noexcept = default;

		constexpr inline ScriptDataTypeAndFlags_t(ScriptDataType_t type) noexcept
		{ set_main_type(type); }

		unsigned int value_{0};

		static constexpr inline unsigned int main_type_bits{6};
		static constexpr inline unsigned int flags_bits{1};
		static constexpr inline unsigned int extra_types_bits{25};

		static constexpr inline unsigned int flags_start_bit{main_type_bits};
		static constexpr inline unsigned int extra_types_start_bit{main_type_bits+flags_bits};

		static constexpr inline unsigned int mask_type       {0b00000000000000000000000000111111};
		static constexpr inline unsigned int mask_flags      {0b00000000000000000000000001000000};
		static constexpr inline unsigned int mask_extra_types{0b11111111111111111111111110000000};

		constexpr inline ScriptDataType_t main_type() const noexcept
		{ return static_cast<ScriptDataType_t>(value_ & mask_type); }

		constexpr inline ScriptParamFlags_t flags() const noexcept
		{ return static_cast<ScriptParamFlags_t>((value_ & mask_flags) >> flags_start_bit); }

		constexpr inline unsigned int extra_types() const noexcept
		{ return ((value_ & mask_extra_types) >> extra_types_start_bit); }

		constexpr inline bool has_extra_types() const noexcept
		{ return (extra_types() != 0); }

		constexpr inline bool has_extra_type(SlimScriptDataType_t type) const noexcept
		{ return (value_ & (1 << (extra_types_start_bit+static_cast<unsigned int>(type)))); }

		constexpr inline void set_main_type(ScriptDataType_t type) noexcept
		{ value_ |= (static_cast<unsigned int>(type) & mask_type); }

		constexpr inline void set_extra_type(ScriptDataType_t type) noexcept
		{ value_ |= (1 << (extra_types_start_bit+static_cast<unsigned int>(slim_datatype(type)))); }

		constexpr inline void add_flags(ScriptParamFlags_t flags) noexcept
		{ value_ |= ((static_cast<unsigned int>(flags) & (mask_flags >> flags_start_bit)) << flags_start_bit); }

		constexpr inline operator ScriptDataType_t() const noexcept
		{ return main_type(); }

		explicit constexpr inline operator ScriptParamFlags_t() const noexcept
		{ return flags(); }

		constexpr inline bool is_optional() const noexcept
		{ return (flags() & FIELD_FLAG_OPTIONAL); }

		constexpr bool can_be_anything() const noexcept
		{ return (has_extra_type(SlimScriptDataType_t::FIELD_VARIANT) || main_type() == FIELD_VARIANT); }

		constexpr bool can_be_null() const noexcept
		{
			if(can_be_anything()) {
				return true;
			}

			switch(main_type()) {
				case FIELD_VOID:
				case FIELD_POSITIVEINTEGER_OR_NULL:
				return true;
				default:
				return false;
			}
		}

		constexpr bool can_be_optional() const noexcept
		{
			if(is_optional()) {
				return true;
			}

			switch(main_type()) {
				case FIELD_VOID:
				case FIELD_POSITIVEINTEGER_OR_NULL:
				return true;
				default:
				return false;
			}
		}
	};

	static_assert(static_cast<unsigned int>(SlimScriptDataType_t::SLIMFIELD_TYPECOUNT) <= ScriptDataTypeAndFlags_t::extra_types_bits);

	static_assert(static_cast<unsigned int>(EXFIELD_TYPECOUNT) <= ScriptDataTypeAndFlags_t::mask_type);
	static_assert((static_cast<unsigned int>(FIELD_FLAG_LAST) << ScriptDataTypeAndFlags_t::main_type_bits) <= ScriptDataTypeAndFlags_t::mask_flags);

	static_assert(sizeof(ScriptDataTypeAndFlags_t) == sizeof(ScriptDataType_t));
	static_assert(alignof(ScriptDataTypeAndFlags_t) == alignof(ScriptDataType_t));
#endif

	enum SVFlags_t : short
	{
		SV_NOFLAGS = 0,
		SV_FREE = 0x01,
	};

	enum ExtendedFieldType : int;

	struct generic_handle__ final
	{
	private:
		~generic_handle__() = delete;
		generic_handle__() = delete;
		generic_handle__(const generic_handle__ &) = delete;
		generic_handle__ &operator=(const generic_handle__ &) = delete;
		generic_handle__(generic_handle__ &&) = delete;
		generic_handle__ &operator=(generic_handle__ &&) = delete;
	};

	using HSCRIPT__ = generic_handle__;
	using HINTERNALVM__ = generic_handle__;

	using HSCRIPT = HSCRIPT__ *;
	using HINTERNALVM = HINTERNALVM__ *;
	using HIDENTITY = int;

	inline HSCRIPT INVALID_HSCRIPT{reinterpret_cast<HSCRIPT>(-1)};

	constexpr HIDENTITY INVALID_IDENTITY{-1};

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

		HSCRIPT release_object() noexcept;

		inline ~CVariantBase() noexcept
		{ free(); }

		inline CVariantBase(const CVariantBase &other) noexcept
		{ operator=(other); }

		inline CVariantBase &operator=(const CVariantBase &other) noexcept
		{
			free();
			m_type = other.m_type;
			std::memcpy(m_data, other.m_data, sizeof(m_data));
			m_flags = (other.m_flags & ~SV_FREE);
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

		inline bool valid() const noexcept
		{
			return (
				m_type != FIELD_TYPEUNKNOWN &&
				m_type != FIELD_VOID
			);
		}

		inline bool is_object() const noexcept
		{
			switch(m_type) {
			case FIELD_HSCRIPT:
			case FIELD_HSCRIPT_NEW_INSTANCE:
			case FIELD_EMBEDDED:
			case FIELD_CUSTOM:
			case FIELD_FUNCTION:
			return true;
			default:
			return false;
			}
		}

		inline bool should_free() const noexcept
		{ return (m_flags & SV_FREE); }

		void reset() noexcept
		{
			free();
			m_type = FIELD_TYPEUNKNOWN;
			m_flags = SV_NOFLAGS;
			std::memset(m_data, 0, sizeof(m_data));
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
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			long long m_longlong;
			unsigned long long m_ulonglong;
		#endif
			float m_float;
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			double m_double;
		#endif
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
		#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
			unsigned char m_data[sizeof(unsigned long long)];
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
			unsigned char m_data[sizeof(unsigned int)];
		#else
			#error
		#endif
		};

		short m_type{FIELD_TYPEUNKNOWN};
		short m_flags{SV_NOFLAGS};
	};

	using ScriptVariant_t = CVariantBase<CVariantDefaultAllocator>;

	enum ScriptHandleType_t : unsigned char
	{
		HANDLETYPE_UNKNOWN,
		HANDLETYPE_FUNCTION,
		HANDLETYPE_TABLE,
		HANDLETYPE_ARRAY,
		HANDLETYPE_SCOPE,
		HANDLETYPE_INSTANCE,
		HANDLETYPE_SCRIPT
	};

	struct ScriptHandleWrapper_t
	{
		ScriptHandleWrapper_t() noexcept = default;

		inline ScriptHandleWrapper_t(std::nullptr_t) noexcept
			: ScriptHandleWrapper_t{}
		{
		}

		ScriptHandleWrapper_t(const ScriptHandleWrapper_t &) = delete;
		ScriptHandleWrapper_t &operator=(const ScriptHandleWrapper_t &) = delete;

		ScriptHandleWrapper_t(ScriptHandleWrapper_t &&other) noexcept;
		ScriptHandleWrapper_t &operator=(ScriptHandleWrapper_t &&other) noexcept;

		ScriptHandleWrapper_t(HSCRIPT) = delete;
		ScriptHandleWrapper_t &operator=(HSCRIPT) = delete;

		bool operator==(HSCRIPT) = delete;
		bool operator!=(HSCRIPT) = delete;

		bool operator==(const ScriptHandleWrapper_t &) = delete;
		bool operator!=(const ScriptHandleWrapper_t &) = delete;

		inline ScriptHandleWrapper_t(ScriptVariant_t &&other) noexcept
		{
			should_free_ = other.should_free();
			object = other.release_object();
			type = HANDLETYPE_UNKNOWN;
		}
		ScriptHandleWrapper_t &operator=(ScriptVariant_t &&) = delete;

		operator HSCRIPT() = delete;

		bool operator!() = delete;
		operator bool() = delete;

		inline ~ScriptHandleWrapper_t() noexcept
		{ free(); }

		inline bool should_free() const noexcept
		{ return should_free_; }

		void free() noexcept;

		HSCRIPT release() noexcept;

		HSCRIPT object{INVALID_HSCRIPT};
		bool should_free_{false};
		ScriptHandleType_t type{HANDLETYPE_UNKNOWN};
	};

#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
	//static_assert(sizeof(ScriptVariant_t) == (sizeof(unsigned long long) + (sizeof(short) * 2)));
#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	static_assert(sizeof(ScriptVariant_t) == (sizeof(unsigned int) + (sizeof(short) * 2)));
#else
	#error
#endif

	struct ScriptFunctionBindingStorageType_t
	{
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		union {
			vmod::generic_func_t func;
			vmod::generic_mfp_t mfp;
		};
		char unk1[sizeof(unsigned long long)
		#ifdef __x86_64__
		* 2
		#endif
		];
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
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
	static_assert(sizeof(ScriptFunctionBindingStorageType_t) == (sizeof(vmod::generic_mfp_t) + (sizeof(unsigned long long)
	#ifdef __x86_64__
	* 2
	#endif
	)));
#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	static_assert(sizeof(ScriptFunctionBindingStorageType_t) == sizeof(vmod::generic_plain_mfp_t));
#else
	#error
#endif

	using ScriptBindingFunc_t = bool(*)(ScriptFunctionBindingStorageType_t, void *, ScriptVariant_t *, int, ScriptVariant_t *);

	struct alignas(ScriptFunctionBindingStorageType_t) CScriptFunctionBindingStorageType : public ScriptFunctionBindingStorageType_t
	{
		inline CScriptFunctionBindingStorageType() noexcept
		{
		#if GSDK_ENGINE == GSDK_ENGINE_TF2
			mfp = nullptr;
			std::memset(unk1, 0, sizeof(unk1));
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
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
		#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
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

	enum ScriptFuncBindingFlags_t : int
	{
		SF_NOFLAGS =     0,
		SF_MEMBER_FUNC = 0x01,

		SF_LAST_FLAG =   SF_MEMBER_FUNC,
		SF_NUM_FLAGS =   1
	};

	enum : int
	{
		SF_VA_FUNC =          (1 << 1),
		SF_FREE_SCRIPT_NAME = (1 << 2),
		SF_FREE_NAME =        (1 << 3),
		SF_FREE_DESCRIPTION = (1 << 4)
	};

	static_assert(gsdk::SF_NUM_FLAGS == 1);
	static_assert(gsdk::SF_LAST_FLAG == (1 << 0));

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
	#ifdef __VMOD_USING_CUSTOM_VM
		CUtlVector<ScriptDataTypeAndFlags_t> m_Parameters;
	#else
		CUtlVector<ScriptDataType_t> m_Parameters;
	#endif

	private:
		ScriptFuncDescriptor_t(const ScriptFuncDescriptor_t &) = delete;
		ScriptFuncDescriptor_t &operator=(const ScriptFuncDescriptor_t &) = delete;
	};

	struct ScriptFunctionBinding_t
	{
	public:
		ScriptFunctionBinding_t() noexcept = default;
		~ScriptFunctionBinding_t() noexcept;

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
		unsigned int m_flags{SF_NOFLAGS};

		bool va_or_last_optional() const noexcept
		{
			if(m_flags & gsdk::SF_VA_FUNC) {
				return true;
			}

			if(!m_desc.m_Parameters.empty() && (m_desc.m_Parameters.end()-1)->can_be_optional()) {
				return true;
			}

			return false;
		}

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

		virtual bool Get(const CUtlConstString &, ScriptVariant_t &) = 0;
	};

	class CSquirrelMetamethodDelegateImpl;

#ifdef __VMOD_USING_CUSTOM_VM
	enum ScriptExecuteFlags_t : unsigned char
	{
		SCRIPT_EXEC_DEFAULT =        0,
		SCRIPT_EXEC_WAIT =     (1 << 0),
		SCRIPT_EXEC_COPYBACK = (1 << 1),
	};

	static_assert(sizeof(ScriptExecuteFlags_t) == sizeof(bool));
	static_assert(alignof(ScriptExecuteFlags_t) == alignof(bool));
#else
	using ScriptExecuteFlags_t = bool;
#endif

#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wweak-vtables"
	#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
	class IScriptVM
	{
	public:
	#ifndef __VMOD_USING_CUSTOM_VM
		static short fixup_var_field(short field) noexcept;
		static ScriptVariant_t &fixup_var(ScriptVariant_t &var) noexcept;
	#endif

	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		static void(IScriptVM::*squirrel_CreateArray_ptr)(ScriptVariant_t &);
		static int(IScriptVM::*squirrel_GetArrayCount_ptr)(HSCRIPT) const;
		static bool(IScriptVM::*squirrel_IsArray_ptr)(HSCRIPT) const;
		static bool(IScriptVM::*squirrel_IsTable_ptr)(HSCRIPT) const;
	#endif

		static constexpr std::size_t unique_id_max{4095 + 6 + 64};

		virtual bool Init() = 0;
		virtual void Shutdown() = 0;
		virtual bool ConnectDebugger() = 0;
		virtual void DisconnectDebugger() = 0;
		virtual ScriptLanguage_t GetLanguage() const = 0;
		virtual const char *GetLanguageName() const = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual HINTERNALVM GetInternalVM() = 0;
	#endif
		virtual void AddSearchPath(const char *) = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual bool ForwardConsoleCommand(const CCommandContext &, const CCommand &) = 0;
	#endif
		virtual bool Frame(float) = 0;
		virtual ScriptStatus_t Run(const char *, bool = true) = 0;
	public:
		virtual HSCRIPT CompileScript_impl(const char *, const char * = nullptr) = 0;
	public:
		ScriptHandleWrapper_t CompileScript(const char *src, const char *name = nullptr) noexcept;
		virtual void ReleaseScript(HSCRIPT) = 0;
		virtual ScriptStatus_t Run(HSCRIPT, HSCRIPT = nullptr, bool = true) = 0;
		virtual ScriptStatus_t Run(HSCRIPT, bool) = 0;
	private:
		virtual HSCRIPT CreateScope_impl(const char *, HSCRIPT = nullptr) = 0;
	public:
		ScriptHandleWrapper_t CreateScope(const char *script, HSCRIPT parent = nullptr) noexcept;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2 || GSDK_ENGINE == GSDK_ENGINE_TF2
	private:
		virtual HSCRIPT ReferenceScope_impl(HSCRIPT) = 0;
	public:
	#endif
		ScriptHandleWrapper_t ReferenceScope(HSCRIPT object) noexcept;
		ScriptHandleWrapper_t ReferenceObject(HSCRIPT object) noexcept;
		ScriptHandleWrapper_t ReferenceFunction(HSCRIPT object) noexcept;
		virtual void ReleaseScope(HSCRIPT) = 0;
	private:
		virtual HSCRIPT LookupFunction_impl(const char *, HSCRIPT = nullptr) = 0;
	public:
		ScriptHandleWrapper_t LookupFunction(const char *name, HSCRIPT scope = nullptr) noexcept;
		virtual void ReleaseFunction(HSCRIPT) = 0;
	private:
		virtual ScriptStatus_t ExecuteFunction_impl(HSCRIPT, ScriptVariant_t *, int, ScriptVariant_t *, HSCRIPT, ScriptExecuteFlags_t) = 0;
	public:
		ScriptStatus_t ExecuteFunction(HSCRIPT func, ScriptVariant_t *args, int num_args, ScriptVariant_t *ret, HSCRIPT scope, ScriptExecuteFlags_t flags) noexcept;
		ScriptStatus_t ExecuteFunction(HSCRIPT func, const ScriptVariant_t *args, int num_args, ScriptVariant_t *ret, HSCRIPT scope, bool wait) noexcept;
		virtual void RegisterFunction(ScriptFunctionBinding_t *) = 0;
		virtual bool RegisterClass(ScriptClassDesc_t *) = 0;
	public:
		virtual HSCRIPT RegisterInstance_impl(ScriptClassDesc_t *, void *) = 0;
	public:
		ScriptHandleWrapper_t RegisterInstance(ScriptClassDesc_t *desc, void *value) noexcept;
		virtual void SetInstanceUniqeId(HSCRIPT, const char *) = 0;
		bool SetInstanceUniqeId2(HSCRIPT instance, const char *root) noexcept;
		virtual void RemoveInstance(HSCRIPT) = 0;
	private:
		virtual void *GetInstanceValue_impl(HSCRIPT, ScriptClassDesc_t * = nullptr) = 0;
	public:
		template <typename T>
		inline T *GetInstanceValue(HSCRIPT instance, ScriptClassDesc_t *desc) noexcept
		{ return static_cast<T *>(GetInstanceValue_impl(instance, desc)); }
		template <typename T>
		T *GetInstanceValue(HSCRIPT instance) noexcept;
		virtual bool GenerateUniqueKey(const char *, char *, int) = 0;
		template <std::size_t S>
		bool GenerateUniqueKey(const char *root, char (&buffer)[S]) noexcept
		{ return GenerateUniqueKey(root, buffer, static_cast<int>(S)); }
		virtual bool ValueExists(HSCRIPT, const char *) = 0;
		virtual bool SetValue(HSCRIPT, const char *, const char *) = 0;
	public:
		virtual bool SetValue_impl(HSCRIPT, const char *, ScriptVariant_t &) = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual bool SetValue_impl(HSCRIPT, int, ScriptVariant_t &) = 0;
	#endif
	public:
		bool SetValue(HSCRIPT scope, const char *name, ScriptVariant_t &var) noexcept;
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
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		bool IsTable(HSCRIPT table) const noexcept;
	#endif
		ScriptHandleWrapper_t CreateTable() noexcept;
		void ReleaseTable(HSCRIPT table) noexcept;
		void ReleaseArray(HSCRIPT array) noexcept;
		virtual int GetNumTableEntries(HSCRIPT) const = 0;
		virtual int GetKeyValue(HSCRIPT, int, ScriptVariant_t *, ScriptVariant_t *) = 0;
		int GetArrayValue(HSCRIPT array, int it, ScriptVariant_t *value) noexcept;
	public:
		virtual bool GetValue_impl(HSCRIPT, const char *, ScriptVariant_t *) = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
	private:
		virtual bool GetValue_impl(HSCRIPT, int, ScriptVariant_t *) = 0;
	public:
		virtual bool GetScalarValue(HSCRIPT, ScriptVariant_t *) = 0;
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		bool GetScalarValue(HSCRIPT object, ScriptVariant_t *var) noexcept;
	#endif
	public:
		bool GetValue(HSCRIPT scope, const char *name, ScriptVariant_t *var) noexcept;
		bool GetValue(HSCRIPT scope, const char *name, ScriptHandleWrapper_t *object) noexcept = delete;
		virtual void ReleaseValue(ScriptVariant_t &) = 0;
		void ReleaseValue(HSCRIPT object) noexcept;
		void ReleaseObject(HSCRIPT object) noexcept;
		virtual bool ClearValue(HSCRIPT, const char *) = 0;
		ScriptHandleWrapper_t CreateArray() noexcept;
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		bool IsArray(HSCRIPT array) const noexcept;
		int GetArrayCount(HSCRIPT array) const noexcept;
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
	private:
		virtual void CreateArray_impl(ScriptVariant_t &) = 0;
	public:
		virtual bool IsArray(HSCRIPT) = 0;
		virtual int GetArrayCount(HSCRIPT) = 0;
		virtual void ArrayAddToTail(HSCRIPT, const ScriptVariant_t &) = 0;
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		bool IsArray(HSCRIPT array) const noexcept;
		int GetArrayCount(HSCRIPT array) const noexcept;
		void ArrayAddToTail(HSCRIPT array, const ScriptVariant_t &var);
	#endif
		void ArrayAddToTail(HSCRIPT array, ScriptVariant_t &&var) noexcept;
		virtual void WriteState(CUtlBuffer *) = 0;
		virtual void ReadState(CUtlBuffer *) = 0;
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		virtual void CollectGarbage(const char *, bool) = 0;
	#endif
	#if GSDK_ENGINE != GSDK_ENGINE_L4D2
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
	private:
		virtual HSCRIPT GetRootTable() = 0;
		virtual HSCRIPT CopyHandle(HSCRIPT) = 0;
	public:
		virtual HIDENTITY GetIdentity(HSCRIPT) = 0;
	#endif
	#if GSDK_ENGINE == GSDK_ENGINE_TF2 || GSDK_ENGINE == GSDK_ENGINE_L4D2
	private:
		virtual CSquirrelMetamethodDelegateImpl *MakeSquirrelMetamethod_Get_impl(HSCRIPT &, const char *, ISquirrelMetamethodDelegate *, bool) = 0;
	public:
		CSquirrelMetamethodDelegateImpl *MakeSquirrelMetamethod_Get(HSCRIPT scope, const char *name, ISquirrelMetamethodDelegate *delegate, bool free) noexcept;
		virtual void DestroySquirrelMetamethod_Get(CSquirrelMetamethodDelegateImpl *) = 0;
		virtual int GetKeyValue2(HSCRIPT, int, ScriptVariant_t *, ScriptVariant_t *) = 0;
	#endif
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		virtual HINTERNALVM GetInternalVM() = 0;
		virtual bool GetScalarValue(HSCRIPT, ScriptVariant_t *) = 0;
		virtual void ArrayAddToTail(HSCRIPT, const ScriptVariant_t &) = 0;
	private:
		virtual HSCRIPT GetRootTable() = 0;
		virtual HSCRIPT CopyHandle(HSCRIPT) = 0;
	public:
		virtual HIDENTITY GetIdentity(HSCRIPT) = 0;
		virtual void CollectGarbage(const char *, bool) = 0;
	#endif
	};
#ifdef __clang__
	#pragma clang diagnostic pop
#else
	#pragma GCC diagnostic pop
#endif

	extern IScriptVM *g_pScriptVM;
}

namespace vmod
{
#ifdef __VMOD_USING_CUSTOM_VM
	#ifdef __clang__
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wweak-vtables"
	#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
	#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
	#endif
	class IScriptVM : public gsdk::IScriptVM
	{
	public:
		//TODO!!! insert missing funcs
	};
	#ifdef __clang__
	#pragma clang diagnostic pop
	#else
	#pragma GCC diagnostic pop
	#endif
#else
	using IScriptVM = gsdk::IScriptVM;
#endif
}

#include "vscript.tpp"
