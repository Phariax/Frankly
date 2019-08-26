////////////////////////////////////////////////////////////////////////////////////////
/*
	Physics Render
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef RENDER_H
#define RENDER_H

// this is the global physics render
extern class PhysicsRender g_physicsRender;

// This class implements debug drawing callbacks that are invoked
// inside b2World::Step.
class PhysicsRender : public b2Draw
{
public:

	PhysicsRender();
	void Render();

	bool GetDebugRender() const { return debugRender; }
	void SetDebugRender(bool debugRender);

	///////////////////////////////////////
	// rendering
	///////////////////////////////////////

	void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color);
	void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color);
	void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color);
	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color);
	void DrawTransform(const b2Transform& xf);
	void DrawBody(b2Body& physicsBody, const Color& color = Color::Black(), const Color& outlineColor = Color::White());
	void DrawFixture(const XForm2& xf, const b2Fixture& fixture, const Color& color = Color::Black(), const Color& outlineColor = Color::White());
	
	static float alpha;
	bool debugRender;
};

#endif
