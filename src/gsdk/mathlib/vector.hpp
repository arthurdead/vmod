#pragma once

//#define GSDK_ALIGN_VECTOR

namespace gsdk
{
	class Vector2D;
	class Vector4D;
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

		vec_t x{0.0f};
		vec_t y{0.0f};
		vec_t z{0.0f};

	private:
		Vector() = delete;
		Vector(const Vector &) = delete;
		Vector &operator=(const Vector &) = delete;
		Vector(Vector &&) = delete;
		Vector &operator=(Vector &&) = delete;
	};

	class QAngle
	{
	public:
		inline vec_t *data() noexcept
		{ return reinterpret_cast<vec_t *>(this); }

		vec_t x{0.0f};
		vec_t y{0.0f};
		vec_t z{0.0f};

	private:
		QAngle() = delete;
		QAngle(const QAngle &) = delete;
		QAngle &operator=(const QAngle &) = delete;
		QAngle(QAngle &&) = delete;
		QAngle &operator=(QAngle &&) = delete;
	};
}
