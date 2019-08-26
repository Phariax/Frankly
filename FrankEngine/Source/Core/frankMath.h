////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Math Lib
	Copyright 2013 Frank Force - http://www.frankforce.com

	- full math libray for use with frank engine
	- most of these are just wrappers that extend the DirectX implementation
	- Vector2 extends functionality of Box2AABBd's b2Vec2 class
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRANK_MATH_H
#define FRANK_MATH_H

#include "box2d/Box2D.h"

namespace FrankMath {

using namespace std;

////////////////////////////////////////////////////////////////////////////////////////
/*
	Forward Declarations & Type Defs
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Vector2;
struct IntVector2;
struct ByteVector2;
struct Vector3;
struct Matrix44;
struct Quaternion;
struct Sphere;
struct Circle;
struct Line3;
struct Line2;
struct XForm2;
struct Color;

////////////////////////////////////////////////////////////////////////////////////////
/*
	Numerical Constants
*/
////////////////////////////////////////////////////////////////////////////////////////

#define FRANK_EPSILON		(0.001f)
#define PI					(3.14159265359f)
#define NATURAL_LOG			(2.71828182846f)
#define ROOT_2				(1.41421356237f)

////////////////////////////////////////////////////////////////////////////////////////
/*
	Asserts and Debug Functions
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifdef _FINALRELEASE
#define _ASSERTS_DISABLE
#endif

// basic assert functionality
#ifndef _ASSERTS_DISABLE
#define ASSERT(e) assert(e)
#else
#define ASSERT(unused) {}
#endif

////////////////////////////////////////////////////////////////////////////////////////
/*
	Useful functions
*/
////////////////////////////////////////////////////////////////////////////////////////

// min, max and capping functions
template <class T> const T& Max(const T& a, const T& b) { return (a > b) ? a: b; }
template <class T> const T& Min(const T& a, const T& b) { return (a < b) ? a: b; }
template <class T> const T& Cap   (const T& v, const T& min, const T& max);

// cap value beween 0 and 1
float CapPercent(float p);

// Cap a wrapped percent value between 0 and 1
float CapWrappedPercent(float p);

// cap angle between -PI and PI
float CapAngle(float a);

// square a value
inline float Square(float v) { return v*v; }

// cue a value
inline float Cube(float v) { return v*v*v; }

inline float GetSign(float v) { return (v==0)? 0.0f : ((v > 0.0f)? 1.0f : -1.0f); }

template <class T> const T Lerp(const float percent, const T&a, const T&b)
{ return a + CapPercent(percent) * (b - a); }

// get what percent value is when it's 0 at a and 1 at b
template <class T> const float Percent(const T& value, const T& a, const T& b)
{ return (a == b)? 0 : CapPercent((value - a) / (b - a)); }

// get percent between a and b, followed by a lerp between c and d
template <class T, class S>
const S PercentLerp(const T& value, const T&a, const T&b, const S&c, const S&d)
{ return Lerp(Percent(value, a, b), c, d); }

inline float PercentSinWave(float a) { return CapPercent(0.5f + 0.5f * sinf(a)); }

// get the number of elements in an array
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(*a))

// generate a sin pulse based off a time, interval and offset
float SinPulse(float time, float interval, float offset = 0);

// convert to and from radians
inline float RadiansToDegrees(float a)		{ return a * (180 / PI); }
inline float DegreesToRadians(float a)		{ return a * (PI / 180); }

inline unsigned Log2Int(unsigned x)
{
	int result = 0;
	while (x >>= 1) ++result;
	return result; 
}

// defines to force alignment
#define FF_ALIGN_16 __declspec(align(16))
#define FF_ALIGN_32 __declspec(align(32))

////////////////////////////////////////////////////////////////////////////////////////
/*
	Base class that can't be coppied
*/
////////////////////////////////////////////////////////////////////////////////////////

