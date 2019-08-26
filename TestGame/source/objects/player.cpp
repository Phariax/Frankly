////////////////////////////////////////////////////////////////////////////////////////
/*
	Player
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "gameGlobals.h"
#include "player.h"
#include "gameObjects.h"
#include "weapons.h"

static bool godMode = false;
ConsoleCommand(godMode, godMode);

void Player::Init()
{
	equippedWeapon = NULL;
	g_player = this;

	// basic object settings
	SetTeam(GameTeam_player);
	lifeTimer.Set();
	light = NULL;

	radius = 0.4f;
	ResetHealth();
	lifeTimer.Set();
	deadTimer.Invalidate();
	
	{
		// create the physics body
		CreatePhysicsBody(GetXFormWorld());
		GetPhysicsBody()->SetLinearDamping(5.0f);
		GetPhysicsBody()->SetAngularDamping(10.0f);
		SetHasGravity(false);
		
		// hook up physics
		b2CircleShape shapeDef;
		shapeDef.m_radius = radius;
		b2FixtureDef fixtureDef;
		fixtureDef.shape = &shapeDef;
		fixtureDef.density = Physics::defaultDensity;
		fixtureDef.friction = 1.0;
		fixtureDef.restitution = 0.2f;
		AddFixture(fixtureDef);
	}
	{	
		// set up the player's weapon
		equippedWeapon = new PlayerGun(Vector2(0,radius), this);
	}
	if (!g_gameControl->IsEditMode())
	{
		// trail effect
		ParticleSystemDef trailEffect = ParticleSystemDef::BuildRibbonTrail
		(
			Color::White(0.7f),			// color1 start
			Color::White(0.7f),			// color2 start
			Color::White(0),			// color1 end
			Color::White(0),			// color2 end
			0.3f,	0.0f,				// start end size
			2.5f, 0, 0.1f,	0,			// particle time, speed, emit rate, emit time
			0, 0, 0						// particle flags, gravity, randomness
		);
		new ParticleEmitter(trailEffect, Vector2(0), this);
	}
	{
		// hook up a spot light to the gun
		light = new Light(Vector2(0,radius), this, 15, Color::White(), true);
		light->SetOverbrightRadius(.4f);
		light->SetHaloRadius(.5f);
		light->SetConeAngles(0.3f, 0.6f);
	}
	{
		// hook up a light for player is damaged
		damageLight = new Light(Vector2(0), this, 20, Color::White(0), true);
		damageLight->SetOverbrightRadius(.6f);
	}

	// reset transforms so player pops to new respawn positon rather then interpolating
	ResetLastWorldTransforms();
}

Player::~Player()
{
	g_player = NULL;
}

void Player::Update()
{
	if (!g_gameControl->IsGameplayMode() || IsDead())
		return;

	{
		// control movement
		Vector2 inputControl(0);
		if (g_input->IsDown(GB_MoveRight))	inputControl.x += 1;
		if (g_input->IsDown(GB_MoveLeft))	inputControl.x -= 1;
		if (g_input->IsDown(GB_MoveUp))		inputControl.y += 1;
		if (g_input->IsDown(GB_MoveDown))	inputControl.y -= 1;
		Vector2 inputControlDirection = inputControl.Normalize();

		if (g_gameControl->IsUsingGamepad())
		{
			// use left stick movement control
			inputControl = g_input->GetGamepadLeftStick(0);
			inputControlDirection = inputControl.Normalize();
		}
		if (!inputControl.IsZero())
		{
			// apply movement acceleration
			static float moveAcceleration = 40;
			ConsoleCommand(moveAcceleration, playerMoveAccel);
			ApplyAcceleration(moveAcceleration*inputControlDirection);
		}
		{	
			// apply force to counter side velocity, makes turning corners more square
			static float sideDragForce = 0.3f;
			ConsoleCommand(sideDragForce, playerSideDrag);
			const Vector2 right = inputControl.RotateRightAngle();
			const Vector2 velocity = GetPhysicsBody()->GetLinearVelocity();
			const float dp = velocity.Dot(right);
			ApplyAcceleration(-right * dp * sideDragForce);
		}
		{
			// cap player's max speed
			static float maxSpeed = 20;
			ConsoleCommand(maxSpeed, playerMaxSpeed);

			static float maxAngularSpeed = 60;
			ConsoleCommand(maxAngularSpeed, playerMaxAngularSpeed);
			CapSpeed(maxSpeed);
			CapAngularSpeed(maxAngularSpeed);
		}
	}
	{
		// automatically regenerate health
		static float refillHealthRate = 0.5f;
		ConsoleCommand(refillHealthRate, playerRefillHealthRate);
		static float refillWaitTime = 1;
		ConsoleCommand(refillWaitTime, playerRefillHealthWaitTime);
		if (timeSinceDamaged > refillWaitTime)
			RefillHealth(refillHealthRate);
	}
	
	if (equippedWeapon)
	{
		// use library aim angle function to calculate best aim trajectory
		bool usingGamepad = g_gameControl->IsUsingGamepad();
		float angle = FrankUtil::GetAimAngle(equippedWeapon->GetPosWorld(), GetVelocity(), equippedWeapon->GetWeaponDef().projectileSpeed, usingGamepad);

		if (!usingGamepad)
		{
			// fixup issue of aimming inside the player
			const Vector2 mousePos = g_input->GetInterpolatedMousePosWorldSpace();
			const Vector2 deltaMousePos = mousePos - GetPosWorld();
			if (deltaMousePos.Magnitude() < radius)
				angle = deltaMousePos.GetAngle();
		}

		// turn to aim weapon
		const Vector2 gamepadRightStick = g_input->GetGamepadRightStick(0);
		static float turnAcceleration = 70;
		ConsoleCommand(turnAcceleration, playerTurnAcceleration);
		float deltaAngle = CapAngle(angle - GetAngleWorld());
		float acceleration = turnAcceleration*deltaAngle;
		if (!usingGamepad || gamepadRightStick.Magnitude() > 0.15f)
			ApplyAngularAcceleration(acceleration);
	
		// fire weapon
		if ((g_input->IsDown(GB_Trigger1) || gamepadRightStick.Magnitude() > 0.8f))
			equippedWeapon->SetTriggerIsDown();
	}

	if (light)
	{
		// turn on light when player pushes trigger2
		Color c = light->GetColor();
		c.a = (g_input->IsDown(GB_Trigger2))? 1.0f : 0.0f;
		light->SetColor(c);
	}

	static bool killPlayer = false;
	ConsoleCommand(killPlayer, kill);
	if (killPlayer)
	{
		// debug command to test killing the player
		ApplyDamage(10000);
		killPlayer = false;
	}

	if (godMode)
	{
		// godmode debug command
		SetMaxHealth(1000);
		SetHealth(1000);
	}
}

void Player::UpdateTransforms()
{
	Actor::UpdateTransforms();

	if (light)
	{
		// update light aim
		static bool joystickMode = false;
		const Vector2 gamepadRightStick = g_input->GetGamepadRightStick(0);
		if (gamepadRightStick.Magnitude() > 0.2f)
			joystickMode = true;

		if (!joystickMode || g_input->GetMouseDeltaLocalSpace().Magnitude() > 5)
		{
			// aim light directly at the mouse
			joystickMode = false;
			const Vector2 mousePos = g_input->GetInterpolatedMousePosWorldSpace();
			const Vector2 deltaPos = mousePos - GetPosWorld();
			const float angle = deltaPos.GetAngle();
			light->SetAngleLocal(angle - GetAngleWorld());
		}
		else
		{
			// in joystick mode keep light aimed straight ahead
			light->SetAngleLocal(0);
		}
		
		light->UpdateTransforms();
	}
}

void Player::Render()
{
	if (IsDead())
		return;
	
	const XForm2 xf = GetInterpolatedXForm();
	if (DeferredRender::GetRenderPassIsShadow())
	{
		g_render->RenderQuad(xf, Vector2(radius),		Color::Black(0.5f), GameTexture_Circle);
		return;
	}

	// draw a circle for the player
	DeferredRender::SpecularRenderBlock specularRenderBlock;
	g_render->RenderQuad(xf, Vector2(radius),		Color::Grey(), GameTexture_Circle);
	g_render->RenderQuad(xf, Vector2(radius*0.9f), Color::White(), GameTexture_Circle);

	{
		// draw a smaller nub where the light and gun are
		DeferredRender::EmissiveRenderBlock emissiveRenderBlock;
		Color c = Color::White();
		if (DeferredRender::GetRenderPassIsEmissive())
		{
			// make numb emissive
			c = Lerp(light->GetColor().a, Color::Red(0.5f), Color::White());
		}

		g_render->RenderQuad(xf.TransformCoord(Vector2(0, radius*0.7f)), Vector2(radius*0.3f),		c, GameTexture_Circle);
	}

	if (!DeferredRender::GetRenderPassIsDeferred())
	{
		// overlay a colored halo to show players health
		DeferredRender::EmissiveRenderBlock emissiveRenderBlock;
		DeferredRender::TransparentRenderBlock transparentRenderBlock;

		const float healthPercent = GetHealthPercent();
		Color color;
		if (healthPercent < 0.6f)
			color = PercentLerp(healthPercent, 0.1f, 0.6f, Color::Red(), Color::Yellow(0.6f));
		else
			color = PercentLerp(healthPercent, 0.6f, 1.0f, Color::Yellow(0.6f), Color::White(0.0f));

		g_render->RenderQuad(xf, Vector2(1.8f*radius),	color, GameTexture_Dot);
		
		// also update our damage light
		damageLight->SetColor(color);
	}
}

void Player::ApplyDamage(float damage, GameObject* attacker, GameDamageType damageType)
{
	if (godMode)
		return;
	
	timeSinceDamaged.Set();
	Actor::ApplyDamage(damage, attacker, damageType);
}

void Player::Kill()
{
	deadTimer.Set();
	DetachEmitters();

	if (equippedWeapon)
	{
		// remove weapon on death
		equippedWeapon->Destroy();
		equippedWeapon = NULL;
	}
	if (light)
	{
		// get rid of our lights
		DetachChild(*light);
		light->SetFadeTimer(0.1f, true);
		light = NULL;
		
		DetachChild(*damageLight);
		damageLight->SetFadeTimer(0.1f, true);
		damageLight = NULL;
	}
	{
		// kick off an explosion
		float effectSize = 5;
		new Explosion(GetXFormWorld(), effectSize, effectSize, GetMaxHealth(), this);
	}
	{
		// create some debris
		Debris::CreateDebris(*this, 20);
	}
	{
		// kick off a light flash
 		Light* light = new Light(Vector2(0), this, 15, Color(1.0f, 0.95f, 0.8f, 1.0f), true);
		light->SetOverbrightRadius(0.8f);
		light->SetFadeTimer(0.8f, true, true);
	}
	{
		// kick off smoke
		ParticleSystemDef smokeEffectDef = ParticleSystemDef::Build
		(
			GameTexture_Smoke,			// particle texture
			Color::Grey(0.5f, 0.6f),	// color1 start
			Color::Grey(0.5f, 0.3f),	// color2 start
			Color::Grey(0, 0.6f),		// color1 end
			Color::Grey(0, 0.3f),		// color2 end
			3.0f,	0.2f,				// particle life time & percent fade in rate
			1.5f,	2.5f,				// particle start & end size
			0.1f,	0.5f,				// particle start linear & angular speed
			0.06f,	0.5f,				// emit rate & time
			0.5f,	0.5f,				// emit size & overall randomness
			PI, PI, PARTICLE_FLAG_CAST_SHADOWS, -0.04f			// cone angles, flags and gravity
		);
		smokeEffectDef.particleSpeedRandomness = 1.0f;
		ParticleEmitter* emitter = new ParticleEmitter(smokeEffectDef, GetXFormWorld());
		emitter->SetRenderGroup(RenderGroup_foregroundEffect);
	}
	{
		// kick off explosion effects
		ParticleSystemDef explosionEffectDef = ParticleSystemDef::Build
		(
			GameTexture_Smoke,	// particle texture
			Color::Red(0.5f),	// color1 start
			Color::Yellow(0.5f),// color2 start
			Color::Red(0),		// color1 end
			Color::Yellow(0),	// color2 end
			0.7f,	0.1f,		// particle life time & percent fade in rate
			0.7f,	1.5f,		// particle start & end size
			.4f,		0.5f,	// particle start linear & angular speed
			0.01f,	0.3f,		// emit rate & time
			.3f,		0.5f,		// emit size & overall randomness
			PI,		PI,			// emit angle & particle angle
			PARTICLE_FLAG_ADDITIVE
		);
		ParticleEmitter* emitter = new ParticleEmitter(explosionEffectDef, GetXFormWorld());
		emitter->SetRenderGroup(RenderGroup_foregroundAdditiveEffect);
	}
	{
		// play the test sound with a low frequency scale
		g_sound->Play(SoundControl_test, *this, 1, 0.2f);
	}
}

bool Player::ShouldCollide(const GameObject& otherObject, const b2Fixture* myFixture, const b2Fixture* otherFixture) const
{
	if (IsDead())
		return false;
	else
		return Actor::ShouldCollide(otherObject, myFixture, otherFixture);
}