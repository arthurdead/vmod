#pragma once

#include <cmath>

namespace gsdk
{
	inline float AngleNormalize(float angle) noexcept
	{
		angle = fmodf(angle, 360.0f);

		if(angle > 180.0f) {
			angle -= 360.0f;
		}

		if(angle < -180.0f) {
			angle += 360.0f;
		}

		return angle;
	}
}
