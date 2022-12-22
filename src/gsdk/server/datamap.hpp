#pragma once

namespace gsdk
{
	class ISaveRestoreOps;
	struct inputdata_t;
	class CBaseEntity;

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
		FIELD_TYPECOUNT,
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
		fieldtype_t fieldType;
		const char *fieldName;
		int fieldOffset[TD_OFFSET_COUNT];
		unsigned short fieldSize;
		short flags;
		const char *externalName;
		ISaveRestoreOps *pSaveRestoreOps;
		inputfunc_t inputFunc;
		datamap_t *td;
		int fieldSizeInBytes;
		typedescription_t *override_field;
		int override_count;
		float fieldTolerance;
	};

	struct datamap_t
	{
		typedescription_t *dataDesc;
		int dataNumFields;
		const char *dataClassName;
		datamap_t *baseMap;
		bool chains_validated;
		bool packed_offsets_computed;
		int packed_size;
	};
}
