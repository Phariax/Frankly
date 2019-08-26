////////////////////////////////////////////////////////////////////////////////////////
/*
	2D Dynamic Lighting System
	Copyright 2013 Frank Force - http://www.frankforce.com

*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../rendering/frankRender.h"
#include "light.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Light
*/
////////////////////////////////////////////////////////////////////////////////////////

Light::Light(const GameObjectStub& stub) :
	GameObject(stub)
{
	// set defaults
	radius = DeferredRender::defaultLightRadius;
	color = Color::White();
	overbrightRadius = 0;
	haloRadius = 0;
	coneAngle = 0;
	coneFadeAngle = 0;
	coneFadeColor = 0;
	gelTexture = GameTexture_Invalid;
	castShadows = true;
	fadeOut = fadeIn  = false;
	isPersistant = false;
	wasCreatedFromStub = true;
	int renderGroup = 10000;	// render halos on top of everything
	height = DeferredRender::shadowLightHeightDefault;

	// radius, red, green, blue, alpha, overbrightRadius, haloRadius, coneAngle, coneFadeAngle, gelTexture, castShadows, renderGroup, height
	sscanf_s(stub.attributes, "%f %f %f %f %f %f %f %f %f %d %d %d %f", 
		&radius,
		&color.r, &color.g, &color.b, &color.a,
		&overbrightRadius,
		&haloRadius,
		&coneAngle,
		&coneFadeAngle,
		&gelTexture,
		&castShadows,
		&renderGroup,
		&height
	);

	// protect against bad values
	SetRenderGroup(renderGroup);
	radius = Max(radius, 0.0f);
	color = color.CapValues();
	overbrightRadius = Max(overbrightRadius, 0.0f);
	haloRadius = Max(haloRadius, 0.0f);
	coneAngle = Cap(coneAngle, 0.0f, 2*PI);
	coneFadeAngle = Cap(coneFadeAngle, 0.0f, 2*PI);
	if (coneAngle == 0 && coneFadeAngle == 0)
		coneAngle = 2*PI; // no cone
	gelTexture = Max(gelTexture, GameTexture_Invalid);

	//if (gelTexture)
	//	ASSERT(g_render->GetTexture(gelTexture));	// gel texture not loaded
}
	
Light::Light(const XForm2& xf, GameObject* _parent, float _radius, const Color& _color, bool _castShadows) :
	GameObject(xf, _parent),
	radius(_radius),
	color(_color),
	castShadows(_castShadows)
{
	SetRenderGroup(10000);		// render halos on top of everything
	overbrightRadius = 0;
	haloRadius = 0;
	coneAngle = 2*PI;
	coneFadeAngle = 0;
	coneFadeColor = 0;
	fadeOut = fadeIn = false;
	isPersistant = false;
	wasCreatedFromStub = false;
	gelTexture = GameTexture_Invalid;
	height = DeferredRender::shadowLightHeightDefault;
}

void Light::Update()
{
	if (fadeTimer.HasElapsed())
	{
		fadeTimer.Invalidate();

		if (fadeOut || !fadeIn)
		{
			Kill();
			return;
		}
	}
}

void Light::Render()
{
	const XForm2 xf = GetInterpolatedXForm();
	
	if (IsDestroyed())
		return;
	
	if (DeferredRender::GetRenderPass())
	{
		if (DeferredRender::lightEnable && haloRadius > 0 && DeferredRender::GetRenderPassIsEmissive())
		{
			// render a light halo texture
			DeferredRender::EmissiveRenderBlock emissiveRenderBlock;
			DeferredRender::AdditiveRenderBlock additiveRenderBlock;
			Color haloColor = color;
			haloColor.a *= GetFadeAlpha() * DeferredRender::haloAlpha;
			g_render->RenderQuad(xf.position, Vector2(haloRadius), haloColor, GameTexture_Dot);
		}

		return;
	}

	if (DeferredRender::lightDebug && !DeferredRender::renderPass && radius > 0 && wasRendered)
	{
		Circle(xf.position, radius).RenderDebug(color);
		Circle(xf.position, overbrightRadius).RenderDebug(color);
	
		if (coneAngle < 2*PI)
		{
			Line2(xf.position, xf.position + radius * xf.GetUp()).RenderDebug(color);
			Line2(xf.position, xf.position + radius * Vector2::BuildFromAngle(xf.angle - coneAngle)).RenderDebug(color);
			Line2(xf.position, xf.position + radius * Vector2::BuildFromAngle(xf.angle + coneAngle)).RenderDebug(color);

			if (coneFadeAngle > 0)
			{
				Line2(xf.position, xf.position + radius * Vector2::BuildFromAngle(xf.angle - coneAngle - coneFadeAngle)).RenderDebug(color);
				Line2(xf.position, xf.position + radius * Vector2::BuildFromAngle(xf.angle + coneAngle + coneFadeAngle)).RenderDebug(color);
			}
		}
	}
}

bool Light::IsSimpleLight() const
{
	// dynamic lights need special rendering for shadows or cone
	return !(castShadows || (coneAngle < 2*PI));
}

float Light::GetFadeAlpha() const
{
	if (!fadeTimer.IsValid())
		return 1;

	// scale alpha if light is fading
	const float p = fadeTimer;
	if (fadeIn)
	{
		if (fadeOut)
		{
			// fading in then out again
			return sinf(p*PI);
		}
		else
			return p;
	}
	else if (fadeOut)
		return 1 - p;

	// light can be timing out but not fading
	return 1;
}

void Light::Kill()
{
	if (wasCreatedFromStub)
		g_terrain->RemoveStub(GetHandle(), g_terrain->GetPatch(GetPosWorld()));
	Destroy();
}

void Light::StubRender(const GameObjectStub& stub, float alpha)
{
	GameObject::StubRender(stub, alpha);

	if (!g_gameControlBase->IsObjectEditMode() || stub.type != g_editor.GetObjectEditor().GetNewStubType())
	{
		// only show light preview when a light is selected
		return;
	}

	// show a preview of the light radius and colors when editing
	float radius = DeferredRender::defaultLightRadius;
	Color color = Color::White();
	float overbright = 0;
	float halo = 0;
	float coneAngle = 0;
	float coneFadeAngle = 0;
	sscanf_s(stub.attributes, "%f %f %f %f %f %f %f %f %f", 
		&radius, &color.r, &color.g, &color.b, &color.a, &overbright, &halo, &coneAngle, &coneFadeAngle
	);
	color.a *= alpha;
	color = color.CapValues();

	coneAngle = Cap(coneAngle, 0.0f, 2*PI);
	coneFadeAngle = Cap(coneFadeAngle, 0.0f, 2*PI);

	if (coneAngle > 0 || coneFadeAngle > 0)
	{
		if (coneAngle > 0)
			g_render->DrawCone(stub.xf, radius, coneAngle, color);
		if (coneFadeAngle > 0)
			g_render->DrawCone(stub.xf, radius, coneAngle + coneFadeAngle, color.ScaleAlpha(0.5f));
	}
	else
		g_render->DrawCircle(stub.xf, radius, color);

	g_render->DrawCircle(stub.xf, Vector2(overbright), color);
}

WCHAR* Light::StubDescription()
{
	return L"Dynamic game light";
}

WCHAR* Light::StubAttributesDescription()
{
	// display attributes format
	return L"radius r g b a overbright halo cone coneFade gel shadows renderGroup height";
}
