////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object Builder
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"

static GameObject* InvalidObjectBuild(const GameObjectStub& stub) { return NULL; }
static bool InvalidObjectIsSerializable() { return false; }
const ObjectTypeInfo ObjectTypeInfo::invalidTypeInfo
(
	L"Invalid Type", 
	GameObjectType(0), 
	InvalidObjectBuild, 
	InvalidObjectIsSerializable, 
	GameObject::StubRender, 
	GameObject::StubAttributesDescription, 
	GameObject::StubDescription
);

ObjectTypeInfo* g_gameObjectInfoArray[GAME_OBJECT_TYPE_MAX_COUNT] = {0};

// what is the maximum registered object type
GameObjectType ObjectTypeInfo::maxType = GameObjectType(0);

// get printable version of attributes with newline characters replaces
void GameObjectStub::GetAttributesPrintable(char* text) const
{
	strncpy_s(text, attributesLength, attributes, attributesLength);

	// replace /n with new lines
	int j = 0;
	for(int i = 0; i < attributesLength-2; ++i)
	{
		if (text[i] == '\\' && text[i+1] == 'n')
		{
			++i;
			text[i] = 10;
		}

		text[j] = text[i];
		++j;
	}
	text[j] = 0;
}
