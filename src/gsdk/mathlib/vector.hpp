#pragma once

//#define GSDK_ALIGN_VECTOR

namespace gsdk
{
	using vec_t = float;

#ifdef GSDK_ALIGN_VECTOR
	class alignas(16) Vector
#else
	class Vector
#endif
	{
	public:
		vec_t x, y, z;
	};

	class QAngle
	{
	public:
		vec_t x, y, z;
	};
}
