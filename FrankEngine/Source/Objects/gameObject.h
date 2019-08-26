////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object
	Copyright 2013 Frank Force - http://www.frankforce.com

	- 2d object in the x/y plain
	- automatically added to world group by default when constructed
	- automatically deleted during garbage collection phase after destroy is called
	- may have a physics object
	- may have a parents or multiple children
	- has a unique handle
	- can't be copied
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "../objects/gameObjectManager.h"
#include "../physics/physics.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object
*/
////////////////////////////////////////////////////////////////////////////////////////

enum GameObjectType;

class GameObject : private Uncopyable
{
public: // basic functionality

	explicit GameObject(const struct GameObjectStub& stub, GameObject* _parent = NULL, bool addToWorld = true);
	explicit GameObject(const XForm2& xf, GameObject* _parent = NULL, GameObjectType _gameObjectType = GameObjectType(0), bool addToWorld = true);
	virtual ~GameObject() = 0;

	// every object has a unique handle, use object handles instead of pointers to keep track of objects
	GameObjectHandle GetHandle() const { return handle; }
	bool operator == (const GameObject& other) const { return (handle == other.GetHandle()); }
	bool operator != (const GameObject& other) const { return (handle != other.GetHandle()); }

	// call this function mark an object for destruction instead of calling delete
	// all of the object's children will also be removed and marked to be destroyed
	virtual void Destroy();
	bool IsDestroyed() const { return flags & ObjectFlag_Destroyed; } 

	// detach and kill particle emitters
	void DetachEmitters();

	// Kill() is how objects die during normal gameplay
	// override this to make special things happen when an object dies
	virtual void Kill() { DetachEmitters(); Destroy(); }
	virtual void ApplyDamage(float damage, GameObject* attacker = NULL, GameDamageType damageType = GameDamageType_Default) {}

	// called for each object when terrain is deformed 
	virtual void TerrainDeform(const Vector2& pos, float radius) {}

public: // game object information

	GameObjectType GetType() const { return gameObjectType; }
	void SetType(GameObjectType type) { gameObjectType = type; }

	Vector2 GetStubSize() const { return stubSize; }
	void SetStubSize(const IntVector2& _stubSize) { stubSize = _stubSize; }

	const GameTeam GetTeam() const { return team; }
	void SetTeam(GameTeam _team) { team = _team; }
	
public: // hack sedd to resolve missing features
	
	// dynamic type info
	virtual bool IsPlayer() const { return false; }
	virtual bool IsOwnedByPlayer() const { return false; }
	virtual bool IsActor() const { return false; }
	virtual bool IsCharacter() const { return false; }
	virtual bool IsParticleEmitter() const { return false; }
	virtual bool IsProjectile() const { return false; }
	virtual bool IsTerrain() const { return false; }
	virtual bool IsLight() const { return false; }
	virtual bool Bounce(const Vector2& direction) { return false; }

	// used for round planets
	bool IsInAtmosphere() const;

	// used for pickups
	virtual bool CanPickup() const { return false; }
	virtual void Pickup(GameObject& owner) {}

	// get position that should be targeted if something aimed at this
	virtual const Vector2 GetPosTarget() const { return GetPosWorld(); }
	
	// can be used to prevent objects from rapidly playing sounds
	void SetSoundTimer() { soundTimer.Set(); }
	bool HasPlayedSound(float time) const { return soundTimer.IsValid() && soundTimer < time; }
	
	virtual bool IsDead() const { return false; }

public: // transforms

	const XForm2& GetXFormLocal() const { return xfLocal; }
	const XForm2& GetXFormWorld() const { return xfWorld; }
	const XForm2& GetXFormWorldLast() const { return xfWorldLast; }

	void SetPosLocal(const Vector2& pos) { xfLocal.position = pos; }
	void SetAngleLocal(float angle) { xfLocal.angle = angle; }
	void SetXFormLocal(const XForm2& xf) { xfLocal = xf; }

	void SetXFormWorld(const XForm2& xf)
	{
		ASSERT(!HasParent()); // childern are in the local space of their parent
		xfWorld = xf; 
		if (HasPhysics()) SetPhysicsXForm(xf);
		else SetXFormLocal(xf);
	}
	void SetPosWorld(const Vector2& pos)
	{
		ASSERT(!HasParent()); // childern are in the local space of their parent
		xfWorld.position = pos; 
		if (HasPhysics()) SetPhysicsPos(pos);
		else SetPosLocal(pos);
	}
	void SetAngleWorld(float angle)
	{
		ASSERT(!HasParent()); // childern are in the local space of their parent
		xfWorld.angle = angle; 
		if (HasPhysics()) SetPhysicsAngle(angle);
		else SetAngleLocal(angle);
	}

