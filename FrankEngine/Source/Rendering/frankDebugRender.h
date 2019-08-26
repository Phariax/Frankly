////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Debug Renderer
	Copyright 2013 Frank Force - http://www.frankforce.com

	- simple render for debug purposes
	- calls do not need to be made within a render loop because they are cached
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRANK_DEBUG_RENDER_H
#define FRANK_DEBUG_RENDER_H

#include "../rendering/frankRender.h"

extern class DebugRender g_debugRender;

class DebugRender
{
public:

	DebugRender();
	~DebugRender();

	void RenderPoint(const Vector3& point, const Color& color = Color::White(0.5f), float radius = 0.1f, float time = 0.0f);
	void RenderLine(const Vector3& start, const Vector3& end, const Color& color = Color::White(0.5f), float time = 0.0f);
	void RenderLine(const Line2& line, const Color& color = Color::White(0.5f), float time = 0.0f);
	void RenderBox(const Box2AABB& box, const Color& color = Color::White(0.5f), float time = 0.0f);
	void RenderCircle(const Circle& circle, const Color& color = Color::White(0.5f), float time = 0.0f);
	void RenderText(const Vector2& position, const wstring& text, const Color& color = Color::White(0.5f), float time = 0.0f);
	void RenderTextFormatted(const Vector2& position, const Color& color, float time, const wstring messageFormat, ...);

	void Update(float delta);
	void Render();
	void Clear();

	void InitDeviceObjects();
	void DestroyDeviceObjects();
	
	void ImmediateRenderTextRaw(const Vector2& point, const WCHAR* strText, const Color& color = Color::White(0.5f), CDXUTTextHelper* textHelper = NULL);
	void ImmediateRenderText(const Vector2& point, const WCHAR* strText, const Color& color = Color::White(0.5f), CDXUTTextHelper* textHelper = NULL);
	void ImmediateRenderTextFormatted(const Vector2& point, const Color& color, const wstring messageFormat, ...);

private:

	struct DebugRenderItemPoint
	{
		Vector3 position;
		Color color;
		float time;
		float scale;
	};

	struct DebugRenderItemLine
	{
		Line3 line;
		Color color;
		float time;
	};
	
	struct DebugRenderItemText
	{
		Vector2 position;
		WCHAR text[256];
		Color color;
		float time;
	};

	static const int itemCountMax;
	static const int textCountMax;

	float debugRenderTimer;

	list<DebugRenderItemPoint *> activeRenderItemPoints;
	list<DebugRenderItemPoint *> inactiveRenderItemPoints;

	list<DebugRenderItemLine *> activeRenderItemLines;
	list<DebugRenderItemLine *> inactiveRenderItemLines;

	list<DebugRenderItemText *> activeRenderItemTexts;
	list<DebugRenderItemText *> inactiveRenderItemTexts;

	void UpdatePrimitiveLineList();
	void BuildPrimitiveLineList();

	FrankRender::RenderPrimitive primitiveLineList;
};

#endif // FRANK_DEBUG_RENDER_H