////////////////////////////////////////////////////////////////////////////////////////
/*
	Weapons
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "gameGlobals.h"
#include "weapons.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	PlayerGun
*/
////////////////////////////////////////////////////////////////////////////////////////

const ParticleSystemDef PlayerGun::fireEffect = ParticleSystemDef::Build
(
	GameTexture_Smoke,	// particle texture
	Color::Red(),		// color1 start
	Color::Orange(),	// color2 start
	Color::Red(0),		// color1 end
	Color::Yellow(0),	// color2 end
	0.2f,	0.0f,		// particle life time & percent fade in rate
	.5f,	.2f,		// particle start & end size
	0,		1,			// particle start linear & angular speed
	0.005f,	0.02f,		// emit rate & time
	0.1f,	0.1f,		// emit size & overall randomness
	0, PI,				// emit angle & particle angle
	PARTICLE_FLAG_ADDITIVE|PARTICLE_FLAG_LOCAL_SPACE	 // flags & gravity
);

const ParticleSystemDef PlayerGun::hitEffect = ParticleSystemDef::Build
(
	GameTexture_Smoke,	// particle texture
	Color::Red(),		// color1 start
	Color::Orange(),	// color2 start
	Color::Red(0),		// color1 end
	Color::Yellow(0),	// color2 end
	0.2f,	0.0f,		// particle life time & percent fade in rate
	.5f,	.2f,		// particle start & end size
	0,		1,			// particle start linear & angular speed
	0.005f,	0.02f,		// emit rate & time
	0.1f,	0.1f,		// emit size & overall randomness
	0, PI,				// emit angle & particle angle
	PARTICLE_FLAG_ADDITIVE // flags & gravity
);

const ParticleSystemDef PlayerGun::smokeEffect = ParticleSystemDef::Build
(
	GameTexture_Smoke,	// particle texture
	Color::Grey(0.3f, 1.0f),	// color1 start
	Color::Grey(0.3f, 0.7f),	// color2 start
	Color::Grey(0, 1.0f),	// color1 end
	Color::Grey(0, 0.7f),	// color2 end
	0.6f,	0.0f,		// particle life time & percent fade in rate
	.3f,	.6f,		// particle start & end size
	.6f,		1,		// particle start linear & angular speed
	0.01f,	0,			// emit rate & time
	.1f,	0.5f,		// emit size & overall randomness
	PI, PI,				// emit angle & particle angle
	0, -0.04f			 // flags & gravity
);

const ParticleSystemDef PlayerGun::trailEffect = ParticleSystemDef::BuildRibbonTrail
(
	Color(1.0f, 0.0f, 0, 1.0f),			// color1 start
	Color(1.0f, 0.5f, 0, 1.0f),			// color2 start
	Color(0.0f, 0.0f, 1.0f, 0),			// color1 end
	Color(0.0f, 0.5f, 1.0f, 0),			// color2 end
	0.05f,	0.1f,						// start end size
	0.5f, 0.1f,	GAME_TIME_STEP,	0,		// particle time, speed, emit rate, emit time
	PARTICLE_FLAG_ADDITIVE, 0, 0		// particle flags, gravity, randomness
);

const WeaponDef PlayerGun::weaponStaticDef
(
	0.15f,					// fire rate
	10,						// damage
	15,						// projectile speed
	0.07f,					// projectile size
	0.02f,					// fire angle
	true,					// fire automatic
	false,					// is it a bullet
	SoundControl_test,			// fire sound
	&PlayerGun::trailEffect,// trail effect
	&PlayerGun::fireEffect,	// fire effect
	&PlayerGun::smokeEffect,// smoke effect
	&PlayerGun::hitEffect,	// weapon hit effect
	SoundControl_test			// weapon hit sound
);

PlayerGun::PlayerGun(const XForm2& xf, GameObject* _parent) :
	Weapon(weaponStaticDef, xf, _parent)
{
}