	// shortcuts to quickly get at parts of the xform
	const Vector2&	GetPosLocal()	const { return xfLocal.position; }
	const Vector2&	GetPosWorld()	const { return xfWorld.position; }
	const float		GetAngleLocal()	const { return xfLocal.angle; }
	const float		GetAngleWorld()	const { return xfWorld.angle; }
	const Vector2	GetRightLocal()	const { return xfLocal.GetRight(); }
	const Vector2	GetRightWorld()	const { return xfWorld.GetRight(); }
	const Vector2	GetUpLocal()	const { return xfLocal.GetUp(); }
	const Vector2	GetUpWorld()	const { return xfWorld.GetUp(); }
	
	// resurcive update of object and child transform
	virtual void UpdateTransforms();

	// call to wipe out interpolation data for this object and it's children
	// so an object can instantly transport to a new position rather then interpolating
	void ResetLastWorldTransforms();

	// get the delta xform used for interpolation
	XForm2 GetXFormDelta() const { return xfWorld - xfWorldLast; }

protected: // lower level object functions

	// automatically called for all object during update phase
	virtual void Update()			{}		// normal game update

	// automatically called for every visible object during render phase
	virtual void Render();

	// automatically called for every visible object during render post phase (after main render is complete)
	virtual void RenderPost()		{}

public: // children and parents

	void AttachChild(GameObject& child);
	void DetachChild(GameObject& child);
	bool HasChildren() const { return !(children.empty()); }
	void DetachParent() { if (parent) parent->DetachChild(*this); }
	void DetachChildren();
	void DestroyChildren();
	virtual void WasDetachedFromParent() {}

	bool HasParent() const { return parent != NULL; }
	GameObject* GetParent() { return parent; }
	const GameObject* GetParent() const { return parent; }

	GameObject* GetFirstChild() { return children.front(); }
	const GameObject* GetFirstChild() const { return children.front(); }

	list<GameObject*>& GetChildren() { return children; }
	const list<GameObject*>& GetChildren() const { return children; }

public: // interpolation (used by rendering)

	// use this transform when rendering to get the interpolated xform
	XForm2 GetInterpolatedXForm() const;
	XForm2 GetInterpolatedXForm(float percent) const;
	Matrix44 GetInterpolatedMatrix() const;

public: // rendering

	void SetVisible(bool visible);
	bool IsVisible() const { return (flags & ObjectFlag_Visible) != 0; }

	// order of rendering for objects within a given render pass
	// lower numbers draw earlier
	int GetRenderGroup() const { return renderGroup; }
	void SetRenderGroup(int _renderGroup) { renderGroup = _renderGroup; }
	
	// called to render a stub in the editor
	static void StubRender(const GameObjectStub& stub, float alpha);
	
	// called to show a description of the object in editor
	static WCHAR* StubDescription();
	
	// called to show a description of the object in editor
	static WCHAR* StubAttributesDescription();

public: // physics

	bool HasPhysics() const { return physicsBody != NULL; }
	b2Body* GetPhysicsBody();
	const b2Body* GetPhysicsBody() const;

	void SetPhysicsFilter(int16 groupIndex = 0, uint16 categoryBits = 0x0001, uint16 maskBits = 0xFFFF);
	int16 GetPhysicsGroup() const;

	// check if objects should collide. if a fixture is null that indicates it is a raycast collide test
	virtual bool ShouldCollide(const GameObject& otherObject, const b2Fixture* myFixture, const b2Fixture* otherFixture) const { return true; }

	// special check for line of sight collision
	virtual bool ShouldCollideSight() const { return true; }

