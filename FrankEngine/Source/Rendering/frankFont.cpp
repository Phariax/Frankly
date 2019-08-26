////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Font System
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include <fstream>
#include <string>
#include "../rendering/frankFont.h"

FrankRender::RenderPrimitive FrankFont::primitiveTris;

FrankFont::FrankFont(GameTextureID _texture, WCHAR* dataFilenameWide) :
	texture(_texture)
{
	ifstream inFile(dataFilenameWide, ios::in);
	
	if (inFile.is_open())
	{
		// read data from file
		ParseFont( inFile, charSet );
		return;
	}

	// Get pointer and size to resource
	HRSRC hRes = FindResource(0, dataFilenameWide, RT_RCDATA);
	HGLOBAL hMem = LoadResource(0, hRes);
	void* dataPointer = LockResource(hMem);
	DWORD size = SizeofResource(0, hRes);

	if (!dataPointer || size == 0)
	{
		g_debugMessageSystem.AddError(L"Count not find font data file '%s'.", dataFilenameWide);
		return;
	}

	// copy the resource into a string stream
	std::stringstream dataStream((char*)dataPointer);

	// free the resource
	UnlockResource(hMem);
	FreeResource(hRes);
	
	ParseFont( dataStream, charSet );
}

Box2AABB FrankFont::GetBBox(const char* string, const XForm2& xf, float size, FontFlags flags)
{
	const float fontScale = size/charSet.lineHeight;

	Vector2 minPos, maxPos;
	GetBounds(string, minPos, maxPos);

	if (flags & FontFlag_CenterY)
	{
		const float offset = (maxPos.y - minPos.y) / 2;
		minPos.y += offset;
		maxPos.y += offset;
	}
	
	if (flags & FontFlag_CenterX)
	{
		const float offset = (maxPos.x - minPos.x) / 2;
		minPos.x -= offset;
		maxPos.x -= offset;
	}

	const Box2AABB localBox(minPos, maxPos);
	return Box2AABB(xf, localBox, fontScale);
}
	
void FrankFont::RenderScreenSpace(const WCHAR* wstring, const XForm2& xf, float size, const Color& color, FontFlags flags)
{
	float s = size / charSet.lineHeight;
	const Matrix44 matrix = FrankRender::GetScreenSpaceMatrix(xf.position.x, xf.position.y, s, s);
	char string[maxStringSize];
	wcstombs_s(NULL, string, maxStringSize, wstring, maxStringSize);
	return Render(string, matrix, color, flags, true);
}

void FrankFont::RenderScreenSpace(const char* string, const XForm2& xf, float size, const Color& color, FontFlags flags)
{
	float s = size / charSet.lineHeight;
	const Matrix44 matrix = FrankRender::GetScreenSpaceMatrix(xf.position.x, xf.position.y, s, s);
	return Render(string, matrix, color, flags, true);
}

void FrankFont::Render(const WCHAR* wstring, const XForm2& xf, float size, const Color& color, FontFlags flags, bool cameraCheck)
{
	char string[maxStringSize];
	wcstombs_s(NULL, string, maxStringSize, wstring, maxStringSize);

	if (cameraCheck)
	{
		// check if on screen
		const Box2AABB box = GetBBox(string, xf, size, flags);
		if (!g_cameraBase->CameraTest(box))
			return;
	}
	
	const Matrix44 matrix = Matrix44::BuildScale(size/charSet.lineHeight) * Matrix44(xf);
	return Render(string, matrix, color, flags);
}

void FrankFont::Render(const char* string, const XForm2& xf, float size, const Color& color, FontFlags flags, bool cameraCheck)
{
	if (cameraCheck)
	{
		// check if on screen
		const Box2AABB box = GetBBox(string, xf, size, flags);
		if (!g_cameraBase->CameraTest(box))
			return;
	}

	const Matrix44 matrix = Matrix44::BuildScale(size/charSet.lineHeight) * Matrix44(xf);
	Render(string, matrix, color, flags);
}

