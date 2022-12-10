#pragma once

//#define GSDK_ALIGN_VECTOR

namespace gsdk
{
	class Vector2D;
	class Quaternion;

	using vec_t = float;

#ifdef GSDK_ALIGN_VECTOR
	class alignas(16) Vector
#else
	class Vector
#endif
	{
	public:
		inline vec_t *data() noexcept
		{ return reinterpret_cast<vec_t *>(this); }

		vec_t x, y, z;
	};

	class QAngle
	{
	public:
		vec_t x, y, z;
	};
}