class Uncopyable
{
protected:
	Uncopyable() {}
	~Uncopyable() {}

private:
	Uncopyable(const Uncopyable&);
	Uncopyable& operator=(const Uncopyable&);
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Colors
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Color : D3DXCOLOR
{
	Color() {}
	Color( FLOAT r, FLOAT g, FLOAT b, FLOAT a = 1 );
	Color( const D3DXCOLOR& c );
	Color( const b2Color& c, FLOAT a = 1 );

	const Color CapValues() const;
	const Color HSVtoRGB() const;
	const Color GetInverse() const { return Color(1 - r, 1 - g, 1 - b, a); }
	static const Color BuildBytes( UINT8 r, UINT8 g, UINT8 b, UINT8 a = 255 );
	static const Color BuildHSV(float h, float s=1, float v=1, float a = 1);

	static const Color RandBetween(const Color& c1, const Color& c2);
	static const Color RandBetweenComponents(const Color& c1, const Color& c2);

	const Color ScaleColor(float scale) const { return Color(r*scale, g*scale, b*scale, a); }
	const Color ScaleAlpha(float scale) const { return Color(r, g, b, a*scale); }

	///////////////////////////////////////
    // Class Statics
	///////////////////////////////////////

	static const Color Red		(float a=1, float v=1)	{ return Color(v, 0, 0, a); }
	static const Color Orange	(float a=1, float v=1)	{ return Color(v, v*0.5f, 0, a); }
	static const Color Yellow	(float a=1, float v=1)	{ return Color(v, v, 0, a); }
	static const Color Green	(float a=1, float v=1)	{ return Color(0, v, 0, a); }
	static const Color Cyan		(float a=1, float v=1)	{ return Color(0, v, v, a); }
	static const Color Blue		(float a=1, float v=1)	{ return Color(0, 0, v, a); }
	static const Color Purple	(float a=1, float v=1)	{ return Color(v*0.7f, 0, v, a); }
	static const Color Magenta	(float a=1, float v=1)	{ return Color(v, 0, v, a); }
	static const Color White	(float a=1)				{ return Color(1, 1, 1, a); }
	static const Color Black	(float a=1)				{ return Color(0, 0, 0, a); }
	static const Color Clear	()						{ return Color(0, 0, 0, 0); }
	static const Color Grey		(float a=1, float v=0.5f) { return Color(v, v, v, a); }
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Random Numbers
*/
////////////////////////////////////////////////////////////////////////////////////////

struct FrankRand
{
	static void SetSeed(unsigned int newSeed);
	static unsigned int GetInt();
	static int GetInt(int min, int max);
	static float GetFloat();
	static float GetFloat(float min, float max);
	static int GetSign();
	static const Color GetColor();
	static float GetAngle();
	static bool RollDie(unsigned int sides);
	static float GetGaussian(float variance = 1, float mean = 0);

private:

	// make sure no one can create an object of this type
	FrankRand() {}

	static unsigned int randomSeed;
	static const unsigned int maxRand = 0xffffffff;
};

// quick rand functions
#define RAND_BETWEEN(min, max)		(FrankRand::GetFloat(min, max))
#define RAND_INT					(FrankRand::GetInt())
#define RAND_INT_BETWEEN(min, max)	(FrankRand::GetInt(min, max))
#define RAND_PERCENT				(FrankRand::GetFloat())
#define RAND_SIGN					(FrankRand::GetSign())
#define RAND_DIE(sides)				(FrankRand::RollDie(sides))
#define RAND_COLOR					(FrankRand::GetColor())
#define RAND_ANGLE					(FrankRand::GetAngle())

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Timer

	simple timer class to automatically keep track of timers
	- uses miliseconds under the hood to keep track of time for maximum accuracy
	- converts to float seconds value automatically
	- also keeps a left over time value so interpolation is automatically taken into account
*/
////////////////////////////////////////////////////////////////////////////////////////

struct GameTimer
{
	GameTimer()										: startTime(invalidTime) {}
	explicit GameTimer(float currentTime)			: startTime(gameTime - (long long)(conversion * currentTime)) {}

	void Set(float currentTime = 0)					{ startTime = gameTime - (long long)(conversion * currentTime); }
	bool HasElapsed()								{ return IsValid()? (gameTime + extraTime) > startTime : false; }
	void Invalidate()								{ startTime = invalidTime; }
	bool IsValid() const							{ return startTime != invalidTime; }
	operator float () const							{ return IsValid()? GetTimeInternal() : 0; }

	static void ResetGlobal()						{ gameTime = 0; }
	static void UpdateGlobal(float timeDelta)		{ gameTime += (long long)(conversion * timeDelta); }
	static void SetLeftOverGlobal(float _extraTime)	{ extraTime = _extraTime; }
	static float GetTimeGlobal()					{ return gameTime/conversion; }

protected:

	float GetTimeInternal() const					{ ASSERT(IsValid()); return (gameTime - startTime)/conversion + extraTime; }
	
	long long startTime;
	static long long gameTime;
	static float extraTime;
	static const long long invalidTime;
	static const float conversion;
};	

// basically the same as GameTimer except it keeps running even when the game is paused
// this is useful for interface type stuff
struct GamePauseTimer
{

	GamePauseTimer()								: startTime(invalidTime) {}
	explicit GamePauseTimer(float currentTime)		: startTime(gameTime - (long long)(conversion * currentTime)) {}

	void Set(float currentTime = 0)					{ startTime = gameTime - (long long)(conversion * currentTime); }
	bool HasElapsed()								{ return IsValid()? gameTime > startTime : false; }
	void Invalidate()								{ startTime = invalidTime; }
	bool IsValid() const							{ return startTime != invalidTime; }
	operator float () const							{ return IsValid()? GetTimeInternal() : 0; }

	static void ResetGlobal()						{ gameTime = 0; }
	static void UpdateGlobal(double timeDelta)		{ gameTime += (long long)(conversion * timeDelta); }
	static float GetTimeGlobal()					{ return gameTime/conversion; }

protected:

	float GetTimeInternal() const					{ ASSERT(IsValid()); return (gameTime - startTime)/conversion; }
	
	long long startTime;
	static long long gameTime;
	static const long long invalidTime;
	static const float conversion;
};	

struct GameTimerPercent : public GameTimer
{
	GameTimerPercent()													: maxTime(0) {}
	explicit GameTimerPercent(float _maxTime, float currentTime = 0)	: maxTime(_maxTime), GameTimer(currentTime)  {}

	void Reset()									{ startTime = gameTime; }
	void Set(float _maxTime, float currentTime = 0)	{ maxTime = _maxTime; GameTimer::Set(currentTime); }
	operator float () const							{ return !IsValid()? 0 : CapPercent(GetTimeInternal() / maxTime); }
	bool HasElapsed() const							{ return IsValid() && (GetTimeInternal() >= maxTime); }
	float GetTimeLeft() const						{ return IsValid()? (maxTime - GetTimeInternal()) : 0; }
	float GetTimePast() const						{ return IsValid()? GetTimeInternal() : 0; }

private:

	float maxTime;
};

///////////////////////////////////////////////////////////////////////////////////////
/*
	ValueInterpolator
		
	- an easier way to make a value interpolated
*/
////////////////////////////////////////////////////////////////////////////////////////

template <class T>
struct ValueInterpolator
{
	ValueInterpolator() {}
	explicit ValueInterpolator(const T& v) : value(v), valueLast(v) {}
	
	operator T&()							{ return value; }
	operator const T&() const				{ return value; }
	T& operator = (const T& v)				{ return value = v; }
	void Init(const T& v)					{ value = valueLast = v; }
	void SaveLast()							{ valueLast = value; }
	const T GetInterpolated(float p) const	{ return Lerp(p, value, valueLast); }
	T& Get()								{ return value; }
	T& GetLast()							{ return valueLast; }
	const T& Get() const					{ return value; }
	const T& GetLast() const				{ return valueLast; }

protected:

	T value;
	T valueLast;
};

struct AngleInterpolator : public ValueInterpolator<float>
{
	AngleInterpolator() : ValueInterpolator(0)	{}
	float operator = (const float v)			{ return value = v; }
	float GetInterpolated(float p) const		{ return value + CapPercent(p) * CapAngle(valueLast - value); }
};

///////////////////////////////////////////////////////////////////////////////////////
/*
	FadeTimer
*/
////////////////////////////////////////////////////////////////////////////////////////

template <class T>
struct FadeTimer
{
	FadeTimer(const T& _startValue = 0) :
		startValue(_startValue),
		endValue(startValue),
		timer(),
		fadeTime(0),
		delay(0)
	{}

	void Set(const T& _endValue, const float _fadeTime, float _delay = 0)
	{ 
		startValue = *this;
		endValue = _endValue;
		timer.Set();
		fadeTime = _fadeTime;
		delay = _delay;
	}

	operator const T () const
	{ 
		if (fadeTime == 0)
			return endValue;
		float percent = CapPercent((timer - delay) / fadeTime);
		return startValue + percent * (endValue - startValue);
	}

	const T GetEnd() const { return endValue; }

	bool HasElapsed() const
	{ return !timer.IsValid() || ((float)timer > fadeTime + delay); }

private:

	T startValue;
	T endValue;
	GameTimer timer;
	float fadeTime;
	float delay;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Vector2 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Vector2 : public b2Vec2
{
	Vector2();
	Vector2(const Vector2& v);
	Vector2(const IntVector2& v);
	Vector2(const ByteVector2& v);
	Vector2(float _x, float _y);
	Vector2(const b2Vec2& v);
	explicit Vector2(float _v);
	explicit Vector2(const Vector3& v);

	// angle of 0 radians is Vector(1, 0)
	static inline const Vector2 BuildFromAngle(float angle);
	static inline const Vector2 BuildRandomUnitVector();
	static inline const Vector2 BuildRandomInCircle(float radius = 1, float minRadius = 0);
	static inline const Vector2 BuildGaussian(const Vector2& variance = Vector2(1), const Vector2& mean = Vector2(0));
	const D3DXVECTOR3 GetD3DXVECTOR3(float z = 0) const;

	void RenderDebug(const Color& color = Color::White(0.5f), float radius = 0.1f, float time = 0.0f) const;

	///////////////////////////////////////
    // Basic Vector Math Functionality
	///////////////////////////////////////
	
	float GetAngle() const;
	float Magnitude() const						{ return (sqrtf(x*x + y*y)); }
	float MagnitudeSquared() const				{ return (x*x + y*y); }
	const Vector2 Normalize() const;
	Vector2& NormalizeThis();
	bool IsNormalized() const;
	const Vector2 Normalize(float magnitude) const;
	float MagnitudeAndNormalize();
	const Vector2 CapMagnitude(float maxMagnitude) const;
	Vector2& CapMagnitudeThis(float maxMagnitude);
	const Vector2 Floor() const;
	Vector2& FloorThis();
	const Vector2 Round() const;
	Vector2& RoundThis();

	float Dot(const Vector2& v) const			{ return (x*v.x + y*v.y); }
	float Cross(const Vector2& v) const			{ return (x*v.y - y*v.x); }
	const Vector2 Abs() const					{ return Vector2(fabs(x), fabs(y)); }
	const Vector2 ZeroThis()					{ return *this = Vector2(0,0); }
	bool IsZero() const							{ return (x == 0) & (y == 0); }
	float LengthManhattan()	const				{ return fabs(x) + fabs(y); }

	// returns angle betwen 2 vecs
	// result is between 0 and 2*PI
	float AngleBetween(const Vector2& v) const;
	
	// returns signed angle betwen 2 vecs
	// result is between -PI and PI
	float SignedAngleBetween(const Vector2& v) const;

	const Vector2 TransformCoord(const XForm2& xf) const;
	Vector2 &TransformThisCoord(const XForm2& xf);

	const Vector2 Rotate(const Vector2& v) const;
	Vector2& RotateThis(const Vector2& v);
	const Vector2 Rotate(float theta) const;
	Vector2& RotateThis(float theta);

	const Vector2 RotateRightAngle() const		{ return Invert(); }
	Vector2& RotateRightAngleThis()				{ return InvertThis(); }
	const Vector2 Invert() const				{ return Vector2(y, -x); }
	Vector2& InvertThis()						{ return *this = this->Invert(); }
	const Vector2 Reflect(const Vector2& normal) const;

	///////////////////////////////////////
	// Operators
	///////////////////////////////////////

	Vector2& operator += (const Vector2& v)				{ return *this = *this + v; }
	Vector2& operator -= (const Vector2& v)				{ return *this = *this - v; }
	Vector2& operator *= (const Vector2& v)				{ return *this = *this * v; }
	Vector2& operator /= (const Vector2& v)				{ return *this = *this / v; }
	Vector2& operator *= (float scale)					{ return *this = *this * scale; }
	Vector2& operator /= (float scale)					{ return *this = *this / scale; }
	const Vector2 operator - () const					{ return Vector2(-x, -y); }
	const Vector2 operator + (const Vector2& v) const	{ return Vector2(x + v.x, y + v.y); }
	const Vector2 operator - (const Vector2& v) const	{ return Vector2(x - v.x, y - v.y); }
	const Vector2 operator * (const Vector2& v) const	{ return Vector2(x * v.x, y * v.y);}
	const Vector2 operator / (const Vector2& v) const	{ return Vector2(x / v.x, y / v.y); }
	const Vector2 operator * (float scale) const		{ return Vector2(x * scale, y * scale); }
	const Vector2 operator / (float scale) const		{ float s = 1.0f/scale; return Vector2(x *s, y *s); }
	friend const Vector2 operator * (float scale, const Vector2& v) { return v * scale;}
	friend const Vector2 operator / (float scale, const Vector2& v) { return Vector2(scale) / v;}
	bool operator == (const Vector2& other) const { return x == other.x && y == other.y; }
	bool operator != (const Vector2& other) const { return !(*this == other); }

	///////////////////////////////////////
	// Tests
	///////////////////////////////////////

	// returns true and sets the point of intersection if segments intersect
	static bool LineSegmentIntersection( const Vector2& s1a, const Vector2& s1b, const Vector2& s2a, const Vector2& s2b, Vector2& point );
	static bool LineIntersection(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Vector2& v4, Vector2& point);

	// distance of point from line segment test
	float DistanceFromLineSegement(const Vector2& lineStart, const Vector2& lineEnd) const;
	static float DistanceFromLineSegement(const Vector2& lineStart, const Vector2& lineEnd, const Vector2& point);

	// returns shortest distance between 1 line segments
	static float DistanceBetweenLineSegments(const Vector2& segment1a, const Vector2& segment1b, const Vector2& segment2a, const Vector2& segment2b);

	// returns true if finite line segments intersect
	static bool LineSegmentIntersection( const Vector2& segment1a, const Vector2& segment1b, const Vector2& segment2a, const Vector2& segment2b);

	// returns true if verts form clockwise triangle
	static bool IsClockwise( const Vector2& v1, const Vector2& v2, const Vector2& v3);

	// returns true if directions are winding clockwise
	static bool IsClockwise(const Vector2& d1, const Vector2& d2) ;

	// returns true if point is inside box
	static inline bool InsideBox(const XForm2& xf, const Vector2& size, const Vector2& point);

	// returns true if point is inside clockwise winding convex polygon
	static inline bool InsideConvexPolygon(const Vector2* vertexList, int vertexCount, const Vector2& point);

	///////////////////////////////////////
	// Class Statics
	///////////////////////////////////////

	static const Vector2 Zero()		{ return Vector2(0, 0); }
	static const Vector2 XAxis()	{ return Vector2(1, 0); }
	static const Vector2 YAxis()	{ return Vector2(0, 1); }

	///////////////////////////////////////
    // Data
	///////////////////////////////////////

	// internals are handled by box2d now
	//float x;
	//float y;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	ByteVector2 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

struct ByteVector2
{
	ByteVector2() {}
	explicit ByteVector2(char _s): x(_s), y(_s) {}
	ByteVector2(char _x, char _y): x(_x), y(_y) {}
	explicit ByteVector2(int _s);
	ByteVector2(int _x, int _y);

	ByteVector2(const IntVector2& v);
	ByteVector2(const Vector2& v);
	bool IsZero() const { return (x == 0 && y == 0); }
	
	bool operator == (const ByteVector2& other) const { return x == other.x && y == other.y; }
	bool operator != (const ByteVector2& other) const { return !(*this == other); }

	char x;
	char y;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	IntVector2 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

struct IntVector2
{
	IntVector2() {}
	explicit IntVector2(int _s) : x(_s), y(_s) {}
	IntVector2(int _x, int _y) : x(_x), y(_y) {}
	IntVector2(const ByteVector2& v) : x(int(v.x)), y(int(v.y)) {}
	IntVector2(const Vector2& v) : x(int(v.x)), y(int(v.y)) {}

	///////////////////////////////////////
	// Operators
	///////////////////////////////////////

	IntVector2& operator += (const IntVector2& v)						{ return *this = *this + v; }
	IntVector2& operator -= (const IntVector2& v)						{ return *this = *this - v; }
	IntVector2& operator *= (const IntVector2& v)						{ return *this = *this * v; }
	IntVector2& operator /= (const IntVector2& v)						{ return *this = *this / v; }
	IntVector2& operator *= (int scale)									{ return *this = *this * scale; }
	IntVector2& operator /= (int scale)									{ return *this = *this / scale; }
	const IntVector2 operator - () const								{ return IntVector2(-x, -y); }
	const IntVector2 operator + (const IntVector2& v) const				{ return IntVector2(x + v.x, y + v.y); }
	const IntVector2 operator - (const IntVector2& v) const				{ return IntVector2(x - v.x, y - v.y); }
	const IntVector2 operator * (const IntVector2& v) const				{ return IntVector2(x * v.x, y * v.y);}
	const IntVector2 operator / (const IntVector2& v) const				{ return IntVector2(x / v.x, y / v.y); }
	const IntVector2 operator * (int s) const							{ return IntVector2(x * s, y * s); }
	const IntVector2 operator / (int s) const							{ return IntVector2(x / s, y /s); }
	friend const IntVector2 operator * (int scale, const IntVector2& v)	{ return v * scale;}
	friend const IntVector2 operator / (int scale, const IntVector2& v)	{ return IntVector2(scale) / v;}
	
	float Magnitude() const						{ return (sqrtf(float(x*x + y*y))); }
	int MagnitudeSquared() const				{ return (x*x + y*y); }
	int MagnitudeManhattan() const				{ return (abs(x) + abs(y)); }
	int MagnitudeChebyshev() const				{ return Max(abs(x), abs(y)); }

	static const IntVector2 BuildIntRotation(int rotation)
	{
		ASSERT(rotation >= 0 && rotation <= 3);
		switch (rotation)
		{
			default:
			case 0: return Vector2( 0, 1);
			case 1: return Vector2( 1, 0);
			case 2: return Vector2( 0,-1);
			case 3: return Vector2(-1, 0);
		};
	}

	bool operator == (const IntVector2& other) const { return x == other.x && y == other.y; }
	bool operator != (const IntVector2& other) const { return !(*this == other); }

	int x, y;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Vector3 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Vector3 : public D3DXVECTOR3
{
	// constructors to make the vector3
	Vector3() {}
	Vector3(const D3DXVECTOR3& v);
	Vector3(float _x, float _y, float _z);
	explicit Vector3(float _v) ;
	Vector3(const Vector2& v);

	const DWORD GetDWORD(float w = 0)
	{
		const Vector3 v = this->Normalize();
		DWORD r = (DWORD)( 127.0f * v.x + 128.0f );
		DWORD g = (DWORD)( 127.0f * v.y + 128.0f );
		DWORD b = (DWORD)( 127.0f * v.z + 128.0f );
		DWORD a = (DWORD)( 255.0f * w );
    
		return( (a<<24L) + (r<<16L) + (g<<8L) + (b<<0L) );
	}

	// quick way to get a randomized vector
	static const Vector3 BuildRandomQuick();

	// normalized evenly distributed vector, slower then quick version
	static inline Vector3 BuildRandomNormalized();

	void RenderDebug(const Color& color = Color::White(0.5f), float radius = 0.1f, float time = 0.0f) const;

	///////////////////////////////////////
    // Basic Math Functionality
	///////////////////////////////////////

	const Vector3 Normalize() const;
	Vector3& NormalizeThis();
	const Vector3 Normalize(float magnitude) const;
	bool IsNormalized() const;
	float Magnitude() const;
	float MagnitudeSquared() const;
	float MagnitudeAndNormalize();
	Vector3& ZeroThis();
	bool IsZero() const;

	// dot product
	float Dot(const Vector3& v) const;
    const Vector3 operator * (const Vector3& v) const;

	// cross product
	const Vector3 Cross(const Vector3& v) const;
    const Vector3 operator ^ (const Vector3& v) const;

	// returns angle betwen 2 vecs,
	// result is between 0 and 2*PI
	// (use cross product to get sign if necessary)
	float AngleBetween(const Vector3& v) const;

	// get the euler rotation this vector represents
	const Vector3 GetRotation() const;
	void CapRotation();

	///////////////////////////////////////
    // Operators
	///////////////////////////////////////

    Vector3& operator += (const Vector3& v);
    Vector3& operator -= (const Vector3& v);
    Vector3& operator *= (float scale);
    Vector3& operator /= (float scale);

    const Vector3 operator - () const;
    const Vector3 operator + (const Vector3& v) const;
    const Vector3 operator - (const Vector3& v) const;
    const Vector3 operator * (float scale) const;
    const Vector3 operator / (float scale) const;
    friend const Vector3 operator * (float scale, const Vector3& v);

	///////////////////////////////////////
    // Tests
	///////////////////////////////////////

	float DistanceFromLineSegementSquared(const Vector3& lineStart, const Vector3& lineEnd) const;

	// A slightly faster version where user provides pre calcualted deltapos
	// useful if you are doing many tests with the same start and end
	float DistanceFromLineSegementSquared(const Vector3& lineStart, const Vector3& lineEnd, const Vector3& deltaPos, float deltaPosMagSquared) const;

	// A slightly faster version where user provides pre calcualted deltapos
	// useful if you are doing many tests with the same start and end
	// this version also returns a percentage of the distance along the line
	float DistanceFromLineSegementSquared(const Vector3& lineStart, const Vector3& lineEnd, const Vector3& deltaPos, float deltaPosMagSquared, float& percentDistance) const;
	float DistanceFromLineSegement(const Vector3& lineStart, const Vector3& lineEnd) const;

	///////////////////////////////////////
    // Class Statics
	///////////////////////////////////////

	static const Vector3 Zero()	{ return Vector3(0, 0, 0); }
	static const Vector3 One()	{ return Vector3(1, 1, 1); }
	static const Vector3 XAxis()	{ return Vector3(1, 0, 0); }
	static const Vector3 YAxis()	{ return Vector3(0, 1, 0); }
	static const Vector3 ZAxis()	{ return Vector3(0, 0, 1); }
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Matrix44
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Matrix44
{
	Matrix44();
	Matrix44(const Matrix44& m);
	Matrix44(const D3DMATRIX& m);
	explicit Matrix44(const XForm2& xf, float z = 0);
	explicit Matrix44(const Vector2& v);
	explicit Matrix44(const Quaternion& q);

    float& operator () (int row, int column);
    float operator () (int row, int column) const;

	///////////////////////////////////////
    // Basic Math Functionality
	///////////////////////////////////////

    Matrix44& operator *= (const Matrix44& m);
    Matrix44& operator += (const Matrix44& m);
    Matrix44& operator -= (const Matrix44& m);
    Matrix44& operator *= (float s);
    Matrix44& operator /= (float s);
    Matrix44& operator += (const Vector3& v);
    Matrix44& operator -= (const Vector3& v);

    const Matrix44 operator - () const;
    const Matrix44 operator * (const Matrix44& m) const;
    const Matrix44 operator + (const Matrix44& m) const;
    const Matrix44 operator - (const Matrix44& m) const;
    const Matrix44 operator * (float s) const;
	const Matrix44 operator / (float s) const;
    friend const Matrix44 operator * (float s, const Matrix44& m);

	const Matrix44 Inverse() const;
	Matrix44& InverseThis();
	const Matrix44 Transpose() const;
	Matrix44& TransposeThis();

	///////////////////////////////////////
    // Transform Constructors
	///////////////////////////////////////

	static const Matrix44 BuildRotate(float x, float y, float z);
	static const Matrix44 BuildXFormZ(const Vector2& pos, float angle, float zPlain = 0);
	static const Matrix44 BuildRotateZ(float angle);
	static const Matrix44 BuildRotate(const Vector3& v);
	static const Matrix44 BuildRotate(const Vector3& axis, float angle);
	static const Matrix44 BuildScale(float x, float y, float z);
	static const Matrix44 BuildScale(float scale);
	static const Matrix44 BuildScale(const Vector3& v);
	static const Matrix44 BuildTranslate(float x, float y, float z);
	static const Matrix44 BuildTranslate(const Vector3& v);
	static const Matrix44 BuildLookAtLH(const Vector3& pos, const Vector3& at, const Vector3& up);

	const Vector3 TransformCoord(const Vector3& v) const;
	const Vector3 TransformNormal(const Vector3& v) const;

	///////////////////////////////////////
    // Accessors
	///////////////////////////////////////

	const Vector3 GetRight() const;
	const Vector3 GetUp() const;
	const Vector3 GetForward() const;
	const Vector3 GetPos() const;
	const Vector2 GetPosXY() const;

	void SetRight(const Vector3& v);
	void SetUp(const Vector3& v);
	void SetForward(const Vector3& v);
	void SetPos(const Vector3& v);

	float GetAngleZ() const;
	void GetYawPitchRoll(Vector3& rotation) const;

	D3DXMATRIX& GetD3DXMatrix();
	const D3DXMATRIX& GetD3DXMatrix() const;

	///////////////////////////////////////
    // Class Statics
	///////////////////////////////////////

	static Matrix44 const Identity();

private:

	///////////////////////////////////////
    // Data
	///////////////////////////////////////

	D3DXMATRIX matrix;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Quaternion
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Quaternion
{
	Quaternion();
	Quaternion(const D3DXQUATERNION& q);
	Quaternion(const Matrix44& m);
	Quaternion(const Vector3& rotation);
	Quaternion(const Vector3& axis, float angle);
	Quaternion(float x, float y, float z, float w);

	D3DXQUATERNION& GetD3DXQuaternion();
	const D3DXQUATERNION& GetD3DXQuaternion() const;

	///////////////////////////////////////
    // Assignment Operators
	///////////////////////////////////////

    Quaternion& operator *= (const Quaternion& q);
    Quaternion& operator += (const Quaternion& q);
    Quaternion& operator -= (const Quaternion& q);
    Quaternion& operator *= (float s);
    Quaternion& operator /= (float s);

	///////////////////////////////////////
    // Basic Math Functionality
	///////////////////////////////////////

    const Quaternion operator * (const Quaternion& q) const;
    const Quaternion operator + (const Quaternion& q) const;
    const Quaternion operator - (const Quaternion& q) const;
    const Quaternion operator * (float s) const;
    const Quaternion operator / (float s) const;
    friend const Quaternion operator * (float s, const Quaternion& q);

	const Quaternion Normalize() const;
	Quaternion& NormalizeThis();
	float DotProduct(const Quaternion& q) const;
	const Quaternion Inverse() const;

	Quaternion& SlerpThis(const Quaternion& q, float percent);
	const Quaternion Slerp(const Quaternion& q, float percent) const;

	void GetAxisAngle(Vector3& axis, float& angle) const;

	///////////////////////////////////////
    // Class Statics
	///////////////////////////////////////

	static const Quaternion Identity();

private:

	D3DXQUATERNION quaternion;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	2d Transform Class
*/
////////////////////////////////////////////////////////////////////////////////////////

struct XForm2
{
	XForm2();
	XForm2(const XForm2& otherXForm);
	XForm2(const b2Transform& otherXForm);
	XForm2(const Vector2& _position, float _angle = 0);
	explicit XForm2(float _angle);

	static XForm2 BuiltIntAngle(int angle)
	{
		switch (angle)
		{
			default:
			case 0: return XForm2(0);
			case 1: return XForm2(PI/2.0f);
			case 2: return XForm2(PI);
			case 3: return XForm2(PI*3.0f/2.0f);
		};
	}

	void RenderDebug(const Color& color = Color::White(0.5f), float size = 0.1f, float time = 0.0f) const;

	const XForm2 Inverse()const;
	const Vector2 GetUp() const;
	const Vector2 GetRight() const;

    XForm2& operator *= (const XForm2& xf);
    const XForm2 operator * (const XForm2& xf) const;
    const XForm2 operator + (const XForm2& xf) const;
    const XForm2 operator - (const XForm2& xf) const;
    const XForm2 operator * (float scale) const;
	friend const XForm2 operator * (float scale, const XForm2& xf) { return xf * scale; }
    const XForm2 operator / (float scale) const { return *this * (1/scale); }
	friend const XForm2 operator / (float scale, const XForm2& xf) { return xf / scale; }
	
	const XForm2 ScalePos(float scale) const { return XForm2(Vector2(position*scale), angle); }

	const XForm2 Interpolate(const XForm2& xfDelta, float percent) const;

	const Vector2 TransformCoord(const Vector2& v) const;
	const Vector2 TransformVector(const Vector2& v) const;

	static const XForm2 Identity();
	bool operator == (const XForm2& other) const { return position == other.position && angle == other.angle; }
	bool operator != (const XForm2& other) const { return !(*this == other); }

	Vector2 position;
	float angle;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Circle
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Circle
{
	Circle();
	Circle(const Circle& _c);
	Circle(const Vector2& _position, float _radius);
	explicit Circle(float _radius);

	void RenderDebug(const Color& color = Color::White(0.5f), float time = 0.0f) const;

    Circle& operator += (const Circle& c);
    const Circle operator + (const Circle& c) const;

	Vector2 position;
	float radius;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Sphere
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Sphere
{
	Sphere();
	Sphere(const Sphere& _s);
	Sphere(const Vector3& _position, float _radius);
	explicit Sphere(float _radius);

	void RenderDebug(const Color& color = Color::White(0.5f), float time = 0.0f) const;

    Sphere& operator += (const Sphere& s);
    const Sphere operator + (const Sphere& s) const;

	Vector3 position;
	float radius;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	BBox
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Box2AABB : public b2AABB
{
	Box2AABB();
	Box2AABB(const Vector2& point);
	Box2AABB(const Vector2& point1, const Vector2& point2);
	Box2AABB(const b2AABB& b);
	Box2AABB(const XForm2& xf, const Vector2& size)
	{
		const Vector2 v1 = size.Rotate(xf.angle);
		const Vector2 v2 = size.Rotate(xf.angle + PI/2);
		const Vector2 v
		(
			Max(Max(Max(v1.x, fabs(v1.x)), v2.x), fabs(v2.x)),
			Max(Max(Max(v1.y, fabs(v1.y)), v2.y), fabs(v2.y))
		);

		lowerBound = xf.position - v;
		upperBound = xf.position + v;
	}
	
	Box2AABB(const XForm2& xf, const Box2AABB& box, float scale = 1)
	{
		// build a bigger box that encapsulates the transformed box
		const Vector2 pos1 = xf.TransformCoord(scale*Vector2(box.lowerBound));
		const Vector2 pos2 = xf.TransformCoord(scale*Vector2(box.lowerBound.x, box.upperBound.y));
		const Vector2 pos3 = xf.TransformCoord(scale*Vector2(box.upperBound));
		const Vector2 pos4 = xf.TransformCoord(scale*Vector2(box.upperBound.x, box.lowerBound.y));

		lowerBound = Vector2(Min(Min(Min(pos1.x, pos2.x), pos3.x), pos4.x), Min(Min(Min(pos1.y, pos2.y), pos3.y), pos4.y));
		upperBound = Vector2(Max(Max(Max(pos1.x, pos2.x), pos3.x), pos4.x), Max(Max(Max(pos1.y, pos2.y), pos3.y), pos4.y));
	}

	Box2AABB SortBounds() const
	{
		return Box2AABB
		(
			Vector2(Min(lowerBound.x, upperBound.x), Min(lowerBound.y, upperBound.y)),
			Vector2(Max(lowerBound.x, upperBound.x), Max(lowerBound.y, upperBound.y))
		);
	}

	operator RECT () const
	{
		RECT r = { int(lowerBound.x), int(lowerBound.y), int(upperBound.x), int(upperBound.y) };
		return r;
	}

	const Vector2 GetSize() const { return (upperBound - lowerBound); }

	void RenderDebug(const Color& color = Color::White(0.5f), float time = 0.0f) const;

	// combine boxes
    const Box2AABB operator + (const Box2AABB& b) const;
    Box2AABB& operator += (const Box2AABB& b);

	bool Contains(const Vector2& v) const;
	bool FullyContains(const Box2AABB& b) const;
	bool PartiallyContains(const Box2AABB& b) const;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Line2 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Line2
{
	Line2();
	Line2(const Vector2& _p1, const Vector2& _p2);

	void RenderDebug(const Color& color = Color::White(0.5f), float time = 0.0f) const;

	// creates a line representing diagonal of a square
	explicit Line2(float size);

	float Length() const { return (p1 - p2).Magnitude(); }

    Line2& operator += (const Vector2& v);
    Line2& operator -= (const Vector2& v);
    const Line2 operator + (const Vector2& v) const;
    const Line2 operator - (const Vector2& v) const;

	Vector2 p1;
	Vector2 p2;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Line3 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

struct Line3
{
	Line3();
	Line3(const Vector3& _start, const Vector3& _end);

	void RenderDebug(const Color& color = Color::White(0.5f), float time = 0.0f) const;

	Vector3 p1;
	Vector3 p2;
};

#include "frankMath.inl"

} // namespace FrankMath

#endif FRANK_MATH_H