void FrankFont::Render(const char* string, const Matrix44& matrix, const Color& color, FontFlags flags, bool screenSpace)
{
	FrankProfilerEntryDefine(L"FrankFont::Render()", Color::White(), 5);

	if (!(flags & FontFlag_CastShadows))
	{
		if (DeferredRender::GetRenderPassIsShadow())
			return;
	}
	
	if (flags & FontFlag_NoNormals)
	{
		if (DeferredRender::GetRenderPassIsNormalMap())
			return;
	}

	Vector2 offset(0);

	if (flags & FontFlag_CenterY)
	{
		Vector2 minPos, maxPos;
		GetBounds(string, minPos, maxPos);
		offset.y = (maxPos.y - minPos.y) / 2;
	}

	BuildTriStrip(string, flags);

	{
		// always use linear filtering for fonts
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
	}

	const Matrix44 finalMatrix = Matrix44(offset) * matrix;
	if (screenSpace)
		g_render->RenderScreenSpace(finalMatrix, color, texture, primitiveTris);
	else
		g_render->Render(finalMatrix, color, texture, primitiveTris);

	if (g_usePointFiltering)
	{
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_NONE );
	}
	else
	{
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
	}
}

void FrankFont::InitDeviceObjects()
{
	const int vertCount = maxStringSize * 6;
	primitiveTris.Create
	(
		0,								// primitiveCount
		vertCount,						// vertexCount
		D3DPT_TRIANGLESTRIP,			// primitiveType
		sizeof(FontVertex),				// stride
		(D3DFVF_XYZ|D3DFVF_TEX1),		// fvf
		true							// dynamic
	);
}

void FrankFont::DestroyDeviceObjects()
{
	primitiveTris.SafeRelease();
}

void FrankFont::BuildTriStrip(const char* string, FontFlags flags)
{
	FontVertex* verts;
	primitiveTris.vb->Lock(0, 0, (VOID**)&verts, D3DLOCK_DISCARD);

	int vertCount = 0;
	int stringLength = strlen(string);
	Vector2 position(0);
	FontVertex* vert = &verts[0];

	if (flags & FontFlag_CenterX)
	{
		float minX = 0, maxX = 0;
		GetLineBounds(string, minX, maxX);
		position.x = -(minX + (maxX - minX)/2);
	}
	else if (flags & FontFlag_AlignRight)
	{
		float minX = 0, maxX = 0;
		GetLineBounds(string, minX, maxX);
		position.x = -(minX + (maxX - minX));
	}

	for(int i = 0; i < stringLength; ++i)
	{
		const char& c = string[i];

		if (!c)
			break;

		if (c == '\n')
		{
			if (flags & FontFlag_CenterX)
			{
				float minX = 0, maxX = 0;
				GetLineBounds(&string[i+1], minX, maxX);
				position.x = -(minX + (maxX - minX)/2);
			}
			else if (flags & FontFlag_AlignRight)
			{
				float minX = 0, maxX = 0;
				GetLineBounds(&string[i+1], minX, maxX);
				position.x = -(minX + (maxX - minX));
			}
			else
				position.x = 0;
			position.y -= charSet.lineHeight;
			continue;
		}

		const CharDescriptor& charDescriptor = charSet.Chars[c];
		
		if (c == ' ')
		{
			position.x += charDescriptor.advanceX;
			continue;
		}

		const Vector2 charUL = charDescriptor.position;
		const Vector2 charUR = charDescriptor.position + Vector2(charDescriptor.size.x, 0);
		const Vector2 charLL = charDescriptor.position + Vector2(0, charDescriptor.size.y);
		const Vector2 charLR = charDescriptor.position + charDescriptor.size;
		
		//lower left
		Vector3 vertPosition(0);
		vertPosition.x = position.x + charDescriptor.offset.x;
		vertPosition.y = position.y - charDescriptor.size.y - charDescriptor.offset.y;
		vert->position = vertPosition;
		++vert;
		++vertCount;
		
		// degenerate tri
		vert->position = vertPosition;
		vert->textureCoords = charLL / charSet.scale;
		++vert;
		++vertCount;

		//lower right
		vertPosition.x = position.x + charDescriptor.size.x + charDescriptor.offset.x;
		vertPosition.y = position.y - charDescriptor.size.y - charDescriptor.offset.y;
		vert->position = vertPosition;
		vert->textureCoords = charLR / charSet.scale;
		++vert;
		++vertCount;
		
		//upper left
		vertPosition.x = position.x + charDescriptor.offset.x;
		vertPosition.y = position.y - charDescriptor.offset.y;
		vert->position = vertPosition;
		vert->textureCoords = charUL / charSet.scale;
		++vert;
		++vertCount;

		//upper right
		vertPosition.x = position.x + charDescriptor.size.x + charDescriptor.offset.x;
		vertPosition.y = position.y - charDescriptor.offset.y;
		vert->position = vertPosition;
		vert->textureCoords = charUR / charSet.scale;
		++vert;
		++vertCount;

		// degenerate tri
		vert->position = vertPosition;
		++vert;
		++vertCount;

		position.x += charDescriptor.advanceX;
	}

	primitiveTris.vb->Unlock();

	primitiveTris.primitiveCount = vertCount - 2;
}

