////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Objects
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "gameGlobals.h"
#include "gameObjects.h"
#include "weapons.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	Light
*/
////////////////////////////////////////////////////////////////////////////////////////

// expose game lights to the editor
GAME_OBJECT_DEFINITION(Light, GameTexture_Arrow, Color::White(0.5f));

////////////////////////////////////////////////////////////////////////////////////////
/*
	MusicBox
*/
////////////////////////////////////////////////////////////////////////////////////////

GAME_OBJECT_DEFINITION(MusicBox, GameTexture_Invalid, Color::Magenta(0.2f));

WCHAR* MusicBox::StubDescription() { return L"Plays ogg music file when box is touched by player."; }
WCHAR* MusicBox::StubAttributesDescription() { return L"filename"; }

MusicBox::MusicBox(const GameObjectStub& stub) :
	GameObject(stub)
{
	// convert attribites to a printable version swapping out escape characters
	stub.GetAttributesPrintable(filename);

	// create the physics
	CreatePhysicsBody(stub.xf, b2_staticBody);
	b2PolygonShape shapeDef;
	shapeDef.SetAsBox(stub.size.x, stub.size.y);
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shapeDef;
	fixtureDef.isSensor = true;
	AddFixture(fixtureDef);
}

void MusicBox::CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture)
{
	if (!otherObject.IsPlayer())
		return;

	if (!g_player || g_player->IsDead())
		return;
	
	string line(filename);
	std::wstring wline;
	wline.assign(line.begin(), line.end());
	g_sound->GetMusicPlayer().Transition(wline.c_str());
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	TextDecal
*/
////////////////////////////////////////////////////////////////////////////////////////

GAME_OBJECT_DEFINITION(TextDecal, GameTexture_Font1, Color::White());

TextDecal::TextDecal(const GameObjectStub& stub) :
	GameObject(stub)
{
	// make it appear below stuff
	SetRenderGroup(RenderGroup_low);

	// convert attribites to a printable version swapping out escape characters
	char stubText[GameObjectStub::attributesLength];
	stub.GetAttributesPrintable(stubText);
	
	size = stub.size;
	color = Color::White();
	type = DisplayType_default;

	FrankUtil::AttributesStringParser parser(stubText);
	parser.parseValue(color.r);
	parser.parseValue(color.g);
	parser.parseValue(color.b);
	parser.parseValue(color.a);
	parser.parseValue((unsigned&)type);
	*text = 0;
	if (parser.skipToMarker('#'))
		strncpy_s(text, GameObjectStub::attributesLength, parser.getString(), GameObjectStub::attributesLength);

	if (type == DisplayType_castShadows)
	{
		// make it appear above stuff
		SetRenderGroup(RenderGroup_superHigh);
	}
}

void TextDecal::RenderPost()
{
	// render text overlay
	if (type != DisplayType_overlay)
		return;
	
	const XForm2 xf = GetInterpolatedXForm();
	FontFlags flags = FontFlags(FontFlag_CenterX | FontFlag_CenterY);
	g_gameFont->Render(text, xf, 2*size.y, color, flags);
}

void TextDecal::Render()
{
	const XForm2 xf = GetInterpolatedXForm();
	FontFlags flags = FontFlags(FontFlag_CenterX | FontFlag_CenterY);

	switch(type)
	{
		default: break;
		case DisplayType_noNormals:
		{
			// dont draw normals
			flags = FontFlags(flags | FontFlag_NoNormals);
			break;
		}
		case DisplayType_emissive:
		{
			// emissive
			DeferredRender::EmissiveRenderBlock emissiveBlock;
			g_gameFont->Render(text, xf, 2*size.y, color, flags);
			return;
		}
		case DisplayType_overlay:
		{
			// renderd in RenderPost
			return;
		}
		case DisplayType_castShadows:
		{
			// shadows
			flags = FontFlags(flags | FontFlag_CastShadows);
		}
	}
	
	//g_render->RenderQuad(xf, g_gameFont->GetBBox(text, xf, 2*size.y, flags), Color::Grey(0.8f));

	g_gameFont->Render(text, xf, 2*size.y, color, flags);
}

void TextDecal::StubRender(const GameObjectStub& stub, float alpha)
{
	char stubText[GameObjectStub::attributesLength];
	stub.GetAttributesPrintable(stubText);
	
	const XForm2 xf = stub.xf;
	Vector2 size = stub.size;
	Color color = Color::White();
	int type = 0;

	FrankUtil::AttributesStringParser parser(stubText);
	parser.parseValue(color.r);
	parser.parseValue(color.g);
	parser.parseValue(color.b);
	parser.parseValue(color.a);
	parser.parseValue((unsigned&)type);
	const char* text = "";
	if (parser.skipToMarker('#'))
		text = parser.getString();

	color.a *= alpha;
	g_gameFont->Render(text, xf, 2*size.y, color, FontFlags(FontFlag_CenterX | FontFlag_CenterY));

}

////////////////////////////////////////////////////////////////////////////////////////
/*
	PopupMessage
*/
////////////////////////////////////////////////////////////////////////////////////////

PopupMessage::PopupMessage(const XForm2& xf, GameObject* _parent, char* _text, float _size, float _time, const Color& _color, bool _autoDestruct) :
	GameObject(xf, _parent),
	size(_size),
	color(_color),
	messageFadeTime(_time),
	autoDestruct(_autoDestruct)
{
	// make it appear above stuff
	SetRenderGroup(RenderGroup_superHigh);

	strncpy_s(text, _text, sizeof(text));
	messageTimer.Set();
}
	
void PopupMessage::SetMessage(const XForm2& xf, const char* _text, float _size, float time, const Color& _color)
{
	SetXFormWorld(xf);
	ResetLastWorldTransforms();
	strncpy_s(text, _text, sizeof(text));
	messageTimer.Set();
	messageFadeTime = time;
	size = _size;
	color = _color;
}

void PopupMessage::Udapte()
{
	if (autoDestruct && messageTimer.IsValid() && messageTimer > messageFadeTime)
		Destroy();
}

void PopupMessage::RenderPost()
{
	if (IsDestroyed())
		return;

	Color renderColor = color;
	renderColor.a *= messageTimer.IsValid()? Percent(float(messageTimer), messageFadeTime, messageFadeTime*0.75f) : 0;

	if (renderColor.a <= 0)
		return;

	const XForm2 xf = GetInterpolatedXForm();
	g_gameFont->Render(text, xf, 2*size, renderColor, FontFlags(FontFlag_CenterX | FontFlag_CenterX | FontFlag_CenterY));
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	HelpBlock
*/
////////////////////////////////////////////////////////////////////////////////////////

GAME_OBJECT_DEFINITION(HelpSwitch, GameTexture_Circle, Color::Green());

HelpSwitch::HelpSwitch(const GameObjectStub& stub) :
	GameObject(stub)
{
	// make it appear below stuff
	SetRenderGroup(RenderGroup_low);

	// convert attribites to a printable version swapping out escape characters
	stub.GetAttributesPrintable(text);

	// make it square
	size = Vector2(stub.size.x);
	
	// hook up physics
	CreatePhysicsBody(GetXFormWorld(), b2_staticBody);
	b2CircleShape shapeDef;
	shapeDef.m_radius = size.x;
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shapeDef;
	fixtureDef.density =Physics::defaultDensity;
	fixtureDef.friction = Physics::defaultFriction;
	fixtureDef.restitution = Physics::defaultRestitution;
	AddFixture(fixtureDef);
}

void HelpSwitch::Update()
{
}

void HelpSwitch::Render()
{
	const XForm2 xf = GetInterpolatedXForm();
	
	// lerp the color when it is hit
	const float p = hitTimer.IsValid()? Percent((float)hitTimer, 0.0f, 3.0f) : 1;
	Color color = Lerp(p, Color::Yellow(), Color::Green());
	
	// make it be emissive
	DeferredRender::EmissiveRenderBlock emissiveRenderBlock;
	DeferredRender::SpecularRenderBlock specularRenderBlock;
	if (DeferredRender::GetRenderPassIsEmissive())
	{
		// make it more emissive when it is hit
		color.a = Lerp(p, 1.0f, 0.2f);
	}
	g_render->RenderQuad(xf, size, color, GameTexture_Circle);
}

void HelpSwitch::CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture)
{
	if (otherObject.GetTeam() != GameTeam_player || !otherObject.IsProjectile())
		return;

	if (hitTimer.IsValid() && hitTimer < 1)
		return;

	if (forceActivateTimer.IsValid() && forceActivateTimer < 3)
		return;

	hitTimer.Set();

	Vector2 messagePos = GetPosWorld();
	g_popupMessage->SetMessage(messagePos, text, 0.5f, 5, Color::White());
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	ObjectSpawner
*/
////////////////////////////////////////////////////////////////////////////////////////

GAME_OBJECT_DEFINITION(ObjectSpawner, GameTexture_Arrow, Color::Purple());

ObjectSpawner::ObjectSpawner(const GameObjectStub& stub) :
	GameObject(stub)
{
	// make it appear below stuff
	SetRenderGroup(RenderGroup_low);

	// set size from stub
	size = stub.size;
	
	spawnType = GOT_Invalid;;
	spawnSize = 0.5f;
	spawnOffset = 0;
	spawnSpeed = 0;
	spawnMax = 1;
	spawnRandomness = 0;
	spawnRate = 0.2f;
	spawnAuto = false;

	// spawnType, spawnSize, spawnOffset, spawnSpeed, spawnMax, spawnRandomness, spawnRate, spawnAuto
	sscanf_s(stub.attributes, "%d %f %f %f %d %f %f %d",
		&spawnType,
		&spawnSize,
		&spawnOffset,
		&spawnSpeed,
		&spawnMax,
		&spawnRandomness,
		&spawnRate,
		&spawnAuto
	);

	// protect against bad input
	if (spawnSize < 0)
		spawnType = GOT_Invalid;
	
	// hook up physics
	CreatePhysicsBody(GetXFormWorld(), b2_staticBody);
	b2CircleShape shapeDef;
	shapeDef.m_radius = size.x;
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shapeDef;
	fixtureDef.density =Physics::defaultDensity;
	fixtureDef.friction = Physics::defaultFriction;
	fixtureDef.restitution = Physics::defaultRestitution;
	AddFixture(fixtureDef);
}

void ObjectSpawner::Update()
{
	if (spawnAuto)
	{
		if (!spawnTimer.IsValid())
			spawnTimer.Set(spawnRate);
		else if (spawnTimer.HasElapsed())
		{
			if (SpawnObject())
				spawnTimer.Set(spawnRate);
		}
	}
}

void ObjectSpawner::Render()
{
	const XForm2 xf = GetInterpolatedXForm();
	
	// lerp the color when it is hit
	const float p = spawnTimer.IsValid()? (float)spawnTimer : 1;
	Color color = Lerp(p, Color::White(), spawnAuto? Color::Red() : Color::Magenta());
	
	// make it be emissive
	DeferredRender::EmissiveRenderBlock emissiveRenderBlock;
	DeferredRender::SpecularRenderBlock specularRenderBlock;
	if (DeferredRender::GetRenderPassIsEmissive())
	{
		// make it more emissive when it is hit
		color.a = Lerp(p, 0.8f, 0.2f);
	}

	g_render->RenderQuad(xf, size, color, GameTexture_Circle);
}

void ObjectSpawner::CollisionAdd(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture)
{
	if (otherObject.GetTeam() != GameTeam_player || !otherObject.IsProjectile())
		return;

	if (forceActivateTimer.IsValid() && forceActivateTimer < 3)
		return;

	spawnAuto = !spawnAuto;
	if (spawnAuto)
	{
		g_popupMessage->SetMessage(GetPosWorld(), "Spawner enabled.", 0.5f, 5, Color::White());
	}
	else
	{
		g_popupMessage->SetMessage(GetPosWorld(), "Spawner disabled.", 0.5f, 5, Color::White());
		spawnTimer.Invalidate();
	}
}

GameObject* ObjectSpawner::SpawnObject()
{
	if (spawnType == GOT_Invalid)
		return NULL;

	// remove any objects from our list that are gone
	for (list<GameObjectHandle>::iterator it = spawnedObjects.begin(); it != spawnedObjects.end(); ) 
	{
		GameObject* gameObject = g_objectManager.GetObjectFromHandle(*it);
		if (!gameObject)
			it = spawnedObjects.erase(it);
		else
			++it;
	}

	if (spawnedObjects.size() >= spawnMax)
	{
		// preventw spawning more then the max
		return NULL;
	}
	
	char attributes[GameObjectStub::attributesLength] = "";
	float size = spawnSize * RAND_BETWEEN(1 - spawnRandomness, 1 + spawnRandomness);
	GameObject* object = GameObjectStub(XForm2(Vector2(0, spawnOffset)) * GetXFormWorld(), Vector2(size), spawnType, attributes).BuildObject();
	if (!object)
		return NULL;

	// keep track of our spawned objects by handle
	spawnedObjects.push_back(object->GetHandle());

	b2Body* body = object->GetPhysicsBody();
	if (body)
	{
		float speed = spawnSpeed * RAND_BETWEEN(1 - spawnRandomness, 1 + spawnRandomness);
		body->SetLinearVelocity(speed * GetUpWorld() + GetVelocity());
	}
	return object;
}

void ObjectSpawner::StubRender(const GameObjectStub& stub, float alpha)
{
	GameObject::StubRender(stub, alpha);

	GameObjectType spawnType = GOT_Invalid;;
	float spawnSize = 0.5f;
	float spawnOffset = 0;
	sscanf_s(stub.attributes, "%d %f %f",
		&spawnType,
		&spawnSize,
		&spawnOffset
	);

	if (!GameObjectStub::HasObjectInfo(spawnType))
		return;
	
	// show a preview of where the spawned objects will be
	const float alphaScale = alpha*0.5f;
	const GameObjectStub spawnStub(XForm2(Vector2(0, spawnOffset)) * stub.xf, Vector2(spawnSize), spawnType);
	spawnStub.Render(alphaScale);
	g_render->DrawOutlinedBox(spawnStub.xf, spawnStub.size, Color::White(0.1f), Color::Black(alphaScale));
	g_render->DrawSegment(Line2(stub.xf.position, spawnStub.xf.position), Color::White(alphaScale));
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Crate
*/
////////////////////////////////////////////////////////////////////////////////////////

GAME_OBJECT_DEFINITION(Crate, GameTexture_Invalid, Color::Grey());

Crate::Crate(const GameObjectStub& stub) :
	Actor(stub),
	size(stub.size)
{
	// create the physics
	CreatePhysicsBody(stub.xf);
	b2PolygonShape shapeDef;
	shapeDef.SetAsBox(stub.size.x, stub.size.y);
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shapeDef;
	fixtureDef.density = Physics::defaultDensity;
	fixtureDef.friction = Physics::defaultFriction;
	fixtureDef.restitution = Physics::defaultRestitution;
	AddFixture(fixtureDef);

	// set health based on size of object
	const float health = 50*size.x*size.y;
	ResetHealth(health);
	
	// scale effects based on size
	effectSize = Min(size.x, size.y);
}

void Crate::Update()
{
	if (explosionTimer.HasElapsed())
		Explode();
}

void Crate::Render()
{
	XForm2 xf = GetInterpolatedXForm();
	g_render->RenderQuad(xf, size, Color::Grey(1, 0.7f), GameTexture_Crate);

	DeferredRender::EmissiveRenderBlock emissiveRenderBlock;
	DeferredRender::SpecularRenderBlock specularRenderBlock;
	const float healthPercent = GetHealthPercent();
	Color color = Lerp(healthPercent, Color::Cyan(), Color(0.8f, 1, 1));
	if (explosionTimer.IsValid())
		color = Lerp((float)explosionTimer, Color::Cyan(), Color::White());

	if (DeferredRender::GetRenderPassIsEmissive())
	{
		// make it more emissive when it is hit
		color.a = Lerp(healthPercent, 0.6f, 0.2f);
		if (explosionTimer.IsValid())
			color.a = Lerp((float)explosionTimer, 0.6f, 1.0f);
	}
	g_render->RenderQuad(xf, size*0.65f, color, GameTexture_Circle);
}

void Crate::Kill()
{
	if (!explosionTimer.IsValid())
	{
		// set an explosion timer instead of being immediatly destroyed
		explosionTimer.Set(RAND_BETWEEN(0.3f, 0.6f));
	}
}

void Crate::CollisionResult(GameObject& otherObject, const ContactResult& contactResult, b2Fixture* myFixture, b2Fixture* otherFixture)
{
	// check for collision damage
	const float impulse = contactResult.normalImpulse + contactResult.tangentImpulse;
	float damage = 0.7f*impulse / GetMass();
	float minDamage = 0.2f*GetMaxHealth();
	if (damage > minDamage)
		ApplyDamage(damage - minDamage);

	Actor::CollisionResult(otherObject, contactResult, myFixture, otherFixture);
}

void Crate::Explode()
{
	if (IsDestroyed())
		return;

	{
		// do an explosion
		new Explosion(GetXFormWorld(), effectSize, 6*effectSize, 0.5f*GetMaxHealth(), this);
	}
	{
		// create some debris
		Debris::CreateDebris(*this, (int)(15*Min(size.x, size.y)));
	}
	{
		// kick off a light flash
		Light* light = new Light(GetPosWorld(), NULL, effectSize*20, Color(0.0f, 1.0f, 1.0f, 0.9f), true);
		light->SetOverbrightRadius(effectSize*1.5f);
		light->SetFadeTimer(0.6f, true, true);
	}
	{
		// kick off smoke
		ParticleSystemDef smokeEffectDef = ParticleSystemDef::Build
		(
			GameTexture_Smoke,			// particle texture
			Color::Grey(0.5f, 0.8f),	// color1 start
			Color::Grey(0.5f, 0.4f),	// color2 start
			Color::Grey(0, 0.8f),		// color1 end
			Color::Grey(0, 0.4f),		// color2 end
			3.0f,	0.2f,				// particle life time & percent fade in rate
			3, 5,						// particle start & end size
			.1f,		0.5f,			// particle start linear & angular speed
			0.08f,	0.6f,				// emit rate & time
			.5f,	0.5f,				// emit size & overall randomness
			PI, PI, PARTICLE_FLAG_CAST_SHADOWS, -0.04f			// cone angles, flags and gravity
		);
		smokeEffectDef.particleSpeedRandomness = 1.0f;
		ParticleEmitter* emitter = new ParticleEmitter(smokeEffectDef, GetXFormWorld(), NULL, effectSize);
		emitter->SetRenderGroup(RenderGroup_foregroundEffect);
	}
	{
		// kick off explosion effects
		ParticleSystemDef explosionEffectDef = ParticleSystemDef::Build
		(
			GameTexture_Smoke,			// particle texture
			Color(0.2f, 1.0f, 1, 0.5f),	// color1 start
			Color(0.2f, 0.2f, 1, 0.5f),	// color2 start
			Color(0.2f, 1.0f, 1, 0),	// color1 end
			Color(0.2f, 0.2f, 1, 0),	// color2 end
			0.6f,		0.1f,			// particle life time & percent fade in rate
			2, 2,						// particle start & end size
			.4f,1,						// particle start linear & angular speed
			0.03f,	0.2f,				// emit rate & time
			.4f,	0.5f,				// emit size & overall randomness
			PI, PI,
			PARTICLE_FLAG_ADDITIVE
		);
		ParticleEmitter* emitter = new ParticleEmitter(explosionEffectDef, GetXFormWorld(), NULL, effectSize);
		emitter->SetRenderGroup(RenderGroup_foregroundAdditiveEffect);
	}
	{
		// play the test sound with a frequency scale based on volume
		const float frequencyPercent = Percent(size.x*size.y, 10.0f, 70.0f);
		const float frequency = Lerp(frequencyPercent, 0.7f, 0.4f);
		g_sound->Play(SoundControl_test, *this, 1, frequency);
	}

	Actor::Destroy();
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	TriggerBox
*/
////////////////////////////////////////////////////////////////////////////////////////

GAME_OBJECT_DEFINITION(TriggerBox, GameTexture_Invalid, Color::Green(0.2f));

float TriggerBox::highTime = 0;

TriggerBox::TriggerBox(const GameObjectStub& stub) :
	GameObject(stub)
{
	// create the physics
	CreatePhysicsBody(stub.xf, b2_staticBody);
	b2PolygonShape shapeDef;
	shapeDef.SetAsBox(stub.size.x, stub.size.y);
	b2FixtureDef fixtureDef;
	fixtureDef.shape = &shapeDef;
	fixtureDef.isSensor = true;
	AddFixture(fixtureDef);

	// make it appear below stuff
	SetRenderGroup(RenderGroup_low);
}
	
void TriggerBox::Update()
{
	if (moshPitTimer.IsValid())
	{
		if (touchedPlayerTimer > 0 || g_player->IsDead())
		{
			// force off all spawners when done moshing
			for (GameObjectHashTable::const_iterator it = g_objectManager.GetObjects().begin(); it != g_objectManager.GetObjects().end(); ++it)
			{
				GameObject* obj = (*it).second;

				if (dynamic_cast<ObjectSpawner*>(obj))
					static_cast<ObjectSpawner*>(obj)->ForceActivate(false);
			}
			moshPitTimer.Invalidate();
			g_sound->Play(SoundControl_test, *g_player, 1, 2.0f);
		}
		else
		{
			// force all spawners on and help switches off when moshing
			for (GameObjectHashTable::const_iterator it = g_objectManager.GetObjects().begin(); it != g_objectManager.GetObjects().end(); ++it)
			{
				GameObject* obj = (*it).second;

				if (dynamic_cast<HelpSwitch*>(obj))
					static_cast<HelpSwitch*>(obj)->ForceActivate(false);
				else if (dynamic_cast<ObjectSpawner*>(obj))
					static_cast<ObjectSpawner*>(obj)->ForceActivate(true);
			}
		}
	}
}

void TriggerBox::Render()
{
	WCHAR buffer[256];
	if (moshPitTimer.IsValid() && touchedPlayerTimer <= 0.1f && !g_player->IsDead())
	{
		const float time = moshPitTimer;
		if (time > highTime)
		{
			highTime = time;
			swprintf_s(buffer, 256, L"Mosh Pit Time\n%.2f\nNew High!", time);
		}
		else
			swprintf_s(buffer, 256, L"Mosh Pit Time\n%.2f\nHigh Time\n%0.2f", time, highTime);
	}
	else
	{
		swprintf_s(buffer, 256, L"Mosh Pit Time\n0\nHigh Time\n%0.2f", highTime);
	}
	
	DeferredRender::EmissiveRenderBlock erb;
	FontFlags flags = FontFlags(FontFlag_CenterX | FontFlag_CenterY | FontFlag_NoNormals);
	g_gameFont->Render(buffer, GetPosWorld(), 2.2f, Color::White(), flags);
}

void TriggerBox::CollisionPersist(GameObject& otherObject, const ContactEvent& contactEvent, b2Fixture* myFixture, b2Fixture* otherFixture)
{
	if (g_player->IsDead())
		return;

	touchedPlayerTimer.Set();
	if (!moshPitTimer.IsValid())
		moshPitTimer.Set();
};