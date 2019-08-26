////////////////////////////////////////////////////////////////////////////////////////
/*
	Physics Render
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../physics/physics.h"
#include "../physics/physicsRender.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	Globals
*/
////////////////////////////////////////////////////////////////////////////////////////

// the one and only physics renderer
PhysicsRender g_physicsRender;

#define D3DFVF_PhysicsRenderVertex (D3DFVF_XYZ|D3DFVF_DIFFUSE)

float PhysicsRender::alpha = 0.5f;
ConsoleCommand(PhysicsRender::alpha, physicsDebugAlpha);

////////////////////////////////////////////////////////////////////////////////////////
/*
	Physics Rendering
*/
////////////////////////////////////////////////////////////////////////////////////////

PhysicsRender::PhysicsRender()
{
	// used for physics debug rendering
	SetDebugRender(false);
}

void PhysicsRender::SetDebugRender(bool _debugRender)
{
	debugRender = _debugRender;
	// set the flags for what should be rendered
	uint32 flags = 0;
	if (debugRender)
	{
		flags += b2Draw::e_shapeBit;
		flags += b2Draw::e_jointBit;
		flags += b2Draw::e_aabbBit;
		flags += b2Draw::e_pairBit;
		flags += b2Draw::e_centerOfMassBit;
	} 
	else
		flags = 0;
	SetFlags(flags);
}

void PhysicsRender::Render()
{
	if (!debugRender)
		return;

	g_physics->GetPhysicsWorld()->DrawDebugData();
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Primitive Rendering
*/
////////////////////////////////////////////////////////////////////////////////////////

void PhysicsRender::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	g_render->DrawPolygon((Vector2*)vertices, vertexCount, Color(color, alpha));
}

void PhysicsRender::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
{
	g_render->DrawSolidPolygon((Vector2*)vertices, vertexCount, Color(color, alpha));
	g_render->DrawPolygon((Vector2*)vertices, vertexCount, Color(color, alpha));
}

void PhysicsRender::DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
{
	g_render->DrawCircle(XForm2(center), Vector2(radius), Color(color, alpha), FrankRender::GetMaxGetCircleVertSides());
}

void PhysicsRender::DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
{
	const float startAngle = atan2f(axis.y, axis.x);
	g_render->DrawOutlinedCircle(XForm2(center, startAngle), Vector2(radius), Color(color, alpha), Color(color), FrankRender::GetMaxGetCircleVertSides());
}

void PhysicsRender::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
{
	g_render->DrawSegment(p1, p2, Color(color, alpha));
}

void PhysicsRender::DrawTransform(const b2Transform& xf)
{
	g_render->DrawSegment(xf.p, xf.p + xf.q.GetXAxis(), Color::Red());
	g_render->DrawSegment(xf.p, xf.p + xf.q.GetYAxis(), Color::Blue());
}

void PhysicsRender::DrawFixture(const XForm2& xf, const b2Fixture& fixture, const Color& color, const Color& outlineColor)
{
	switch (fixture.GetType())
	{
		case b2Shape::e_circle:
		{
			b2CircleShape* circle = (b2CircleShape*)fixture.GetShape();
			g_render->DrawOutlinedCircle(XForm2(circle->m_p)*xf, Vector2(circle->m_radius), color, outlineColor, FrankRender::GetMaxGetCircleVertSides());
			break;
		}

		case b2Shape::e_edge:
		{
			b2EdgeShape* edge = (b2EdgeShape*)fixture.GetShape();
			g_render->DrawSegment(xf.TransformCoord(edge->m_vertex1), xf.TransformCoord(edge->m_vertex2), color);
			break;
		}

		case b2Shape::e_chain:
		{
			b2ChainShape* chain = (b2ChainShape*)fixture.GetShape();
			int32 count = chain->m_count;
			const b2Vec2* vertices = chain->m_vertices;

			Vector2 v1 = xf.TransformCoord(vertices[0]);
			for (int32 i = 1; i < count; ++i)
			{
				Vector2 v2 = xf.TransformCoord(vertices[i]);
				g_render->DrawSegment(v1, v2, color);
				g_render->DrawOutlinedCircle(XForm2(v1)*xf, 0.05f, color, outlineColor, 12);
				v1 = v2;
			}
			break;
		}

		case b2Shape::e_polygon:
		{
			b2PolygonShape* poly = (b2PolygonShape*)fixture.GetShape();
			int32 vertexCount = poly->m_vertexCount;
			b2Assert(vertexCount <= b2_maxPolygonVertices);
			b2Vec2 vertices[b2_maxPolygonVertices];

			for (int32 i = 0; i < vertexCount; ++i)
				vertices[i] = xf.TransformCoord(poly->m_vertices[i]);

			g_render->DrawSolidPolygon((Vector2*)vertices, vertexCount, color);
			g_render->DrawPolygon((Vector2*)vertices, vertexCount, outlineColor);
			break;
		}
	}
}

void PhysicsRender::DrawBody(b2Body& physicsBody, const Color& color, const Color& outlineColor)
{
	const b2Transform xf = physicsBody.GetTransform();
	for (const b2Fixture* f = physicsBody.GetFixtureList(); f; f = f->GetNext())
	{
		DrawFixture(xf, *f, color, outlineColor);
	}

	for (const b2JointEdge* j = physicsBody.GetJointList(); j; j = j->next)
	{
		b2Joint* joint = j->joint;
		g_render->DrawOutlinedCircle(Vector2(joint->GetAnchorA()), Vector2(0.1f), color, outlineColor, 12);
		g_render->DrawOutlinedCircle(Vector2(joint->GetAnchorB()), Vector2(0.1f), color, outlineColor, 12);
		g_render->DrawSegment(joint->GetAnchorA(), joint->GetAnchorB(), outlineColor);
	}
}