	// collision callbacks (these are batched up and called after physics)
	// getting a null fixture for the other object indicates that it is a raycast collision
	virtual void CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture) {}
	virtual void CollisionPersist(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture) {}
	virtual void CollisionRemove(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture) {}
	virtual void CollisionResult(GameObject& otherObject, const ContactResult& contactResult, b2Fixture* myFixture, b2Fixture* otherFixture) {}

	// collision pre-solve callback, called immediatly during physics update
	virtual void CollisionPreSolve(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture, const b2Manifold* oldManifold) {}

	// create/destroy functions for physics
	void CreatePhysicsBody(const XForm2& xf, b2BodyType type = b2_dynamicBody, bool fixedRotation = false, bool allowSleeping = true);
	void CreatePhysicsBody(const b2BodyDef& bodyDef);
	void DestroyPhysicsBody();
	b2Fixture* AddFixture(const b2FixtureDef& fixtureDef);
	void RemoveFixture(b2Fixture& fixture);
	static GameObject* GetFromPhysicsBody(b2Body& body) { return static_cast<GameObject*>(body.GetUserData()); }

	// get info about the attached physics body
	virtual Vector2 GetVelocity() const;
	virtual void SetVelocity(const Vector2& v);
	virtual void SetAngularVelocity(const float v);
	virtual float GetSpeed() const;
	virtual float GetSpeedSquared() const;
	virtual float GetAngularSpeed() const;
	virtual float GetMass() const;
	virtual float GetKineticEnergy() const;
	virtual XForm2 GetPhysicsXForm() const;
	virtual bool IsStatic() const;
	virtual void SetPhysicsXForm(const Vector2& position, float angle = 0) { SetPhysicsXForm(XForm2(position, angle)); }
	virtual void SetPhysicsXForm(const XForm2& xf);
	virtual void SetPhysicsPos(const Vector2& pos);
	virtual void SetPhysicsAngle(float angle);
	virtual void SetLinearDamping(float damp) { if (GetPhysicsBody()) GetPhysicsBody()->SetLinearDamping(damp); }
	virtual void SetAngularDamping(float damp) { if (GetPhysicsBody()) GetPhysicsBody()->SetAngularDamping(damp); }
	virtual void SetMass(float mass);
	virtual void SetMassCenter(const Vector2& center);
	virtual void SetRotationalInertia(float I);
	virtual void SetRestitution(float restitution);
	virtual void SetFriction(float friction);

	// apply forces to the attached physics body
	virtual void ApplyTorque(float torque);
	virtual void ApplyAngularAcceleration(float angularAcceleration);
	virtual void ApplyForce(const Vector2& force, const Vector2& pos);
	virtual void ApplyForce(const Vector2& force);
	virtual void ApplyImpulse(const Vector2& impulse, const Vector2& pos);
	virtual void ApplyImpulse(const Vector2& impulse);
	virtual void ApplyAcceleration(const Vector2& a, const Vector2& pos);
	virtual void ApplyAcceleration(const Vector2& a);
	virtual void ApplyRotation(float angle);
	virtual void CapSpeed(float speed);
	virtual void CapAngularSpeed(float speed);

	// stuff that has to do with gravity
	bool HasGravity() const {return (flags & ObjectFlag_Gravity) != 0;}
	void SetHasGravity(bool hasGravity);
	Vector2 GetGravity() const;
	float GetAltitude() const;
	float GetAltitudePercent() const;

	// move up heiarachy until we find a physical object if there is one
	const GameObject* GetAttachedPhysics() const;
	GameObject* GetAttachedPhysics();

	// allow water objects to effect this
	void SetIsBuoyant(bool isBuoyant);
	bool IsBuoyant() const {return (flags & ObjectFlag_Buoyant) != 0;}

public: // streaming

	// persistant objects will not be removed or put to sleep when outside the window
	virtual bool IsPersistant() const { return false; }

	// automatically called when an object gets streamed out
	virtual void StreamOut() { DetachEmitters(); Destroy(); }

	// true if the object should do seralized setreaming
	static bool IsSerializable() { return false; }

	// saves this object out to a stub for seralized streaming
	virtual GameObjectStub Serialize() const;

	// is this object (and it's children) fully contained inside the aabb?
	bool FullyContainedBy(const Box2AABB& bbox) const;

	// is this object (or it's children) partially contained inside the aabb?
	bool PartiallyContainedBy(const Box2AABB& bbox) const;

private: // private stuff

	GameObjectHandle handle;			// the unique handle of this object
	GameObjectType gameObjectType;		// the type of game object if it has one
	b2Body* physicsBody;				// the physics body if it has one
	int physicsGroup;					// objects in same group never collide, except group 0
	int renderGroup;					// order objects are rendered in with their render pass
	Vector2 stubSize;					// size from the stub, used for streaming
	GameTimer soundTimer;				// can be used to limit how often objects play sounds

	XForm2 xfLocal;						// transform in local space
	XForm2 xfWorld;						// transform from local to world space (only updated once per frame)
	XForm2 xfWorldLast;					// world space transform from last frame, used for interpolation

	list<GameObject*> children;			// list of children	
	GameObject* parent;					// parent if it has one
	GameTeam team;						// team object is on

	enum ObjectFlags
	{
		ObjectFlag_Destroyed	= 0x01,		// is marked for deletion
		ObjectFlag_Visible		= 0x02,		// should be rendered
		ObjectFlag_JustAdded	= 0x04,		// was just created this frame
		ObjectFlag_Gravity		= 0x08,		// should gravity be applied
		ObjectFlag_Buoyant		= 0x10,		// can be affected by water
	};
	UINT flags;								// bit field of flags for the object

	static GameObjectHandle nextUniqueHandleValue;	// used only internaly to give out unique handles

	// should object be destroyed when world is reset? 
	// only a few special objects like the camera and terrain will need to override this
	virtual bool DestroyOnWorldReset() const { return true; }

