////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../terrain/terrain.h"
#include "../objects/particleSystem.h"
#include "../objects/gameObject.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object Class Globals
*/
////////////////////////////////////////////////////////////////////////////////////////

GameObjectHandle GameObject::nextUniqueHandleValue = 1;	// starting unique handle
const GameObjectHandle GameObject::invalidHandle = 0;	// invalid handle value

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object Member Functions
*/
////////////////////////////////////////////////////////////////////////////////////////

GameObject::GameObject(const GameObjectStub& stub, GameObject* _parent, bool addToWorld) :
	gameObjectType(stub.type),
	stubSize(stub.size),
	xfLocal(stub.xf),
	xfWorld(stub.xf),
	xfWorldLast(stub.xf),
	parent(NULL),
	flags(ObjectFlag_JustAdded|ObjectFlag_Visible|ObjectFlag_Gravity),
	physicsBody(NULL),
	physicsGroup(PhysicsGroup(0)),
	renderGroup(1),
	team(GameTeam(0))
{
	// give it the stubs handle
	if (stub.handle != invalidHandle)
		handle = stub.handle;
	else
		handle = nextUniqueHandleValue++;

	// automatically add the object to the world
	if (addToWorld)
		g_objectManager.Add(*this);

	if (_parent)
	{
		_parent->AttachChild(*this);

		// refresh my world transform since I am in parent space
		UpdateTransforms();

		// reset world delta so we dont get a huge offset from 0 to where the parent was
		xfWorldLast = xfWorld;
	}
}

GameObject::GameObject(const XForm2& xf, GameObject* _parent, GameObjectType _gameObjectType, bool addToWorld) :
	gameObjectType(_gameObjectType),
	stubSize(0),
	xfLocal(xf),
	xfWorld(xf),
	xfWorldLast(xf),
	parent(NULL),
	handle(nextUniqueHandleValue++),
	flags(ObjectFlag_JustAdded|ObjectFlag_Visible|ObjectFlag_Gravity),
	physicsBody(NULL),
	physicsGroup(PhysicsGroup(0)),
	renderGroup(1),
	team(GameTeam(0))
{
	// automatically add the object to the world
	if (addToWorld)
		g_objectManager.Add(*this);

	if (_parent)
	{
		_parent->AttachChild(*this);

		// refresh my world transform since I am in parent space
		UpdateTransforms();

		// reset world delta so we dont get a huge offset from 0 to where the parent was
		xfWorldLast = xfWorld;
	}
}

GameObject::~GameObject()
{
	// never delete game objects directly, instead call Destroy()
	// this also prevents objects from being accidentally created on the stack
	ASSERT(!GameObjectManager::GetLockDeleteObjects()); 
	DestroyPhysicsBody();
}

GameObjectStub GameObject::Serialize() const
{ 
	return GameObjectStub(GetXFormWorld(), stubSize, gameObjectType, NULL, handle); 
}

void GameObject::CreatePhysicsBody(const XForm2& xf, b2BodyType type, bool fixedRotation, bool allowSleeping)
{
	b2BodyDef bodyDef;
	bodyDef.type = type;
	bodyDef.position = xf.position;
	bodyDef.angle = xf.angle;
	bodyDef.linearDamping = Physics::defaultLinearDamping;
	bodyDef.angularDamping = Physics::defaultAngularDamping;
	bodyDef.userData = this;
	bodyDef.fixedRotation = fixedRotation;
	bodyDef.allowSleep = allowSleeping;
	CreatePhysicsBody(bodyDef);
	
	// update xform so it appears in the correct spot
	xfLocal = physicsBody->GetTransform();
	xfWorld = xfLocal;
}

b2Fixture* GameObject::AddFixture(const b2FixtureDef& fixtureDef)
{
	ASSERT(physicsBody);

	// only static bodies can have 0 density
	ASSERT(fixtureDef.density != 0 || fixtureDef.isSensor || physicsBody->GetType() == b2_staticBody);

	// attach the shape
	b2Fixture* fixture = physicsBody->CreateFixture(&fixtureDef);

	return fixture;
}
	
void GameObject::RemoveFixture(b2Fixture& fixture)
{
	ASSERT(physicsBody);

	// destroy the fixture
	physicsBody->DestroyFixture(&fixture);
}

