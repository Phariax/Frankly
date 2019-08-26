////////////////////////////////////////////////////////////////////////////////////////
/*
	Weapon
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../objects/weapon.h"

WeaponDef::WeaponDef
(
	float _fireRate,							// amount of time between shots
	float _damage,								// amount of damage applied when bullets hit
	float _projectileSpeed,						// speed the projectiles when fired
	float _projectileRadius,					// size of the projectiles
	float _fireAngle,							// randomness in angle applied to fire direction
	bool _fireAutomatic,						// does weapon continue firing when trigger is held?
	bool _projectileBullet,						// are projectiles bullets or physical?
	SoundControl_ID _fireSound,					// sound effect when weapon is fired
	const ParticleSystemDef* _trailEffect,		// trail applied to projectiles
	const ParticleSystemDef* _fireFlashEffect,	// flash effect when weapon is fired
	const ParticleSystemDef* _fireSmokeEffect,	// smoke effect when weapon is fired
	const ParticleSystemDef* _hitEffect,		// effect when projectile hits
	SoundControl_ID _hitSound						// sound effect when projectile hits
) :
	fireRate(_fireRate),
	damage(_damage),
	projectileSpeed(_projectileSpeed),
	projectileRadius(_projectileRadius),
	fireAngle(_fireAngle),
	fireAutomatic(_fireAutomatic),
	projectileBullet(_projectileBullet),
	fireSound(_fireSound),
	trailEffect(_trailEffect),
	fireFlashEffect(_fireFlashEffect),
	fireSmokeEffect(_fireSmokeEffect),
	hitEffect(_hitEffect),
	hitSound(_hitSound)
{}

Weapon::Weapon(const WeaponDef &_weaponDef, const XForm2& xf, GameObject* _parent, const XForm2& _xfFire) :
	GameObject(xf, _parent),
	weaponDef(_weaponDef),
	fireTimer(0),
	triggerIsDown(false),
	triggerWasDown(false),
	xfFire(_xfFire),
	owner(_parent),
	smokeEmitter(NULL)
{
}

void Weapon::Update()
{
	fireTimer += GAME_TIME_STEP;
	fireTimer = Min(fireTimer, GAME_TIME_STEP);

	if (triggerIsDown)
		while (Fire());

	if (triggerWasDown && !triggerIsDown)
		TriggerReleased();

	triggerWasDown = triggerIsDown;
	triggerIsDown = false;

	if (smokeEmitter && lastFiredTimer > weaponDef.fireRate)
	{
		smokeEmitter->Kill();
		smokeEmitter = NULL;
	}
}

// will be called automatically in Update() depending if TiggerIsDown() was called and the weapon can fire
// this can be called directly to try firing a single shot, but may not fire if unable to
// retuns true if weapon was fired
bool Weapon::Fire()
{
	if (!CanFire())
		return false;

	// update the fire timer
	lastFiredTimer.Set();
	if (GetWeaponDef().fireAutomatic)
		fireTimer -= weaponDef.fireRate;
	else
		fireTimer = -weaponDef.fireRate;

	PlayFireEffect();

	const float maxFireAngle = GetWeaponDef().fireAngle;
	const float fireAngle = GetAngleWorld() + RAND_BETWEEN(-maxFireAngle, maxFireAngle);
	LaunchProjectile(xfFire * XForm2(GetPosWorld(), fireAngle));

	return true;
}

// weapons automatically launch a projectile when they fire
// this may be a physical or a bullet type projectile
GameObject* Weapon::LaunchProjectile(const XForm2& xf)
{
	if (GetWeaponDef().projectileBullet)
		return new BulletProjectile(xf, *this);
	else
		return new SolidProjectile(xf, *this);
}

// play particle and sound effect when the weapon fires
void Weapon::PlayFireEffect()
{
	// play sound effect
	if (weaponDef.fireSound)
		g_sound->Play(weaponDef.fireSound, *this);

	// start flash effect
	if (weaponDef.fireFlashEffect)
		new ParticleEmitter(*weaponDef.fireFlashEffect, xfFire, this);

	// startfire smoke effect
	if (weaponDef.fireSmokeEffect && !smokeEmitter)
		smokeEmitter = new ParticleEmitter(*weaponDef.fireSmokeEffect, xfFire, this);
}