public:

	// was the object added this frame?
	bool WasJustAdded() const { return (flags & ObjectFlag_JustAdded) != 0; }

public:	// public data members

	static const GameObjectHandle invalidHandle;	// represents an invalid object handle

private: // special functions to get and reset the next unique handle

	// these friends are really only necessary so they can call to the special get and set handle functions
	friend class ObjectEditor;
	friend class Terrain;
	friend class GameObjectManager;

	// these functions should only be used by editors and startup code
	static GameObjectHandle GetNextUniqueHandleValue() { return nextUniqueHandleValue; }
	static void SetNextUniqueHandleValue(GameObjectHandle _nextUniqueHandleValue) { nextUniqueHandleValue = _nextUniqueHandleValue; }
};

////////////////////////////////////////////////////////////////////////////////////////
/*
	Inline functions
*/
////////////////////////////////////////////////////////////////////////////////////////

inline void GameObject::DetachChildren()
{
	for (list<GameObject*>::iterator it = children.begin(); it != children.end(); ++it)
	{
		(**it).WasDetachedFromParent();
		(**it).parent = NULL;
	}
	children.clear();
}

inline void GameObject::DestroyChildren()
{
	for (list<GameObject*>::iterator it = children.begin(); it != children.end(); ++it)
		(**it).Destroy();
}

inline XForm2 GameObject::GetInterpolatedXForm() const
{ 
	return xfWorld.Interpolate(GetXFormDelta(), g_interpolatePercent); 
}

inline XForm2 GameObject::GetInterpolatedXForm(float percent) const
{ 
	return xfWorld.Interpolate(GetXFormDelta(), percent); 
}

inline Matrix44 GameObject::GetInterpolatedMatrix() const
{ 
	return Matrix44(xfWorld.Interpolate(GetXFormDelta(), g_interpolatePercent)); 
}

inline bool GameObject::IsStatic() const
{
	return (!physicsBody || physicsBody->GetType() == b2_staticBody);
}

inline Vector2 GameObject::GetVelocity() const
{ 
	return physicsBody? physicsBody->GetLinearVelocity() : Vector2::Zero(); 
}

inline void GameObject::SetVelocity(const Vector2& v)
{ 
	if (physicsBody)
		physicsBody->SetLinearVelocity(v); 
}

inline void GameObject::SetAngularVelocity(const float v)
{ 
	if (physicsBody)
		physicsBody->SetAngularVelocity(v); 
}

inline float GameObject::GetSpeed() const
{ 
	return physicsBody? physicsBody->GetLinearVelocity().Length() : 0;
}

inline float GameObject::GetSpeedSquared() const
{ 
	return physicsBody? physicsBody->GetLinearVelocity().LengthSquared() : 0;
}

inline float GameObject::GetAngularSpeed() const
{ 
	return physicsBody? physicsBody->GetAngularVelocity() : 0;
}

inline float GameObject::GetMass() const
{ 
	return physicsBody? physicsBody->GetMass() : 0;
}

inline float GameObject::GetKineticEnergy() const 
{ 
	return physicsBody? physicsBody->GetMass() * physicsBody->GetLinearVelocity().LengthSquared() : 0;
}

inline void GameObject::ApplyTorque(float torque)
{ 
	if (physicsBody)
		physicsBody->ApplyTorque(torque); 
}

inline void GameObject::ApplyAngularAcceleration(float angularAcceleration)
{ 
	if (physicsBody)
		physicsBody->ApplyTorque(angularAcceleration * physicsBody->GetInertia()); 
}

inline void GameObject::ApplyImpulse(const Vector2& impulse, const Vector2& pos)
{ 
	if (physicsBody)
		physicsBody->ApplyLinearImpulse(impulse, pos); 
}

inline void GameObject::ApplyImpulse(const Vector2& impulse)
{ 
	if (physicsBody)
		physicsBody->ApplyLinearImpulse(impulse, physicsBody->GetWorldCenter()); 
}

inline void GameObject::ApplyForce(const Vector2& force, const Vector2& pos)
{ 
	if (physicsBody)
		physicsBody->ApplyForce(force, pos); 
}

