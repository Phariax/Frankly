////////////////////////////////////////////////////////////////////////////////////////
/*
	Projectile
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../objects/weapon.h"
#include "../objects/projectile.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Projectile Base Class
*/
////////////////////////////////////////////////////////////////////////////////////////

float Projectile::defaultDensity = Physics::defaultDensity;
ConsoleCommand(Projectile::defaultDensity, projectilesDensity);

bool Projectile::projectilesCollide = true;
ConsoleCommand(Projectile::projectilesCollide, projectilesCollide);

float Projectile::projectileSpeedScale = 1;
ConsoleCommand(Projectile::projectileSpeedScale, projectileSpeedScale);

bool Projectile::applyAttackerVelocity = true;
ConsoleCommand(Projectile::applyAttackerVelocity, projectileApplyAttackerVelocity);

Projectile::Projectile
(
	const XForm2& xf, 
	float speed, 
	GameObject* attacker, 
	const ParticleSystemDef* particleDef, 
	float _damage, 
	float _radius,
	const ParticleSystemDef* _hitEffect, 
	SoundControl_ID _hitSound
) :
	GameObject(xf),
	damage(_damage),
	damageType(GameDamageType_Default),
	radius(_radius),
	hitEffect(_hitEffect),
	hitSound(_hitSound)
{
	Init(attacker, particleDef);
}
	
Projectile::Projectile(const XForm2& xf, Weapon& weapon) :
	GameObject(xf),
	damage(weapon.GetWeaponDef().damage),
	damageType(GameDamageType_Default),
	radius(weapon.GetWeaponDef().projectileRadius),
	hitEffect(weapon.GetWeaponDef().hitEffect),
	hitSound(weapon.GetWeaponDef().hitSound)
{
	Init(weapon.GetOwner(), weapon.GetWeaponDef().trailEffect);
}
	
Projectile::Projectile(const XForm2& xf, const WeaponDef& weaponDef, GameObject* attacker) :
	GameObject(xf),
	damage(weaponDef.damage),
	damageType(GameDamageType_Default),
	radius(weaponDef.projectileRadius),
	hitEffect(weaponDef.hitEffect),
	hitSound(weaponDef.hitSound)
{
	Init(attacker, weaponDef.trailEffect);
}

void Projectile::Init(GameObject* attacker, const ParticleSystemDef* particleDef)
{
	if (attacker)
		SetTeam(attacker->GetTeam());

	// attach the particle system
	if (particleDef)
		trailEmitter = new ParticleEmitter(*particleDef, XForm2::Identity(), this);
	else
		trailEmitter = NULL;

	// store the attacker handle
	attackerHandle = attacker? attacker->GetHandle() : invalidHandle;
}

bool Projectile::ShouldCollide(const GameObject& otherObject, const b2Fixture* myFixture, const b2Fixture* otherFixture) const
{
	// dont collide with our attacker
	if (IsAttacker(otherObject))
		return false;

	// projectiles dont collide other objects in the same team except for team 0
	if (GetTeam() != 0 && otherObject.GetTeam() == GetTeam())
		return false;

	// projectiles can be set to not collide with eachother
	if (!projectilesCollide && otherObject.IsProjectile())
		return false;

	// projectiles should collide with the world
	//if (otherObject.IsStatic())
	//	return true;

	// projectiles dont collide with their attacker until they have collided with something else first
	//if (collided && IsAttacker(otherObject))
	//	return false;

	return true;
}

void Projectile::CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture)
{
	if (otherFixture && otherFixture->IsSensor())
		return;
	
	if (!HasPhysics())
	{
		// for simulated physics set our pos to be the hit position
		SetPosWorld(contactEvent.point);
	}

	// apply damage to the other object
	otherObject.ApplyDamage(damage, GetAttacker(), damageType);

	// let this projectile process a hit
	HitObject(otherObject);

	if (IsDestroyed())
	{
		if (myFixture && otherFixture && (myFixture->IsSensor() || otherFixture->IsSensor()))
		{
			// apply impulse to the object manually since it hit a sensor
			const float impulseCoef = 0.02f;
			const Vector2 impulse = GetMass() * GetVelocity() * GetVelocity().Magnitude() * impulseCoef;
			otherObject.ApplyImpulse(impulse);
		}

		if (HasTrailEmitter() && (GetTrailEmitter().GetDef().particleFlags & (PARTICLE_FLAG_TRAIL_LINE|PARTICLE_FLAG_TRAIL_RIBBON)))
		{
			// force partice to be set to at impact positon
			GetTrailEmitter().SetTrailEnd(contactEvent.point);
		}
	}
}

