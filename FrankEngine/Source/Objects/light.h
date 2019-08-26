////////////////////////////////////////////////////////////////////////////////////////
/*
	2D Dynamic Lighting System
	Copyright 2013 - Frank Force
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef LIGHT_H
#define LIGHT_H

class Light : public GameObject
{
public:
	
	Light(const GameObjectStub& stub);
	Light(const XForm2& xf, GameObject* _parent, float radius, const Color& color, bool castShadows = true);

	void SetRadius(float _radius)					{ radius = _radius; }
	void SetHaloRadius(float _radius)				{ haloRadius = _radius; }
	void SetOverbrightRadius(float _radius)			{ overbrightRadius = _radius; }
	void SetHeight(float _height)					{ height = _height; }
	void SetColor(const Color& _color)				{ color = _color; }
	void SetCastShadows(bool _castShadows)			{ castShadows = _castShadows; }
	void SetIsPersistant(bool _isPersistant)		{ isPersistant = _isPersistant; }
	void SetGelTexture(GameTextureID _gelTexture)	{ gelTexture = _gelTexture; }
	void SetConeAngles(float _coneAngle = 2*PI, float _coneFadeAngle=0, float _coneFadeColor = 0) { coneAngle = _coneAngle; coneFadeAngle = _coneFadeAngle; coneFadeColor = _coneFadeColor; }
	void SetFadeTimer(float time, bool _fadeOut = true, bool _fadeIn = false) { fadeTimer.Set(time); fadeOut = _fadeOut; fadeIn = _fadeIn; }

	float GetRadius() const							{ return radius; }
	float GetHeight() const							{ return height; }
	float GetHaloRadius() const						{ return haloRadius; }
	float GetOverbrightRadius() const				{ return overbrightRadius; }
	Color GetColor() const							{ return color; }
	bool  GetCastShadows() const					{ return castShadows; }
	float GetFadeAlpha() const;
	
	bool IsLight() const { return true; }
	bool IsSimpleLight() const;

	static void StubRender(const GameObjectStub& stub, float alpha);
	static WCHAR* StubAttributesDescription();
	static WCHAR* StubDescription();

private:

	void Update();
	void Render();
	virtual bool IsPersistant() const { return isPersistant; }
	virtual void Kill();

	Color color;
	float radius;
	float overbrightRadius;
	float haloRadius;
	float height;
	float coneAngle;
	float coneFadeAngle;
	float coneFadeColor;
	bool castShadows;
	bool fadeOut;
	bool fadeIn;
	bool isPersistant;
	bool wasRendered;
	bool wasCreatedFromStub;
	GameTextureID gelTexture;
	GameTimerPercent fadeTimer;

	friend DeferredRender;
};

#endif // LIGHT_H