inline void GameObject::ApplyForce(const Vector2& force)
{ 
	if (physicsBody)
		physicsBody->ApplyForce(force, physicsBody->GetWorldCenter()); 
}

inline void GameObject::ApplyAcceleration(const Vector2& a, const Vector2& pos)
{ 
	if (physicsBody)
		physicsBody->ApplyForce(a*physicsBody->GetMass(), pos); 
}


inline void GameObject::ApplyAcceleration(const Vector2& a)
{ 
	if (physicsBody)
		physicsBody->ApplyForce(a*physicsBody->GetMass(), physicsBody->GetWorldCenter()); 
}

inline void GameObject::ApplyRotation(float angle)
{ 
	if (physicsBody)
	{
		const XForm2 xf = GetPhysicsXForm();
		SetPhysicsXForm(xf.position, xf.angle + angle); 
	}
	else
		xfLocal.angle += angle;
}

inline b2Body* GameObject::GetPhysicsBody() 
{ 
	return physicsBody; 
}

inline const b2Body* GameObject::GetPhysicsBody() const 
{ 
	return physicsBody; 
}

inline void GameObject::SetPhysicsXForm(const XForm2& xf)
{
	ASSERT(physicsBody);
	physicsBody->SetTransform(xf.position, xf.angle);
}

inline void GameObject::SetPhysicsPos(const Vector2& pos)
{
	ASSERT(physicsBody);
	physicsBody->SetTransform(pos, GetPhysicsXForm().angle);
}

inline void GameObject::SetPhysicsAngle(float angle)
{
	ASSERT(physicsBody);
	physicsBody->SetTransform(physicsBody->GetTransform().p, angle);
}

inline XForm2 GameObject::GetPhysicsXForm() const
{
	return physicsBody? physicsBody->GetTransform() : xfWorld;
}

inline void GameObject::CapSpeed(float maxSpeed)
{
	if (!physicsBody)
		return;

	const Vector2 velocity = physicsBody->GetLinearVelocity();
	const float currentSpeed = velocity.Magnitude();
	if (currentSpeed > maxSpeed)
		physicsBody->SetLinearVelocity(velocity * (maxSpeed/currentSpeed));
}

inline void GameObject::CapAngularSpeed(float maxSpeed)
{
	if (!physicsBody)
		return;

	const float currentSpeed = physicsBody->GetAngularVelocity();
	if (fabs(currentSpeed) > maxSpeed)
		physicsBody->SetAngularVelocity(maxSpeed * (currentSpeed > 0? 1 : -1));
}

inline void GameObject::CreatePhysicsBody(const b2BodyDef& bodyDef)
{
	ASSERT(!physicsBody);
	ASSERT(!HasParent());
	ASSERT(bodyDef.userData); // must be connected to an object

	physicsBody = g_physics->CreatePhysicsBody(bodyDef);

	// update xform so it appears in the correct spot
	xfLocal = GetPhysicsBody()->GetTransform();
	xfWorld = xfLocal;
}

inline void GameObject::DestroyPhysicsBody()
{
	if (physicsBody)
	{
		g_physics->DestroyPhysicsBody(physicsBody);
		physicsBody = NULL;
	}
}

inline const GameObject* GameObject::GetAttachedPhysics() const
{
	// search for the physical object in the heiarachy
	const GameObject* physicsObject = this;
	while (physicsObject && !physicsObject->HasPhysics())
	{ physicsObject = physicsObject->GetParent(); }
	return physicsObject;
}

inline GameObject* GameObject::GetAttachedPhysics()
{
	return const_cast<GameObject*>(static_cast<const GameObject*>(this)->GetAttachedPhysics());
}

inline void GameObject::SetHasGravity(bool hasGravity) 
{ 
	if (hasGravity)
	{
		if (physicsBody)
			physicsBody->SetGravityScale(1);
		flags |= ObjectFlag_Gravity;
	}
	else
	{
		if (physicsBody)
			physicsBody->SetGravityScale(0);
		flags &= ~ObjectFlag_Gravity;
	}
}

inline void GameObject::SetIsBuoyant(bool isBuoyant) 
{ 
	if (isBuoyant)
		flags |= ObjectFlag_Buoyant;
	else
		flags &= ~ObjectFlag_Buoyant;
}

inline void GameObject::SetVisible(bool visible) 
{ 
	if (visible)
		flags |= ObjectFlag_Visible;
	else
		flags &= ~ObjectFlag_Visible;
}

#endif // GAME_OBJECT_H