bool GameObject::FullyContainedBy(const Box2AABB& bbox) const
{
	if (physicsBody)
	{
		// test all my shapes
		for (const b2Fixture* f = physicsBody->GetFixtureList(); f; f = f->GetNext())
		{
			if (f->IsSensor())
				continue;	// we only care about solid shapes

			for(int i = 0; i < f->GetProxyCount(); ++i)
			{
				const b2AABB shapeAABB = f->GetAABB(i);
				//g_debugRender.RenderBox(shapeAABB);
				if (!bbox.FullyContains(shapeAABB))
					return false;
			}
		}
	}

	// test all children shapes
	for (list<GameObject*>::const_iterator it = GetChildren().begin(); it != GetChildren().end(); ++it) 
	{
		const GameObject& obj = **it;
		if (!obj.FullyContainedBy(bbox))
			return false;
	}

	return bbox.Contains(xfWorld.position);
}

bool GameObject::PartiallyContainedBy(const Box2AABB& bbox) const
{
	if (physicsBody)
	{
		// test all my shapes
		for (const b2Fixture* f = physicsBody->GetFixtureList(); f; f = f->GetNext())
		{
			if (f->IsSensor())
				continue;	// we only care about solid shapes

			for(int i = 0; i < f->GetProxyCount(); ++i)
			{
				const b2AABB shapeAABB = f->GetAABB(i);
				//g_debugRender.RenderBox(shapeAABB);
				if (!bbox.PartiallyContains(shapeAABB))
					return false;
			}
		}
	}

	// test all children shapes
	for (list<GameObject*>::const_iterator it = GetChildren().begin(); it != GetChildren().end(); ++it) 
	{
		const GameObject& obj = **it;
		if (obj.PartiallyContainedBy(bbox))
			return true;
	}

	return bbox.Contains(xfWorld.position);
}


void GameObject::DetachEmitters() 
{ 
	// detach and kill all particle emitters
	for (list<GameObject*>::const_iterator it = GetChildren().begin(); it != GetChildren().end();) 
	{
		GameObject& obj = **it;
		if (!obj.IsParticleEmitter())
		{
			++it;
			continue;
		}

		ParticleEmitter& emitter = static_cast<ParticleEmitter&>(obj);
		it = GetChildren().erase(it);
		emitter.parent = NULL;
		// update this child's xform since it is no longer childed
		emitter.SetXFormLocal(emitter.GetXFormWorld());
		emitter.Kill();
	}
}

void GameObject::Destroy()
{
	if (IsDestroyed())
		return;

	// TODO: handle joints
	// go through all joints to this body
	// callback to JointDestroyed function
	// JointDestroyed will by default tell that object to be destroyed
	// so all connected objects in a joint chain will be destroyed
	// objects can override JointDestroyed if they dont want to be destroyed

	DetachParent();

	// recursivly destroy all children
	for (list<GameObject*>::iterator it = children.begin(); it != children.end(); ++it)
	{
		(**it).parent = NULL;
		(**it).Destroy();
	}
	children.clear();
	flags |= ObjectFlag_Destroyed;
}

void GameObject::AttachChild(GameObject& child) 
{ 
	ASSERT(!IsDestroyed());
	ASSERT(!child.HasParent());
	children.push_back(&child); 
	child.parent = this;
}

void GameObject::DetachChild(GameObject& child)
{
	ASSERT(!IsDestroyed());
	// find this in the parent's list of children
	for (list<GameObject*>::iterator it = children.begin(); it != children.end(); ++it) 
	{
		if (*it == &child) 
		{
			child.parent = NULL;
			// update this child's xform since it is no longer childed
			child.SetXFormLocal(child.GetXFormWorld());
			children.erase(it);
			return;
		}
	}
	// child not found
	ASSERT(false);
}

void GameObject::UpdateTransforms()
{
	xfWorldLast = xfWorld;

	// update local transform if necessary
	if (parent)
	{
		ASSERT(!physicsBody);
		xfWorld = xfLocal * parent->xfWorld;
	}
	else
	{
		if (physicsBody)
			xfLocal = physicsBody->GetTransform();
		xfWorld = xfLocal;
	}

	// update children transforms
	for (list<GameObject*>::iterator it = children.begin(); it != children.end(); ++it) 
	{
		GameObject& obj = **it;
		obj.UpdateTransforms();
	}
}

void GameObject::Render()
{
	ASSERT(IsVisible());

	if (!HasPhysics())
		return;

	g_physicsRender.DrawBody(*physicsBody);
}

