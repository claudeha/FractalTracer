#pragma once

#include <algorithm>

#include "SceneObject.h"



// Base class for analytically / hand-derived distance estimated (DE) objects
struct AnalyticDEObject : public SceneObject
{
	vec3r centre = { 0, 0, 0 };
	real  radius = 1; 
	real  step_scale = 1; // Method of last resort to prevent overstepping


	// Get the distance estimate for point p in object space
	virtual real getDE(const vec3r & p_os) noexcept = 0;

	// Numeric normal vector calculation by forward differencing
	virtual vec3r getNormal(const vec3r & p) noexcept override final
	{
		const vec3r p_os = p - centre;
#if USE_DOUBLE
		const real DE_s = 1e-8f;
#else
		const real DE_s = 1e-4f;
#endif
		const real DE_0 = getDE(p_os);
		const vec3r grad =
		{
			getDE(p_os + vec3r{ DE_s, 0, 0 }) - DE_0,
			getDE(p_os + vec3r{ 0, DE_s, 0 }) - DE_0,
			getDE(p_os + vec3r{ 0, 0, DE_s }) - DE_0
		};
		return normalise(grad);
	}

	virtual real intersect(const Ray & r) noexcept override final
	{
		const vec3r s = r.o - centre;
		const real  b = dot(s, r.d);
		const real  c = dot(s, s) - radius * radius;

		const real discriminant = b * b - c;
		if (discriminant < 0)
			return -1;

		// Compute bounding interval
		const real t1 = -b - std::sqrt(discriminant);
		const real t2 = -b + std::sqrt(discriminant);
		if (t2 <= ray_epsilon) return -1;

		// Ray could be inside bounding sphere, start from ray epsilon
		real t = std::max(ray_epsilon, t1);
		while (t < t2)
		{
			const vec3r p_os = s + r.d * t;
			const real DE = getDE(p_os) * step_scale;
			t += DE;

			// If we're close enough to the surface, return a valid intersection
			if (DE < DE_thresh)
				return t;
		}

		return -1; // No intersection found
	}
};
