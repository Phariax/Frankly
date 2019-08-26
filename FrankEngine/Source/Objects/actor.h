////////////////////////////////////////////////////////////////////////////////////////
/*
	Actor
	Copyright 2013 Frank Force - http://www.frankforce.com

	- has health, can die if it recieves too much damage
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef ACTOR_H
#define ACTOR_H

#include "gameObject.h"

class Actor : public GameObject
{
public:

	explicit Actor(const GameObjectStub& stub, float _health = 100) :
		GameObject(stub, NULL),
		health(_health),
		maxHealth(_health)
	{}

	explicit Actor(const XForm2& xf, float _health = 100, bool addToWorld = true) :
		GameObject(xf, NULL, addToWorld),
		maxHealth(_health)
	{}
		
	virtual bool IsActor() const { return true; }
	float GetHealth() const { return health; }
	float GetMaxHealth() const { return maxHealth; }
	float GetHealthPercent() const { return health / maxHealth; }
	void SetHealth(float _health) { health = _health; }
	void SetMaxHealth(float _maxHealth) { maxHealth = _maxHealth; }
	void ResetHealth(float _maxHealth) { health = maxHealth = _maxHealth; }
	void ResetHealth() { health = maxHealth; } 
	virtual bool IsDead() const { return health <= 0; }
	virtual void ApplyDamage(float damage, GameObject* attacker = NULL, GameDamageType damageType = GameDamageType_Default);

	void RefillHealth(float deltaHealth) 
	{
		if (IsDead())
			return;
		ASSERT(deltaHealth > 0);
		health += deltaHealth;
		health = Min(health, maxHealth);
	}

protected:

	float health;
	float maxHealth;
};

#endif // ACTOR_H