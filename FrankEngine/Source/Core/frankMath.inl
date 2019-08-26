////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Math Lib
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRANK_MATH_INLINE
#define FRANK_MATH_INLINE

inline Color::Color( FLOAT r, FLOAT g, FLOAT b, FLOAT a ) :
	D3DXCOLOR(r, g, b, a )
{
	ASSERT(r >= 0 && r <= 1);
	ASSERT(g >= 0 && g <= 1);
	ASSERT(b >= 0 && b <= 1);
	ASSERT(a >= 0 && a <= 1);
}

inline Color::Color( const D3DXCOLOR& c ) :
	D3DXCOLOR(c)
{}
	

inline Color::Color( const b2Color& c, FLOAT a) :
	D3DXCOLOR(c.r, c.g, c.b, a)
{
	ASSERT(r >= 0 && r <= 1);
	ASSERT(g >= 0 && g <= 1);
	ASSERT(b >= 0 && b <= 1);
	ASSERT(a >= 0 && a <= 1);
}

inline const Color Color::CapValues() const
{
	return Color(CapPercent(r), CapPercent(g), CapPercent(b), CapPercent(a));
}

inline const Color Color::BuildHSV(float h, float s, float v, float a)
{
	return Color(h, s, v, a).HSVtoRGB();
}

inline const Color Color::BuildBytes( UINT8 r, UINT8 g, UINT8 b, UINT8 a )
{
	return Color(r/255.f, g/255.f, b/255.f, a/255.f);
}

inline const Color Color::HSVtoRGB() const
{
	float x = r;
	float y = g;
	float z = b;

	ASSERT(x >= 0 && x <= 1 && y >= 0 && y <=1 && z >= 0 && z <= 1);

	// adapted from http://www.cs.rit.edu/~ncs/color/t_convert.html
	Color cRGB;
	cRGB.a = a;

	if (y == 0) 
	{
		// grey
		cRGB.r = cRGB.g = cRGB.b = z;
		return cRGB;
	}

	const float h = x * 6;
	const int sector = (int)(h);
	const float f = h - sector;
	const float p = z * (1 - y);
	const float q = z * (1 - y * f);
	const float t = z * (1 - y * (1 - f));

	switch (sector) 
	{
		case 0: // red
			cRGB.r = z;
			cRGB.g = t;
			cRGB.b = p;
			break;

		case 1: // orange
			cRGB.r = q;
			cRGB.g = z;
			cRGB.b = p;
			break;

		case 2: // yellow
			cRGB.r = p;
			cRGB.g = z;
			cRGB.b = t;
			break;

		case 3: // green
			cRGB.r = p;
			cRGB.g = q;
			cRGB.b = z;
			break;

		case 4: // blue 
			cRGB.r = t;
			cRGB.g = p;
			cRGB.b = z;
			break;

		default: // purple
			cRGB.r = z;
			cRGB.g = p;
			cRGB.b = q;
			break;
	}
	return cRGB;
}

inline const Color Color::RandBetween(const Color& c1, const Color& c2)
{
	return c1 + RAND_PERCENT * (c2 - c1);
}

