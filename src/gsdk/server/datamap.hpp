#pragma once

namespace gsdk
{
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

	struct typedescription_t;

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
