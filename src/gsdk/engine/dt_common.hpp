#pragma once

#include "../config.hpp"

#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
	#define GSDK_DT_SUPPORTS_INT64
#endif

#include "../mathlib/vector.hpp"
#include <cstring>

namespace gsdk
{
	enum SendPropType : int
	{
		DPT_Int,
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
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		SPROP_CHANGES_OFTEN =             (1 << 10),
		SPROP_IS_A_VECTOR_ELEM =          (1 << 11),
		SPROP_COLLAPSIBLE =               (1 << 12),
		SPROP_COORD_MP =                  (1 << 13),
		SPROP_COORD_MP_LOWPRECISION =     (1 << 14),
		SPROP_COORD_MP_INTEGRAL =         (1 << 15),
		//SPROP_CELL_COORD =                SPROP_COORD_MP,
		//SPROP_CELL_COORD_LOWPRECISION =   SPROP_COORD_MP_LOWPRECISION,
		//SPROP_CELL_COORD_INTEGRAL =       SPROP_COORD_MP_INTEGRAL,
		SPROP_NUMFLAGBITS_NETWORKED =           16,
		SPROP_ENCODED_AGAINST_TICKCOUNT = (1 << 16),
		SPROP_NUMFLAGBITS =                     17,
	#elif GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2010, >=, GSDK_ENGINE_BRANCH_2010_V0)
		SPROP_IS_A_VECTOR_ELEM =          (1 << 10),
		SPROP_COLLAPSIBLE =               (1 << 11),
		SPROP_COORD_MP =                  (1 << 12),
		SPROP_COORD_MP_LOWPRECISION =     (1 << 13),
		SPROP_COORD_MP_INTEGRAL =         (1 << 14),
		SPROP_CELL_COORD =                (1 << 15),
		SPROP_CELL_COORD_LOWPRECISION =   (1 << 16),
		SPROP_CELL_COORD_INTEGRAL =       (1 << 17),
		SPROP_CHANGES_OFTEN =             (1 << 18),
		SPROP_NUMFLAGBITS_NETWORKED =           19,
		SPROP_ENCODED_AGAINST_TICKCOUNT = (1 << 19),
		SPROP_NUMFLAGBITS =                     20,
	#else
		#error
	#endif
	#if GSDK_CHECK_BRANCH_VER(GSDK_ENGINE_BRANCH_2007, >=, GSDK_ENGINE_BRANCH_2007_V0)
		SPROP_VARINT =                    SPROP_NORMAL,
	#endif
	};

	class DVariant
	{
	public:
		inline DVariant() noexcept
		{
			std::memset(m_data, 0, sizeof(m_data));
		}

		union
		{
			float m_Float;
			int m_Int;
			const char *m_pString;
			void *m_ptr;
		#if 0
			float m_Vector[4];
		#else
			float m_Vector[3];
		#endif
		#ifdef GSDK_DT_SUPPORTS_INT64
			long long m_Int64;
		#endif
			unsigned char m_data[sizeof(float) * 3];
		};

		SendPropType m_Type{DPT_Int};

	private:
		DVariant(const DVariant &) = delete;
		DVariant &operator=(const DVariant &) = delete;
		DVariant(DVariant &&) = delete;
		DVariant &operator=(DVariant &&) = delete;
	};

	static_assert(sizeof(DVariant) == (sizeof(Vector) + sizeof(SendPropType)));
}
