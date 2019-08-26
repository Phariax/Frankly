////////////////////////////////////////////////////////////////////////////////////////
/*
	Weapon
	Copyright 2013 Frank Force - http://www.frankforce.com

	- behaves like a firearm by default
	- fire projectiles when trigger is held
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef WEAPON_H
#define WEAPON_H

#include "../sound/soundControl.h"
#include "../objects/projectile.h"

struct WeaponDef
{
	WeaponDef
	(
		float _fireRate = 0,								// amount of time between shots
		float _damage = 0,									// amount of damage applied when bullets hit
		float _projectileSpeed = 0,							// speed the projectiles when fired
		float _projectileRadius = 0,						// size of the projectiles
		float _fireAngle = 0,								// randomness in angle applied to fire direction
		bool _fireAutomatic = true,							// does weapon continue firing when trigger is held?
		bool _projectileBullet = true,						// are projectiles bullets or physical?
		SoundControl_ID _fireSound = SoundControl_Invalid,		// sound effect when weapon is fired
		const ParticleSystemDef* _trailEffect = NULL,		// trail applied to projectiles
		const ParticleSystemDef* _fireFlashEffect = NULL,	// flash effect when weapon is fired (in local space of weapon)
		const ParticleSystemDef* _fireSmokeEffect = NULL,	// looping smoke effect when weapon is being fired
		const ParticleSystemDef* _hitEffect = NULL,			// effect when projectile hits
		SoundControl_ID _hitSound = SoundControl_Invalid			// sound effect when projectile hits
	);

	float fireRate;
	float damage;
	float projectileSpeed;
	float projectileRadius;
	float fireAngle;
	bool fireAutomatic;
	bool projectileBullet;
	SoundControl_ID fireSound;
	SoundControl_ID hitSound;
	const ParticleSystemDef* trailEffect;
	const ParticleSystemDef* fireFlashEffect;
	const ParticleSystemDef* fireSmokeEffect;
	const ParticleSystemDef* hitEffect;
};


class Weapon : public GameObject
{
public:

	Weapon(const WeaponDef &_weaponDef, const XForm2& xf = XForm2::Identity(), GameObject* _parent = NULL, const XForm2& _xfFire = XForm2::Identity());

	// call this to indicate that the trigger is being held down
	virtual void SetTriggerIsDown() { triggerIsDown = true; }
	bool IsTriggerDown() const { return triggerIsDown; }
	bool WasTriggerDown() const { return triggerWasDown; }

	// weapon's keep a copy of their parameters and their trail effect parameters
	// so these can be tweaked to effect how a weapon looks and works
	WeaponDef& GetWeaponDef() { return weaponDef; }
	const WeaponDef& GetWeaponDef() const { return weaponDef; }

	// fire will be called automatically in Update() continuously until the weapon can no longer fire
	// can be called directly to try firing a single shot, but may not fire if unable to
	// retuns true if weapon was fired
	virtual bool Fire();

	// return true if a weapon is able to fire
	virtual bool CanFire() const 
	{ return ((fireTimer >= 0) && (weaponDef.fireAutomatic || !triggerWasDown)); }

	float GetFireTimePercent() const { return (weaponDef.fireRate > 0)? CapPercent((fireTimer + weaponDef.fireRate) / weaponDef.fireRate) : 0; }
	const GameTimer& GetLastFiredTimer() const	{ return lastFiredTimer; }

	GameObject* GetOwner() { return owner? owner : GetParent(); }
	void SetOwner(GameObject* _owner) { owner = _owner; }

	virtual float GetOverheatPercent() const { return 0; }
	virtual bool IsOverheated() const { return false; }

	// weapons automatically launch a projectile when they fire
	// this may be a physical object or a bullet/raycast type projectile
	virtual GameObject* LaunchProjectile(const XForm2& xf);

	void SetFireXForm(const XForm2& xf)	{ xfFire = xf; }
	XForm2 GetFireXForm() const			{ return xfFire; }

protected:

	// play particle and sound effect when the weapon fires
	virtual void PlayFireEffect();

	// weapons update
	void Update();

	// automatically called by weapon when trigger is released
	virtual void TriggerReleased() {}

	// was the trigger down at the last update
	bool triggerWasDown;

	// is the trigger currently down
	bool triggerIsDown;

	// the transform in local space of the weapon to where the bullet is launched from
	XForm2 xfFire;

	// keep track of left over time so weapons maintain a constant firing rate
	float fireTimer;

	// last time weapon was fired
	GameTimer lastFiredTimer;
	
	// emit smoke when weapon is being fired
	ParticleEmitter* smokeEmitter;

private:

	// keep a copy of the weapon definition
	// some weapons may want to change things about the definition
	// depending of differnet states the weapon can be in
	WeaponDef weaponDef;

	// weapons can be owned, if owner is null, parent is considered to be owner
	GameObject* owner;
};


#endif // WEAPON_H