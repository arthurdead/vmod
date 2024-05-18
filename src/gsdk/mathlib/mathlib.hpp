#pragma once

#include <cmath>
#include <cfloat>

namespace gsdk
{
	#pragma push_macro("M_PI")
	#undef M_PI
	constexpr inline double M_PI{3.14159265358979323846};
	#pragma push_macro("M_PI")

	constexpr inline float M_PI_F{3.14159265358979323846f};

	constexpr inline float DEG2RAD_PI{M_PI_F / 180.0f};
	constexpr inline float RAD2DEG_PI{180.0f / M_PI_F};

	constexpr inline float DEG2RAD(float x) noexcept
	{
		return (x * DEG2RAD_PI);
	}

	constexpr inline float EQUAL_EPSILON{0.001f};

	inline bool CloseEnough( float a, float b, float epsilon = EQUAL_EPSILON )
	{
		return __builtin_fabsf( a - b ) <= epsilon;
	}

	constexpr inline float AngleNormalize(float angle) noexcept
	{
		angle = __builtin_fmodf(angle, 360.0f);

		if(angle > 180.0f) {
			angle -= 360.0f;
		}

		if(angle < -180.0f) {
			angle += 360.0f;
		}

		return angle;
	}

	inline void SinCos(float x, float *sine, float *cosine) noexcept
	{
	#ifdef __clang__
		::sincosf(x, sine, cosine);
	#else
		__builtin_sincosf(x, sine, cosine);
	#endif
	}

	inline float FastSqrt(float x) noexcept
	{
		return __builtin_sqrtf(x);
	}
}