Projectile* PlayerGun::LaunchProjectile(const XForm2& xf)
{
	{
		// kick off a light flash
		const Color c1(1, 1, 0.5f, 1);
		const Color c2(1, 0.8f, 0.5f, 1);
		Light* light = new Light(GetXFormWorld(), NULL, 100, Color::RandBetween(c1, c2), true);
		light->SetOverbrightRadius(0.5f);
		light->SetFadeTimer(0.1f, true);
	}

	return new PlayerGunProjectile(xf, *this);
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	PlayerGunProjectile
*/
////////////////////////////////////////////////////////////////////////////////////////

PlayerGunProjectile::PlayerGunProjectile(const XForm2& xf, Weapon& weapon) :
	SolidProjectile(xf, weapon)
{
	SetRenderGroup(RenderGroup_effect);
}

void PlayerGunProjectile::Render()
{
	DeferredRender::EmissiveRenderBlock emissiveRenderBlock;
	const XForm2 xf = GetInterpolatedXForm();
	g_render->RenderQuad(xf, Vector2(radius*2.0f), Color::White(), GameTexture_Circle);
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Explosion
*/
////////////////////////////////////////////////////////////////////////////////////////

static bool explosionDebug = false;
ConsoleCommand(explosionDebug, explosionDebug);

Explosion::Explosion(const XForm2& xf, float _force, float _radius, float _damage, GameObject* _attacker, float _time, bool _damageSelf) :
	GameObject(xf),
	force(_force),
	radius(_radius),
	damage(_damage),
	attacker(_attacker? _attacker->GetHandle() : invalidHandle),
	lifeTime(-_time),
	damageSelf(_damageSelf)
{
	damage *= 4;
	force *= 10;

	if (explosionDebug)
		GetPosWorld().RenderDebug(Color::Red(0.3f), radius, _time > 0? _time : 1);

	if (_attacker)
		SetTeam(_attacker->GetTeam());
	
	CreatePhysicsBody(xf, b2_staticBody);
	b2CircleShape shapeDef;
	shapeDef.m_radius = radius;
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shapeDef;
	fixtureDef.density = 0;
	fixtureDef.friction = 0;
	fixtureDef.restitution = 0;
	fixtureDef.isSensor = true;
	AddFixture(fixtureDef);
}

void Explosion::Update()
{
	if (lifeTime >= 0)
		Destroy();

	hitObjects.clear();
}

void Explosion::CollisionPersist(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture)
{
	if (!damageSelf && otherObject.GetHandle() == attacker)
		return;

	for (list<GameObjectHandle>::iterator it = hitObjects.begin(); it != hitObjects.end(); ++it) 
	{
		// prevent multiple hits on the same object
		if (*it == otherObject.GetHandle())
			return;
	}
	hitObjects.push_back(otherObject.GetHandle());

	Vector2 pos = otherObject.GetPosWorld();

	if (explosionDebug)
	{
		Line2(GetPosWorld(), pos).RenderDebug(Color::White(), 1);
		pos.RenderDebug(Color::White(), 1, 1);
	}

	const Vector2 deltaPos = pos - GetPosWorld();
	const float distance = deltaPos.Length();
	const float p = 1 - CapPercent(distance/radius);
	
	if (force > 0)
	{
		const float appliedForce = Lerp(p, 0.0f, force);
		otherObject.ApplyForce(appliedForce*deltaPos.Normalize(), pos);
	}

	if (damage > 0)
		otherObject.ApplyDamage(p*damage, g_objectManager.GetObjectFromHandle(attacker), GameDamageType_Explosion);
}

void Explosion::CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture)
{
	CollisionPersist(otherObject, contactEvent, myFixture, otherFixture);
}


////////////////////////////////////////////////////////////////////////////////////////
/*
	Debris
*/
////////////////////////////////////////////////////////////////////////////////////////

// default trail effect
ParticleSystemDef Debris::theTrailEffect = ParticleSystemDef::BuildRibbonTrail
(
	Color(0.9f, 0.9f, 0.9f, 0.5f),	// color1 start
	Color(1.0f, 1.0f, 1.0f, 0.5f),	// color2 start
	Color(0.9f, 0.9f, 0.9f, 0.0f),	// color1 end
	Color(1.0f, 1.0f, 1.0f, 0.0f),	// color2 end
	0.2f,	0.0f,					// start end size
	1.5f, .2f, 0.2f,	0,			// particle time, speed, emit rate, emit time
	0, 0, 0.4f						// flags, gravity, randomness
);

// throw out some debris
void Debris::CreateDebris(GameObject& object, int count, float speedScale)
{
	// fire out some random debris
	const int shotCount = RAND_INT_BETWEEN(count, (int)(count*1.5f));
	XForm2 shotXF(object.GetXFormWorld());
	for (int shots = 0; shots < shotCount; ++shots)
	{
		// create the debris
		shotXF.angle = RAND_BETWEEN(-PI, PI);
		float speed = speedScale*RAND_BETWEEN(6.0f, 12.0f);
		float radius = RAND_BETWEEN(.1f, .15f);

		theTrailEffect.particleSizeStart = 0.8f*radius;
		theTrailEffect.particleSizeEnd = 2*radius;

		// give some randomness to the brightness
		float brightness = RAND_BETWEEN(0.4f, 0.9f);
		theTrailEffect.colorStart1 = Color::Grey(0.8f, brightness);
		theTrailEffect.colorStart2 = Color::Grey(0.8f, brightness*0.4f);
		theTrailEffect.colorEnd1 = Color::Grey(0.0f, brightness);
		theTrailEffect.colorEnd2 = Color::Grey(0.0f, brightness*0.4f);

		Debris* debris = new Debris(shotXF, speed, &object, radius, &theTrailEffect, brightness);
		debris->SetTeam(GameTeam_none);
		debris->SetRenderGroup(RenderGroup_low);
		debris->GetTrailEmitter().SetRenderGroup(RenderGroup_low-1);
	}
}

Debris::Debris(const XForm2& xf, float speed, GameObject* attacker, float radius, const ParticleSystemDef *trailEffect, float _brightness) :
	SolidProjectile(xf, speed, attacker, trailEffect, 0, radius),
	brightness(_brightness)
{
	lifeTimer.Set(RAND_BETWEEN(2.5f, 3.0f));
}

void Debris::Update()
{
	if (lifeTimer.HasElapsed())
		Kill();

	// fade off alpha of trail by life time
	ParticleSystemDef& def = GetTrailEmitter().GetDef();
	const float a = 0.8f*(1 - lifeTimer);
	def.colorStart1.a = a;
	def.colorStart2.a = a;

	SolidProjectile::Update();
}

void Debris::Render()
{     
	g_render->DrawSolidCircle(GetInterpolatedXForm(), Vector2(radius), Color::Grey((1 - lifeTimer), brightness));
}    

void Debris::HitObject(GameObject& object)
{  
	// fade off quicker if we hit something
	if (lifeTimer.GetTimeLeft() > 1.0)
		lifeTimer.Set(1.0f);
}

bool Debris::ShouldCollide(const GameObject& otherObject, const b2Fixture* myFixture, const b2Fixture* otherFixture) const
{
	// only collide with terrain
	return (otherObject.IsStatic());
}
