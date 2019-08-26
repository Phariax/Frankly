////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Debug Renderer
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../rendering/frankDebugRender.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	Debug Render Globals
*/
////////////////////////////////////////////////////////////////////////////////////////

const int DebugRender::itemCountMax = 1000;
const int DebugRender::textCountMax = 100;

DebugRender g_debugRender;

#define D3DFVF_LineListVertex (D3DFVF_XYZ|D3DFVF_DIFFUSE)
struct LineListVertex
{
    D3DXVECTOR3 position;		// vertex position
	DWORD color;				// color
};

// console command to clear all primitves in the buffer
static void ConsoleCommandCallback_clearDebugPrimitives(const wstring& text) { g_debugRender.Clear(); }
ConsoleCommand(ConsoleCommandCallback_clearDebugPrimitives, clearDebugPrimitives);

////////////////////////////////////////////////////////////////////////////////////////
/*
	Debug Render Member Functions
*/
////////////////////////////////////////////////////////////////////////////////////////

DebugRender::DebugRender()
{
	for (int i = 0; i < itemCountMax; ++i)
		inactiveRenderItemPoints.push_back(new DebugRenderItemPoint);

	for (int i = 0; i < itemCountMax; ++i)
		inactiveRenderItemLines.push_back(new DebugRenderItemLine);
	
	for (int i = 0; i < textCountMax; ++i)
		inactiveRenderItemTexts.push_back(new DebugRenderItemText);

	debugRenderTimer = 0;
}

DebugRender::~DebugRender()
{
	while (!activeRenderItemPoints.empty())
	{
		delete activeRenderItemPoints.back();
		activeRenderItemPoints.pop_back();
	}
	while (!inactiveRenderItemPoints.empty())
	{
		delete inactiveRenderItemPoints.back();
		inactiveRenderItemPoints.pop_back();
	}
	while (!activeRenderItemLines.empty())
	{
		delete activeRenderItemLines.back();
		activeRenderItemLines.pop_back();
	}
	while (!inactiveRenderItemLines.empty())
	{
		delete inactiveRenderItemLines.back();
		inactiveRenderItemLines.pop_back();
	}
	while (!activeRenderItemTexts.empty())
	{
		delete activeRenderItemTexts.back();
		activeRenderItemTexts.pop_back();
	}
	while (!inactiveRenderItemTexts.empty())
	{
		delete inactiveRenderItemTexts.back();
		inactiveRenderItemTexts.pop_back();
	}
}

void DebugRender::RenderPoint(const Vector3 &position, const Color &color, float scale, float time)
{
	ASSERT(time >= 0 || scale > 0);

	if (inactiveRenderItemPoints.empty())
		return;

	DebugRenderItemPoint *newItem = inactiveRenderItemPoints.back();
	inactiveRenderItemPoints.pop_back();
	activeRenderItemPoints.push_back(newItem);

	newItem->position = position;
	newItem->color = color;
	newItem->scale = scale;
	newItem->time = debugRenderTimer + time;
}

void DebugRender::RenderTextFormatted(const Vector2& position, const Color& color, float time, const wstring messageFormat, ...)
{
	if (time < 0 || inactiveRenderItemTexts.empty())
		return;
	
	DebugRenderItemText *newItem = inactiveRenderItemTexts.back();
	inactiveRenderItemTexts.pop_back();
	activeRenderItemTexts.push_back(newItem);

	newItem->position = position;
	newItem->color = color;
	newItem->time = time;
	
	// fill in the message
    va_list args;
    va_start(args, messageFormat);
	StringCchVPrintf( newItem->text, 256, messageFormat.c_str(), args );
	newItem->text[255] = L'\0';
    va_end(args);
}

void DebugRender::RenderText(const Vector2& position, const wstring& text, const Color& color, float time)
{
	if (time < 0 || inactiveRenderItemTexts.empty())
		return;
	
	DebugRenderItemText *newItem = inactiveRenderItemTexts.back();
	inactiveRenderItemTexts.pop_back();
	activeRenderItemTexts.push_back(newItem);

	newItem->position = position;
	newItem->color = color;
	newItem->time = time;
	wcsncpy_s(newItem->text, 256, text.c_str(), 256);
}

