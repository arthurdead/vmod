#pragma once

#include "../config.hpp"

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
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		FIELD_MODELNAME,
	#endif
		FIELD_SOUNDNAME,
		FIELD_INPUT,
		FIELD_FUNCTION,
		FIELD_VMATRIX,
		FIELD_VMATRIX_WORLDSPACE,
		FIELD_MATRIX3X4_WORLDSPACE,
		FIELD_INTERVAL,
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		FIELD_UNUSED,
	#endif
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		FIELD_MODELINDEX,
		FIELD_MATERIALINDEX,
	#endif
		FIELD_VECTOR2D,
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		FIELD_INTEGER64,
		FIELD_VECTOR4D,
		FIELD_RESOURCE,
	#endif
		FIELD_TYPECOUNT,
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		FIELD_MODELNAME = 45,
		FIELD_MODELINDEX,
		FIELD_MATERIALINDEX
	#endif
	};

	struct datamap_t;

	enum : int
	{
		TD_OFFSET_NORMAL,
		TD_OFFSET_PACKED,
		TD_OFFSET_COUNT,
	};

	enum : int
	{
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
		FTYPEDESC_VIEW_NEVER =        0x8000
	};

	using inputfunc_t = void(CBaseEntity::*)(inputdata_t &);

	struct typedescription_t
	{
		fieldtype_t fieldType{FIELD_VOID};
		const char *fieldName{nullptr};
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		int fieldOffset[TD_OFFSET_COUNT]{0,0};
	#elif GSDK_ENGINE == GSDK_ENGINE_L4D2
		int fieldOffset{0};
	#else
		#error
	#endif
		unsigned short fieldSize{0};
		short flags{0};
		const char *externalName{nullptr};
		ISaveRestoreOps *pSaveRestoreOps{nullptr};
		inputfunc_t inputFunc{nullptr};
		datamap_t *td{nullptr};
		int fieldSizeInBytes{0};
		typedescription_t *override_field{nullptr};
		int override_count{0};
		float fieldTolerance{0.0f};
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		int flatOffset[TD_OFFSET_COUNT]{0,0};
		unsigned short flatGroup{0};
	#endif

	private:
		typedescription_t() = delete;
		typedescription_t(const typedescription_t &) = delete;
		typedescription_t &operator=(const typedescription_t &) = delete;
		typedescription_t(typedescription_t &&) = delete;
		typedescription_t &operator=(typedescription_t &&) = delete;
	};

	struct datamap_t
	{
		typedescription_t *dataDesc{nullptr};
		int dataNumFields{0};
		const char *dataClassName{nullptr};
		datamap_t *baseMap{nullptr};
	#if GSDK_ENGINE == GSDK_ENGINE_L4D2
		int packed_size{0};
		optimized_datamap_t *m_pOptimizedDataMap{nullptr};
	#endif
	#if GSDK_ENGINE == GSDK_ENGINE_TF2
		bool chains_validated{false};
		bool packed_offsets_computed{false};
		int packed_size{0};
	#endif

	private:
		datamap_t() = delete;
		datamap_t(const datamap_t &) = delete;
		datamap_t &operator=(const datamap_t &) = delete;
		datamap_t(datamap_t &&) = delete;
		datamap_t &operator=(datamap_t &&) = delete;
	};
}