void Projectile::Kill()
{
	if (IsDestroyed())
		return;	// can only die once

	// update to latest position for death effects
	const XForm2 xf = HasPhysics()? GetPhysicsBody()->GetTransform() : GetXFormLocal();

	// todo: sound radius must be gameside
	g_sound->Play(hitSound, xf.position);
	if (hitEffect)
		new ParticleEmitter(*hitEffect, xf);

	GameObject::Kill();
}

void Projectile::Update()
{
	// get rid of projectiles that go too high
	//if (Terrain::isCircularPlanet && !IsInAtmosphere())
	//	Destroy();
}

void Projectile::Render() 
{ 
	//if (radius > 0) 
	//	g_render->DrawSolidCircle(GetInterpolatedXForm().position, radius, Color::White()); 
	//else
	{
		// make it so we can still see 0 radius projectiles when particle system is disabled
		if (!ParticleEmitter::AreParticlesEnabled())
			g_render->DrawSolidCircle(GetInterpolatedXForm().position, 0.5f, Color::White()); 
	}
}

void Projectile::GetRadarRender(Color &color, float& distance) const
{ 
	GameObject* attacker = GetAttacker();

	if (!attacker || !attacker->IsOwnedByPlayer())
	{
		color = Color::White();
		distance = 40; 
	}
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Projectile Solid
*/
////////////////////////////////////////////////////////////////////////////////////////

SolidProjectile::SolidProjectile
(
	const XForm2& xf, 
	float speed, 
	GameObject* attacker, 
	const ParticleSystemDef* particleDef, 
	float _damage, 
	float _radius,
	const ParticleSystemDef* _hitEffectDef, 
	SoundControl_ID _hitSound,
	float density,
	float friction,
	float restitution
) :
	Projectile(xf, speed, attacker, particleDef, _damage, _radius, _hitEffectDef, _hitSound)
{
	Init(attacker, speed, density, friction, restitution);
}

SolidProjectile::SolidProjectile(const XForm2& xf, Weapon& weapon, float density, float friction, float restitution) :
	Projectile(xf, weapon)
{
	Init(weapon.GetOwner(), weapon.GetWeaponDef().projectileSpeed, density, friction, restitution);
}

SolidProjectile::SolidProjectile(const XForm2& xf, const WeaponDef& weaponDef, GameObject* attacker, float density, float friction, float restitution) :
	Projectile(xf, weaponDef, attacker)
{
	Init(attacker, weaponDef.projectileSpeed, density, friction, restitution);
}

void SolidProjectile::Init(GameObject* attacker, float speed, float density, float friction, float restitution)
{
	ASSERT(radius > 0);

	// set up the physics
	CreatePhysicsBody(GetXFormLocal());
	GetPhysicsBody()->SetBullet(true);
	GetPhysicsBody()->SetLinearDamping(Physics::defaultLinearDamping);
	
	// create the fixture
	b2CircleShape shapeDef;
	shapeDef.m_radius = radius;
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shapeDef;
	fixtureDef.density = density;
	fixtureDef.friction = friction;
	fixtureDef.restitution = restitution;
	AddFixture(fixtureDef);

	// set up velocity incorporating the parent's velocity
  	Vector2 velocity = Vector2::BuildFromAngle(GetXFormLocal().angle)*speed*projectileSpeedScale;
	if (applyAttackerVelocity && attacker)
		velocity += attacker->GetVelocity();
	GetPhysicsBody()->SetLinearVelocity(velocity);
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Bullets
*/
////////////////////////////////////////////////////////////////////////////////////////

float BulletProjectile::defaultDamping = 0.005f;
ConsoleCommand(BulletProjectile::defaultDamping, bulletDamping);

BulletProjectile::BulletProjectile
(
	const XForm2& xf, 
	float speed, 
	GameObject* attacker, 
	const ParticleSystemDef* particleDef, 
	float _damage, 
	float _radius,
	const ParticleSystemDef* _hitEffectDef, 
	SoundControl_ID _hitSound
) :
	Projectile(xf, speed, attacker, particleDef, _damage, _radius, _hitEffectDef, _hitSound)
{
	Init(attacker, speed);
}

BulletProjectile::BulletProjectile(const XForm2& xf, Weapon& weapon) :
	Projectile(xf, weapon)
{
	Init(weapon.GetOwner(), weapon.GetWeaponDef().projectileSpeed);
}

BulletProjectile::BulletProjectile(const XForm2& xf, const WeaponDef& weaponDef, GameObject* attacker) :
	Projectile(xf, weaponDef, attacker)
{
	Init(attacker, weaponDef.projectileSpeed);
}

void BulletProjectile::Init(GameObject* attacker, float speed)
{
	damping = defaultDamping;
	if (radius > 0)
		mass = PI*radius*radius*defaultDensity;
	
	// cap the minimum bullet mass
	mass = Max(mass, 0.001f);

	// set up velocity incorporating the parent's velocity
	velocity = Vector2::BuildFromAngle(GetXFormLocal().angle)*speed*projectileSpeedScale;
	if (applyAttackerVelocity && attacker)
		velocity += attacker->GetVelocity();
}
	
void BulletProjectile::ApplyAcceleration(const Vector2& a) 
{ 
	velocity += a * GAME_TIME_STEP; 
}

void BulletProjectile::UpdateTransforms()
{
	// simulate physics on the projectile
	CollisionTest();
	if (IsDestroyed())
		return;

	// apply gravity since it doesn't really have physics
	Vector2 gravity = GetGravity();
	velocity += gravity * GAME_TIME_STEP;

	// apply damping
	velocity *= 1 - damping;

	// cap velocity
	// todo: make this a console command
	//velocity.CapMagnitudeThis(1000.0f);

	// update transform
	SetPosLocal(GetPosLocal() + velocity * GAME_TIME_STEP + 0.5f * gravity * (GAME_TIME_STEP * GAME_TIME_STEP));
	SetAngleLocal(velocity.Normalize().GetAngle());

	Projectile::UpdateTransforms();
}
	
void BulletProjectile::CollisionTest()
{
	// raycast between current and next position
	SimpleRaycastResult result;
	Vector2 gravity = GetGravity();
	Vector2 nextPos = GetPosLocal() + (velocity + gravity * GAME_TIME_STEP) * GAME_TIME_STEP + 0.5f * gravity * (GAME_TIME_STEP * GAME_TIME_STEP);
	const Line2 line(GetPosLocal(), nextPos);
	GameObject *hitObject = g_physics->RaycastSimple(line, &result, this);
	if (hitObject)
	{
		if (!result.hitFixture->IsSensor())
		{
			// move to the hit point
			SetPosLocal(result.point);

			// apply impulse to the object manually since there is no physics on the bullet
			const float impulseCoef = 0.1f;
			const Vector2 impulse = mass * velocity * impulseCoef;
			hitObject->ApplyImpulse(impulse, result.point);
		}
		
		//{
		//	// for simulated physics set our pos to be the hit position
		//	SetPosWorld(result.m_point);
		//}

		// spoof the contact event
		ContactEvent ce;
		ce.fixtureA = result.hitFixture;
		ce.fixtureB = NULL;
		ce.contact = NULL;
		ce.point = result.point;
		ce.normal = result.normal;

		CollisionAdd(*hitObject, ce, NULL, result.hitFixture);
		hitObject->CollisionAdd(*this, ce, result.hitFixture, NULL);
	}
}
	
void BulletProjectile::Update()
{
	Projectile::Update();
}