void DebugRender::RenderLine(const Vector3& start, const Vector3& end, const Color& color, float time)
{
	if (time < 0 || inactiveRenderItemLines.empty())
		return;

	DebugRenderItemLine *newItem = inactiveRenderItemLines.back();
	inactiveRenderItemLines.pop_back();
	activeRenderItemLines.push_back(newItem);

	newItem->line.p1 = start;
	newItem->line.p2 = end;
	newItem->color = color;
	newItem->time = debugRenderTimer + time;
}

void DebugRender::Clear()
{
	activeRenderItemPoints.clear();
	activeRenderItemLines.clear();
}

void DebugRender::Update(float delta)
{
	debugRenderTimer += delta;

	for (list<DebugRenderItemPoint*>::iterator it = activeRenderItemPoints.begin(); it != activeRenderItemPoints.end();)
	{
		DebugRenderItemPoint *renderItem = (*it);

		if (renderItem->time < debugRenderTimer)
		{
			it = activeRenderItemPoints.erase(it);
			inactiveRenderItemPoints.push_back(renderItem);
		}
		else
			++it;
	}

	for (list<DebugRenderItemLine*>::iterator it = activeRenderItemLines.begin(); it != activeRenderItemLines.end();)
	{
		DebugRenderItemLine *renderItem = (*it);

		if (renderItem->time < debugRenderTimer)
		{
			it = activeRenderItemLines.erase(it);
			inactiveRenderItemLines.push_back(renderItem);
		}
		else
			++it;
	}

	for (list<DebugRenderItemText*>::iterator it = activeRenderItemTexts.begin(); it != activeRenderItemTexts.end();)
	{
		DebugRenderItemText *renderItem = (*it);

		if (renderItem->time < debugRenderTimer)
		{
			it = activeRenderItemTexts.erase(it);
			inactiveRenderItemTexts.push_back(renderItem);
		}
		else
			++it;
	}
}

void DebugRender::Render()
{
	IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();

	pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE); 
	pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

	for (list<DebugRenderItemPoint*>::iterator it = activeRenderItemPoints.begin(); it != activeRenderItemPoints.end(); ++it)
	{
		DebugRenderItemPoint& renderItem = (**it);

		const Matrix44 matrixTranslate = Matrix44::BuildTranslate(renderItem.position);
		const Matrix44 matrixScale = Matrix44::BuildScale(renderItem.scale);
		const Matrix44 matrixRender = matrixScale * matrixTranslate;
		g_render->RenderCircle(matrixRender, renderItem.color);
	}

	UpdatePrimitiveLineList();

	/*list<FrankRender::LineListItem> lineList;
	for (list<DebugRenderItemLine*>::iterator it = activeRenderItemLines.begin(); it != activeRenderItemLines.end(); ++it)
	{
		DebugRenderItemLine& renderItem = (**it);
		
		FrankRender::LineListItem newItem = {renderItem.line, renderItem.color};
		lineList.push_back(newItem);
	}

	g_render->RenderLineList(lineList);*/

	pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	if (primitiveLineList.primitiveCount > 0)
		g_render->Render(Matrix44::Identity(), Color::White(), GameTexture_Invalid, primitiveLineList);
	
	for (list<DebugRenderItemText*>::iterator it = activeRenderItemTexts.begin(); it != activeRenderItemTexts.end(); ++it)
	{
		DebugRenderItemText& renderItem = (**it);
		ImmediateRenderText(renderItem.position, renderItem.text, renderItem.color);
	}
}

void DebugRender::InitDeviceObjects()
{
	BuildPrimitiveLineList();
}

void DebugRender::DestroyDeviceObjects()
{
	primitiveLineList.SafeRelease();
}

void DebugRender::UpdatePrimitiveLineList()
{
	if (activeRenderItemLines.empty())
	{
		primitiveLineList.primitiveCount = 0;
		return;
	}

	// lock the vb while we update it, use d3dlock_discard since we will overwrite everything
	LineListVertex* vertices;
	if (SUCCEEDED(primitiveLineList.vb->Lock(0, 0, (VOID**)&vertices, D3DLOCK_DISCARD)))
	{
		int i = -1;
		for (list<DebugRenderItemLine*>::iterator it = activeRenderItemLines.begin(); it != activeRenderItemLines.end(); ++it)
		{
			DebugRenderItemLine& renderItem = (**it);
			const Color& c = renderItem.color;

			LineListVertex& vertexStart = vertices[++i];
			vertexStart.position = renderItem.line.p1;
			vertexStart.color = c;

			LineListVertex& vertexEnd = vertices[++i];
			vertexEnd.position = renderItem.line.p2;
			vertexEnd.color = c;
			
			if (i + 1 >= (int)primitiveLineList.vertexCount)
				break;
		}

		primitiveLineList.vb->Unlock();

		primitiveLineList.primitiveCount = (i+1) / 2;
		//Render(Matrix44::identityMatrix, Color::White(), FrankRender::TI_NoTexture, primitiveLineList);
	}
}

