////////////////////////////////////////////////////////////////////////////////////////
/*
	Weapons
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef WEAPONS_H
#define WEAPONS_H

////////////////////////////////////////////////////////////////////////////////////////
/*
	Player Gun
*/
////////////////////////////////////////////////////////////////////////////////////////

class PlayerGun : public Weapon
{
public:

	PlayerGun(const XForm2& xf = XForm2::Identity(), GameObject* _parent = NULL);

private:

	Projectile* LaunchProjectile(const XForm2& xf);

	static const WeaponDef weaponStaticDef;
	static const ParticleSystemDef fireEffect;
	static const ParticleSystemDef smokeEffect;
	static const ParticleSystemDef hitEffect;
	static const ParticleSystemDef trailEffect;
};

class PlayerGunProjectile : public SolidProjectile
{
public:

	PlayerGunProjectile(const XForm2& xf, Weapon& weapon);
	void Render();
};


////////////////////////////////////////////////////////////////////////////////////////
/*
	Explosion
*/
////////////////////////////////////////////////////////////////////////////////////////

class Explosion : public GameObject
{
public:

	Explosion(const XForm2& xf, float _force, float _radius, float _damage = 0, GameObject* _attacker = NULL, float _time = 0, bool damageSelf = false);

private:
	
	void Update();
	void Render() {}
	void GetRadarRender(Color &color, float& distance) const {};
	void CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture);
	void CollisionPersist(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture);

	bool ShouldCollide(const GameObject& otherObject, const b2Fixture* myFixture, const b2Fixture* otherFixture) const
	{
		return !(otherObject.IsStatic() || otherObject.IsProjectile() || (otherFixture && otherFixture->IsSensor()));
	}

	float force;
	float radius;
	float damage;
	GameObjectHandle attacker;
	GameTimer lifeTime;
	list<GameObjectHandle> hitObjects;
	bool damageSelf;
};


////////////////////////////////////////////////////////////////////////////////////////
/*
	Debris
*/
////////////////////////////////////////////////////////////////////////////////////////

class Debris : public SolidProjectile
{
public:

	Debris(const XForm2& xf, float speed, GameObject* attacker, float radius, const ParticleSystemDef *trailEffect = &theTrailEffect, float _brightness = 1);
	static void CreateDebris(GameObject& object, int count, float speedScale = 1);

private:

	void Render();
	void Update();
	void HitObject(GameObject& object);
	bool ShouldCollide(const GameObject& otherObject, const b2Fixture* myFixture, const b2Fixture* otherFixture) const;

	float brightness;
	GameTimerPercent lifeTimer;
	static ParticleSystemDef theTrailEffect;
};

#endif // WEAPONS_H