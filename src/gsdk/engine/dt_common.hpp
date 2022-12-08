#pragma once

//#define GSDK_DT_SUPPORTS_INT64

namespace gsdk
{
	enum SendPropType : int
	{
		DPT_Int=0,
		DPT_Float,
		DPT_Vector,
		DPT_VectorXY,
		DPT_String,
		DPT_Array,
		DPT_DataTable,
	#if 0
		DPT_Quaternion,
	#endif
	#ifdef GSDK_DT_SUPPORTS_INT64
		DPT_Int64,
	#endif
		DPT_NUMSendPropTypes
	};

	enum : int
	{
		SPROP_UNSIGNED =                  (1 << 0),
		SPROP_COORD =                     (1 << 1),
		SPROP_NOSCALE =                   (1 << 2),
		SPROP_ROUNDDOWN =                 (1 << 3),
		SPROP_ROUNDUP =                   (1 << 4),
		SPROP_NORMAL =                    (1 << 5),
		SPROP_EXCLUDE =                   (1 << 6),
		SPROP_XYZE =                      (1 << 7),
		SPROP_INSIDEARRAY =               (1 << 8),
		SPROP_PROXY_ALWAYS_YES =          (1 << 9),
		SPROP_CHANGES_OFTEN =             (1 << 10),
		SPROP_IS_A_VECTOR_ELEM =          (1 << 11),
		SPROP_COLLAPSIBLE =               (1 << 12),
		SPROP_COORD_MP =                  (1 << 13),
		SPROP_COORD_MP_LOWPRECISION =     (1 << 14),
		SPROP_COORD_MP_INTEGRAL =         (1 << 15),
		SPROP_VARINT =                    SPROP_NORMAL,
		SPROP_NUMFLAGBITS_NETWORKED =           16,
		SPROP_ENCODED_AGAINST_TICKCOUNT = (1 << 16),
		SPROP_NUMFLAGBITS =                     17
	};

	class DVariant
	{
	public:
		union
		{
			float m_Float;
			int m_Int;
			const char *m_pString;
			void *m_pData;
		#if 0
			float m_Vector[4];
		#else
			float m_Vector[3];
		#endif
		#ifdef GSDK_DT_SUPPORTS_INT64
			long long m_Int64;
		#endif
		};

		SendPropType m_Type;
	};
}
