////////////////////////////////////////////////////////////////////////////////////////
/*
	Physics Controller
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef PHYSICS_H
#define PHYSICS_H

// this is the global physics controller
extern class Physics* g_physics;

class ContactFilter : public b2ContactFilter
{
public:

	/// Return true if contact calculations should be performed between these two shapes.
	virtual bool ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB);
};

class DestructionListener : public b2DestructionListener
{
public:
	void SayGoodbye(b2Fixture* fixture) { B2_NOT_USED(fixture); }
	void SayGoodbye(b2Joint* joint) { B2_NOT_USED(joint); }

};

class ContactListener : public b2ContactListener
{
public:
	void BeginContact(b2Contact* contact);
	void EndContact(b2Contact* contact);
	void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
	void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse);
};

struct ContactEvent
{
	b2Fixture* fixtureA;
	b2Fixture* fixtureB;
	b2Contact* contact;
	Vector2 point;
	Vector2 normal;
};

struct ContactResult
{
	ContactEvent ce;
	float32 normalImpulse;
	float32 tangentImpulse;
};

struct SimpleRaycastResult
{
	Vector2 point;
	Vector2 normal;
	b2Fixture* hitFixture;
	float lambda;

	void RenderDebug(const Color& color = Color::White(0.5f), float radius = 0.1f, float time = 0.0f) const;
};

class Physics : private Uncopyable
{
public:

	Physics();
	~Physics();

	void Init();
	void Render();
	void Update(float delta);

	int GetBodyCount() const;
	int GetProxyCount() const;

	void Raycast(const Line2& line, b2RayCastCallback& raycastResult);
	class GameObject* RaycastSimple(const Line2& line, SimpleRaycastResult* result = NULL, const GameObject* ignoreObject = NULL, bool sightCheck = false);

	unsigned QueryAABB(const Box2AABB& box, GameObject** hitObjects = NULL, unsigned maxHitObjectCount = 0, const GameObject* ignoreObject = NULL, bool ignoreSensors = true);
	bool QueryAABBSimple(const Box2AABB& box, const GameObject* ignoreObject = NULL, bool ignoreSensors = true);

	void AddBody(const Vector2& pos, const Vector2& size, float density = defaultDensity, float friction = defaultFriction, float restitution = defaultRestitution);

	b2Body* CreatePhysicsBody(const b2BodyDef& bodyDef);
	void DestroyPhysicsBody(b2Body* body)				{ world->DestroyBody(body); }
	b2Joint* CreateJoint(const b2JointDef& jointDef)	{ return world->CreateJoint(&jointDef); }
	void DestroyJoint(b2Joint* joint)					{ return world->DestroyJoint(joint); }

	b2World* GetPhysicsWorld()							{ return world; }
	bool IsInWorld(const Vector2& pos) const			{ return true; }//worldAABB.TestPoint(pos); }
	int GetRaycastCount() const							{ return raycastCount; }

public:	// settings

	static float defaultFriction;			// how much friction an object has
	static float defaultRestitution;		// how bouncy objects are
	static float defaultDensity;			// how heavy objects are
	static float defaultLinearDamping;		// how quickly objects come to rest
	static float defaultAngularDamping;		// how quickly objects stop rotating
	static float worldSize;					// maximum extents of physics world
	static int32 velocityIterations;		// settings for physics world update
	static int32 positionIterations;		// settings for physics world update

private:

	void ProcessEvents();

	friend DestructionListener;
	friend ContactListener;
	friend ContactFilter;

	DestructionListener m_destructionListener;
	ContactListener m_contactListener;
	ContactFilter m_contactFilter;

	b2AABB worldAABB;
	b2World* world;
	int raycastCount;
	bool debugRender;

	list<b2Contact*> contacts;

	// contact point buffers
	list<ContactEvent> contactAddEvents;
	list<ContactEvent> contactRemoveEvents;
	list<ContactResult> contactResults;
};

#endif // PHYSICS_H