////////////////////////////////////////////////////////////////////////////////////////
/*
	Particle Emitter
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../terrain/terrain.h"
#include "../objects/particleSystem.h"

// example particle definiton for a basic smoke effect
const ParticleSystemDef g_testParticleSystemDef = ParticleSystemDef::Build
(
	GameTexture_Smoke,				// particle texture
	Color(1.0f, 0.0f, 0.0f, 1.0f),	// color1 start
	Color(0.0f, 0.0f, 1.0f, 1.0f),	// color2 start
	Color(1.0f, 0.0f, 0.0f, 0.0f),	// color1 end
	Color(0.0f, 0.0f, 1.0f, 0.0f),	// color2 end
	0.5f,	0.1f,					// particle life time & percent fade in rate
	2.0f,	5.0f,					// particle start & end size
	10.0f,	3.0f,					// particle start linear & angular speed
	0.01f,	0.1f,					// emit rate & time
	1.0f,	0.5f,					// emit size & overall randomness
	PI,		PI,						// emit & particle cone angles
	0,		0						// particle flags & gravity
);

int ParticleEmitter::totalEmitterCount = 0;
bool ParticleEmitter::enableParticles = true;
int ParticleEmitter::defaultRenderGroup = -2;
int ParticleEmitter::defaultAdditiveRenderGroup = -1;
ConsoleCommand(ParticleEmitter::enableParticles, particleEnable);

float ParticleEmitter::particleStopRadius = 0;
ConsoleCommand(ParticleEmitter::particleStopRadius, particleStopRadius);

float particleDebug = 0;
ConsoleCommand(particleDebug, particleDebug);

DWORD Particle::simpleVertsLastColor = 0;
bool Particle::simpleVertsNeedsCap = false;

float ParticleEmitter::shadowRenderAlpha = 0.6f;
ConsoleCommand(ParticleEmitter::shadowRenderAlpha, particleShadowRenderAlpha);

///////////////////////////////////////////////////////////////////////////////////////////////////////////

ParticleEmitter::ParticleEmitter(const ParticleSystemDef& _systemDef, const XForm2& xf, GameObject* _parent, float scale) :
	GameObject(xf, _parent),
	systemDef(_systemDef),
	emitRateTimer(0),
	setTrailEnd(false),
	emitterPaused(false)
{
	++totalEmitterCount;
	time.Set();
	systemDef.Scale(scale);

	if (systemDef.particleFlags & (PARTICLE_FLAG_TRAIL_LINE|PARTICLE_FLAG_TRAIL_RIBBON) && systemDef.emitRate > 0)
	{
		// create 2 particles so there is something we can draw and it connects to the start position
		const XForm2 xfStart = (systemDef.particleFlags & PARTICLE_FLAG_LOCAL_SPACE)? XForm2::Identity() : GetXFormWorld();
		AddParticle(xfStart);
		AddParticle(xfStart);
		emitRateTimer -= systemDef.emitRate;
	}

	// render additive particles on top of normal particles and both under the terrain 
	SetRenderGroup((systemDef.particleFlags & PARTICLE_FLAG_ADDITIVE)? defaultAdditiveRenderGroup : defaultRenderGroup);
}

ParticleEmitter::~ParticleEmitter()
{
	for (list<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it)
		DeleteParticle(*it);
	--totalEmitterCount;
}

void ParticleEmitter::WarmUp(float time)
{
	while (time >= GAME_TIME_STEP) 
	{
		time -= GAME_TIME_STEP;
		Update();
	}
}

void ParticleEmitter::SpawnParticle()
{
	if (systemDef.particleFlags & PARTICLE_FLAG_LOCAL_SPACE)
		AddParticle(XForm2::Identity());
	else
		AddParticle(GetXFormWorld());
}

void ParticleEmitter::Update()
{
	if (!enableParticles)
	{
		for (list<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it)
			DeleteParticle(*it);
		particles.clear();
		if (IsDead())
		{
			Destroy();
			return;
		}
	}

	// check if we are past our life time
	if (IsDead())
	{
		// self destruct if we are past life time and have no particles
		if (particles.empty())
			Destroy();
	}
	else if (systemDef.emitRate > 0 && !emitterPaused)
	{
		// emit new particles
		emitRateTimer += GAME_TIME_STEP;
		while (emitRateTimer >= 0)
		{
			emitRateTimer -= systemDef.emitRate;

			// update sub step interpolation
			const float interpolation = CapPercent(emitRateTimer / GAME_TIME_STEP);

			if (systemDef.particleFlags & PARTICLE_FLAG_LOCAL_SPACE)
				AddParticle(XForm2::Identity(), interpolation * GAME_TIME_STEP);
			else
				AddParticle(GetInterpolatedXForm(interpolation), interpolation * GAME_TIME_STEP);
		}
	}

	// update particles
	for (list<Particle*>::iterator it = particles.begin(); it != particles.end();) 
	{
		Particle& particle = **it;

		if (particle.IsDead())
		{ 
			bool canRemove = true;
			if (systemDef.particleFlags & (PARTICLE_FLAG_TRAIL_LINE|PARTICLE_FLAG_TRAIL_RIBBON) && it != --particles.end())
			{
				// hack: peek at the next particle
				// we must wait for the next one to die to avoid a glich when we are destroyed
				const Particle& nextParticle = **((++it)--);
				if (!nextParticle.IsDead())
					canRemove = false;
			}

			if (canRemove)
			{
				// remove dead particle
				it = particles.erase(it);
				DeleteParticle(&particle);
				continue;
			}
		}

		particle.Update();
		++it;
	}
	
	if (systemDef.particleFlags & (PARTICLE_FLAG_TRAIL_LINE|PARTICLE_FLAG_TRAIL_RIBBON))
	{
		if ((!IsDead() || setTrailEnd) && particles.size() > 1)
		{
			setTrailEnd = false;
			Particle& particle = *particles.back();
			if (systemDef.particleFlags & PARTICLE_FLAG_LOCAL_SPACE)
			{
				particle.xf.position = Vector2::Zero();
				particle.xfDelta.position = Vector2::Zero();
			}
			else
			{
				particle.xf.position = GetXFormWorld().position;
				particle.xfDelta.position = GetXFormDelta().position;
			}

			if (IsDead() && !(systemDef.particleFlags & PARTICLE_FLAG_LOCAL_SPACE))
			{
				particle.xf.position += g_cameraBase->GetXFormDelta().position;
				particle.xfDelta.position += g_cameraBase->GetXFormDelta().position;
			}
		}
	}
}

// some effects may need this for when there is no parent or the parent dies
void ParticleEmitter::SetTrailEnd(const Vector2& trailEndPos)
{
	DetachParent();
	Kill();
	SetPosLocal(trailEndPos);
	setTrailEnd = true;

	if (particles.size() > 1)
	{
		// set the first particle to be emiter position
		Particle& particle = *particles.back();
		particle.xfDelta.position += trailEndPos - particle.xf.position;
		particle.xf.position = trailEndPos;// - g_cameraBase->GetXFormDelta().position;
	}
	if (particles.size() > 1)
	{
		// set the first particle to be emiter position
		Particle& particle = *particles.back();
		particle.xf.position = trailEndPos;
	}
}
	
void ParticleEmitter::RenderInternal(bool allowAdditive)
{
	FrankProfilerEntryDefine(L"ParticleEmitter::Render()", Color::White(), 6);
	const XForm2 xf =  GetInterpolatedXForm();
	const XForm2 xfParticles = (systemDef.particleFlags & PARTICLE_FLAG_LOCAL_SPACE)? xf : XForm2::Identity();
	//xfParticles.RenderDebug(Color::White(), 1, 1);

	if (systemDef.particleFlags & PARTICLE_FLAG_TRAIL_LINE)
	{
		if (particles.size() > 1)
		{
			// set additive rendering for simple verts
			g_render->SetSimpleVertsAreAdditive(allowAdditive && (systemDef.particleFlags & PARTICLE_FLAG_ADDITIVE) != 0);

			Particle::simpleVertsNeedsCap = true;
			for (list<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it) 
				(**it).RenderTrail(xfParticles);
   
			// connect to the end and cap it off
			const Vector2 endPos = xf.position;
			g_render->AddPointToLineVerts(endPos, Particle::simpleVertsLastColor);
			g_render->CapLineVerts(endPos);
		}
	}
	else if (systemDef.particleFlags & PARTICLE_FLAG_TRAIL_RIBBON)
	{
		if (particles.size() > 1)
		{
			// set additive rendering for simple verts
			g_render->SetSimpleVertsAreAdditive(allowAdditive && (systemDef.particleFlags & PARTICLE_FLAG_ADDITIVE) != 0);

			Vector2 previousPos = (**particles.begin()).GetInterpolatedXForm(xfParticles).position;
			if (particles.size() > 1)
			{
				// if ribbion has more then 1 particle, create a fake previous pos based on where the next particle is
				// this used to orient the ribbon properly
				const Vector2 nextPos = (**(++particles.begin())).GetInterpolatedXForm(xfParticles).position;
				previousPos = previousPos - (nextPos - previousPos);
			}

			Particle::simpleVertsNeedsCap = true;
			for (list<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it) 
				(**it).RenderRibbon(xfParticles, previousPos);
		
			if (IsDead() && systemDef.particleFlags & PARTICLE_FLAG_CAMERA_SPACE)
			{
				// for camera space systems that are no longer spawning particles, cap on the last particle instead of the emitter
				// this fixes a tiny glitch where the end point will see to stick in world space since only particles update their camera space offsets
				g_render->CapTriVerts(previousPos);
			}
			else
			{
				// connect to the end and cap it off
				const Vector2 endPos = xf.position;
				g_render->AddPointToTriVerts(endPos, Particle::simpleVertsLastColor);
				g_render->CapTriVerts(endPos);
				//endPos.RenderDebug();
			}
		}
	}
	else
	{
		const bool additive = (allowAdditive && (systemDef.particleFlags & PARTICLE_FLAG_ADDITIVE) != 0);
		DeferredRender::AdditiveRenderBlock additiveRenderBlock(additive);
		DeferredRender::EmissiveRenderBlock emissiveRenderBlock(additive);

		for (list<Particle*>::iterator it = particles.begin(); it != particles.end(); ++it) 
			(**it).Render(xfParticles);
	}
}

void ParticleEmitter::ManualRender()
{
	if (DeferredRender::GetRenderPassIsShadow())
	{
		// todo: add flag for particles blocking vision
		if (DeferredRender::GetRenderPassIsVision())
			return; 

		if (DeferredRender::GetRenderPassIsLight())
		{
			if (systemDef.particleFlags & PARTICLE_FLAG_CAST_SHADOWS)
			{
				if (shadowRenderAlpha <= 0)
					return;

				RenderInternal(false);
			}
		}
		else
		{
			if (systemDef.particleFlags & PARTICLE_FLAG_CAST_SHADOWS_DIRECTIONAL)
			{
				if (shadowRenderAlpha <= 0)
					return;

				RenderInternal(false);
			}
		}
	}
	else
	{
		if (particleDebug)
		{
			if (!DeferredRender::GetRenderPassIsDeferred())
			{
				g_render->DrawCircle(GetInterpolatedXForm(), systemDef.emitSize);
				g_render->DrawXForm(GetInterpolatedXForm());
			}
		}

		RenderInternal();
	}
}

void ParticleEmitter::Render()
{
	if (systemDef.particleFlags & PARTICLE_FLAG_MANUAL_RENDER)
		return;

	ManualRender();
}

inline void ParticleEmitter::AddParticle(const XForm2& xf, float startTime)
{
	if (!enableParticles)
		return;

	if (particleStopRadius > 0 && g_cameraBase && !g_cameraBase->CameraTest(GetPosWorld(), particleStopRadius))
		return; // off screen

	// random position within the emitter radius
	const Vector2 particlePosition = xf.position + Vector2::BuildRandomInCircle(systemDef.emitSize);

	ASSERT(systemDef.particleSpeedRandomness >= 0 && systemDef.particleSpeedRandomness <= 1);
	ASSERT(systemDef.particleAngularSpeedRandomness >= 0 && systemDef.particleAngularSpeedRandomness <= 1);

	// world space direction is randomized in cone
	const float angle = xf.angle + RAND_BETWEEN(-systemDef.emitConeAngle, systemDef.emitConeAngle);
	const Vector2 direction = Vector2::BuildFromAngle(angle);

	// randomize speed of particle
	const float speed = systemDef.particleSpeed * (1 + systemDef.particleSpeedRandomness*RAND_BETWEEN(-1.0f,1.0f));
	float angularSpeed = systemDef.particleAngularSpeed * (1 + systemDef.particleAngularSpeedRandomness*RAND_BETWEEN(-1.0f,1.0f));
	if (!(systemDef.particleFlags & PARTICLE_FLAG_DONT_FLIP_ANGULAR))
		angularSpeed *= RAND_SIGN;

	Particle *particle = NewParticle();
	if (!particle)
		return;

	particle->Init(systemDef, XForm2(particlePosition, angle), direction*speed, angularSpeed, startTime);
	particles.push_back(particle);
}

///////////////////////////////////////////////////////////
// particle member functions
///////////////////////////////////////////////////////////

void Particle::Init(const ParticleSystemDef& _systemDef, const XForm2& _xf, const Vector2& _velocity, float _angularSpeed, float _startTime)
{
	cachedFrame = 0;
	systemDef = &_systemDef;
	xf = _xf;
	xfDelta = XForm2::Identity();
	velocity = _velocity;
	angularSpeed = _angularSpeed;
	time = _startTime;

	// randomize particle values
	lifeTime = systemDef->particleLifeTime * (1 + systemDef->particleLifeTimeRandomness*RAND_BETWEEN(-1.0f,1.0f));

	// randomly flip the texture for more randomness
	const float randomFlip = (systemDef->particleFlags & (PARTICLE_FLAG_TRAIL_LINE|PARTICLE_FLAG_TRAIL_RIBBON|PARTICLE_FLAG_DONT_FLIP))? 1 : (float)RAND_SIGN;
	sizeStart = randomFlip * systemDef->particleSizeStart * (1 + systemDef->particleSizeStartRandomness*RAND_BETWEEN(-1.0f,1.0f));
	sizeEnd = randomFlip * systemDef->particleSizeEnd * (1 + systemDef->particleSizeEndRandomness*RAND_BETWEEN(-1.0f,1.0f));
	xf.angle += RAND_BETWEEN(-systemDef->particleConeAngle, systemDef->particleConeAngle);

	colorStart = Color::RandBetween(systemDef->colorStart1, systemDef->colorStart2);
	const Color colorEnd = Color::RandBetween(systemDef->colorEnd1, systemDef->colorEnd2);
	colorDelta = (colorEnd - colorStart);
}

inline void Particle::Update()
{
	// update time
	time += GAME_TIME_STEP;

	const XForm2 xfLast = xf;

	// update gravity
	if (systemDef->particleGravity)
		velocity += GAME_TIME_STEP * systemDef->particleGravity * FrankUtil::CalculateGravity(xf.position); 

	// update transform
	xf.position += velocity * GAME_TIME_STEP;
	xf.angle += angularSpeed * GAME_TIME_STEP;

	// camera space particles move along with the camera
	if (systemDef->particleFlags & PARTICLE_FLAG_CAMERA_SPACE)
		xf *= g_cameraBase->GetXFormDelta();
	
	xfDelta = xf - xfLast;
}

inline void Particle::Render(const XForm2& xfParent)
{
	if (cachedFrame != g_gameControlBase->GetRenderFrameCount())
	{
		// cache data for faster rendering
		cachedFrame = g_gameControlBase->GetRenderFrameCount();

		// caluculate percent of particle life time
		const float percent = CapPercent((lifeTime == 0) ? 0 : (time - g_interpolatePercent*GAME_TIME_STEP)/lifeTime);

		// cache the particle info for this render frame
		const XForm2 xfWorld = GetInterpolatedXForm(xfParent);
		cachedSize = sizeStart + percent * (sizeEnd - sizeStart);

		cachedColor = colorStart + percent * colorDelta;
		if (percent < systemDef->particleFadeInTime)
			cachedColor.a *= (percent / systemDef->particleFadeInTime);

		cachedPos1 = xfWorld.position;
		cachedAngle = xfWorld.angle;
	}

	if (!g_cameraBase->CameraTest(cachedPos1, fabs(cachedSize) * ROOT_2))
		return;
	
	Color color = cachedColor;
	if (DeferredRender::GetRenderPassIsShadow())
		color.a *= ParticleEmitter::shadowRenderAlpha;
	
	const XForm2 xfWorld(cachedPos1, cachedAngle);
	g_render->RenderQuad(xfWorld, Vector2(cachedSize), color, systemDef->texture);
}

inline void Particle::RenderTrail(const XForm2& xfParent)
{
	if (cachedFrame != g_gameControlBase->GetRenderFrameCount())
	{
		// cache data for faster rendering
		cachedFrame = g_gameControlBase->GetRenderFrameCount();

		// caluculate percent of particle life time
		const float percent = CapPercent((lifeTime == 0) ? 0 : (time - g_interpolatePercent*GAME_TIME_STEP)/lifeTime);

		cachedColor = colorStart + percent * colorDelta;
		if (percent < systemDef->particleFadeInTime)
			cachedColor.a *= (percent / systemDef->particleFadeInTime);

		cachedPos1 = GetInterpolatedXForm(xfParent).position;
	}
	
	Color color = cachedColor;
	if (DeferredRender::GetRenderPassIsShadow())
		color.a *= ParticleEmitter::shadowRenderAlpha;

	if (Particle::simpleVertsNeedsCap)
	{
		g_render->CapLineVerts(cachedPos1);
		Particle::simpleVertsNeedsCap = false;
	}
	
	const DWORD colorDword = color;
	g_render->AddPointToLineVerts(cachedPos1, colorDword);
	simpleVertsLastColor = colorDword;
}

inline void Particle::RenderRibbon(const XForm2& xfParent, Vector2& previousPos)
{
	if (cachedFrame != g_gameControlBase->GetRenderFrameCount())
	{
		// cache data for faster rendering
		cachedFrame = g_gameControlBase->GetRenderFrameCount();

		// caluculate percent of particle life time
 		const float percent = CapPercent((lifeTime == 0) ? 0 : (time - g_interpolatePercent*GAME_TIME_STEP)/lifeTime);

		cachedColor = colorStart + percent * colorDelta;
		if (percent < systemDef->particleFadeInTime)
			cachedColor.a *= (percent / systemDef->particleFadeInTime);

		const XForm2 xfWorld = GetInterpolatedXForm(xfParent);
		const float size = sizeStart + percent * (sizeEnd - sizeStart);

		const Vector2 offset = size*(xfWorld.position - previousPos).Normalize().RotateRightAngle();

		cachedPos1 = xfWorld.position - offset;
		cachedPos2 = xfWorld.position + offset;

		// pass out the position of the particle
		previousPos = xfWorld.position;

		//cachedPos1.RenderDebug();
	}
	
	Color color = cachedColor;
	if (DeferredRender::GetRenderPassIsShadow())
		color.a *= ParticleEmitter::shadowRenderAlpha;

	if (Particle::simpleVertsNeedsCap)
	{
		g_render->CapTriVerts(cachedPos1);
		Particle::simpleVertsNeedsCap = false;
	}

	const DWORD colorDword = color;
	g_render->AddPointToTriVerts(cachedPos1, colorDword);
	g_render->AddPointToTriVerts(cachedPos2, colorDword);
	simpleVertsLastColor = colorDword;
}

///////////////////////////////////////////////////////////
// particle memory management
///////////////////////////////////////////////////////////

// global particle pools
list<Particle*> ParticleEmitter::freeParticleList;
Particle ParticleEmitter::particlePool[maxParticles];

void ParticleEmitter::InitParticleSystem()
{
	// prep the free particle list
	for(int i = 0; i < maxParticles; ++i)
		freeParticleList.push_back(&particlePool[i]);
}

Particle* ParticleEmitter::NewParticle()
{
	if (freeParticleList.empty())
		return NULL; // ran out of particles

	list<Particle*>::iterator it = freeParticleList.begin();
	Particle* particle = *it;
	freeParticleList.erase(it);
	return particle;
}

void ParticleEmitter::DeleteParticle(Particle* particle)
{
	freeParticleList.push_back(particle);
}

////////////////////////////////////////////////////////////////////////////////////////

void ParticleSystemDef::Scale(float scale)
{
	particleSizeStart *= scale;
	particleSizeEnd *= scale;
	particleSpeed *= scale;
	emitSize *= scale;
}