////////////////////////////////////////////////////////////////////////////////////////
/*
	Projectile
	Copyright 2013 Frank Force - http://www.frankforce.com

	- a simple flying object, 
	- can fired by a weapon or created from scratch
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef PROJECTILE_H
#define PROJECTILE_H

#include "../objects/gameObject.h"
#include "../objects/particleSystem.h"
#include "../sound/soundControl.h"

class Weapon;
struct WeaponDef;

class Projectile : public GameObject
{
public:

	GameObject* GetAttacker() const { return g_objectManager.GetObjectFromHandle(attackerHandle); }
	void SetAttacker(GameObject* attacker) { attackerHandle = attacker? attacker->GetHandle() : GameObject::invalidHandle; }
	bool IsAttacker(const GameObject &obj) const { return (attackerHandle == obj.GetHandle()); }

	float GetRadius() const { return radius; }
	void SetRadius(float _radius) { radius = _radius; }
	float GetDamage() const { return damage; }
	void SetDamage(float _damage) { damage = _damage; }
	int GetDamageType() const { return damageType; }
	void SetDamageType(GameDamageType _damageType) { damageType = _damageType; }

	bool ShouldCollide(const GameObject& otherObject, const b2Fixture* myShape, const b2Fixture* otherFixture) const;
	void CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture);
	virtual void HitObject(GameObject& object) { Kill(); }

	bool HasTrailEmitter() const { return trailEmitter != NULL; }
	ParticleEmitter& GetTrailEmitter() const { return *trailEmitter; }

	void Kill();

	virtual bool IsProjectile() const { return true; }

	static float defaultDensity;
	static float projectileSpeedScale;
	static bool projectilesCollide;
	static bool applyAttackerVelocity;

protected:

	Projectile
	(
		const XForm2& xf, 
		float speed, 
		GameObject* attacker = NULL, 
		const ParticleSystemDef* particleDef = NULL, 
		float damage = 100, 
		float radius = 0.0f, 
		const ParticleSystemDef* _hitEffect = NULL, 
		SoundControl_ID _hitSound = SoundControl_Invalid
	);
	Projectile(const XForm2& xf, Weapon& weapon);
	Projectile(const XForm2& xf, const WeaponDef& weaponDef, GameObject* attacker = NULL);

	void Init(GameObject* attacker, const ParticleSystemDef* particleDef);
	virtual void Update();
	virtual void Render();
	virtual void GetRadarRender(Color &color, float& distance) const;

	float radius;
	float damage;
	GameDamageType damageType;
	const ParticleSystemDef* hitEffect;
	SoundControl_ID hitSound;
	GameObjectHandle attackerHandle;
	class ParticleEmitter* trailEmitter;
};


class SolidProjectile : public Projectile
{
public:

	SolidProjectile
	(
		const XForm2& xf,
		float speed, 
		GameObject* attacker = NULL, 
		const ParticleSystemDef* particleDef = NULL, 
		float damage = 100, 
		float radius = 0.0f,
		const ParticleSystemDef* _hitEffectDef = NULL, 
		SoundControl_ID _hitSound = SoundControl_Invalid,
		float density = Projectile::defaultDensity, 
		float friction = Physics::defaultFriction,
		float restitution = Physics::defaultRestitution

	);
	SolidProjectile(const XForm2& xf, Weapon& weapon, float density = Projectile::defaultDensity, float friction = Physics::defaultFriction, float restitution = Physics::defaultRestitution);
	SolidProjectile(const XForm2& xf, const WeaponDef& weaponDef, GameObject* attacker = NULL, float density = Projectile::defaultDensity, float friction = Physics::defaultFriction, float restitution = Physics::defaultRestitution);

private:

	void Init(GameObject* attacker, float speed, float density, float friction, float restitutionn);
};


class BulletProjectile : public Projectile
{
public:

	BulletProjectile
	(
		const XForm2 &xf, 
		float speed, 
		GameObject* attacker = NULL, 
		const ParticleSystemDef* particleDef = NULL, 
		float damage = 100, 
		float radius = 0.2f,
		const ParticleSystemDef* _hitEffectDef = NULL, 
		SoundControl_ID _hitSound = SoundControl_Invalid
	);
	BulletProjectile(const XForm2& xf, Weapon& weapon);
	BulletProjectile(const XForm2& xf, const WeaponDef& weaponDef, GameObject* attacker = NULL);

	void SetLinearDamping(float _damping) { damping = _damping; }
	
	virtual void ApplyAcceleration(const Vector2& a);

	void SetMass(float _mass) { mass = _mass; }
	virtual float GetMass() const { return mass; }

	virtual Vector2 GetVelocity() const { return velocity; }
	virtual void SetVelocity(const Vector2& v) { velocity = v; }
	
	static float defaultDamping;

protected:
	
	virtual void CollisionTest();
	virtual void Update();
	virtual void UpdateTransforms();
	virtual bool IsStatic() const { return false; }

protected:

	void Init(GameObject* attacker, float speed);

	Vector2 velocity;
	float mass;
	float damping;
};


#endif // PROJECTILE_H