float GameObject::GetAltitude() const
{
	return FrankUtil::GetAltitude(GetPosWorld());
}

float GameObject::GetAltitudePercent() const 
{ 
	return FrankUtil::GetAltitudePercent(GetPosWorld()); 
}

Vector2 GameObject::GetGravity() const 
{ 
	const Vector2 gravity = FrankUtil::CalculateGravity(GetPosWorld());

	if (HasGravity())
		return gravity; 
	else
	{
		// hack: all objects automatically recieve gravity above a certain altitude
		// this to make orbiting the planet work correctly
		/*float percentGravity = 0;
		{
			const float minAltitude = 300;
			const float maxAltitude = 100;
			const float altitude = GetAltitude();

			if (altitude > minAltitude)
				percentGravity = (altitude - minAltitude) / (maxAltitude - minAltitude);
			percentGravity = CapPercent(percentGravity);
		}

		return percentGravity * gravity; */
		return Vector2::Zero();
	}
}

bool GameObject::IsInAtmosphere() const 
{ 
	const Vector2 planetDeltaPos = GetPosWorld();
	return (planetDeltaPos.MagnitudeSquared() <  Terrain::planetMaxAltitude * Terrain::planetMaxAltitude);
}
	
// called to render a stub in the editor
// can be overriden by child classes
void GameObject::StubRender(const GameObjectStub& stub, float alpha)
{
	const ObjectTypeInfo& info = stub.GetObjectInfo();
	Color c = info.GetColor();
	c.a *= alpha;

	g_render->RenderQuad(stub.xf, stub.size, c, info.GetTexture());
}
	
// called to show a discription of a stub in the editor
// can be overriden by child classes
WCHAR* GameObject::StubDescription()
{
	return L"Unknown object type.";
}
	
// called to show a discription of a stub in the editor
// can be overriden by child classes
WCHAR* GameObject::StubAttributesDescription()
{
	return L"N/A";
}

void GameObject::ResetLastWorldTransforms()
{
	if (parent)
	{
		ASSERT(!physicsBody);
		xfWorld = xfLocal * parent->xfWorld;
	}
	else
	{
		if (physicsBody)
			xfLocal = physicsBody->GetTransform();
		xfWorld = xfLocal;
	}

	xfWorldLast = xfWorld;

	// update children transforms
	for (list<GameObject*>::iterator it = children.begin(); it != children.end(); ++it) 
	{
		GameObject& obj = **it;
		obj.ResetLastWorldTransforms();
	}
}

void GameObject::SetMass(float mass)
{
	if (!physicsBody)
		return;

	b2MassData massData;
	physicsBody->GetMassData(&massData);
	massData.mass = mass;
	physicsBody->SetMassData(&massData);
}

void GameObject::SetMassCenter(const Vector2& center)
{
	if (!physicsBody)
		return;

	b2MassData massData;
	physicsBody->GetMassData(&massData);
	massData.center = center;
	physicsBody->SetMassData(&massData);
}

void GameObject::SetRotationalInertia(float I)
{
	if (!physicsBody)
		return;

	b2MassData massData;
	physicsBody->GetMassData(&massData);
	massData.I = I;
	physicsBody->SetMassData(&massData);
}

void GameObject::SetRestitution(float restitution)
{
	if (!HasPhysics())
		return;
	
	for (b2Fixture* f = physicsBody->GetFixtureList(); f; f = f->GetNext())
		f->SetRestitution(restitution);
}

void GameObject::SetFriction(float friction)
{
	if (!HasPhysics())
		return;

	for (b2Fixture* f = physicsBody->GetFixtureList(); f; f = f->GetNext())
		f->SetFriction(friction);
}

void GameObject::SetPhysicsFilter(int16 groupIndex, uint16 categoryBits, uint16 maskBits)
{
	ASSERT(HasPhysics());

	b2Filter filter;
	filter.groupIndex = groupIndex;
	filter.categoryBits = categoryBits;
	filter.maskBits = maskBits;
	for (b2Fixture* f = physicsBody->GetFixtureList(); f; f = f->GetNext())
		f->SetFilterData(filter);
}

int16 GameObject::GetPhysicsGroup() const
{
	if (!HasPhysics())
		return 0;

	// return the group of the first fixture it finds
	for (b2Fixture* f = physicsBody->GetFixtureList(); f; f = f->GetNext())
		return f->GetFilterData().groupIndex;

	return 0;
}