#pragma once

#include "../config.hpp"
#include <cstdlib>
#include <string_view>
#include "../tier1/utlvector.hpp"

namespace gsdk
{
	class ISaveRestoreOps;
	struct inputdata_t;
	class CBaseEntity;
	struct optimized_datamap_t;

	enum fieldtype_t : int
	{
		FIELD_VOID,
		FIELD_FLOAT,
		FIELD_STRING,
		FIELD_VECTOR,
		FIELD_QUATERNION,
		FIELD_INTEGER,
		FIELD_BOOLEAN,
		FIELD_SHORT,
		FIELD_CHARACTER,
		FIELD_COLOR32,
		FIELD_EMBEDDED,
		FIELD_CUSTOM,
		FIELD_CLASSPTR,
		FIELD_EHANDLE,
		FIELD_EDICT,
		FIELD_POSITION_VECTOR,
		FIELD_TIME,
		FIELD_TICK,
		FIELD_MODELNAME,
		FIELD_SOUNDNAME,
		FIELD_INPUT,
		FIELD_FUNCTION,
		FIELD_VMATRIX,
		FIELD_VMATRIX_WORLDSPACE,
		FIELD_MATRIX3X4_WORLDSPACE,
		FIELD_INTERVAL,
		FIELD_MODELINDEX,
		FIELD_MATERIALINDEX,
		FIELD_VECTOR2D,
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		FIELD_INTEGER64,
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V1)
		FIELD_VECTOR4D,
	#endif
		FIELD_TYPECOUNT
	};

	enum ExtendedFieldType : int
	{
		FIELD_TYPEUNKNOWN = FIELD_TYPECOUNT,
		FIELD_CSTRING,
		FIELD_HSCRIPT,
		FIELD_VARIANT,
		FIELD_UINT64,
		FIELD_FLOAT64,
		FIELD_POSITIVEINTEGER_OR_NULL,
		FIELD_HSCRIPT_NEW_INSTANCE,
		FIELD_UINT32,
		FIELD_UTLSTRINGTOKEN,
		FIELD_QANGLE,
	};

	static_assert(sizeof(fieldtype_t) == sizeof(int));
	static_assert(sizeof(ExtendedFieldType) == sizeof(int));

	struct datamap_t;

	enum : int
	{
		TD_OFFSET_NORMAL,
		TD_OFFSET_PACKED,
		TD_OFFSET_COUNT,
	};

	enum : int
	{
		PC_NON_NETWORKED_ONLY,
		PC_NETWORKED_ONLY,
		PC_COPYTYPE_COUNT,
		PC_EVERYTHING = PC_COPYTYPE_COUNT,
	};

	enum : int
	{
		FTYPEDESC_NOFLAGS =           0,
		FTYPEDESC_GLOBAL =            0x0001,
		FTYPEDESC_SAVE =              0x0002,
		FTYPEDESC_KEY =               0x0004,
		FTYPEDESC_INPUT =             0x0008,
		FTYPEDESC_OUTPUT =            0x0010,
		FTYPEDESC_FUNCTIONTABLE =     0x0020,
		FTYPEDESC_PTR =               0x0040,
		FTYPEDESC_OVERRIDE =          0x0080,
		FTYPEDESC_INSENDTABLE =       0x0100,
		FTYPEDESC_PRIVATE =           0x0200,
		FTYPEDESC_NOERRORCHECK =      0x0400,
		FTYPEDESC_MODELINDEX =        0x0800,
		FTYPEDESC_INDEX =             0x1000,
		FTYPEDESC_VIEW_OTHER_PLAYER = 0x2000,
		FTYPEDESC_VIEW_OWN_TEAM =     0x4000,
		FTYPEDESC_VIEW_NEVER =        0x8000,

		FTYPEDESC_LAST_FLAG =         FTYPEDESC_VIEW_NEVER,
		FTYPEDESC_NUM_FLAGS =         16,
	};

	enum : int
	{
		FTYPEDESC_FREE_DATAMAP =      (1 << 16),
		FTYPEDESC_FREE_NAME =         (1 << 17),
		FTYPEDESC_FREE_EXTERNALNAME = (1 << 18),
	};

	static_assert(FTYPEDESC_NUM_FLAGS == 16);
	static_assert(FTYPEDESC_LAST_FLAG == (1 << 15));

	using inputfunc_t = void(CBaseEntity::*)(inputdata_t &);

	struct typedescription_t
	{
		typedescription_t() noexcept = default;
		inline ~typedescription_t() noexcept
		{ free(); }

		void free() noexcept;

		inline typedescription_t(typedescription_t &&other) noexcept
		{ operator=(std::move(other)); }
		typedescription_t &operator=(typedescription_t &&other) noexcept;

		inline typedescription_t(const typedescription_t &other) noexcept
		{ operator=(other); }
		typedescription_t &operator=(const typedescription_t &other) noexcept;

		static typedescription_t empty;

		bool operator==(const typedescription_t &other) const noexcept;
		inline bool operator!=(const typedescription_t &other) const noexcept
		{ return !operator==(other); }

		int fieldType{FIELD_VOID};
		const char *fieldName{nullptr};
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		int fieldOffset[TD_OFFSET_COUNT]{0,0};
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		int fieldOffset{0};
	#else
		#error
	#endif
		unsigned short fieldSize{0};
		short flags{FTYPEDESC_NOFLAGS};
		const char *externalName{nullptr};
		ISaveRestoreOps *pSaveRestoreOps{nullptr};
		inputfunc_t inputFunc{nullptr};
		datamap_t *td{nullptr};
		int fieldSizeInBytes{0};
		typedescription_t *override_field{nullptr};
		int override_count{0};
		float fieldTolerance{0.0f};
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		int flatOffset[TD_OFFSET_COUNT]{0,0};
		unsigned short flatGroup{0};
	#endif
	};

	struct datarun_t
	{
		int m_nStartFlatField;
		int m_nEndFlatField;
		int m_nStartOffset[TD_OFFSET_COUNT];
		int m_nLength;

	private:
		datarun_t() = delete;
		datarun_t(const datarun_t &) = delete;
		datarun_t &operator=(const datarun_t &) = delete;
		datarun_t(datarun_t &&) = delete;
		datarun_t &operator=(datarun_t &&) = delete;
	};

	struct datacopyruns_t
	{
		CUtlVector<datarun_t> m_vecRuns;

	private:
		datacopyruns_t() = delete;
		datacopyruns_t(const datacopyruns_t &) = delete;
		datacopyruns_t &operator=(const datacopyruns_t &) = delete;
		datacopyruns_t(datacopyruns_t &&) = delete;
		datacopyruns_t &operator=(datacopyruns_t &&) = delete;
	};

	struct flattenedoffsets_t
	{
		CUtlVector<typedescription_t> m_Flattened;
		int m_nPackedSize;
		int m_nPackedStartOffset;

	private:
		flattenedoffsets_t() = delete;
		flattenedoffsets_t(const flattenedoffsets_t &) = delete;
		flattenedoffsets_t &operator=(const flattenedoffsets_t &) = delete;
		flattenedoffsets_t(flattenedoffsets_t &&) = delete;
		flattenedoffsets_t &operator=(flattenedoffsets_t &&) = delete;
	};

	struct datamapinfo_t
	{
		flattenedoffsets_t m_Flat;
		datacopyruns_t m_CopyRuns;

	private:
		datamapinfo_t() = delete;
		datamapinfo_t(const datamapinfo_t &) = delete;
		datamapinfo_t &operator=(const datamapinfo_t &) = delete;
		datamapinfo_t(datamapinfo_t &&) = delete;
		datamapinfo_t &operator=(datamapinfo_t &&) = delete;
	};

	struct optimized_datamap_t
	{
		datamapinfo_t m_Info[PC_COPYTYPE_COUNT];

	private:
		optimized_datamap_t() = delete;
		optimized_datamap_t(const optimized_datamap_t &) = delete;
		optimized_datamap_t &operator=(const optimized_datamap_t &) = delete;
		optimized_datamap_t(optimized_datamap_t &&) = delete;
		optimized_datamap_t &operator=(optimized_datamap_t &&) = delete;
	};

	struct datamap_t
	{
		datamap_t() noexcept = default;
		inline ~datamap_t() noexcept
		{ free(); }

		void free() noexcept;

		inline datamap_t(const datamap_t &other) noexcept
		{ operator=(other); }
		datamap_t &operator=(const datamap_t &other) noexcept;

		bool operator==(const datamap_t &other) const noexcept;
		inline bool operator!=(const datamap_t &other) const noexcept
		{ return !operator==(other); }

		short *get_flags() const noexcept;

		typedescription_t *dataDesc{nullptr};
		int dataNumFields{0};
		const char *dataClassName{nullptr};
		datamap_t *baseMap{nullptr};
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		int packed_size{0};
		optimized_datamap_t *m_pOptimizedDataMap{nullptr};
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		bool chains_validated{false};
		bool packed_offsets_computed{false};
		int packed_size{0};
	#endif

	private:
		datamap_t(datamap_t &&) = delete;
		datamap_t &operator=(datamap_t &&) = delete;
	};
}
