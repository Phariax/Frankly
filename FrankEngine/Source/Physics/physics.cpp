////////////////////////////////////////////////////////////////////////////////////////
/*
	Physics Controller
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../objects/gameObject.h"
#include "../physics/physicsRender.h"
#include "../physics/physics.h"

////////////////////////////////////////////////////////////////////////////////////////
// default physics settings

float Physics::defaultFriction				= 0.3f;	// how much friction an object has
ConsoleCommand(Physics::defaultFriction, defaultFriction);

float Physics::defaultRestitution			= 0.2f;	// how bouncy objects are
ConsoleCommand(Physics::defaultRestitution, defaultRestitution);

float Physics::defaultDensity				= 1.0f;	// how heavy objects are
ConsoleCommand(Physics::defaultDensity, defaultDensity);

float Physics::defaultLinearDamping		= 0.2f;	// how quickly objects come to rest
ConsoleCommand(Physics::defaultLinearDamping, defaultLinearDamping);

float Physics::defaultAngularDamping		= 0.1f;	// how quickly objects stop rotating
ConsoleCommand(Physics::defaultAngularDamping, defaultAngularDamping);

float Physics::worldSize					= 2000;	// maximum extents of physics world

int32 Physics::velocityIterations = 8;
ConsoleCommand(Physics::velocityIterations, physicsVelocityIterations);

int32 Physics::positionIterations = 3;
ConsoleCommand(Physics::positionIterations, physicsPositionIterations);

static bool showRaycasts = false;
ConsoleCommand(showRaycasts, showRaycasts);

static bool showContacts = false;
ConsoleCommand(showContacts, showContacts);

static bool showPhysicsWorldBoundaries = false;
ConsoleCommand(showPhysicsWorldBoundaries, showPhysicsWorldBoundaries);

static bool enablePhysics = true;
ConsoleCommand(enablePhysics, enablePhysics);

////////////////////////////////////////////////////////////////////////////////////////
/*
	Boundary Wall Object
*/
////////////////////////////////////////////////////////////////////////////////////////

class BoundaryObject : public GameObject
{
public:

	BoundaryObject(const Vector2& pos, const Vector2& size) :
		GameObject(pos)
	{
		CreatePhysicsBody(pos, b2_staticBody);

		b2PolygonShape shapeDef;
		shapeDef.SetAsBox(size.x, size.y);
		
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &shapeDef;
		fixtureDef.friction = Physics::defaultFriction;
		fixtureDef.restitution = Physics::defaultRestitution;
		fixtureDef.density = Physics::defaultDensity;
		AddFixture(fixtureDef);
	}

	void Update() { SetVisible(showPhysicsWorldBoundaries); }

private:

	virtual bool IsPersistant() const			{ return true; }
	virtual bool DestroyOnWorldReset() const	{ return false; }
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Main Physics Functions
*/
////////////////////////////////////////////////////////////////////////////////////////

Physics::Physics()
{
	ASSERT(!g_physics);
	worldAABB.lowerBound.Set(-worldSize, -worldSize);
	worldAABB.upperBound.Set(worldSize, worldSize);
	b2Vec2 gravity;
	gravity.Set(0, 0);
	world = new b2World(gravity);

	world->SetContactListener(&m_contactListener);
	world->SetContactFilter(&m_contactFilter);
	world->SetDebugDraw(&g_physicsRender);
}

Physics::~Physics()
{
	SAFE_DELETE(world);
}

void Physics::Init()
{
	// call physics init only on startup!
	static bool initialized = false;
	ASSERT(!initialized);
	initialized = true;

	{
		// create boundry objects to keep everything inside the world
		const float size = 10;
		new BoundaryObject
		(
			Vector2(0, -worldSize + 2*size), 
			Vector2(worldSize - size, size)
		);
		new BoundaryObject
		(
			Vector2(0, worldSize - 2*size), 
			Vector2(worldSize - size, size)
		);
		new BoundaryObject
		(
			Vector2(-worldSize + 2*size, 0), 
			Vector2(size, worldSize - size)
		);
		new BoundaryObject
		(
			Vector2(worldSize - 2*size, 0),
			Vector2(size, worldSize - size)
		);
	}
}

void Physics::Render()
{
	g_physicsRender.Render();
}

void Physics::Update(float delta)
{
	FrankProfilerEntryDefine(L"Physics::Update()", Color::White(), 5);

	raycastCount = 0;
	world->SetGravity(Terrain::gravity);

	if (enablePhysics)
		world->Step(delta, velocityIterations, positionIterations);

	ProcessEvents();
}

void Physics::ProcessEvents()
{
	// process buffered collision events after physics update
	// this is to fix problems with changing the physics state during an update

	// process contact add events
	for (list<ContactEvent>::iterator it = contactAddEvents.begin(); it != contactAddEvents.end(); ++it) 
	{
		ContactEvent& ce = *it;
		GameObject* obj1 = GameObject::GetFromPhysicsBody(*ce.fixtureA->GetBody());
		GameObject* obj2 = GameObject::GetFromPhysicsBody(*ce.fixtureB->GetBody());

		ASSERT(obj1 && obj2);
		if (!obj1->IsDestroyed())
			obj1->CollisionAdd(*obj2, ce, ce.fixtureA, ce.fixtureB);
		if (!obj2->IsDestroyed())
			obj2->CollisionAdd(*obj1, ce, ce.fixtureB, ce.fixtureA);
	}
	contactAddEvents.clear();
	
	// process contact persist events
	for (list<b2Contact*>::iterator it = g_physics->contacts.begin(); it != g_physics->contacts.end(); ++it) 
	{
		b2Contact* contact = *it;
		b2Fixture* fixtureA = contact->GetFixtureA();
		b2Fixture* fixtureB = contact->GetFixtureB();
		GameObject* obj1 = GameObject::GetFromPhysicsBody(*fixtureA->GetBody());
		GameObject* obj2 = GameObject::GetFromPhysicsBody(*fixtureB->GetBody());

		ContactEvent ce;
		ce.fixtureA = fixtureA;
		ce.fixtureB = fixtureB;
		ce.contact = contact;
		
		if (fixtureA->IsSensor() || fixtureB->IsSensor())
		{
			// sensors don't have manifolds, just use midpoint as center
			const Vector2 p1 = fixtureA->GetAABB(0).GetCenter();
			const Vector2 p2 = fixtureB->GetAABB(0).GetCenter();
			ce.point = 0.5f*p1 + 0.5f*p2;
			ce.normal = (p1 - p2).Normalize();
		}
		else
		{
			b2WorldManifold worldManifold;
			contact->GetWorldManifold(&worldManifold);
			ce.point = worldManifold.points[0];
			ce.normal = worldManifold.normal;
		}

		if (showContacts)
		{
			Line2(ce.point, ce.fixtureA->GetAABB(0).GetCenter()).RenderDebug();
			Line2(ce.point, ce.fixtureB->GetAABB(0).GetCenter()).RenderDebug();
			ce.point.RenderDebug(Color::Yellow(0.5f));
		}

		ASSERT(obj1 && obj2);
		if (!obj1->IsDestroyed())
			obj1->CollisionPersist(*obj2, ce, fixtureA, fixtureB);
		if (!obj2->IsDestroyed())
			obj2->CollisionPersist(*obj1, ce, fixtureB, fixtureA);
	}

	// process contact remove events
	for (list<ContactEvent>::iterator it = contactRemoveEvents.begin(); it != contactRemoveEvents.end(); ++it) 
	{
		ContactEvent& ce = *it;
		GameObject* obj1 = GameObject::GetFromPhysicsBody(*ce.fixtureA->GetBody());
		GameObject* obj2 = GameObject::GetFromPhysicsBody(*ce.fixtureB->GetBody());

		ASSERT(obj1 && obj2);
		if (!obj1->IsDestroyed())
			obj1->CollisionRemove(*obj2, ce, ce.fixtureA, ce.fixtureB);
		if (!obj2->IsDestroyed())
			obj2->CollisionRemove(*obj1, ce, ce.fixtureB, ce.fixtureA);
	}
	contactRemoveEvents.clear();

	// process contact result events
	for (list<ContactResult>::iterator it = contactResults.begin(); it != contactResults.end(); ++it) 
	{
		ContactResult& cr = *it;
		GameObject* obj1 = GameObject::GetFromPhysicsBody(*cr.ce.fixtureA->GetBody());
		GameObject* obj2 = GameObject::GetFromPhysicsBody(*cr.ce.fixtureB->GetBody());

		ASSERT(obj1 && obj2);
		if (!obj1->IsDestroyed())
			obj1->CollisionResult(*obj2, cr, cr.ce.fixtureA, cr.ce.fixtureB);
		if (!obj2->IsDestroyed())
			obj2->CollisionResult(*obj1, cr, cr.ce.fixtureB, cr.ce.fixtureA);
	}
	contactResults.clear();
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Physics Helper Functions
*/
////////////////////////////////////////////////////////////////////////////////////////

int Physics::GetBodyCount() const
{
	ASSERT(world);
	return world->GetBodyCount();
}

int Physics::GetProxyCount() const
{
	ASSERT(world);
	return world->GetProxyCount();
}

b2Body* Physics::CreatePhysicsBody(const b2BodyDef& bodyDef)
{
	ASSERT(worldAABB.Contains(bodyDef.position));	// physics body out of world
	return world->CreateBody(&bodyDef);
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Listeners
*/
////////////////////////////////////////////////////////////////////////////////////////

bool ContactFilter::ShouldCollide(b2Fixture* fixtureA, b2Fixture* fixtureB)
{
	if (!b2ContactFilter::ShouldCollide(fixtureA, fixtureB))
		return false;

	const GameObject* obj1 = GameObject::GetFromPhysicsBody(*fixtureA->GetBody());
	const GameObject* obj2 = GameObject::GetFromPhysicsBody(*fixtureB->GetBody());
	ASSERT(obj1 && obj2);

	if (!obj1->ShouldCollide(*obj2, fixtureA, fixtureB))
		return false;

	if (!obj2->ShouldCollide(*obj1, fixtureB, fixtureA))
		return false;

	return true;

	// box2d filter data collide code
	//const b2FilterData& filter1 = shape1->GetFilterData();
	//const b2FilterData& filter2 = shape2->GetFilterData();
	//if (filter1.groupIndex == filter2.groupIndex && filter1.groupIndex != 0)
	//	return filter1.groupIndex > 0;
	//
	//return (filter1.maskBits & filter2.categoryBits) != 0 && (filter1.categoryBits & filter2.maskBits) != 0;
}

void ContactListener::BeginContact(b2Contact* contact)
{
	ContactEvent ce;
	ce.fixtureA = contact->GetFixtureA();
	ce.fixtureB = contact->GetFixtureB();
	ce.contact = contact;

	if (ce.fixtureA->IsSensor() || ce.fixtureB->IsSensor())
	{
		// sensors don't have manifolds, just use midpoint as center
		const Vector2 p1 = ce.fixtureA->GetAABB(0).GetCenter();
		const Vector2 p2 = ce.fixtureB->GetAABB(0).GetCenter();
		ce.point = 0.5f*p1 + 0.5f*p2;
		ce.normal = (p1 - p2).Normalize();
	}
	else
	{
		b2WorldManifold worldManifold;
		contact->GetWorldManifold(&worldManifold);
		ce.point = worldManifold.points[0];
		ce.normal = worldManifold.normal;
	}

	g_physics->contactAddEvents.push_back(ce);
	g_physics->contacts.push_back(contact);
}

void ContactListener::EndContact(b2Contact* contact)
{
	// dont process remove events now
	// there is a problem when a body is destroyed it is also sending a contact remove event
	ContactEvent ce;
	ce.fixtureA = contact->GetFixtureA();
	ce.fixtureB = contact->GetFixtureB();
	ce.contact = contact;
	
	if (ce.fixtureA->IsSensor() || ce.fixtureB->IsSensor())
	{
		// sensors don't have manifolds, just use midpoint as center
		const Vector2 p1 = ce.fixtureA->GetAABB(0).GetCenter();
		const Vector2 p2 = ce.fixtureB->GetAABB(0).GetCenter();
		ce.point = 0.5f*p1 + 0.5f*p2;
		ce.normal = (p1 - p2).Normalize();
	}
	else
	{
		b2WorldManifold worldManifold;
		contact->GetWorldManifold(&worldManifold);
		ce.point = worldManifold.points[0];
		ce.normal = worldManifold.normal;
	}
	
	g_physics->contacts.push_back(contact);

	// search and remove from the list
	for (list<b2Contact*>::iterator it = g_physics->contacts.begin(); it != g_physics->contacts.end(); ) 
	{
		const b2Contact* checkContact = *it;
		if (checkContact == contact)
		{
			it = g_physics->contacts.erase(it);
			continue;
		}

		++it;
	}
}

void ContactListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
	ContactEvent ce;
	ce.fixtureA = contact->GetFixtureA();
	ce.fixtureB = contact->GetFixtureB();
	ce.contact = contact;
	
	if (ce.fixtureA->IsSensor() || ce.fixtureB->IsSensor())
	{
		// sensors don't have manifolds, just use midpoint as center
		const Vector2 p1 = ce.fixtureA->GetAABB(0).GetCenter();
		const Vector2 p2 = ce.fixtureB->GetAABB(0).GetCenter();
		ce.point = 0.5f*p1 + 0.5f*p2;
		ce.normal = (p1 - p2).Normalize();
	}
	else
	{
		b2WorldManifold worldManifold;
		contact->GetWorldManifold(&worldManifold);
		ce.point = worldManifold.points[0];
		ce.normal = worldManifold.normal;
	}

	GameObject* obj1 = GameObject::GetFromPhysicsBody(*ce.fixtureA->GetBody());
	GameObject* obj2 = GameObject::GetFromPhysicsBody(*ce.fixtureB->GetBody());
	ASSERT(obj1 && obj2);

	obj1->CollisionPreSolve(*obj2, ce, ce.fixtureA, ce.fixtureB, oldManifold);
	obj2->CollisionPreSolve(*obj1, ce, ce.fixtureB, ce.fixtureA, oldManifold);
}

void ContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)
{
	ContactResult cr;
	cr.ce.fixtureA = contact->GetFixtureA();
	cr.ce.fixtureB = contact->GetFixtureB();
	cr.ce.contact = contact;
	
	if (cr.ce.fixtureA->IsSensor() || cr.ce.fixtureB->IsSensor())
	{
		// sensors don't have manifolds, just use midpoint as center
		const Vector2 p1 = cr.ce.fixtureA->GetAABB(0).GetCenter();
		const Vector2 p2 = cr.ce.fixtureB->GetAABB(0).GetCenter();
		cr.ce.point = 0.5f*p1 + 0.5f*p2;
		cr.ce.normal = (p1 - p2).Normalize();
	}
	else
	{
		b2WorldManifold worldManifold;
		contact->GetWorldManifold(&worldManifold);
		cr.ce.point = worldManifold.points[0];
		cr.ce.normal = worldManifold.normal;
	}
	
    for (int32 i = 0; i < impulse->count; ++i)
        cr.normalImpulse = Max(cr.normalImpulse, impulse->normalImpulses[i]);
    for (int32 i = 0; i < impulse->count; ++i)
        cr.tangentImpulse = Max(cr.tangentImpulse, impulse->tangentImpulses[i]);

	g_physics->contactResults.push_back(cr);
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Raycasts
*/
////////////////////////////////////////////////////////////////////////////////////////

static FrankProfilerEntry rayCastProfilerEntry(L"Physics::Raycast()", Color::White(), 5);

void SimpleRaycastResult::RenderDebug(const Color& color, float radius, float time) const
{
	point.RenderDebug(color, radius, time);
	Line2(point, point + 2*radius*normal).RenderDebug(color, time);
}

class RaycastQueryCallback : public b2QueryCallback
{
public:

	RaycastQueryCallback(const GameObject* ignoreObject, const b2Vec2& point, bool sightCheck)
	{
		m_ignoreObject = ignoreObject;
		m_fixture = NULL;
		m_point = point;
		m_sightCheck = sightCheck;
	}

	/// Called for each fixture found in the query AABB.
	/// @return false to terminate the query.
	virtual bool ReportFixture(b2Fixture* fixture)
	{
		b2Body* body = fixture->GetBody();
		GameObject* object = GameObject::GetFromPhysicsBody(*body);
		ASSERT(object); // all bodies should have objects
		if (object == m_ignoreObject)
			return true;

		// sight raycast check
		if (m_sightCheck && !object->ShouldCollideSight())
			return true;
		
		// check if it inside the fixture
		if (!fixture->TestPoint(m_point))
			return true;

		// do object should collide check
		if (m_ignoreObject)
		{
			if (!object->ShouldCollide(*m_ignoreObject, fixture, NULL))
				return true;

			if (!m_ignoreObject->ShouldCollide(*object, NULL, fixture))
				return true;
		}
		
		m_fixture = fixture;
		return false;
	}

	const GameObject* m_ignoreObject;
	b2Fixture* m_fixture;
	b2Vec2 m_point;
	bool m_sightCheck;
};

class RayCastClosestCallback : public b2RayCastCallback
{
public:
	
	RayCastClosestCallback(const GameObject* ignoreObject = NULL, bool sightCheck = false)
	{
		m_ignoreObject = ignoreObject;
		m_hitObject = NULL;
		m_hitFixture = NULL;
		m_lambda = 0;
		m_sightCheck = sightCheck;
	}
	
	float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction)
	{
		b2Body* body = fixture->GetBody();
		GameObject* object = static_cast<GameObject*>(body->GetUserData());
		ASSERT(object); // all bodies should have objects
		if (object == m_ignoreObject)
			return -1.0f;

		// sight raycast check
		if (m_sightCheck && !object->ShouldCollideSight())
			return -1.0f;
	
		// do object should collide check
		if (m_ignoreObject)
		{
			if (!object->ShouldCollide(*m_ignoreObject, fixture, NULL))
				return -1.0f;

			if (!m_ignoreObject->ShouldCollide(*object, NULL, fixture))
				return -1.0f;
		}

		m_hitObject = object;
		m_point = point;
		m_normal = normal;
		m_hitFixture = fixture;
		m_lambda = fraction;
		return fraction;
	}

	const GameObject* m_ignoreObject;
	GameObject* m_hitObject;
	b2Fixture* m_hitFixture;
	b2Vec2 m_point;
	b2Vec2 m_normal;
	float m_lambda;
	bool m_sightCheck;
};

GameObject* Physics::RaycastSimple(const Line2& line, SimpleRaycastResult* result, const GameObject* ignoreObject, bool sightCheck)
{
	FrankProfilerBlockTimer profilerBlock(rayCastProfilerEntry);
	{
		// check inside solid fixtures
		// First check if we are starting inside an object
		RaycastQueryCallback queryCallback(ignoreObject, line.p1, sightCheck);

		// make a small box
		b2Vec2 d(0.001f, 0.001f);
		b2AABB aabb = {b2Vec2(line.p1) - d, b2Vec2(line.p1) + d};
		world->QueryAABB(&queryCallback, aabb);

		if (queryCallback.m_fixture)
		{
			if (result)
			{
				// raycast is starting inside a fixture
				result->point = line.p1;
				result->normal = (line.p1 - line.p2).Normalize();
				result->lambda = 0;
				result->hitFixture = queryCallback.m_fixture;
			}
			return GameObject::GetFromPhysicsBody(*queryCallback.m_fixture->GetBody());
		}
	}

	RayCastClosestCallback raycastResult(ignoreObject, sightCheck);
	Raycast(line, raycastResult);

	if (result)
	{
		if (raycastResult.m_hitObject)
		{
			result->point = raycastResult.m_point;
			result->normal = raycastResult.m_normal;
			result->lambda = raycastResult.m_lambda;
			result->hitFixture = raycastResult.m_hitFixture;
		}
		else
		{
			// if there was no hit then put raycast at the end
			result->point = line.p2;
			result->normal = (line.p1 - line.p2).Normalize();
			result->lambda = 1;
			result->hitFixture = NULL;
		}
	}

	return raycastResult.m_hitObject;
}

void Physics::Raycast(const Line2& line, b2RayCastCallback& raycastResult)
{
	FrankProfilerBlockTimer profilerBlock(rayCastProfilerEntry);

	if (line.p1.x == line.p2.x && line.p1.y == line.p2.y)
		return;

	if (showRaycasts)
		line.RenderDebug(Color(1.0f, 0.8f, 0.5f, 0.5f));
	
	++raycastCount;
	world->RayCast(&raycastResult, line.p1, line.p2);
}

class QueryAABBCallback : public b2QueryCallback
{
public:

