////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Utilities
	Copyright 2013 Frank Force - http://www.frankforce.com

	- Utility library, helper functions for game stuff
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRANK_UTIL_H
#define FRANK_UTIL_H

#include "../core/frankMath.h"

#define DEBUG_OUTPUT( v ) _DEBUG_OUTPUT( #v, v )
#define _DEBUG_OUTPUT( n, v )					\
{												\
	std::wostringstream string;					\
	string << (n) << " = " << (v) << L"\n";		\
	OutputDebugStringW(string.str().c_str());	\
}

namespace FrankUtil
{

// returns best angle to hit target
// returns false if it is not possible to hit the target
bool GetTrajectoryAngleWorld(const Vector2& startPos, const Vector2& targetPos, float speed, float& angle);
bool GetTrajectoryAngleWorld(const GameObject& attacker, const GameObject& target, float speed, float& angle);
bool GetTrajectoryAngleWorld(const GameObject& attacker, const Vector2& targetPos, float speed, float& angle);
bool GetTrajectoryAngleWorld(const Vector2& startPos, const Vector2& attackerVelocity, const Vector2& targetPos, float speed, float& angle);

// simple way to aim a weapon taking gravity and owner velocity into account
float GetAimAngle(const Vector2& ownerPos, const Vector2& ownerVelocity, float speed, bool usingGamepad);

// gravity functions
inline float GetAltitude(const Vector2& pos) 
{ 
	if (!g_terrain->isCircularPlanet)
		return pos.y;

	return pos.Magnitude(); 
}

inline float GetAltitudePercent(const Vector2& pos)
{
	if (!g_terrain->isCircularPlanet)
		return 0;

	// objects have some things change as it increases in altitude
	// this is to make things transition well over an altitude band
	return Percent(GetAltitude(pos), g_terrain->planetMinAltitude, g_terrain->planetMaxAltitude);
}

inline Vector2 CalculateOrbitalVelocity(const Vector2& pos)
{
	if (!g_terrain->isCircularPlanet)
		return Vector2::Zero();

	// use distance from origin
	Vector2 direction = pos;
	const float distance = direction.MagnitudeAndNormalize();
	if (distance < 10)
		return Vector2::Zero(); // prevent divide by zero

	const float speed = sqrtf(g_terrain->planetGravityConstant / distance);
	//ASSERT(speed < b2_maxLinearVelocity);
	return speed * direction.Rotate(-PI/2);
}

inline Vector2 CalculateGravity(const Vector2& pos)
{
	if (!g_terrain->isCircularPlanet)
		return g_terrain->gravity;

	// use distance from origin
	Vector2 direction = pos;
	const float distance = direction.MagnitudeAndNormalize();

	// gravity fades out twoards the center for now
	float a = 0;
	if (distance > g_terrain->planetRadius)
		a = g_terrain->planetGravityConstant / (distance*distance);
	else
		a = (distance / g_terrain->planetRadius) * (g_terrain->planetGravityConstant / (g_terrain->planetRadius*g_terrain->planetRadius));

	return -direction * a;
}

struct AttributesStringParser
{
	AttributesStringParser(const char* _string) : string(_string), stringStart(_string) {}
	
	void parseValue(float& _value);
	void parseValue(int& _value);
	void parseValue(unsigned& _value);
	void parseValue(short& _value);
	void parseValue(unsigned char& _value);
	void parseValue(char& _value);
	void parseValue(bool& _value);
	void parseValue(GameTextureID& _value, IntVector2& tileSize);

	bool skipToNextValue(char stringMarker);
	bool skipToMarker(char marker);
	int getOffset() const { return string - stringStart; }

	void skipWhiteSpace()
	{
		ASSERT(string);
		while(*string && (*string == ' ')) ++string;
	}

	bool isAtEnd() 
	{
		ASSERT(string);
		return *string != 0; 
	}

	void moveBack()
	{
		if (string <= stringStart)
			return;
		
		--string;
	}

	void setString(const char* _string) { string = _string; }
	const char* getString() const { return string; }
	const char* getStringStart() const { return stringStart; }

private:

	const char* string;
	const char* stringStart;
};

};	// namespace FrankUtil

#endif // FRANK_UTIL_H