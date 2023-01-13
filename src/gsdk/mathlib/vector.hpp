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
		Vector() noexcept = default;
		Vector(const Vector &) noexcept = default;
		Vector &operator=(const Vector &) noexcept = default;
		Vector(Vector &&) noexcept = default;
		Vector &operator=(Vector &&) noexcept = default;

		inline Vector(vec_t x_ = 0.0f, vec_t y_ = 0.0f, vec_t z_ = 0.0f) noexcept
			: x{x_}, y{y_}, z{z_}
		{
		}

		vec_t x{0.0f};
		vec_t y{0.0f};
		vec_t z{0.0f};
	};

	class QAngle
	{
	public:
		QAngle() noexcept = default;
		QAngle(const QAngle &) noexcept = default;
		QAngle &operator=(const QAngle &) noexcept = default;
		QAngle(QAngle &&) noexcept = default;
		QAngle &operator=(QAngle &&) noexcept = default;

		inline QAngle(vec_t x_ = 0.0f, vec_t y_ = 0.0f, vec_t z_ = 0.0f) noexcept
			: x{x_}, y{y_}, z{z_}
		{
		}

		vec_t x{0.0f};
		vec_t y{0.0f};
		vec_t z{0.0f};
	};
}