inline const Color Color::RandBetweenComponents(const Color& c1, const Color& c2)
{
	return Color
	(
		c1.r + RAND_PERCENT * (c2.r - c1.r),
		c1.g + RAND_PERCENT * (c2.g - c1.g),
		c1.b + RAND_PERCENT * (c2.b - c1.b),
		c1.a + RAND_PERCENT * (c2.a - c1.a)
	);
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Capping functions
*/
////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline const T& Cap(const T& v, const T& min, const T& max) 
{
	if (v < min)
		return min;
	else if (v > max)
		return max;
	else
		return v;
}

inline float CapPercent(float p) 
{
	return Cap(p, 0.0f, 1.0f);
}

inline float CapWrappedPercent(float p) 
{
	if (p < 0)
		p -= (int)(p-1);
	p -= (int)(p);

	ASSERT(p >= 0 && p <= 1);
	return p;
}

// cap angle between -PI and PI
inline float CapAngle(float a) 
{
	a -= (PI * 2)*(int)(a / (PI * 2));

	if (a < -PI)
		a += PI * 2;
	else if (a > PI)
		a -= PI * 2;

	ASSERT(a >= -PI && a <= PI);

	return a;
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Helpful functions
*/
////////////////////////////////////////////////////////////////////////////////////////

// generate a sin pulse based off a time, interval and offset
inline float SinPulse(float time, float interval, float offset)
{
	const float timeAdjusted = time + offset;
	const float pulseTime = timeAdjusted - interval * (int)((timeAdjusted) / interval);
	float pct = CapPercent(pulseTime / interval);
	return sinf(2 * PI * pct);
}

inline void ColorMultiply(Color& color, float x) 
{
	color.r *= x;
	color.g *= x;
	color.b *= x;
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Random Numbers
*/
////////////////////////////////////////////////////////////////////////////////////////

inline void FrankRand::SetSeed(unsigned int newSeed)
{ 
	randomSeed = newSeed; 
}

inline unsigned int FrankRand::GetInt()
{
	ASSERT(randomSeed != 0);

	// George Marsaglia's xor-shift random number generator
	randomSeed ^= (randomSeed << 13);
	randomSeed ^= (randomSeed >> 17);
	randomSeed ^= (randomSeed << 5);
	return randomSeed;
}

inline int FrankRand::GetInt(int min, int max)
{
	ASSERT(min <= max);
	return min + (GetInt() % (max - min + 1));
}

inline float FrankRand::GetFloat()
{ 
	return ((float)GetInt() / (float)maxRand); 
}

inline float FrankRand::GetFloat(float min, float max)
{
	ASSERT(min <= max);
	return (min + (max - min) * GetFloat());
}

inline bool FrankRand::RollDie(unsigned int sides)
{ 
	return (GetInt() % sides == 0); 
}

inline int FrankRand::GetSign()
{ 
	return ((GetInt() % 2)? 1 : -1); 
}

inline const Color FrankRand::GetColor()
{ 
	return Color(GetFloat(), GetFloat(), GetFloat(), GetFloat()); 
}

inline float FrankRand::GetAngle()
{ 
	return GetFloat(0, 2 * PI); 
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Vector3 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

inline Vector3::Vector3(const D3DXVECTOR3& v) :
	D3DXVECTOR3(v)
{}

inline Vector3::Vector3(float _x, float _y, float _z) :
	D3DXVECTOR3(_x, _y, _z)
{}

inline Vector3::Vector3(float _v) :
	D3DXVECTOR3(_v, _v, _v)
{}

inline Vector3::Vector3(const Vector2& v) :
	D3DXVECTOR3(v.x, v.y, 0)
{}

// quick way to get a randomized vector
inline const Vector3 Vector3::BuildRandomQuick()
{ 
	return Vector3(RAND_BETWEEN(-1, 1), RAND_BETWEEN(-1, 1), RAND_BETWEEN(-1, 1)); 
}

// normalized evenly distributed vector, slower then quick version
inline Vector3 Vector3::BuildRandomNormalized()
{
	Vector3 position;
	position.z = RAND_BETWEEN(-1.0f, 1.0f);
	const float t = RAND_BETWEEN(0, 2 * PI);
	const float r = sqrtf(1 - position.z * position.z);
	position.x = r * cosf(t);
	position.y = r * sinf(t);

	return position;
}

///////////////////////////////////////
// Basic Functionality
///////////////////////////////////////

// pass in the magnitude if you know it for a faster calculation
inline const Vector3 Vector3::Normalize(float magnitude) const
{
	ASSERT(magnitude > 0);
	ASSERT(Magnitude() == magnitude);
	return ((*this) / magnitude);
}

inline const Vector3 Vector3::Normalize() const
{
	Vector3 result;
	D3DXVec3Normalize(&result, this);
	return result;
}

inline Vector3& Vector3::NormalizeThis()
{
	D3DXVec3Normalize(this, this);
	return *this;
}

inline bool Vector3::IsNormalized() const
{
	const float epsilon = 0.0001f;
	const float mag2 = MagnitudeSquared();
	return fabs(mag2 - 1) < epsilon;
}

inline float Vector3::Magnitude() const
{
	return D3DXVec3Length(this);
}

inline float Vector3::MagnitudeSquared() const
{
	return D3DXVec3LengthSq(this);
}

inline float Vector3::MagnitudeAndNormalize()
{
	const float magnitude = Magnitude();
	ASSERT(magnitude > 0);
	*this /= magnitude;
	return magnitude;
}

inline Vector3& Vector3::ZeroThis()
{
	*this = Vector3(0, 0, 0);
	return (*this);
}

inline bool Vector3::IsZero() const
{
	return (fabs(x) < FRANK_EPSILON || fabs(y) < FRANK_EPSILON || fabs(z) < FRANK_EPSILON);
}

///////////////////////////////////////
// Dot & Cross Products
///////////////////////////////////////

inline float Vector3::Dot(const Vector3& v) const
{
	return D3DXVec3Dot(this, &v);
}

inline const Vector3 Vector3::operator * (const Vector3& v) const
{
	return Vector3(v.x * x, v.y * y, v.z * z);
}

// returns angle betwen 2 vecs,
// result is between 0 and 2*PI
// (use cross product to get sign if necessary)
inline float Vector3::AngleBetween(const Vector3& v) const
{
	ASSERT(IsNormalized() && v.IsNormalized());
	float dp = Dot(v);
	if (dp > 1)
		dp = 1;
	else if (dp < -1)
		dp = -1;

	return acosf(dp);
}

inline const Vector3 Vector3::Cross(const Vector3& v) const
{
	Vector3 result;
	D3DXVec3Cross(&result, this, &v);
	return result;
}

inline const Vector3 Vector3::operator ^ (const Vector3& v) const
{
	return Cross(v);
}

// get the rotation this vector represents
inline const Vector3 Vector3::GetRotation() const
{
	Vector3 v = Normalize();
	const float pitch = v.AngleBetween(Vector3::YAxis()) - PI / 2;

	v.y = 0;
	v.NormalizeThis();
	const float yaw = v.AngleBetween(Vector3::ZAxis());
	const float yawSign = ((Vector3::ZAxis() ^ v).y > 0)? 1.0f : -1.0f;

	return Vector3(yaw * yawSign, pitch, 0);
}

inline void Vector3::CapRotation()
{
	x = CapAngle(x);
	y = CapAngle(y);
	z = CapAngle(z);
}

///////////////////////////////////////
// Operators
///////////////////////////////////////

inline const Vector3 Vector3::operator - () const 
{ 
	return *this * -1.0f; 
}

inline Vector3& Vector3::operator += (const Vector3& v)
{ 
	return (Vector3&)(*(D3DXVECTOR3*)this += v); 
}

inline Vector3& Vector3::operator -= (const Vector3& v) 
{ 
	return (Vector3&)(*(D3DXVECTOR3*)this -= v); 
}

inline Vector3& Vector3::operator *= (float scale) 
{ 
	return (Vector3&)(*(D3DXVECTOR3*)this *= scale); 
}

inline Vector3& Vector3::operator /= (float scale) 
{ 
	return (Vector3&)(*(D3DXVECTOR3*)this /= scale); 
}

inline const Vector3 Vector3::operator + (const Vector3& v) const 
{ 
	return *(D3DXVECTOR3*)this + v; 
}

inline const Vector3 Vector3::operator - (const Vector3& v) const 
{ 
	return *(D3DXVECTOR3*)this - v; 
}

inline const Vector3 Vector3::operator * (float scale) const 
{ 
	return *(D3DXVECTOR3*)this * scale; 
}

inline const Vector3 Vector3::operator / (float scale) const 
{ 
	return *(D3DXVECTOR3*)this / scale; 
}

inline const Vector3 operator * (float scale, const Vector3& v) 
{ 
	return (D3DXVECTOR3&)v * scale; 
}

///////////////////////////////////////
// Tests
///////////////////////////////////////

inline float Vector3::DistanceFromLineSegementSquared(const Vector3& lineStart, const Vector3& lineEnd) const
{
	const Vector3 deltaPos(lineEnd - lineStart);
	const float deltaPosMagSquared = deltaPos.MagnitudeSquared();
	const float dp =
	(
		  (x - lineStart.x)*deltaPos.x
		+ (y - lineStart.y)*deltaPos.y
		+ (z - lineStart.z)*deltaPos.z
	);

	if (dp >= deltaPosMagSquared)
		return (*this - lineEnd).MagnitudeSquared();
	else if (dp <= 0)
		return (*this - lineStart).MagnitudeSquared();
	else
	{
		const Vector3 pointNearest(lineStart + deltaPos * (dp / deltaPosMagSquared));
		return (*this - pointNearest).MagnitudeSquared();
	}
}

// A slightly faster version where user provides pre calcualted deltapos
// useful if you are doing many tests with the same start and end
inline float Vector3::DistanceFromLineSegementSquared(const Vector3& lineStart, const Vector3& lineEnd, const Vector3& deltaPos, float deltaPosMagSquared) const
{
	const float dp =
	(
		  (x - lineStart.x)*(deltaPos.x)
		+ (y - lineStart.y)*(deltaPos.y)
		+ (z - lineStart.z)*(deltaPos.z)
	);

	if (dp >= deltaPosMagSquared)
		return (*this - lineEnd).MagnitudeSquared();
	else if (dp <= 0)
		return (*this - lineStart).MagnitudeSquared();
	else
	{
		const Vector3 pointNearest(lineStart + deltaPos * (dp / deltaPosMagSquared));
		return (*this - pointNearest).MagnitudeSquared();
	}
}

// A slightly faster version where user provides pre calcualted deltapos
// useful if you are doing many tests with the same start and end
// this version also returns a percentage of the distance along the line
inline float Vector3::DistanceFromLineSegementSquared(const Vector3& lineStart, const Vector3& lineEnd, const Vector3& deltaPos, float deltaPosMagSquared, float& percentDistance) const
{
	const float dp =
	(
		  (x - lineStart.x)*(deltaPos.x)
		+ (y - lineStart.y)*(deltaPos.y)
		+ (z - lineStart.z)*(deltaPos.z)
	);

	percentDistance = dp / deltaPosMagSquared;

	if (dp >= deltaPosMagSquared)
		return (*this - lineEnd).MagnitudeSquared();
	else if (dp <= 0)
		return (*this - lineStart).MagnitudeSquared();
	else
	{
		const Vector3 pointNearest(lineStart + deltaPos * percentDistance);
		return (*this - pointNearest).MagnitudeSquared();
	}
}

inline float Vector3::DistanceFromLineSegement(const Vector3& lineStart, const Vector3& lineEnd) const
{
	return sqrtf(DistanceFromLineSegementSquared(lineStart, lineEnd));
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	ByteVector2 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

inline ByteVector2::ByteVector2(int _s) : x(_s), y(_s) 
{
	ASSERT(_s <= 255 && _s >= 0); 
}

inline ByteVector2::ByteVector2(int _x, int _y) : x(_x), y(_y) 
{
	ASSERT(_x <= 255 && _x >= 0 && _y <= 255 && _y >= 0); 
}

inline ByteVector2::ByteVector2(const IntVector2& v) : x(int(v.x)), y(int(v.y)) 
{ 
	ASSERT(v.x <= 255 && v.x >= 0 && v.y <= 255 && v.y >= 0); 
}

inline ByteVector2::ByteVector2(const Vector2& v) : x(int(v.x)), y(int(v.y)) 
{ 
	ASSERT(v.x <= 255 && v.x >= 0 && v.y <= 255 && v.y >= 0); 
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Vector2 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

inline Vector2::Vector2() {}
inline Vector2::Vector2(const Vector2& v) : b2Vec2(v.x, v.y) {}
inline Vector2::Vector2(float _x, float _y) : b2Vec2(_x, _y) {}
inline Vector2::Vector2(const b2Vec2& v) : b2Vec2(v) {}
inline Vector2::Vector2(float _v) : b2Vec2(_v, _v) {}
inline Vector2::Vector2(const Vector3& v) : b2Vec2(v.x, v.y) {}
inline Vector2::Vector2(const ByteVector2& v) : b2Vec2(float(v.x), float(v.y)) {}
inline Vector2::Vector2(const IntVector2& v) : b2Vec2(float(v.x), float(v.y)) {}

// angle of 0 radians is up, Vector(0, 1)
inline const Vector2 Vector2::BuildFromAngle(float angle)
{
	return Vector2(-sinf(angle), cosf(angle));
}

// angle of 0 radians is up, Vector(0, 1)
inline float Vector2::GetAngle() const
{ 
	return atan2f(-x, y); 
}

inline const Vector2 Vector2::BuildGaussian(const Vector2& variance, const Vector2& mean)
{
	return mean + Vector2
	(
		FrankRand::GetGaussian(variance.x),
		FrankRand::GetGaussian(variance.y)
	);
}

inline const Vector2 Vector2::BuildRandomUnitVector()
{
	const float angle = RAND_BETWEEN(0, 2 * PI);
	return Vector2(cosf(angle), sinf(angle));
}

inline const Vector2 Vector2::BuildRandomInCircle(float radius, float minRadius)
{
	if (radius == 0)
		return Vector2::Zero();

	ASSERT(radius >= 0 && minRadius >= 0);
	ASSERT(minRadius <= radius);
	const float angle = RAND_BETWEEN(0, 2 * PI);
	const float randPercent = RAND_BETWEEN(minRadius / radius, 1);
	if (randPercent == 0)
		return Vector2::Zero();

	const float randOffset = radius * sqrtf(randPercent);
	return Vector2
	(
		cosf(angle) * randOffset,
		sinf(angle) * randOffset
	);
}

inline const D3DXVECTOR3 Vector2::GetD3DXVECTOR3(float z) const 
{
	return D3DXVECTOR3(x, y, z);
}

///////////////////////////////////////
// Basic Functionality
///////////////////////////////////////


inline const Vector2 Vector2::Normalize(float magnitude) const
{
	if (magnitude > FRANK_EPSILON)
		return ((*this) / magnitude);
	else
		return (Vector2(0,1));
}

inline const Vector2 Vector2::Normalize() const
{
	const float magnitude = Magnitude();
	if (magnitude > FRANK_EPSILON)
		return ((*this) / magnitude);
	else
		return (Vector2(0,1));
}

inline bool Vector2::IsNormalized() const
{
	const float epsilon = 0.0001f;
	const float mag2 = MagnitudeSquared();
	return fabs(mag2 - 1) < epsilon;
}

inline Vector2& Vector2::NormalizeThis()
{
	const float magnitude = Magnitude();
	if (magnitude > FRANK_EPSILON)
		(*this) /= magnitude;
	else
	{
		x = 0;
		y = 1;
	}
	return (*this);
}

inline float Vector2::MagnitudeAndNormalize()
{
	const float magnitude = Magnitude();
	if (magnitude > FRANK_EPSILON)
	{
		*this /= magnitude;
		return magnitude;
	} else {
		x = 0;
		y = 1;
		return 0;
	}
}

inline const Vector2 Vector2::CapMagnitude(float maxMagnitude) const
{
	const float magnitude2 = MagnitudeSquared();
	if (magnitude2 > maxMagnitude*maxMagnitude)
	   return (*this) / (maxMagnitude / sqrtf(magnitude2));
	return (*this);
}

inline Vector2& Vector2::CapMagnitudeThis(float maxMagnitude)
{
	const float magnitude2 = MagnitudeSquared();
	if (magnitude2 > maxMagnitude*maxMagnitude)
	   *this *= (maxMagnitude / sqrtf(magnitude2));
	return (*this);
}

inline const Vector2 Vector2::Floor() const
{
	return Vector2(floor(x), floor(y));
}

inline Vector2& Vector2::FloorThis()
{
	x = floor(x);
	y = floor(y);
	return *this;
}

inline const Vector2 Vector2::Round() const
{
	return Vector2(floor(x + 0.5f), floor(y + 0.5f));
}

inline Vector2& Vector2::RoundThis()
{
	x = floor(x + 0.5f);
	y = floor(y + 0.5f);
	return *this;
}

inline const Vector2 Vector2::Rotate(const Vector2& v) const
{
	return
	(
		Vector2
		(
			x*v.x - y*v.y,
			x*v.y + y*v.x
		)
	);
}

inline Vector2& Vector2::RotateThis(const Vector2& v)
{
	const float x_temp = x*v.x - y*v.y;
	y = x*v.y + y*v.x;
	x = x_temp;
	return *this;
}

inline const Vector2 Vector2::Rotate(float theta) const
{
	const float c = cosf(theta);
	const float s = sinf(theta);
	return Vector2((x*c - y*s), (x*s + y*c));
}

inline Vector2& Vector2::RotateThis(float theta)
{
	*this = Rotate(theta);
	return *this;
}

inline const Vector2 Vector2::Reflect(const Vector2& normal) const
{
	assert(normal.IsNormalized());
	return (*this - (2*Dot(normal))*normal);
}

// returns angle betwen 2 vecs
// result is between 0 and 2*PI
inline float Vector2::AngleBetween(const Vector2& v) const
{
	ASSERT(IsNormalized() && v.IsNormalized());
	float dp = Dot(v);
	if (dp > 1)
		dp = 1;
	else if (dp < -1)
		dp = -1;
	return acosf(dp);
}

// returns angle betwen 2 vecs
// result is between -PI and PI
inline float Vector2::SignedAngleBetween(const Vector2& v) const
{
	ASSERT(IsNormalized() && v.IsNormalized());
	float dp = Dot(v);
	if (dp > 1)
		dp = 1;
	else if (dp < -1)
		dp = -1;

	const float angle = CapAngle(acosf(dp));
	return (Cross(v) > 0)? angle : -angle;
}

///////////////////////////////////////
// Tests
///////////////////////////////////////

// 2d finite line segment intersection test
// returns true and sets the point of intersection if segments intersect
inline bool Vector2::LineSegmentIntersection
(
	const Vector2& segment1a, const Vector2& segment1b,
	const Vector2& segment2a, const Vector2& segment2b,
	Vector2& intersectionPoint
)
{
	const Vector2 delta1 = segment1b - segment1a;
	const Vector2 delta2 = segment2b - segment2a;
	const Vector2 delta3 = segment1a - segment2a;
	const float d = delta1.Cross(delta2);
	const float ud1 = delta2.Cross(delta3);

	if (d > 0)
	{
		if (ud1 < 0 || ud1 > d)
			return false; // outside bounds of segment 1

		const float ud2 = delta1.Cross(delta3);
		if (ud2 < 0 || ud2 > d)
			return false; // outside bounds of segment 2
	}
	else if (d < 0)
	{
		if (ud1 > 0 || ud1 < d)
			return false; // outside bounds of segment 1

		const float ud2 = delta1.Cross(delta3);
		if (ud2 > 0 || ud2 < d)
			return false; // outside bounds of segment 2
	}
	else // d == 0
		return false; // segments are parallel

	// compute intersection point
	intersectionPoint = segment1a + (ud1/d) * delta1;
	return true;
}

inline bool Vector2::LineIntersection(const Vector2& v1, const Vector2& v2, const Vector2& v3, const Vector2& v4, Vector2& point)
{
	const Vector2 delta1 = v2 - v1;
	const Vector2 delta2 = v4 - v3;
	const Vector2 delta3 = v1 - v3;
	const float d = delta1.Cross(delta2);

	if (d == 0)
		return false;	// lines are parallel

	const float ua = delta2.Cross(delta3) / d;
	point = v1 + ua * delta1;
	return true;
}

inline float Vector2::DistanceFromLineSegement
(
	const Vector2& lineStart, 
	const Vector2& lineEnd
) const
{
	const Vector2 deltaPos(lineEnd - lineStart);
	const float deltaPosMagSquared = deltaPos.MagnitudeSquared();
	const Vector2 deltaStart(*this - lineStart);
	const float dp = deltaStart.Dot(deltaPos);

	if (dp >= deltaPosMagSquared)
		return (*this - lineEnd).Magnitude();
	else if (dp <= 0)
		return (*this - lineStart).Magnitude();
	else
	{
		const Vector2 pointNearest(lineStart + deltaPos * (dp / deltaPosMagSquared));
		return (*this - pointNearest).Magnitude();
	}
}

// distance from line segment test
// returns true if lines intersect
inline float Vector2::DistanceFromLineSegement
(
	const Vector2& lineStart,	// start of line
	const Vector2& lineEnd,		// end of line
	const Vector2& point		// point to check against
)
{
	const Vector2 deltaPos(lineEnd - lineStart);
	const float deltaPosMagSquared = deltaPos.MagnitudeSquared();
	const Vector2 deltaStart(point - lineStart);
	const float dp = deltaStart.Dot(deltaPos);

	if (dp >= deltaPosMagSquared)
		return (point - lineEnd).Magnitude();
	else if (dp <= 0)
		return (point - lineStart).Magnitude();
	else
	{
		const Vector2 pointNearest(lineStart + deltaPos * (dp / deltaPosMagSquared));
		return (point - pointNearest).Magnitude();
	}
}

// 2d line segment intersection test
// returns true if finite line segments intersect
inline bool Vector2::LineSegmentIntersection
(
	const Vector2& segment1a, const Vector2& segment1b,
	const Vector2& segment2a, const Vector2& segment2b
)
{
	const Vector2 d1a1b = segment1a - segment1b;
	const Vector2 d1b2a = segment1b - segment2a;
	const Vector2 d1b2b = segment1b - segment2b;

	if (IsClockwise(d1a1b, d1b2a) == IsClockwise(d1a1b, d1b2b))
		return false;

	const Vector2 d1a2a = segment1a - segment2a;
	const Vector2 d2a2b = segment2a - segment2b;

	return (IsClockwise(d1a2a, d2a2b) != IsClockwise(d1b2a, d2a2b));
}

// returns shortest distance between 2 line segments
inline float Vector2::DistanceBetweenLineSegments
(
	const Vector2& segment1a, const Vector2& segment1b,
	const Vector2& segment2a, const Vector2& segment2b
)
{
	if (LineSegmentIntersection(segment1a, segment1b, segment2a, segment2b))
		return 0; // lines intersect

	const float d1 = DistanceFromLineSegement(segment1a, segment1b, segment2a);
	const float d2 = DistanceFromLineSegement(segment1a, segment1b, segment2b);
	const float d3 = DistanceFromLineSegement(segment2a, segment2b, segment1a);
	const float d4 = DistanceFromLineSegement(segment2a, segment2b, segment1b);
	return Min(Min(d1, d2), Min(d3, d4));
}

// clockwise winding test
// returns true if verts form clockwise triangle
inline bool Vector2::IsClockwise
(
	const Vector2& v1, 
	const Vector2& v2,
	const Vector2& v3
)
{
	const Vector2 d1 = v1 - v2;
	const Vector2 d2 = v2 - v3;
	return IsClockwise(d1, d2);
}

// clockwise winding test
// returns true if directions are winding clockwise
inline bool Vector2::IsClockwise(const Vector2& d1, const Vector2& d2) 
{ 
	return (d1.y*d2.x > d2.y*d1.x); 
}

inline const Vector2 Vector2::TransformCoord(const XForm2& xf) const 
{ 
	return xf.TransformCoord(*this); 
}

inline Vector2& Vector2::TransformThisCoord(const XForm2& xf) 
{ 
	return (*this = xf.TransformCoord(*this)); 
}

inline bool Vector2::InsideConvexPolygon(const Vector2* vertexList, int vertexCount, const Vector2& point)
{
	for (int i = 0; i < vertexCount - 1; ++i)
	{
		if (!Vector2::IsClockwise(vertexList[i], vertexList[i+1], point))
			return false;
	}

	return (Vector2::IsClockwise(vertexList[vertexCount - 1], vertexList[0], point));
}

inline bool Vector2::InsideBox(const XForm2& xf, const Vector2& size, const Vector2& point)
{
	// check if position is within the box
	const Vector2 v1 = xf.TransformCoord(Vector2(size.x, size.y));
	const Vector2 v2 = xf.TransformCoord(Vector2(size.x, -size.y));
	if (!IsClockwise(v1, v2, point))
		return false;

	const Vector2 v3 = xf.TransformCoord(Vector2(-size.x, -size.y));
	if (!IsClockwise(v2, v3, point))
		return false;

	const Vector2 v4 = xf.TransformCoord(Vector2(-size.x, size.y));
	if (!IsClockwise(v3, v4, point))
		return false;

	if (!IsClockwise(v4, v1, point))
		return false;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Matrix44
*/
////////////////////////////////////////////////////////////////////////////////////////

inline Matrix44::Matrix44() {}
inline Matrix44::Matrix44(const Matrix44& m) : matrix(m.matrix) {}
inline Matrix44::Matrix44(const D3DMATRIX& m) : matrix(m) {}
inline Matrix44::Matrix44(const Vector2& v) 
{ 
	D3DXMatrixTranslation(&matrix, v.x, v.y, 0.0f); 
}

inline Matrix44::Matrix44(const Quaternion& q)
{
	D3DXMatrixRotationQuaternion(&matrix, &q.GetD3DXQuaternion());
}

inline Matrix44::Matrix44(const XForm2& xf, float z)
{
	D3DXMatrixRotationZ(&matrix, xf.angle);
	SetPos(Vector3(xf.position.x, xf.position.y, z));
}

inline float& Matrix44::operator () (int row, int column) { return matrix(row, column); }
inline float Matrix44::operator () (int row, int column) const { return matrix(row, column); }

inline D3DXMATRIX& Matrix44::GetD3DXMatrix() { return matrix; }
inline const D3DXMATRIX& Matrix44::GetD3DXMatrix() const { return matrix; }

///////////////////////////////////////
// Assignment Operators
///////////////////////////////////////

inline Matrix44& Matrix44::operator *= (const Matrix44& m) { (matrix *= m.matrix); return (*this); }
inline Matrix44& Matrix44::operator += (const Matrix44& m) { matrix += m.matrix; return (*this); }
inline Matrix44& Matrix44::operator -= (const Matrix44& m) { matrix -= m.matrix; return (*this); }
inline Matrix44& Matrix44::operator *= (float s) { matrix *= s; return (*this); }
inline Matrix44& Matrix44::operator /= (float s) { matrix /= s; return (*this); }
inline Matrix44& Matrix44::operator += (const Vector3& v) { matrix._41 += v.x; matrix._42 += v.y; matrix._43 += v.z; return (*this); }
inline Matrix44& Matrix44::operator -= (const Vector3& v) { matrix._41 -= v.x; matrix._42 -= v.y; matrix._43 -= v.z; return (*this); }

///////////////////////////////////////
// Unary Operators
///////////////////////////////////////

inline const Matrix44 Matrix44::operator - () const  {  return Matrix44(-matrix); }

inline const Matrix44 Matrix44::Inverse() const
{
	D3DXMATRIX m;
	D3DXMatrixInverse(&m, NULL, &matrix);
	return Matrix44(m);
}

inline Matrix44& Matrix44::InverseThis()
{
	D3DXMatrixInverse(&matrix, NULL, &matrix);
	return *this;
}

inline const Matrix44 Matrix44::Transpose() const
{
	Matrix44 m;
	D3DXMatrixTranspose(&m.GetD3DXMatrix(), &matrix);
	return m;
}

inline Matrix44& Matrix44::TransposeThis()
{
	D3DXMatrixTranspose(&matrix, &matrix);
	return *this;
}

///////////////////////////////////////
// Binary Operators
///////////////////////////////////////

inline const Matrix44 Matrix44::operator * (const Matrix44& m) const { return Matrix44(matrix * m.matrix); }
inline const Matrix44 Matrix44::operator + (const Matrix44& m) const { return Matrix44(matrix + m.matrix); }
inline const Matrix44 Matrix44::operator - (const Matrix44& m) const { return Matrix44(matrix - m.matrix); }
inline const Matrix44 Matrix44::operator * (float s) const { return Matrix44(matrix * s); }
inline const Matrix44 Matrix44::operator / (float s) const { return Matrix44(matrix / s); }
inline const Matrix44 operator * (float s, const Matrix44& m) { return Matrix44(m * m.matrix); }

///////////////////////////////////////
// Transform Constructors
///////////////////////////////////////

inline const Matrix44 Matrix44::BuildRotate(float x, float y, float z)
{
	Matrix44 m;
	D3DXMatrixRotationYawPitchRoll(&m.GetD3DXMatrix(), x, y, z);
	return m;
}

inline const Matrix44 Matrix44::BuildXFormZ(const Vector2& pos, float angle, float zPlain)
{
	Matrix44 m;
	D3DXMatrixRotationZ(&m.GetD3DXMatrix(), angle);
	m.SetPos(Vector3(pos.x, pos.y, zPlain));
	return m;
}

inline const Matrix44 Matrix44::BuildRotateZ(float angle)
{
	Matrix44 m;
	D3DXMatrixRotationZ(&m.GetD3DXMatrix(), angle);
	return m;
}

inline const Matrix44 Matrix44::BuildRotate(const Vector3& v)
{
	return BuildRotate(v.x, v.y, v.z);
}

inline const Matrix44 Matrix44::BuildRotate(const Vector3& axis, float angle)
{
	Matrix44 m;
	D3DXMatrixRotationAxis(&m.GetD3DXMatrix(), &axis, angle);
	return m;
}

inline const Matrix44 Matrix44::BuildScale(float x, float y, float z)
{
	Matrix44 m;
	D3DXMatrixScaling(&m.GetD3DXMatrix(), x, y, z);
	return m;
}

inline const Matrix44 Matrix44::BuildScale(float scale)
{
	Matrix44 m;
	D3DXMatrixScaling(&m.GetD3DXMatrix(), scale, scale, scale);
	return m;
}

inline const Matrix44 Matrix44::BuildScale(const Vector3& v) 
{ 
	return BuildScale(v.x, v.y, v.z); 
}

inline const Matrix44 Matrix44::BuildTranslate(float x, float y, float z)
{
	Matrix44 m;
	D3DXMatrixTranslation(&m.GetD3DXMatrix(), x, y, z);
	return m;
}

inline const Matrix44 Matrix44::BuildTranslate(const Vector3& v)
{
	return BuildTranslate(v.x, v.y, v.z);
}

inline const Matrix44 Matrix44::BuildLookAtLH(const Vector3& pos, const Vector3& at, const Vector3& up)
{
	Matrix44 m;
	D3DXMatrixLookAtLH(&m.GetD3DXMatrix(), &pos, &at, &up);
	return m;
}

inline const Vector3 Matrix44::TransformCoord(const Vector3& v) const
{
	D3DXVECTOR3 v_out;
	D3DXVec3TransformCoord(&v_out, &v, &matrix);
	return Vector3(v_out);
}

inline const Vector3 Matrix44::TransformNormal(const Vector3& v) const
{
	D3DXVECTOR3 v_out;
	D3DXVec3TransformNormal(&v_out, &v, &matrix);
	return Vector3(v_out);
}

///////////////////////////////////////
// Accessors
///////////////////////////////////////

inline const Vector3 Matrix44::GetRight() const { return Vector3(matrix._11, matrix._12, matrix._13); }
inline const Vector3 Matrix44::GetUp() const { return Vector3(matrix._21, matrix._22, matrix._23); }
inline const Vector3 Matrix44::GetForward() const { return Vector3(matrix._31, matrix._32, matrix._33); }
inline const Vector3 Matrix44::GetPos() const { return Vector3(matrix._41, matrix._42, matrix._43); }

inline const Vector2 Matrix44::GetPosXY() const { return Vector2(matrix._41, matrix._42); }

inline void Matrix44::SetRight(const Vector3& v) { matrix._11 = v.x; matrix._12 = v.y; matrix._13 = v.z; }
inline void Matrix44::SetUp(const Vector3& v) { matrix._21 = v.x; matrix._22 = v.y; matrix._23 = v.z; }
inline void Matrix44::SetForward(const Vector3& v) { matrix._31 = v.x; matrix._32 = v.y; matrix._33 = v.z; }
inline void Matrix44::SetPos(const Vector3& v) { matrix._41 = v.x; matrix._42 = v.y; matrix._43 = v.z; }

inline float Matrix44::GetAngleZ() const
{
	const Vector3 up = GetUp();
	ASSERT(up.z == 0); // must be in x/y plain!
	return up.AngleBetween(Vector3::YAxis());
}

inline void Matrix44::GetYawPitchRoll(Vector3& rotation) const
{
	// note: this function may not work and needs to be optimized

	Vector3 forward = GetForward();
	Vector3 forwardFlat = forward;
	forwardFlat.y = 0;
	forwardFlat.NormalizeThis();
	rotation.x = forwardFlat.AngleBetween(Vector3::ZAxis());
	rotation.y = forwardFlat.AngleBetween(forward);

	Vector3 right = GetRight();
	Vector3 rightFlat = right;
	rightFlat.y = 0;
	rightFlat.Normalize();
	rotation.z = rightFlat.AngleBetween(right);
}

///////////////////////////////////////
// Class Statics
///////////////////////////////////////

inline const Matrix44 Matrix44::Identity()	
{ 
	return Matrix44
	(
		D3DXMATRIX
		(
			1,	0,	0,	0,
			0,	1,	0,	0,
			0,	0,	1,	0,
			0,	0,	0,	1
		)
	);
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Quaternion
*/
////////////////////////////////////////////////////////////////////////////////////////

inline Quaternion::Quaternion() {}

inline Quaternion::Quaternion(const D3DXQUATERNION& q) : quaternion(q) {}

inline Quaternion::Quaternion(const Matrix44& m) {	D3DXQuaternionRotationMatrix(&quaternion, &m.GetD3DXMatrix()); }

inline Quaternion::Quaternion(const Vector3& rotation) { D3DXQuaternionRotationYawPitchRoll(&quaternion, rotation.x, rotation.y, rotation.z); }

inline Quaternion::Quaternion(const Vector3& axis, float angle) { D3DXQuaternionRotationAxis(&quaternion, &axis, angle); }

inline Quaternion::Quaternion(float x, float y, float z, float w) : quaternion(x, y, z, w) {}

inline D3DXQUATERNION& Quaternion::GetD3DXQuaternion() { return quaternion; }
inline const D3DXQUATERNION& Quaternion::GetD3DXQuaternion() const { return quaternion; }

///////////////////////////////////////
// Assignment Operators
///////////////////////////////////////

inline Quaternion& Quaternion::operator *= (const Quaternion& q)
{
	(quaternion *= q.quaternion);
	return (*this);
}

inline Quaternion& Quaternion::operator += (const Quaternion& q)
{
	quaternion += q.quaternion;
	return (*this);
}

inline Quaternion& Quaternion::operator -= (const Quaternion& q)
{
	quaternion -= q.quaternion;
	return (*this);
}

inline Quaternion& Quaternion::operator *= (float s)
{
	quaternion *= s;
	return (*this);
}

inline Quaternion& Quaternion::operator /= (float s)
{
	quaternion /= s;
	return (*this);
}

///////////////////////////////////////
// Operators
///////////////////////////////////////

inline const Quaternion Quaternion::operator * (const Quaternion& q) const
{ return Quaternion(quaternion * q.quaternion); }

inline const Quaternion Quaternion::operator + (const Quaternion& q) const
{ return Quaternion(quaternion + q.quaternion); }

inline const Quaternion Quaternion::operator - (const Quaternion& q) const
{ return Quaternion(quaternion - q.quaternion); }

inline const Quaternion Quaternion::operator * (float s) const
{ return Quaternion(quaternion * s); }

inline const Quaternion Quaternion::operator / (float s) const
{ return Quaternion(quaternion / s); }

inline const Quaternion operator * (float s, const Quaternion& q)
{ return Quaternion(q * q.quaternion); }

inline const Quaternion Quaternion::Inverse() const
{
	Quaternion q;
	D3DXQuaternionInverse(&q.GetD3DXQuaternion(), &GetD3DXQuaternion());
	return q;
}

inline Quaternion& Quaternion::SlerpThis(const Quaternion& q, float percent)
{
	D3DXQuaternionSlerp(&quaternion, &quaternion, &q.quaternion, percent);
	return *this;
}

inline const Quaternion Quaternion::Slerp(const Quaternion& q, float percent) const
{
	Quaternion qOut;
	D3DXQuaternionSlerp(&qOut.GetD3DXQuaternion(), &quaternion, &q.quaternion, percent);
	return qOut;
}

inline float Quaternion::DotProduct(const Quaternion& q) const { return D3DXQuaternionDot(&quaternion, &q.quaternion); }

inline const Quaternion Quaternion::Normalize() const
{
	Quaternion q;
	D3DXQuaternionNormalize(&q.GetD3DXQuaternion(), &quaternion);
	return q;
}

inline Quaternion& Quaternion::NormalizeThis()
{
	D3DXQuaternionNormalize(&quaternion, &quaternion);
	return *this;
}

inline void Quaternion::GetAxisAngle(Vector3& axis, float& angle) const
{
	D3DXVECTOR3 v;
	D3DXQuaternionToAxisAngle(&quaternion, &v, &angle);
	axis = Vector3(v);
}

///////////////////////////////////////
// Class Statics
///////////////////////////////////////

inline const Quaternion Quaternion::Identity()	{ return Quaternion(0, 0, 0, 1); }

////////////////////////////////////////////////////////////////////////////////////////
/*
Circle
*/
////////////////////////////////////////////////////////////////////////////////////////

inline Circle::Circle()
{}

inline Circle::Circle(const Circle& _c) :
	position(_c.position),
	radius(_c.radius)
{}

inline Circle::Circle(const Vector2& _position, float _radius) :
	position(_position),
	radius(_radius)
{}

inline Circle::Circle(float _radius) :
	position(Vector2::Zero()),
	radius(_radius)
{}

inline Circle& Circle::operator += (const Circle& c)
{
	const Vector2 deltaPos = position - c.position;
	const float distance = deltaPos.Magnitude();

	if (distance + c.radius <= radius)
	{
		// it is already totally inside this one, no need to process further
	} 
	else if (distance + radius <= c.radius)
	{
		// this one is inside one of its childrens spheres, just copy it over
		*this = c;
	}
	else
	{
		// they are seperated by a gap
		const float diameter = distance + radius + c.radius;

		const Vector2 direction = deltaPos.Normalize(distance);
		position -= direction * (diameter * 0.5f - radius);
		radius = diameter * 0.5f;
	}
	return *this;
}

inline const Circle Circle::operator + (const Circle& c) const
{
	Circle circleCombined(*this);
	circleCombined += c;
	return circleCombined;
}

////////////////////////////////////////////////////////////////////////////////////////
/*
Sphere
*/
////////////////////////////////////////////////////////////////////////////////////////

inline Sphere::Sphere()
{}

inline Sphere::Sphere(const Sphere& _s) :
	position(_s.position),
	radius(_s.radius)
{}

inline Sphere::Sphere(const Vector3& _position, float _radius) :
	position(_position),
	radius(_radius)
{}

inline Sphere::Sphere(float _radius) :
position(Vector3::Zero()),
	radius(_radius)
{}

inline Sphere& Sphere::operator += (const Sphere& s)
{
	const Vector3 deltaPos = position - s.position;
	const float distance = deltaPos.Magnitude();

	if (distance + s.radius <= radius)
	{
		// it is already totally inside this one, no need to process further
	} 
	else if (distance + radius <= s.radius)
	{
		// this one is inside one of its childrens spheres, just copy it over
		*this = s;
	}
	else
	{
		// they are seperated by a gap
		const float diameter = distance + radius + s.radius;

		const Vector3 direction = deltaPos.Normalize(distance);
		position -= direction * (diameter * 0.5f - radius);
		radius = diameter * 0.5f;
	}
	return *this;
}

inline const Sphere Sphere::operator + (const Sphere& s) const
{
	Sphere sphereCombined(*this);
	sphereCombined += s;
	return sphereCombined;
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	2d Transform Class
*/
////////////////////////////////////////////////////////////////////////////////////////

inline XForm2::XForm2() {}

inline XForm2::XForm2(const XForm2& otherXForm) :
	position(otherXForm.position),
	angle(otherXForm.angle)
{}

inline XForm2::XForm2(const b2Transform& otherXForm) :
	position(otherXForm.p),
	angle(otherXForm.q.GetAngle())
{}

inline XForm2::XForm2(const Vector2& _position, float _angle) :
	position(_position),
	angle(_angle)
{}

inline XForm2::XForm2(float _angle) :
	position(Vector2::Zero()),
	angle(_angle)
{}

inline const XForm2 XForm2::Inverse() const { return XForm2(-position) * XForm2(-angle); }
inline const Vector2 XForm2::GetUp() const { return Vector2::BuildFromAngle(angle); }
inline const Vector2 XForm2::GetRight() const  { return Vector2::BuildFromAngle(angle + PI/2); }

inline const XForm2 XForm2::Interpolate(const XForm2& xfDelta, float percent) const 
{
	ASSERT(percent >=0 && percent <= 1);

	return XForm2
	(
		position - percent * xfDelta.position,
		angle - percent * xfDelta.angle
	);
}

///////////////////////////////////////
// Assignment Operators
///////////////////////////////////////

inline const XForm2 XForm2::operator + (const XForm2& xf) const
{
	return XForm2
	(
		position + xf.position,
		CapAngle(angle + xf.angle)
	);
}

inline const XForm2 XForm2::operator - (const XForm2& xf) const
{
	return XForm2
	(
		position - xf.position,
		CapAngle(angle - xf.angle)
	);
}

inline const XForm2 XForm2::operator * (float scale) const
{
	return XForm2
	(
		position*scale,
		CapAngle(angle*scale)
	);
}

inline XForm2& XForm2::operator *= (const XForm2& xf)
{
	position = position.Rotate(xf.angle) + xf.position;
	angle = angle + xf.angle;

	return (*this);
}

inline const XForm2 XForm2::operator * (const XForm2& xf) const 
{ 
	return XForm2
	(
		position.Rotate(xf.angle) + xf.position,
		CapAngle(angle + xf.angle)
	); 
}

inline const Vector2 XForm2::TransformCoord(const Vector2& v) const
{
	return Vector2(v.Rotate(angle) + position);
}

inline const Vector2 XForm2::TransformVector(const Vector2& v) const
{
	return Vector2(v.Rotate(angle));
}

///////////////////////////////////////
// Class Statics
///////////////////////////////////////

inline const XForm2 XForm2::Identity()	{ return XForm2(Vector2(0,0), 0); }


////////////////////////////////////////////////////////////////////////////////////////
/*
	Line3 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

inline Line3::Line3() {}
inline Line3::Line3(const Vector3& _p1, const Vector3& _p2) : 
	p1(_p1), 
	p2(_p2)
{};

////////////////////////////////////////////////////////////////////////////////////////
/*
	BBox
*/
////////////////////////////////////////////////////////////////////////////////////////

inline Box2AABB::Box2AABB() {}
inline Box2AABB::Box2AABB(const Vector2& point)
{
	lowerBound = point;
	upperBound = point;
}

inline Box2AABB::Box2AABB(const Vector2& point1, const Vector2& point2)
{
	if (point1.x < point2.x)
	{
		lowerBound.x = point1.x;
		upperBound.x = point2.x;
	}
	else
	{
		lowerBound.x = point2.x;
		upperBound.x = point1.x;
	}
	if (point1.y < point2.y)
	{
		lowerBound.y = point1.y;
		upperBound.y = point2.y;
	}
	else
	{
		lowerBound.y = point2.y;
		upperBound.y = point1.y;
	}
};

inline Box2AABB::Box2AABB(const b2AABB& b)
{
	lowerBound = b.lowerBound;
	upperBound = b.upperBound;
}

// return the combined boxes
inline const Box2AABB Box2AABB::operator + (const Box2AABB& b) const
{
	return Box2AABB
	(
		Vector2(Min(lowerBound.x, b.lowerBound.x), Min(lowerBound.y, b.lowerBound.y)),
		Vector2(Max(upperBound.x, b.upperBound.x), Max(upperBound.y, b.upperBound.y))
	);
}

// return the combined boxes
inline Box2AABB& Box2AABB::operator += (const Box2AABB& b)
{
	lowerBound.x = Min(lowerBound.x, b.lowerBound.x);
	lowerBound.y = Min(lowerBound.y, b.lowerBound.y);
	upperBound.x = Max(upperBound.x, b.upperBound.x);
	upperBound.y = Max(upperBound.y, b.upperBound.y);
	return (*this);
}

inline bool Box2AABB::Contains(const Vector2& v) const
{
	return
	(
		v.x >= lowerBound.x && 
		v.x <= upperBound.x && 
		v.y >= lowerBound.y &&
		v.y <= upperBound.y
	);
}

inline bool Box2AABB::FullyContains(const Box2AABB& b) const
{
	return
	(
		b.lowerBound.x >= lowerBound.x && 
		b.upperBound.x <= upperBound.x && 
		b.lowerBound.y >= lowerBound.y &&
		b.upperBound.y <= upperBound.y
	);
}

inline bool Box2AABB::PartiallyContains(const Box2AABB& b) const
{
	return
	(
		b.upperBound.x >= lowerBound.x && 
		b.lowerBound.x <= upperBound.x && 
		b.upperBound.y >= lowerBound.y &&
		b.lowerBound.y <= upperBound.y
	);
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Line2 Class
*/
////////////////////////////////////////////////////////////////////////////////////////

inline Line2::Line2() {}
inline Line2::Line2(const Vector2& _p1, const Vector2& _p2) : 
	p1(_p1), 
	p2(_p2)
	{};

// creates a line representing diagonal of a square
inline Line2::Line2(float size) : 
	p1(size), 
	p2(-size)
{};

inline Line2& Line2::operator += (const Vector2& v)
{
	p1 += v;
	p2 += v;
	return (*this);
}

inline Line2& Line2::operator -= (const Vector2& v)
{
	p1 -= v;
	p2 -= v;
	return (*this);
}

inline const Line2 Line2::operator + (const Vector2& v) const
{
	return (Line2(p1 + v, p2 + v));
}

inline const Line2 Line2::operator - (const Vector2& v) const
{
	return (Line2(p1 - v, p2 - v));
}

#endif // FRANK_MATH_INLINE