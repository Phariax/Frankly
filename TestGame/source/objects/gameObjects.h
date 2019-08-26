////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Objects
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_OBJECTS_H
#define GAME_OBJECTS_H

////////////////////////////////////////////////////////////////////////////////////////
/*
	MusicBox
*/
////////////////////////////////////////////////////////////////////////////////////////

class MusicBox : public GameObject
{
public:
	
	static WCHAR* StubDescription();
	static WCHAR* StubAttributesDescription();

	MusicBox(const GameObjectStub& stub);
	void Render() {}
	bool ShouldCollideSight() const { return false; }
	void CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture);
	bool ShouldCollide(const GameObject& otherObject, const b2Fixture* myFixture, const b2Fixture* otherFixture) const
	{
		return otherObject.IsPlayer() && otherFixture;
	}

	char filename[GameObjectStub::attributesLength];
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	TextDecal

	- displays text in world space
*/
////////////////////////////////////////////////////////////////////////////////////////

class TextDecal : public GameObject
{
public:

	TextDecal(const GameObjectStub& stub);
	void Render();
	void RenderPost();
	static WCHAR* StubDescription() { return L"Draws a text decal"; }
	static WCHAR* StubAttributesDescription() { return L"r g b a type #text"; }
	static void StubRender(const GameObjectStub& stub, float alpha);

private:

	enum DisplayType
	{
		DisplayType_default,
		DisplayType_noNormals,		// skip normal render, uses normals of whatever was below it
		DisplayType_emissive,		// render emissive text
		DisplayType_overlay,		// render text on top of everything
		DisplayType_castShadows,	// allows text to cast shadows
	};

	char text[GameObjectStub::attributesLength];
	Vector2 size;
	Color color;
	DisplayType type;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	PopupMessage
*/
////////////////////////////////////////////////////////////////////////////////////////

class PopupMessage : public GameObject
{
public:

	PopupMessage(const XForm2& xf, GameObject* _parent, char* _text, float _size, float time=0, const Color& _color = Color::White(), bool autoDestruct = true);
	
	void Udapte();
	void RenderPost();
	void SetMessage(const XForm2& xf, const char* _text, float size, float time, const Color& color = Color::White());

	const char* GetText() const			{ return text; }
	Color GetColor() const				{ return color; }
	float GetSize()	const				{ return size; }
	
	void SetText(const char* _text)		{ strncpy_s(text, _text, sizeof(text)); }
	void SetColor(const Color& _color)	{ color = _color; }
	void SetSize(float _size)			{ size = _size; }

	// prevent messages from getting destroyed when outside the window
	bool IsPersistant() const			{ return true; }
	
	static WCHAR* StubDescription() { return L"Shows a popup message when hit"; }
	static WCHAR* StubAttributesDescription() { return L"text"; }

private:

	char text[1024];
	float size;
	Color color;
	GameTimer messageTimer;
	float messageFadeTime;
	bool autoDestruct;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	HelpSwitch

	- displays the attributes string when switch is hit
*/
////////////////////////////////////////////////////////////////////////////////////////

class HelpSwitch : public GameObject
{
public:

	HelpSwitch(const GameObjectStub& stub);
	void Update();
	void Render();
	virtual void CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture);
	static WCHAR* StubDescription() { return L"attributes text will display when switch is triggered"; }
	static WCHAR* StubAttributesDescription() { return L"text"; }
	void ForceActivate(bool activate) { if (activate) forceActivateTimer.Set(); }

private:

	Vector2 size;
	char text[GameObjectStub::attributesLength];
	GameTimer hitTimer;
	GameTimer forceActivateTimer;
};


////////////////////////////////////////////////////////////////////////////////////////
/*
	ObjectSpawner

	- complex object that can spawn other objects via it's attributes
*/
////////////////////////////////////////////////////////////////////////////////////////

class ObjectSpawner : public GameObject
{
public:

	ObjectSpawner(const GameObjectStub& stub);
	void Update();
	void Render();
	GameObject* SpawnObject();
	virtual void CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture);
	static WCHAR* StubDescription() { return L"Spawns objects of a given type"; }
	static WCHAR* StubAttributesDescription() { return L"type size offset speed max randomness rate auto"; }
	static void StubRender(const GameObjectStub& stub, float alpha);
	void ForceActivate(bool activate) { spawnAuto = activate; if (activate) forceActivateTimer.Set(); }

private:

	Vector2 size;
	GameTimerPercent spawnTimer;
	list<GameObjectHandle> spawnedObjects;
	UINT spawnMax;
	float spawnSize;
	GameObjectType spawnType;
	float spawnSpeed;
	float spawnOffset;
	float spawnRandomness;
	float spawnRate;
	bool spawnAuto;
	GameTimer forceActivateTimer;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Crate

	- explodable crate that takes damage from weapons or collisions
*/
////////////////////////////////////////////////////////////////////////////////////////

class Crate : public Actor
{
public:

	Crate(const GameObjectStub& stub);	
	
	void Update();
	void Render();
	void Kill();
	void Explode();
	void CollisionResult(GameObject& otherObject, const ContactResult& contactResult, b2Fixture* myFixture, b2Fixture* otherFixture);
	
	static bool IsSerializable() { return true; } 
	static WCHAR* StubDescription() { return L"A physical crate."; }

	Vector2 size;
	float effectSize;
	GameTimerPercent explosionTimer;
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Test Area

	- track how long player is in the test area and displays messages
*/
////////////////////////////////////////////////////////////////////////////////////////

class TriggerBox : public GameObject
{
public:

	TriggerBox(const GameObjectStub& stub);	
	
	void Update();
	void Render();

	bool ShouldCollide(const GameObject& otherObject, const b2Fixture* myFixture, const b2Fixture* otherFixture) const
	{ return otherObject.IsPlayer(); } // only collide with player

	void CollisionPersist(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture);
	
	static WCHAR* StubDescription() { return L"Trigger box area used for the mosh pit test."; }

	static float highTime;
	GameTimer moshPitTimer;
	GameTimer touchedPlayerTimer;
};

#endif // GAME_OBJECTS_H