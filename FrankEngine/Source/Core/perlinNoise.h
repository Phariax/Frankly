////////////////////////////////////////////////////////////////////////////////////////

/* coherent noise function over 1, 2 or 3 dimensions */
/* (copyright Ken Perlin) */

////////////////////////////////////////////////////////////////////////////////////////

#ifndef PERLIN_NOISE_H
#define PERLIN_NOISE_H

namespace PerlineNoise
{
	double noise1(double arg);
	float noise2(float vec[2]);
	float noise3(float vec[3]);

	inline double noise(double arg)			{ return noise1(arg); }
	inline float noise(const Vector2& v)	{ float p[2] = {v.x, v.y}; return noise2(p); }
	inline float noise(const Vector3& v)	{ float p[3] = {v.x, v.y, v.z}; return noise3(p); }
}

#endif // PERLIN_NOISE_H