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

		inline Vector(vec_t x_, vec_t y_, vec_t z_) noexcept
			: x{x_}, y{y_}, z{z_}
		{
		}

		inline vec_t operator[](std::size_t i) const noexcept
		{ return reinterpret_cast<const vec_t *>(this)[i]; }
		inline vec_t &operator[](std::size_t i) noexcept
		{ return reinterpret_cast<vec_t *>(this)[i]; }

		inline Vector &operator+=(const Vector &other) noexcept
		{
			x += other.x;
			y += other.y;
			z += other.z;
			return *this;
		}

		inline Vector &operator+=(vec_t other) noexcept
		{
			x += other;
			y += other;
			z += other;
			return *this;
		}

		inline Vector &operator-=(const Vector &other) noexcept
		{
			x -= other.x;
			y -= other.y;
			z -= other.z;
			return *this;
		}

		inline Vector &operator-=(vec_t other) noexcept
		{
			x -= other;
			y -= other;
			z -= other;
			return *this;
		}

		inline Vector &operator*=(const Vector &other) noexcept
		{
			x *= other.x;
			y *= other.y;
			z *= other.z;
			return *this;
		}

		inline Vector &operator*=(vec_t other) noexcept
		{
			x *= other;
			y *= other;
			z *= other;
			return *this;
		}

		inline Vector &operator/=(const Vector &other) noexcept
		{
			x /= other.x;
			y /= other.y;
			z /= other.z;
			return *this;
		}

		inline Vector &operator/=(vec_t other) noexcept
		{
			x /= other;
			y /= other;
			z /= other;
			return *this;
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

		inline QAngle(vec_t x_, vec_t y_, vec_t z_) noexcept
			: x{x_}, y{y_}, z{z_}
		{
		}

		inline vec_t operator[](std::size_t i) const noexcept
		{ return reinterpret_cast<const vec_t *>(this)[i]; }
		inline vec_t &operator[](std::size_t i) noexcept
		{ return reinterpret_cast<vec_t *>(this)[i]; }

		inline QAngle &operator+=(const QAngle &other) noexcept
		{
			x = AngleNormalize(x + other.x);
			y = AngleNormalize(y + other.y);
			z = AngleNormalize(z + other.z);
			return *this;
		}

		inline QAngle &operator+=(vec_t other) noexcept
		{
			x = AngleNormalize(x + other);
			y = AngleNormalize(y + other);
			z = AngleNormalize(z + other);
			return *this;
		}

		inline QAngle &operator-=(const QAngle &other) noexcept
		{
			x = AngleNormalize(x - other.x);
			y = AngleNormalize(y - other.y);
			z = AngleNormalize(z - other.z);
			return *this;
		}

		inline QAngle &operator-=(vec_t other) noexcept
		{
			x = AngleNormalize(x - other);
			y = AngleNormalize(y - other);
			z = AngleNormalize(z - other);
			return *this;
		}

		inline QAngle &operator*=(const QAngle &other) noexcept
		{
			x = AngleNormalize(x * other.x);
			y = AngleNormalize(y * other.y);
			z = AngleNormalize(z * other.z);
			return *this;
		}

		inline QAngle &operator*=(vec_t other) noexcept
		{
			x = AngleNormalize(x * other);
			y = AngleNormalize(y * other);
			z = AngleNormalize(z * other);
			return *this;
		}

		inline QAngle &operator/=(const QAngle &other) noexcept
		{
			x = AngleNormalize(x / other.x);
			y = AngleNormalize(y / other.y);
			z = AngleNormalize(z / other.z);
			return *this;
		}

		inline QAngle &operator/=(vec_t other) noexcept
		{
			x = AngleNormalize(x / other);
			y = AngleNormalize(y / other);
			z = AngleNormalize(z / other);
			return *this;
		}

		vec_t x{0.0f};
		vec_t y{0.0f};
		vec_t z{0.0f};
	};
}
