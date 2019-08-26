////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Utilities
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"

namespace FrankUtil
{

// simple way to aim a weapon taking gravity and owner velocity into account
float GetAimAngle(const Vector2& ownerPos, const Vector2& ownerVelocity, float speed, bool usingGamepad)
{
	float angle = 0;

	// update weapon aiming
	static bool playerAimDebug = false;
	ConsoleCommand(playerAimDebug, playerAimDebug);

	if (usingGamepad)
	{
		// gamepad control
		const Vector2 gamepadRightStick = g_input->GetGamepadRightStick(0);
		angle = gamepadRightStick.Normalize().GetAngle();
	}
	else
	{
		// automatically aim gun at best possible trajectory to hit mouse pos
		const Vector2 mousePos = g_input->GetMousePosWorldSpace();

		// calculate the trajectory angle to hit the mouse pos
		bool canHit = FrankUtil::GetTrajectoryAngleWorld(ownerPos, ownerVelocity, mousePos, speed, angle);
		if (!canHit)
		{
			// if we cant hit mouse just try to shoot farthest towards target as possible
			Vector2 startPos = ownerPos;
			Vector2 endPos = mousePos;
			if (playerAimDebug)
				mousePos.RenderDebug();
			for (int i = 0; i < 20; ++i)
			{
				// iterate to find a close solution

				// test half way betwen start and end
				const Vector2 testPos = startPos + (endPos - startPos) * 0.5f;
				canHit = FrankUtil::GetTrajectoryAngleWorld(ownerPos, ownerVelocity, testPos, speed, angle);
				if (canHit)
				{
					if (playerAimDebug)
						testPos.RenderDebug(Color::Green());
					startPos = testPos;
				}
				else
				{
					if (playerAimDebug)
						testPos.RenderDebug(Color::Yellow());
					endPos = testPos;
				}
			}
			if (playerAimDebug)
				startPos.RenderDebug(Color::Red());

			// get the best trajectory
			canHit = FrankUtil::GetTrajectoryAngleWorld(ownerPos, ownerVelocity, startPos, speed, angle);
		}

		if (!canHit)
		{
			// just point the weapon in the direction of the mouse
			const Vector2 deltaPos = mousePos - ownerPos;
			angle = deltaPos.GetAngle();
		}
	}

	if (playerAimDebug)
	{
		// trajectory display
		float deltaTime = 0.1f;
		float time = 0;
		float projectileSpeed = speed;
		Vector2 velocityStart = ownerVelocity + Vector2::BuildFromAngle(angle) * projectileSpeed;
		Vector2 posStart = ownerPos;
		Vector2 pos = posStart;
		Vector2 lastPos = pos;
		while (time < 2)
		{
			pos = posStart + velocityStart * time + 0.5f * CalculateGravity(lastPos) * time * time;
			Line2(lastPos, pos).RenderDebug();
			lastPos = pos;
			time += deltaTime;
		}
	}
	
	return angle;
}


// returns trajectory angles for a deltaPos with gravity in the -y direction
// returns false if it is not possible to hit the target
bool GetTrajectoryAngle(Vector2 deltaPos, float speed, float gravity, float& angle1, float& angle2)
{
	// equation from wikipedia article on trajectory of a projectile under "Angle required to hit coordinate (x,y)"
	const float speed2 = speed * speed;
	const float temp = speed2 * speed2 - gravity * (gravity * deltaPos.x * deltaPos.x + 2 * deltaPos.y * speed2);

	if (temp <= 0)
		return false;

	if (gravity * deltaPos.x == 0)
		return false;

	angle1 = atanf((speed2 + sqrtf(temp)) / (gravity * deltaPos.x));
	angle2 = atanf((speed2 - sqrtf(temp)) / (gravity * deltaPos.x));
	if (deltaPos.x > 0)
	{
		// reverse the angle when shooting to the left
		angle1 -= PI;
		angle2 -= PI;
	}

	// correct for space where 0 radians is up instead of right
	angle1 += PI/2;
	angle2 += PI/2;

	return true;
}

bool GetTrajectoryAngleWorld(const Vector2& startPos, const Vector2& targetPos, float speed, float& angle)
{
	// just use gravity at the start position to simplify
	const Vector2 gravity = CalculateGravity(startPos);

	// get transform where gravity is down
	float gravityAngle = (-gravity).GetAngle();

	// put delta pos in gravity space
	const Vector2 deltaPos = (targetPos - startPos).Rotate(-gravityAngle);

	// calculate trajectory
	float angle1, angle2;
	if (!GetTrajectoryAngle(deltaPos, speed, gravity.Magnitude(), angle1, angle2))
		return false;

	// figure out which angle is closer to a straight shot
	const float targetAngle = deltaPos.GetAngle();
	const float deltaAngle1 = CapAngle(angle1 - targetAngle);
	const float deltaAngle2 = CapAngle(angle2 - targetAngle);
	angle = (fabs(deltaAngle1) < fabs(deltaAngle2))? angle1 : angle2;

	// put result angle back in world space
	angle += gravityAngle;

	// debug render
	//Line2(startPos, startPos + 20 * Vector2::BuildFromAngle(angle)).RenderDebug(Color::White());
	//Line2(startPos, startPos + 10 * Vector2::BuildFromAngle(angle1 + gravityAngle)).RenderDebug(Color::Red());
	//Line2(startPos, startPos + 10 * Vector2::BuildFromAngle(angle2 + gravityAngle)).RenderDebug(Color::Blue());
	//Line2(startPos, startPos + 10*Vector2::BuildFromAngle(targetAngle + gravityAngle)).RenderDebug(Color::Yellow());

	return true;
}

bool GetTrajectoryAngleWorld(const GameObject& attacker, const GameObject& target, float speed, float& angle)
{
	const Vector2 startPos = attacker.GetPosWorld();
	const Vector2 endPos = target.GetPosWorld();
	const Vector2 deltaPos = endPos - startPos;
	const Vector2 deltaVelocity = target.GetVelocity() - attacker.GetVelocity();

	// estimate where target would be if we had a straight shot
	float time = 0;
	for (int i = 0; i < 2; ++i)
	{
		// iterate a few times to get a better estimate
		time = (deltaPos + time * deltaVelocity).Magnitude() / speed;
		//(endPos + time * target.GetVelocity()).RenderDebug();
	}

	return GetTrajectoryAngleWorld(startPos, endPos + time * deltaVelocity, speed, angle);
}

bool GetTrajectoryAngleWorld(const GameObject& attacker, const Vector2& targetPos, float speed, float& angle)
{
	const Vector2 startPos = attacker.GetPosWorld();
	const Vector2 endPos = targetPos;
	const Vector2 deltaPos = endPos - startPos;
	const Vector2 deltaVelocity = -attacker.GetVelocity();

	// estimate where target would be if we had a straight shot
	float time = 0;
	for (int i = 0; i < 2; ++i)
	{
		// iterate a few times to get a better estimate
		time = (deltaPos + time * deltaVelocity).Magnitude() / speed;
		//(endPos + time * target.GetVelocity()).RenderDebug();
	}

	return GetTrajectoryAngleWorld(startPos, endPos + time * deltaVelocity, speed, angle);
}

bool GetTrajectoryAngleWorld(const Vector2& startPos, const Vector2& attackerVelocity, const Vector2& targetPos, float speed, float& angle)
{
	const Vector2 endPos = targetPos;
	const Vector2 deltaPos = endPos - startPos;
	const Vector2 deltaVelocity = -attackerVelocity;

	// estimate where target would be if we had a straight shot
	float time = 0;
	for (int i = 0; i < 2; ++i)
	{
		// iterate a few times to get a better estimate
		time = (deltaPos + time * deltaVelocity).Magnitude() / speed;
		//(endPos + time * target.GetVelocity()).RenderDebug();
	}

	return GetTrajectoryAngleWorld(startPos, endPos + time * deltaVelocity, speed, angle);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Attributes String Parser
//

bool AttributesStringParser::skipToNextValue(char stringMarker)
{
	// skips over the next value, returns true if a value was found
	ASSERT(string);

	// skip start white space
	skipWhiteSpace();

	// skip to next value
	if (!(*string))
		return false;

	if (*string == stringMarker)
	{
		++string;
		// skip until another string marker
		while(*string && (*string != stringMarker)) ++string;
	}
	else
	{
		// skip any non-white space
		while(*string && (*string != ' ')) ++string;
	}
	
	// skip end white space
	skipWhiteSpace();
	
	return true;
}

bool AttributesStringParser::skipToMarker(char marker)
{
	ASSERT(string);

	while(1)
	{
		if (!*string)
			return false;
		else if (*string == marker)
		{
			++string;
			return true;
		}
		++string;
	}
}

void AttributesStringParser::parseValue(int& _value)
{
	ASSERT(string);
		
	skipWhiteSpace();

	// check the sign
	int sign = 1;
	if (*string == '-')
	{
		sign = -1;
		++string;
	}
	
	if (*string < '0' || *string > '9')
	{
		// value is not changed if a number isn't found
		return;
	}

	// read in the int
	int value = 0;
	while (*string >= '0' && *string <= '9')
	{
		value = 10*value + *string - '0';
		++string;
	}
		
	// save the value
	_value = value * sign;
}

void AttributesStringParser::parseValue(unsigned& _value)
{
	int valueInt = _value;
	parseValue(valueInt);
	ASSERT(valueInt >= 0);
	_value = valueInt;
}

void AttributesStringParser::parseValue(unsigned char& _value)
{
	int valueInt = _value;
	parseValue(valueInt);
	ASSERT(valueInt <= 255 && valueInt >= 0);
	_value = valueInt;
}

void AttributesStringParser::parseValue(char& _value)
{
	int valueInt = _value;
	parseValue(valueInt);
	ASSERT(valueInt <= 127 && valueInt >= -127);
	_value = valueInt;
}

void AttributesStringParser::parseValue(short& _value)
{
	int valueInt = _value;
	parseValue(valueInt);
	_value = valueInt;
}

void AttributesStringParser::parseValue(bool& _value)
{
	int valueInt = _value;
	parseValue(valueInt);
	_value = (valueInt != 0);
}

void AttributesStringParser::parseValue(float& _value)
{
	ASSERT(string);
		
	skipWhiteSpace();

	// check the sign
	float sign = 1;
	if (*string == '-')
	{
		sign = -1;
		++string;
	}

	if ((*string < '0' || *string > '9') &&  *string != '.')
	{
		// value is not changed if a number isn't found
		return;
	}

	// read in the integer part
	float value = 0;
	while (*string >= '0' && *string <= '9')
	{
		value = 10*value + *string - '0';
		++string;
	}
	
	// check for a decimal point
	if (*string == '.')
	{
		// skip the decimal point
		++string;
	}
	else
	{
		// save the value
		_value = value * sign;
		return;
	}
	
	// read in the fractional part
	float place = 0;
	while (*string >= '0' && *string <= '9')
	{
		float a =  (*string - '0') / (pow(10, ++place));
		value = value + a;
		++string;
	}
		
	// save the value
	_value = value * sign;
}

struct NameImagePair
{
	NameImagePair(GameTextureID textureId, const IntVector2& _tileSize, const char* name) : image(textureId), tileSize(_tileSize), hash(hashString(name)) {}

	const GameTextureID image;
	IntVector2 tileSize;
	unsigned long hash;

	static unsigned long hashString(const char* str)
	{
		unsigned long hash = 5381;
		unsigned long c;

		while ((c = *str++))
			hash = ((hash << 5) + hash) + c;

		return hash;
	}
};

static const NameImagePair nameImagePairMap[] =
{
	NameImagePair(GameTexture_Invalid,			IntVector2(16, 16),		"test"),
};
static const int nameImagePairCount = ARRAY_SIZE(nameImagePairMap);

void AttributesStringParser::parseValue(GameTextureID& ti, IntVector2& tileSize)
{
	// read in a string and convert it to an asset image
	skipWhiteSpace();
	
	int i = 0;
	char imageName[256];
	while (*string != ' ' && *string != '#' && *string != '\0')
	{
		// convert to lower case
		char c = *string;
		if (c >= 'A' && c <= 'Z')
			c += 'a' - 'A';
		imageName[i] = c;
		++i;
		++string;
	} 
	imageName[i] = '\0';

	// look for a match
	const unsigned long hash = NameImagePair::hashString(imageName);
	for (int i = 0; i < nameImagePairCount; ++i)
	{
		if (hash == nameImagePairMap[i].hash)
		{
			ti = nameImagePairMap[i].image;
			tileSize = nameImagePairMap[i].tileSize;
			const IntVector2 textureSize = g_render->GetTextureSize(ti);
			tileSize = textureSize / nameImagePairMap[i].tileSize;
			return;
		}
	}

	if (ti == GameTexture_Invalid)
		return;

	// fixup the passed in tile size
	const IntVector2 textureSize = g_render->GetTextureSize(ti);
	tileSize = textureSize / tileSize;
}















};	// namespace FrankUtil