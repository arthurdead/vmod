#pragma once

//#define GSDK_ALIGN_VECTOR

#include <cstddef>
#include "mathlib.hpp"

namespace gsdk
{
	class Vector2D;
	class Vector4D;
	class Quaternion;

	using vec_t = float;

	class QAngle;

#ifdef GSDK_ALIGN_VECTOR
	class alignas(16) Vector
#else
	class Vector
#endif
	{
	public:
		constexpr Vector() noexcept = default;
		constexpr Vector(const Vector &) noexcept = default;
		constexpr Vector &operator=(const Vector &) noexcept = default;
		constexpr Vector(Vector &&) noexcept = default;
		constexpr Vector &operator=(Vector &&) noexcept = default;

		constexpr inline Vector(vec_t x_, vec_t y_, vec_t z_) noexcept
			: x{x_}, y{y_}, z{z_}
		{
		}

		inline vec_t operator[](std::size_t i) const noexcept
		{ return reinterpret_cast<const vec_t *>(this)[i]; }
		inline vec_t &operator[](std::size_t i) noexcept
		{ return reinterpret_cast<vec_t *>(this)[i]; }

		constexpr inline Vector &operator+=(const Vector &other) noexcept
		{
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}

		constexpr inline Vector &operator+=(vec_t other) noexcept
		{
			x += other;
			y += other;
			z += other;
			return *this;
		}

		constexpr inline Vector &operator-=(const Vector &other) noexcept
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}

		constexpr inline Vector &operator-=(vec_t other) noexcept
		{
			x -= other;
			y -= other;
			z -= other;
			return *this;
		}

		constexpr inline Vector &operator*=(const Vector &other) noexcept
		{
			x *= other.x;
			y *= other.y;
			z *= other.z;
			return *this;
		}

		constexpr inline Vector &operator*=(vec_t other) noexcept
		{
			x *= other;
			y *= other;
			z *= other;
			return *this;
		}

		constexpr inline Vector &operator/=(const Vector &other) noexcept
		{
			x /= other.x;
			y /= other.y;
			z /= other.z;
			return *this;
		}

		constexpr inline Vector &operator/=(vec_t other) noexcept
		{
			x /= other;
			y /= other;
			z /= other;
			return *this;
		}

		vec_t x{0.0f};
		vec_t y{0.0f};
		vec_t z{0.0f};

		inline float length() const noexcept
		{
			return FastSqrt(x*x + y*y + z*z + FLT_EPSILON);
		}

		constexpr inline float length_sqr() const noexcept
		{
			return x*x + y*y + z*z;
		}

		inline float length2d() const noexcept
		{
			return FastSqrt(x*x + y*y);
		}

		constexpr inline float length2d_sqr() const noexcept
		{
			return x*x + y*y;
		}

		constexpr inline float dot(const Vector &b) const noexcept
		{
			return x*b.x + y*b.y + z*b.z;
		}

		constexpr inline Vector cross(const Vector &b) const noexcept
		{
			return Vector(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x);
		}

		inline void normalize() noexcept
		{
			float iradius = 1.0f / ( __builtin_sqrtf(x*x + y*y + z*z) + FLT_EPSILON );

			x *= iradius;
			y *= iradius;
			z *= iradius;
		}

