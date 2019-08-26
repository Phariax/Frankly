////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object Builder
	Copyright 2013 Frank Force - http://www.frankforce.com

	- use this instead of new to build game objects
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_OBJECT_BUILDER_H
#define GAME_OBJECT_BUILDER_H

#include "../objects/gameObject.h"

enum GameObjectType;
const GameObjectType GAME_OBJECT_TYPE_MAX_COUNT = (GameObjectType)256;
const GameObjectType GAME_OBJECT_TYPE_INVALID = (GameObjectType)0;

struct ObjectTypeInfo;
struct GameObjectStub;
extern ObjectTypeInfo* g_gameObjectInfoArray[GAME_OBJECT_TYPE_MAX_COUNT];

// holds information about an object type / class
// constructor automatically adds this to the global array
struct ObjectTypeInfo
{
public:
	
	typedef GameObject* (*BuildObjectFunction)(const GameObjectStub& stub);
	typedef bool (*IsSerializableFunction)();
	typedef void (*StubRenderFunction)(const GameObjectStub& stub, float alpha);
	typedef WCHAR* (*StubDescriptionFunction)();
	
	ObjectTypeInfo
	(
		const WCHAR* _name, 
		GameObjectType _type, 
		BuildObjectFunction _buildObjectFunction, 
		IsSerializableFunction _isSerializableFunction,
		StubRenderFunction _stubRenderFunction, 
		StubDescriptionFunction _stubAttributesDescriptionFunction, 
		StubDescriptionFunction _stubDescriptionFunction, 
		GameTextureID _ti = GameTextureID(0), 
		const Color& _stubColor = Color::White()
	) :
		name(_name),
		type(_type),
		ti(_ti),
		stubColor(_stubColor),
		buildObjectFunction(_buildObjectFunction),
		isSerializableFunction(_isSerializableFunction),
		stubRenderFunction(_stubRenderFunction),
		stubAttributesDescriptionFunction(_stubAttributesDescriptionFunction),
		stubDescriptionFunction(_stubDescriptionFunction)
	{
		// init my entry in the global object info index array
		ASSERT(!g_gameObjectInfoArray[type]);
		g_gameObjectInfoArray[type] = this;

		if (type > maxType)
			maxType = type;
	}

	const WCHAR* GetName() const		{ return name; }
	GameObjectType GetType() const		{ return type; }
	GameTextureID GetTexture() const	{ return ti; }
	const Color& GetColor() const		{ return stubColor; }
	GameObject* BuildObject(const GameObjectStub& stub) const { return buildObjectFunction(stub); }
	void StubRender(const GameObjectStub& stub, float alpha) const { stubRenderFunction(stub, alpha); }
	const WCHAR* GetAttributesDescription() const { return stubAttributesDescriptionFunction(); }
	const WCHAR* GetDescription() const { return stubDescriptionFunction(); }
	bool IsSerializable() const { return isSerializableFunction(); }

	// get maximum registered object type, used by editors
	static GameObjectType GetMaxType() { return maxType; }

	static const ObjectTypeInfo invalidTypeInfo;

private:

	const WCHAR* name;
	GameObjectType type;
	GameTextureID ti;
	Color stubColor;
	BuildObjectFunction buildObjectFunction;
	IsSerializableFunction isSerializableFunction;
	StubRenderFunction stubRenderFunction;
	StubDescriptionFunction stubAttributesDescriptionFunction;
	StubDescriptionFunction stubDescriptionFunction;
	static GameObjectType maxType;
};

// holds information about how to create a specific object
struct GameObjectStub
{
public:
	GameObjectStub
	(
		const XForm2& _xf = XForm2::Identity(), 
		const Vector2& _size = Vector2(1), 
		GameObjectType _type = GAME_OBJECT_TYPE_INVALID, 
		const char* _attributes = NULL, 
		GameObjectHandle _handle = GameObject::invalidHandle
	) :
		xf(_xf),
		size(_size),
		type(_type),
		handle(_handle)
	{
		if (_attributes)
			strncpy_s(attributes, sizeof(attributes), _attributes, sizeof(attributes));	
		else
			*attributes = 0;
	}

	static const int attributesLength = 256;// length of attributes string
	GameObjectType type;					// type of object
	XForm2 xf;								// pos and angle of object
	Vector2 size;							// width and height (may not apply to some)
	char attributes[attributesLength];		// this value can be different depending on the object
	GameObjectHandle handle;				// static handle for this stub / object
	
	static bool HasObjectInfo(GameObjectType type) 
	{ 
		ASSERT(type >= 0 && type < GAME_OBJECT_TYPE_MAX_COUNT);
		return (type > GAME_OBJECT_TYPE_INVALID && type <= ObjectTypeInfo::GetMaxType()) && g_gameObjectInfoArray[g_gameObjectInfoArray[type]? type : 0] != NULL;
	}
	static const ObjectTypeInfo& GetObjectInfo(GameObjectType type) 
	{ 
		ASSERT(type >= 0 && type < GAME_OBJECT_TYPE_MAX_COUNT);
		if (HasObjectInfo(type))
			return *g_gameObjectInfoArray[g_gameObjectInfoArray[type]? type : 0]; 
		else
			return ObjectTypeInfo::invalidTypeInfo;
	}
	const ObjectTypeInfo& GetObjectInfo() const { return GetObjectInfo(type); }
	GameObject* BuildObject() const { return GetObjectInfo().BuildObject(*this); }
	bool HasObjectInfo() const	{ return g_gameObjectInfoArray[type] != NULL; }

	void Render(float alpha = 1) const { if (HasObjectInfo()) GetObjectInfo().StubRender(*this, alpha); }

	// test if a point is inside the box defined by the stub
	bool TestPosition(const Vector2& pos) const { return Vector2::InsideBox(xf, size, pos); }

	int GetAttributesInt() const
	{ int attributesInt = 0; sscanf_s(attributes, "%d", &attributesInt); return attributesInt; }

	void GetAttributesPrintable(char* text) const;

	Box2AABB GetAABB() const { return Box2AABB(xf, size); }
};

// use this macro to expose a class
#define	GAME_OBJECT_DEFINITION(className, stubTexture, stubColor) \
static GameObject* className##Build(const GameObjectStub& stub) \
{ return new className(stub); } \
static ObjectTypeInfo className##Info(L#className, GOT_##className, className##Build, className##::IsSerializable, className##::StubRender, className##::StubAttributesDescription, className##::StubDescription, stubTexture, stubColor);

#endif // GAME_OBJECT_BUILDER_H