	QueryAABBCallback(Box2AABB _box, GameObject** _hitObjects, unsigned _maxHitObjectCount, const GameObject* _ignoreObject, bool _ignoreSensors) :
	  box(_box),
	  hitObjects(_hitObjects),
	  ignoreObject(_ignoreObject),
	  maxHitObjectCount(_maxHitObjectCount),
	  resultCount(0),
	  ignoreSensors(_ignoreSensors)
	{}

	/// Called for each fixture found in the query AABB.
	/// @return false to terminate the query.
	virtual bool ReportFixture(b2Fixture* fixture)
	{
		if (ignoreSensors && fixture->IsSensor())
			return true;

		const int proxyCount = fixture->GetProxyCount();
		for(int i = 0; i < proxyCount; ++i)
		{
			const b2AABB& aabb = fixture->GetAABB(i);
			if (!box.PartiallyContains(aabb))
				return true;
		}

		b2Body* body = fixture->GetBody();
		GameObject* object = GameObject::GetFromPhysicsBody(*body);
		ASSERT(object); // all bodies should have objects
		if (object == ignoreObject)
			return true;
		
		hitObjects[resultCount] = object;
		++resultCount;

		return (resultCount < maxHitObjectCount);
	}

	Box2AABB box;
	GameObject** hitObjects;
	const GameObject* ignoreObject;
	const unsigned maxHitObjectCount;
	unsigned resultCount;
	bool ignoreSensors;
};

unsigned Physics::QueryAABB(const Box2AABB& box, GameObject** hitObjects, unsigned maxHitObjectCount, const GameObject* ignoreObject, bool ignoreSensors)
{
	ASSERT(hitObjects && maxHitObjectCount > 0);
	FrankProfilerBlockTimer profilerBlock(rayCastProfilerEntry);

	if (showRaycasts)
		box.RenderDebug(Color(1.0f, 0.8f, 0.5f, 0.5f));
	
	QueryAABBCallback queryCallback(box, hitObjects, maxHitObjectCount, ignoreObject, ignoreSensors);

	world->QueryAABB(&queryCallback, box);

	return queryCallback.resultCount;
}	

bool Physics::QueryAABBSimple(const Box2AABB& box, const GameObject* ignoreObject, bool ignoreSensors)
{
	GameObject* hitObject[1];
	QueryAABBCallback queryCallback(box, hitObject, 1, ignoreObject, ignoreSensors);
	world->QueryAABB(&queryCallback, box);

	return queryCallback.resultCount > 0;
}