		inline QAngle angles() const noexcept;
	};

	class QAngle
	{
	public:
		constexpr QAngle() noexcept = default;
		constexpr QAngle(const QAngle &) noexcept = default;
		constexpr QAngle &operator=(const QAngle &) noexcept = default;
		constexpr QAngle(QAngle &&) noexcept = default;
		constexpr QAngle &operator=(QAngle &&) noexcept = default;

		constexpr inline QAngle(vec_t x_, vec_t y_, vec_t z_) noexcept
			: x{x_}, y{y_}, z{z_}
		{
		}

		inline vec_t operator[](std::size_t i) const noexcept
		{ return reinterpret_cast<const vec_t *>(this)[i]; }
		inline vec_t &operator[](std::size_t i) noexcept
		{ return reinterpret_cast<vec_t *>(this)[i]; }

		constexpr inline QAngle &operator+=(const QAngle &other) noexcept
		{
			x = AngleNormalize(x + other.x);
			y = AngleNormalize(y + other.y);
			z = AngleNormalize(z + other.z);
			return *this;
		}

		constexpr inline QAngle &operator+=(vec_t other) noexcept
		{
			x = AngleNormalize(x + other);
			y = AngleNormalize(y + other);
			z = AngleNormalize(z + other);
			return *this;
		}

		constexpr inline QAngle &operator-=(const QAngle &other) noexcept
		{
			x = AngleNormalize(x - other.x);
			y = AngleNormalize(y - other.y);
			z = AngleNormalize(z - other.z);
			return *this;
		}

		constexpr inline QAngle &operator-=(vec_t other) noexcept
		{
			x = AngleNormalize(x - other);
			y = AngleNormalize(y - other);
			z = AngleNormalize(z - other);
			return *this;
		}

		constexpr inline QAngle &operator*=(const QAngle &other) noexcept
		{
			x = AngleNormalize(x * other.x);
			y = AngleNormalize(y * other.y);
			z = AngleNormalize(z * other.z);
			return *this;
		}

		constexpr inline QAngle &operator*=(vec_t other) noexcept
		{
			x = AngleNormalize(x * other);
			y = AngleNormalize(y * other);
			z = AngleNormalize(z * other);
			return *this;
		}

		constexpr inline QAngle &operator/=(const QAngle &other) noexcept
		{
			x = AngleNormalize(x / other.x);
			y = AngleNormalize(y / other.y);
			z = AngleNormalize(z / other.z);
			return *this;
		}

		constexpr inline QAngle &operator/=(vec_t other) noexcept
		{
			x = AngleNormalize(x / other);
			y = AngleNormalize(y / other);
			z = AngleNormalize(z / other);
			return *this;
		}

		vec_t x{0.0f};
		vec_t y{0.0f};
		vec_t z{0.0f};

		Vector forward() const noexcept
		{
			float sp;
			float cp;
			SinCos(DEG2RAD(x), &sp, &cp);
			float sy;
			float cy;
			SinCos(DEG2RAD(y), &sy, &cy);
			return Vector(cp*cy, cp*sy, -sp);
		}

		Vector left() const noexcept
		{
			float sp;
			float cp;
			SinCos(DEG2RAD(x), &sp, &cp);
			float sy;
			float cy;
			SinCos(DEG2RAD(y), &sy, &cy);
			float sr;
			float cr;
			SinCos(DEG2RAD(z), &sr, &cr);
			return Vector(sr*sp*cy+cr*-sy, sr*sp*sy+cr*cy, sr*cp);
		}

		Vector right() const noexcept
		{
			float sp;
			float cp;
			SinCos(DEG2RAD(x), &sp, &cp);
			float sy;
			float cy;
			SinCos(DEG2RAD(y), &sy, &cy);
			float sr;
			float cr;
			SinCos(DEG2RAD(z), &sr, &cr);
			return Vector(-1*sr*sp*cy+-1*cr*-sy, -1*sr*sp*sy+-1*cr*cy, -1*sr*cp);
		}

		Vector up() const noexcept
		{
			float sp;
			float cp;
			SinCos(DEG2RAD(x), &sp, &cp);
			float sy;
			float cy;
			SinCos(DEG2RAD(y), &sy, &cy);
			float sr;
			float cr;
			SinCos(DEG2RAD(z), &sr, &cr);
			return Vector(cr*sp*cy+-sr*-sy, cr*sp*sy+-sr*cy, cr*cp);
		}
	};

	inline QAngle Vector::angles() const noexcept
	{
		QAngle ang;

		if(CloseEnough(y, 0.0f) && CloseEnough(x, 0.0f)) {
			ang.y = 0.0f;
			if(z > 0.0f) {
				ang.x = 270.0f;
			} else {
				ang.x = 90.0f;
			}
		} else {
			ang.y = (__builtin_atan2f(y, x) * RAD2DEG_PI);
			if(ang.y < 0.0f) {
				ang.y += 360.0f;
			}

			float tmp = FastSqrt(x*x + y*y);
			ang.x = (__builtin_atan2f(-z, tmp) * RAD2DEG_PI);
			if(ang.x < 0.0f) {
				ang.x += 360.0f;
			}
		}

		ang.z = 0.0f;

		return ang;
	}
}
