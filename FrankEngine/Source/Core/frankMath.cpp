////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Math Lib
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../core/frankMath.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	Math Lib Globals
*/
////////////////////////////////////////////////////////////////////////////////////////

// random seed
unsigned int FrankRand::randomSeed = 0;

float FrankRand::GetGaussian(float variance, float mean)
{
	// Marsaglia Polar Method
	// http://en.wikipedia.org/wiki/Marsaglia_polar_method

	static bool hasSpare = false;
	static float spare;
 
	if (hasSpare)
	{
		hasSpare = false;
		return mean + variance * spare;
	}
 
	hasSpare = true;
	float u, v, s;

	do
	{
		u = RAND_BETWEEN(-1.0f, 1.0f);
		v = RAND_BETWEEN(-1.0f, 1.0f);
		s = u * u + v * v;
	}
	while (s >= 1 || s <= 0);
 
	s = sqrtf(-2.0f * logf(s) / s);
	spare = v * s;
	return mean + variance * u * s;
}

// GameTimer
long long GameTimer::gameTime = 0;
float GameTimer::extraTime = 0;
const float GameTimer::conversion = 1000000;
const long long GameTimer::invalidTime = LLONG_MAX;
long long GamePauseTimer::gameTime = 0;
const long long GamePauseTimer::invalidTime = LLONG_MAX;
const float GamePauseTimer::conversion = 1000000;

/*float Vector3::DistanceBetweenLineSegementsSquared(const Vector3& lineStart1, const Vector3& lineEnd1, const Vector3& lineStart2, const Vector3& lineEnd2)
{
	// http://geomalgorithms.com/a07-_distance.html

	// this code does not work yet
	Vector3 u = (lineEnd1 - lineStart1).Normalize();
	Vector3 v = (lineEnd2 - lineStart2).Normalize();
	Vector3 w0 = lineStart1 - lineStart2;

	float a = u.Dot(u);
	float b = u.Dot(v);
	float c = v.Dot(v);
	float d = u.Dot(w0);
	float e = v.Dot(w0);

	Vector3 deltaClosest = w0 + ((b*e-c*d)*u - (a*e-b*d)*v) / (a*c - b*b);
	float distanceSquared = deltaClosest.MagnitudeSquared();

	return distanceSquared;
}*/

////////////////////////////////////////////////////////////////////////////////////////
/*
	Debug Rendering
*/
////////////////////////////////////////////////////////////////////////////////////////

void Vector2::RenderDebug(const Color& color, float radius, float time) const
{
	g_debugRender.RenderPoint(*this, color, radius, time);
}

void Vector3::RenderDebug(const Color& color, float radius, float time) const
{
	g_debugRender.RenderPoint(*this, color, radius, time);
}
	
void XForm2::RenderDebug(const Color& color, float size, float time) const
{
	g_debugRender.RenderLine(position, this->TransformCoord(Vector2(size, 0)), color, time);
	g_debugRender.RenderLine(position, this->TransformCoord(Vector2(0, size)), color, time);
	//g_debugRender.RenderLine(position, GetUp()*size, color, time);
	//g_debugRender.RenderLine(position, GetRight()*size, color, time);
}

void Circle::RenderDebug(const Color& color, float time) const
{
	g_debugRender.RenderCircle(*this, color, time);
}

void Sphere::RenderDebug(const Color& color, float time) const
{
	g_debugRender.RenderPoint(position, color, radius, time);
}

void Box2AABB::RenderDebug(const Color& color, float time) const
{
	g_debugRender.RenderBox(*this, color, time);
}

void Line2::RenderDebug(const Color& color, float time) const
{
	g_debugRender.RenderLine(p1, p2, color, time);
}

void Line3::RenderDebug(const Color& color, float time) const
{
	g_debugRender.RenderLine(p1, p2, color, time);
}