void FrankFont::GetBounds(const char* string, Vector2& minPos, Vector2& maxPos)
{
	minPos = Vector2(FLT_MAX);
	maxPos = Vector2(0);
	
	int stringLength = strlen(string);
	Vector2 position(0);
	for(int i = 0; i < stringLength; ++i)
	{
		const char& c = string[i];

		if (c == '\n')
		{
			position.x = 0;
			position.y -= charSet.lineHeight;
			continue;
		}

		const CharDescriptor& charDescriptor = charSet.Chars[c];
		
		if (c == ' ')
		{
			position.x += charDescriptor.advanceX;
			continue;
		}
		
		//lower left
		Vector3 vertPosition(0);
		vertPosition.x = position.x + charDescriptor.offset.x;
		vertPosition.y = position.y - charDescriptor.size.y - charDescriptor.offset.y;
		minPos.x = Min(vertPosition.x, minPos.x);
		minPos.y = Min(vertPosition.y, minPos.y);
		maxPos.x = Max(vertPosition.x, maxPos.x);
		maxPos.y = Max(vertPosition.y, maxPos.y);

		//lower right
		vertPosition.x = position.x + charDescriptor.size.x + charDescriptor.offset.x;
		vertPosition.y = position.y - charDescriptor.size.y - charDescriptor.offset.y;
		minPos.x = Min(vertPosition.x, minPos.x);
		minPos.y = Min(vertPosition.y, minPos.y);
		maxPos.x = Max(vertPosition.x, maxPos.x);
		maxPos.y = Max(vertPosition.y, maxPos.y);

		//upper left
		vertPosition.x = position.x + charDescriptor.offset.x;
		vertPosition.y = position.y - charDescriptor.offset.y;
		minPos.x = Min(vertPosition.x, minPos.x);
		minPos.y = Min(vertPosition.y, minPos.y);
		maxPos.x = Max(vertPosition.x, maxPos.x);
		maxPos.y = Max(vertPosition.y, maxPos.y);

		//upper right
		vertPosition.x = position.x + charDescriptor.size.x + charDescriptor.offset.x;
		vertPosition.y = position.y - charDescriptor.offset.y;
		minPos.x = Min(vertPosition.x, minPos.x);
		minPos.y = Min(vertPosition.y, minPos.y);
		maxPos.x = Max(vertPosition.x, maxPos.x);
		maxPos.y = Max(vertPosition.y, maxPos.y);

		position.x += charDescriptor.advanceX;
	}
}

void FrankFont::GetLineBounds(const char* string, float& minX, float& maxX)
{
	minX = FLT_MAX;
	maxX = 0;
	
	int stringLength = strlen(string);
	Vector2 position(0);
	for(int i = 0; i < stringLength; ++i)
	{
		const char& c = string[i];

		if (c == '\n' || !c)
			return;

		const CharDescriptor& charDescriptor = charSet.Chars[c];
		
		if (c == ' ')
		{
			position.x += charDescriptor.advanceX;
			continue;
		}
		
		//lower left
		Vector3 vertPosition(0);
		vertPosition.x = position.x + charDescriptor.offset.x;
		minX = Min(vertPosition.x, minX);
		maxX = Max(vertPosition.x, maxX);

		//lower right
		vertPosition.x = position.x + charDescriptor.size.x + charDescriptor.offset.x;
		minX = Min(vertPosition.x, minX);
		maxX = Max(vertPosition.x, maxX);

		//upper left
		vertPosition.x = position.x + charDescriptor.offset.x;
		minX = Min(vertPosition.x, minX);
		maxX = Max(vertPosition.x, maxX);

		//upper right
		vertPosition.x = position.x + charDescriptor.size.x + charDescriptor.offset.x;
		minX = Min(vertPosition.x, minX);
		maxX = Max(vertPosition.x, maxX);

		position.x += charDescriptor.advanceX;
	}
}
