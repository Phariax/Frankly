////////////////////////////////////////////////////////////////////////////////////////
/*
	Particle Emitter
	Copyright 2013 Frank Force - http://www.frankforce.com

	- controls particle system for a 2d game
	- simple, fast, easy to use particle system
	- does not keep references to particle or emitter definitions
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef PARTICLE_EMITTER_H
#define PARTICLE_EMITTER_H

#include "../objects/gameObject.h"

#define PARTICLE_FLAG_LOCAL_SPACE				(1 << 0) // are particles in the emitter's local space?
#define PARTICLE_FLAG_CAMERA_SPACE				(1 << 1) // are particles in camera space?
#define PARTICLE_FLAG_ADDITIVE					(1 << 2) // should particles use additive blending?
#define PARTICLE_FLAG_TRAIL_LINE				(1 << 3) // should particles render as a line trail?
#define PARTICLE_FLAG_TRAIL_RIBBON				(1 << 4) // should particles render as a ribbon trail?
#define PARTICLE_FLAG_DONT_FLIP_ANGULAR			(1 << 5) // should we not randomly flip the angular speed
#define PARTICLE_FLAG_DONT_FLIP					(1 << 6) // should we not randomly flip the sprite
#define PARTICLE_FLAG_CAST_SHADOWS				(1 << 7) // should this particle system be used for drawing shadows
#define PARTICLE_FLAG_CAST_SHADOWS_DIRECTIONAL	(1 << 8) // this particle casts shadows only for the background
#define PARTICLE_FLAG_MANUAL_RENDER				(1 << 9) // this particle is only rendered manually

struct ParticleSystemDef
{
	// constructor for normal particle systems
	static ParticleSystemDef Build
	(
		GameTextureID _texture,				// texture of particle (you must set this)
		const Color& _colorStart1,				// color at start of life
		const Color& _colorStart2,				// color at start of life
		const Color& _colorEnd1,				// color at end of life
		const Color& _colorEnd2,				// color at end of life
		float _particleLifeTime,				// amount of time particle can live for, (0 = render exactaly 1 frame)
		float _particleFadeInTime,				// how quickly to fade in particle (0 = instant, 1 = lifeTime)
		float _particleSizeStart,				// size at start of life
		float _particleSizeEnd,					// size at end of life
		float _particleSpeed,					// start speed for particles
		float _particleAngularSpeed,			// start angular speed for particles
		float _emitRate,						// rate in seconds per particle (higher = slower)
		float _emitLifeTime,					// how long to emit for before dying (0 = forever)
		float _emitSize,						// size of the emit radius (0 = point emitter)
		float _randomness = 0.2f,				// use the same randomness for all settings
		float _emitConeAngle = PI,				// angle in radians of the emit cone in local space
		float _particleConeAngle = PI,			// angle in radians of the particle's initial rotation cone
		UINT _particleFlags = 0,				// list of flags for the system
		float _particleGravity = 0				// how much does gravity effect particles
	);

	// constructor for trail particles
	static ParticleSystemDef BuildLineTrail
	(
		const Color& _colorStart1,			// color at start of life
		const Color& _colorStart2,			// color at start of life
		const Color& _colorEnd1,			// color at end of life
		const Color& _colorEnd2,			// color at end of life
		float _particleLifeTime,			// amount of time particle can live for, (0 = render exactaly 1 frame)
		float _particleSpeed = 0,			// particle speed
		float _emitRate = GAME_TIME_STEP,	// how often to emit new particles
		float _emitLifeTime = 0,			// how long to emit for before dying (0 = forever)
		UINT _particleFlags = 0,			// list of flags for the system (will be or'd with trail line flag)
		float _particleGravity = 0			// how much does gravity effect particles
	);

	// constructor for trail particles
	static ParticleSystemDef BuildRibbonTrail
	(
		const Color& _colorStart1,			// color at start of life
		const Color& _colorStart2,			// color at start of life
		const Color& _colorEnd1,			// color at end of life
		const Color& _colorEnd2,			// color at end of life
		float _particleSizeStart,			// size at start of life
		float _particleSizeEnd,				// size at end of life
		float _particleLifeTime,			// amount of time particle can live for, (0 = render exactaly 1 frame)
		float _particleSpeed,				// particle speed
		float _emitRate = GAME_TIME_STEP,	// how often to emit new particles
		float _emitLifeTime = 0,			// how long to emit for before dying (0 = forever)
		UINT _particleFlags = 0,			// list of flags for the system (will be or'd with trail line flag)
		float _particleGravity = 0,			// how much does gravity effect particles
		float _randomness = 0.2f			// use the same randomness for all settings
	);

	static ParticleSystemDef Lerp(float percent, const ParticleSystemDef& p1, const ParticleSystemDef& p2);

	// default constructor doesn't init anything!
	ParticleSystemDef() {}

	// full constructor for particle systems
	ParticleSystemDef
	(
		GameTextureID _texture,				// texture of particle (you must set this)
		const Color& _colorStart1,				// color at start of life
		const Color& _colorStart2,				// color at start of life
		const Color& _colorEnd1,				// color at end of life
		const Color& _colorEnd2,				// color at end of life

		float _particleLifeTime,				// amount of time particle can live for, (0 = render exactaly 1 frame)
		float _particleFadeInTime,				// how quickly to fade in particle (0 = instant, 1 = lifeTime)
		float _particleSizeStart,				// size at start of life
		float _particleSizeEnd,					// size at end of life
		float _particleSpeed,					// start speed for particles
		float _particleAngularSpeed,			// start angular speed for particles
		float _emitRate,						// rate in seconds per particle (higher = slower)
		float _emitLifeTime,					// how long to emit for before dying (0 = forever)
		float _emitSize,						// size of the emit radius (0 = point emitter)

		float _particleSpeedRandomness,			// randomness for particle speed
		float _particleAngularSpeedRandomness,	// randomness for particle angular speed
		float _particleSizeStartRandomness,		// randomness for particle start size
		float _particleSizeEndRandomness,		// randomness for particle end size
		float _particleLifeTimeRandomness,		// randomness for particle life time

		float _emitConeAngle,					// angle in radians of the emit cone in local space
		float _particleConeAngle,				// angle in radians of the particle's initial rotation cone
		UINT _particleFlags,					// list of flags for the system
		float _particleGravity					// how much does gravity effect particles
	);

	void Scale(float scale);

public:

	GameTextureID texture;					// texture of particle

	Color colorStart1;						// color at start of life
	Color colorStart2;						// color at start of life
	Color colorEnd1;						// color at end of life
	Color colorEnd2;						// color at end of life

	float particleLifeTime;					// amount of time particle can live for, (0 = render for 1 frame)
	float particleLifeTimeRandomness;		// percent randomness for lifetime
	float particleFadeInTime;				// how quickly to fade in particle (0 = instant, 1 = lifeTime)

	float particleSizeStart;				// size at start of life
	float particleSizeStartRandomness;		// percent randomness for start size
	float particleSizeEnd;					// size at end of life
	float particleSizeEndRandomness;		// percent randomness for end size

	float particleSpeed;					// start speed for particles
	float particleSpeedRandomness;			// percent randomness for particles start speed
	float particleAngularSpeed;				// start angular speed for particles
	float particleAngularSpeedRandomness;	// percent randomness for particles start angular speed
	float particleGravity;					// how much does gravity effect particles

	float emitRate;							// rate in seconds per particle (higher = slower)
	float emitLifeTime;						// how long to emit for before dying (0 = forever)
	float emitSize;							// size of the emit radius (0 = point emitter)

	float particleConeAngle;				// how much to rotate angle by
	float emitConeAngle;					// angle in radians of the emit cone in local space (0 = directional, PI = omnidirectional)

	UINT particleFlags;						// list of flags for the system
};

class Particle;

class ParticleEmitter : public GameObject
{
public:

	ParticleEmitter(const ParticleSystemDef& _systemDef, const XForm2& xf = XForm2::Identity(), GameObject* _parent = NULL, float scale = 1.0f);
	virtual ~ParticleEmitter();

	virtual bool IsParticleEmitter() const { return true; }

	virtual void WasDetachedFromParent()
	{ 
		// kill emitter when detached from parent
		Kill();
	}

	bool IsDead() const
	{
		return (systemDef.emitLifeTime > 0 && time >= systemDef.emitLifeTime);
	}

	void Kill()
	{
		// hack: set emitter to be past end of life
		if (systemDef.emitLifeTime == 0)
			systemDef.emitLifeTime = 0.0001f;
		time.Set(systemDef.emitLifeTime + 1.0f);
	}

	void SetEmitterPaused(bool _emitterPaused)	{ emitterPaused = _emitterPaused; }
	bool GetEmitterPaused() const				{ return emitterPaused; }

	// some effects may need this for when there is no parent or the parent dies
	void SetTrailEnd(const Vector2& trailEndPos);

	// get our copy of the system def
	ParticleSystemDef& GetDef() { return systemDef; }

	// let the emitter run a bit to warm up the system
	void WarmUp(float time);

	// force it to spawn a particle
	void SpawnParticle();

	// just kill the particle system when streamed out instead of destroying it
	virtual void StreamOut() { Kill(); }

	// particle systems can be rendered manually
	void ManualRender();

public: // static functions

	static void InitParticleSystem();

	static void EnableParticles(bool enable) { enableParticles = enable; }
	static bool AreParticlesEnabled() { return enableParticles; }

	static int GetTotalParticleCount() { return maxParticles - freeParticleList.size(); }
	static int GetTotalEmitterCount() { return totalEmitterCount; }

	static void SetParticleStopRadius(float radius) { particleStopRadius = radius; }

	static bool enableParticles;
	static float particleStopRadius;		// does and on screen test with this radius and won't emit particles from offscreen (0 = always spawn)
	static int defaultRenderGroup;
	static int defaultAdditiveRenderGroup;
	static float shadowRenderAlpha;

protected:

	static Particle* NewParticle();
	static void DeleteParticle(Particle* particle);

	// add a new particle to the emitter
	void AddParticle(const XForm2& xf, float startTime = 0);

	virtual void Render();
	virtual void Update();
	void RenderInternal(bool allowAdditive=true);

private:

	bool setTrailEnd;
	bool emitterPaused;		// user control to pause particle emitters

	GameTimer time;
	float emitRateTimer;
	ParticleSystemDef systemDef;

	list<Particle*> particles;

	static int totalEmitterCount;
	
	static const UINT32 maxParticles = 2000;
	static list<Particle*> freeParticleList;
	static Particle particlePool[maxParticles];
};

class Particle : private Uncopyable
{
public:

	friend ParticleEmitter;

	Particle() {}
	void Init(const ParticleSystemDef& _systemDef, const XForm2& _xf, const Vector2& _velocity, float _angularSpeed, float _startTime);

	void Update();

	void Render(const XForm2& xfParent);
	void RenderTrail(const XForm2& xfParent);
	void RenderRibbon(const XForm2& xfParent, Vector2& previousPos);

	bool IsDead() const { return time >= lifeTime + GAME_TIME_STEP; }

	XForm2 GetInterpolatedXForm(const XForm2& xfParent) const
	{ 
		const XForm2 xfInterpolated = xf.Interpolate(xfDelta, g_interpolatePercent);
		return xfInterpolated * xfParent;
	}

private:

	XForm2 xf;
	XForm2 xfDelta;

	Vector2 velocity;
	float angularSpeed;
	float time;

	float lifeTime;
	float sizeStart;
	float sizeEnd;
	Color colorStart;
	Color colorDelta;

	// cache some particle data for faster rendering
	UINT cachedFrame;
	Color cachedColor;
	Vector2 cachedPos1;
	Vector2 cachedPos2;
	float cachedAngle;
	float cachedSize;

	const ParticleSystemDef* systemDef;

	static DWORD simpleVertsLastColor;
	static bool simpleVertsNeedsCap;
};

extern const ParticleSystemDef g_testParticleSystemDef;

// full constructor for particle systems
inline ParticleSystemDef::ParticleSystemDef
(
	GameTextureID _texture,				// texture of particle (you must set this)
	const Color& _colorStart1,				// color at start of life
	const Color& _colorStart2,				// color at start of life
	const Color& _colorEnd1,				// color at end of life
	const Color& _colorEnd2,				// color at end of life

	float _particleLifeTime,				// amount of time particle can live for, (0 = render exactaly 1 frame)
	float _particleFadeInTime,				// how quickly to fade in particle (0 = instant, 1 = lifeTime)
	float _particleSizeStart,				// size at start of life
	float _particleSizeEnd,					// size at end of life
	float _particleSpeed,					// start speed for particles
	float _particleAngularSpeed,			// start angular speed for particles
	float _emitRate,						// rate in seconds per particle (higher = slower)
	float _emitLifeTime,					// how long to emit for before dying (0 = forever)
	float _emitSize,						// size of the emit radius (0 = point emitter)

	float _particleSpeedRandomness,			// randomness for particle speed
	float _particleAngularSpeedRandomness,	// randomness for particle angular speed
	float _particleSizeStartRandomness,		// randomness for particle start size
	float _particleSizeEndRandomness,		// randomness for particle end size
	float _particleLifeTimeRandomness,		// randomness for particle life time

	float _emitConeAngle,					// angle in radians of the emit cone in local space
	float _particleConeAngle,				// angle in radians of the particle's initial rotation cone
	UINT _particleFlags,					// list of flags for the system
	float _particleGravity					// how much does gravity effect particles
) :
	texture(_texture),
	colorStart1(_colorStart1),
	colorStart2(_colorStart2),
	colorEnd1(_colorEnd1),
	colorEnd2(_colorEnd2),
	particleLifeTime(_particleLifeTime),
	particleFadeInTime(_particleFadeInTime),
	particleSizeStart(_particleSizeStart),
	particleSizeEnd(_particleSizeEnd),
	particleSpeed(_particleSpeed),
	particleAngularSpeed(_particleAngularSpeed),
	emitRate(_emitRate),
	emitLifeTime(_emitLifeTime),
	emitSize(_emitSize),
	emitConeAngle(_emitConeAngle),
	particleConeAngle(_particleConeAngle),
	particleFlags(_particleFlags),
	particleGravity(_particleGravity),
	particleSpeedRandomness(_particleSpeedRandomness),
	particleAngularSpeedRandomness(_particleAngularSpeedRandomness),
	particleSizeStartRandomness(_particleSizeStartRandomness),
	particleSizeEndRandomness(_particleSizeEndRandomness),
	particleLifeTimeRandomness(_particleLifeTimeRandomness)
{
}
	
// constructor for normal particle systems
inline ParticleSystemDef ParticleSystemDef::Build
(
	GameTextureID _texture,		// texture of particle (you must set this)
	const Color& _colorStart1,		// color at start of life
	const Color& _colorStart2,		// color at start of life
	const Color& _colorEnd1,		// color at end of life
	const Color& _colorEnd2,		// color at end of life

	float _particleLifeTime,		// amount of time particle can live for, (0 = render exactaly 1 frame)
	float _particleFadeInTime,		// how quickly to fade in particle (0 = instant, 1 = lifeTime)
	float _particleSizeStart,		// size at start of life
	float _particleSizeEnd,			// size at end of life
	float _particleSpeed,			// start speed for particles
	float _particleAngularSpeed,	// start angular speed for particles
	float _emitRate,				// rate in seconds per particle (higher = slower)
	float _emitLifeTime,			// how long to emit for before dying (0 = forever)
	float _emitSize,				// size of the emit radius (0 = point emitter)

	float _randomness,				// use the same randomness for all settings
	float _emitConeAngle,			// angle in radians of the emit cone in local space
	float _particleConeAngle,		// angle in radians of the particle's initial rotation cone
	UINT _particleFlags,			// list of flags for the system
	float _particleGravity			// how much does gravity effect particles
)
{
	return ParticleSystemDef
	(
		_texture,
		_colorStart1, _colorStart2,
		_colorEnd1, _colorEnd2,
		_particleLifeTime, _particleFadeInTime,
		_particleSizeStart, _particleSizeEnd,
		_particleSpeed, _particleAngularSpeed,
		_emitRate, _emitLifeTime,
		_emitSize, 1.0f,
		_randomness, _randomness, _randomness, _randomness,
		_emitConeAngle, _particleConeAngle,
		_particleFlags, _particleGravity
	);
}

// constructor for trail particles
inline ParticleSystemDef ParticleSystemDef::BuildLineTrail
(
	const Color& _colorStart1,	// color at start of life
	const Color& _colorStart2,	// color at start of life
	const Color& _colorEnd1,	// color at end of life
	const Color& _colorEnd2,	// color at end of life
	float _particleLifeTime,	// amount of time particle can live for, (0 = render exactaly 1 frame)
	float _particleSpeed,		// particle speed
	float _emitRate,			// how often to emit new particles
	float _emitLifeTime,		// how long to emit for before dying (0 = forever)
	UINT _particleFlags,		// list of flags for the system (will be or'd with trail line flag)
	float _particleGravity		// how much does gravity effect particles
)
{
	return ParticleSystemDef
	(
		GameTexture_Invalid,
		_colorStart1, _colorStart2,
		_colorEnd1, _colorEnd2,
		_particleLifeTime, 0,
		0, 0,
		_particleSpeed, 0,
		_emitRate, _emitLifeTime,
		0, 1, 0, 0, 0, 0, PI, PI,
		_particleFlags|PARTICLE_FLAG_TRAIL_LINE,
		_particleGravity
	);
}

// constructor for trail particles
inline ParticleSystemDef ParticleSystemDef::BuildRibbonTrail
(
	const Color& _colorStart1,		// color at start of life
	const Color& _colorStart2,		// color at start of life
	const Color& _colorEnd1,		// color at end of life
	const Color& _colorEnd2,		// color at end of life
	float _particleSizeStart,		// size at start of life
	float _particleSizeEnd,			// size at end of life
	float _particleLifeTime,		// amount of time particle can live for, (0 = render exactaly 1 frame)
	float _particleSpeed,			// particle speed
	float _emitRate,				// how often to emit new particles
	float _emitLifeTime,			// how long to emit for before dying (0 = forever)
	UINT _particleFlags,			// list of flags for the system (will be or'd with trail line flag)
	float _particleGravity,			// how much does gravity effect particles
	float _randomness				// use the same randomness for all settings
)
{
	return ParticleSystemDef
	(
		GameTexture_Invalid,
		_colorStart1, _colorStart2,
		_colorEnd1, _colorEnd2,
		_particleLifeTime, 0,
		_particleSizeStart, _particleSizeEnd,
		_particleSpeed, 0,
		_emitRate, _emitLifeTime,
		0, 1, _randomness, _randomness, _randomness, 0, PI, 0,
		_particleFlags|PARTICLE_FLAG_TRAIL_RIBBON,
		_particleGravity
	);
}

inline ParticleSystemDef ParticleSystemDef::Lerp(float percent, const ParticleSystemDef& p1, const ParticleSystemDef& p2)
{
	return ParticleSystemDef
	(
		p1.texture,
		FrankMath::Lerp(percent, p1.colorStart1,						p2.colorStart1),					
		FrankMath::Lerp(percent, p1.colorStart2,						p2.colorStart2),
		FrankMath::Lerp(percent, p1.colorEnd1,							p2.colorEnd1),
		FrankMath::Lerp(percent, p1.colorEnd2,							p2.colorEnd2),
		FrankMath::Lerp(percent, p1.particleLifeTime,					p2.particleLifeTime),
		FrankMath::Lerp(percent, p1.particleFadeInTime,					p2.particleFadeInTime),
		FrankMath::Lerp(percent, p1.particleSizeStart,					p2.particleSizeStart),
		FrankMath::Lerp(percent, p1.particleSizeEnd,					p2.particleSizeEnd),
		FrankMath::Lerp(percent, p1.particleSpeed,						p2.particleSpeed),
		FrankMath::Lerp(percent, p1.particleAngularSpeed,				p2.particleAngularSpeed),
		FrankMath::Lerp(percent, p1.emitRate,							p2.emitRate),
		FrankMath::Lerp(percent, p1.emitLifeTime,						p2.emitLifeTime),
		FrankMath::Lerp(percent, p1.emitSize,							p2.emitSize),
		FrankMath::Lerp(percent, p1.particleSpeedRandomness,			p2.particleSpeedRandomness),
		FrankMath::Lerp(percent, p1.particleAngularSpeedRandomness,		p2.particleAngularSpeedRandomness),
		FrankMath::Lerp(percent, p1.particleLifeTimeRandomness,			p2.particleSizeStartRandomness),
		FrankMath::Lerp(percent, p1.particleSizeStartRandomness,		p2.particleSizeEndRandomness),
		FrankMath::Lerp(percent, p1.particleSizeEndRandomness,			p2.particleLifeTimeRandomness),
		FrankMath::Lerp(percent, p1.emitConeAngle,						p2.emitConeAngle),
		FrankMath::Lerp(percent, p1.particleConeAngle,					p2.particleConeAngle),
		p1.particleFlags,
		FrankMath::Lerp(percent, p1.particleGravity,					p2.particleGravity)
	);
}


#endif // PARTICLE_EMITTER_H