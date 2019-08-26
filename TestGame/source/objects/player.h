////////////////////////////////////////////////////////////////////////////////////////
/*
	Player
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef PLAYER_H
#define PLAYER_H

class Player : public Actor
{
public:	// basic functionality

	Player(const XForm2& xf = XForm2::Identity()) : 
		Actor(GameObjectStub(xf), 100), 
		light(NULL) 
	{ Init(); }
	
	// create 
	Player(const GameObjectStub& stub) : 
		Actor(stub, 100), 
		light(NULL) 
	{ Init(); }

	~Player();

	void Init();
	void Update();
	void Render();
	void Kill();
	void UpdateTransforms();
	
	void ApplyDamage(float damage, GameObject* attacker = NULL, GameDamageType damageType = GameDamageType_Default);
	virtual bool IsPlayer() const			{ return true; }
	virtual bool IsOwnedByPlayer() const	{ return true; }
	bool IsPersistant() const				{ return true; }

	// keep track of player's life/death time
	float GetLifeTime() const				{ return lifeTimer; }
	float GetDeadTime() const				{ return (IsDead() ? deadTimer : 0.0f); }

private:
	
	bool ShouldCollideSight() const { return false; }
	bool ShouldCollide(const GameObject& otherObject, const b2Fixture* myFixture, const b2Fixture* otherFixture) const;

	float radius;
	GameTimer lifeTimer;
	GameTimer deadTimer;
	GameTimer timeSinceDamaged;
	Light* light;
	Light* damageLight;
	Weapon* equippedWeapon;
};

#endif // PLAYER_H