void DebugRender::BuildPrimitiveLineList()
{
	static const int lineCountMax = itemCountMax * 2;

	primitiveLineList.Create
	(
		lineCountMax,				// primitiveCount
		lineCountMax * 2,			// vertexCount
		D3DPT_LINELIST,				// primitiveType
		sizeof(LineListVertex),		// stride
		D3DFVF_LineListVertex,		// fvf
		true						// dynamic
	);

	// start with no lines
	primitiveLineList.primitiveCount = 0;
}

void DebugRender::RenderLine(const Line2& line, const Color& color, float time)
{
	RenderLine(line.p1, line.p2, color), time;
}

void DebugRender::RenderBox(const Box2AABB& box, const Color& color, float time)
{	
	RenderLine(Vector2(box.lowerBound.x, box.lowerBound.y), Vector2(box.upperBound.x, box.lowerBound.y), color, time);
	RenderLine(Vector2(box.lowerBound.x, box.upperBound.y), Vector2(box.upperBound.x, box.upperBound.y), color, time);
	RenderLine(Vector2(box.lowerBound.x, box.lowerBound.y), Vector2(box.lowerBound.x, box.upperBound.y), color, time);
	RenderLine(Vector2(box.upperBound.x, box.lowerBound.y), Vector2(box.upperBound.x, box.upperBound.y), color, time);
}

void DebugRender::RenderCircle(const Circle& circle, const Color& color, float time)
{	
	const int sides = g_render->GetMaxGetCircleVertSides();
	const Vector2* circleVerts = g_render->GetCircleVerts(sides);

	for(int i = 0; i < sides-1; ++i)
		RenderLine(circle.radius*circleVerts[i] + circle.position, circle.radius*circleVerts[i+1] + circle.position, color, time);
	RenderLine(circle.radius*circleVerts[sides-1] + circle.position, circle.radius*circleVerts[0] + circle.position, color, time);
}

void DebugRender::ImmediateRenderTextFormatted
(
	const Vector2& position, 
	const Color& color, 
	const wstring messageFormat, ...
)
{
	if (DeferredRender::GetRenderPass())
		return;
	
	// fill in the message
    va_list args;
    va_start(args, messageFormat);
	WCHAR output[1024];
	StringCchVPrintf( output, 1024, messageFormat.c_str(), args );
	output[1023] = L'\0';
    va_end(args);

	const Vector2 screenSpacePos = g_render->WorldSpaceToScreenSpace(position);
	g_textHelper->Begin();
	g_textHelper->DrawTextLine((int)screenSpacePos.x, (int)screenSpacePos.y, output, color, true, true);
	g_textHelper->End();
}

void DebugRender::ImmediateRenderText
(
	const Vector2& position, 
	const WCHAR* strText, 
	const Color& color, 
	CDXUTTextHelper* textHelper
)
{
	if (!textHelper)
		textHelper = g_textHelper;
	
	if (DeferredRender::GetRenderPass())
		return;
	
	const Vector2 screenSpacePos = g_render->WorldSpaceToScreenSpace(position);
	textHelper->Begin();
	textHelper->DrawTextLine((int)screenSpacePos.x, (int)screenSpacePos.y, strText, color, true, true);
	textHelper->End();
}

void DebugRender::ImmediateRenderTextRaw
(
	const Vector2& position, 
	const WCHAR* strText, 
	const Color& color, 
	CDXUTTextHelper* textHelper
)
{
	if (!textHelper)
		textHelper = g_textHelper;
	
	if (DeferredRender::GetRenderPass())
		return;
	
	textHelper->Begin();
	textHelper->DrawTextLine((int)position.x, (int)position.y, strText, color, true, true);
	textHelper->End();
}