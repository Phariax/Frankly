////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Rendering
	Copyright 2013 Frank Force - http://www.frankforce.com

	- low level rendering for Frank engine
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRANK_RENDER_H
#define FRANK_RENDER_H

#include "objects/camera.h"

#define MAX_TEXTURE_COUNT	(1024)

enum GameTextureID;
const GameTextureID GameTexture_Invalid			= GameTextureID(0);
const GameTextureID GameTexture_Smoke				= GameTextureID(1);
const GameTextureID GameTexture_Circle				= GameTextureID(2);
const GameTextureID GameTexture_Dot				= GameTextureID(3);
const GameTextureID GameTexture_Arrow				= GameTextureID(4);
const GameTextureID GameTexture_LightMask			= GameTextureID(5);
const GameTextureID GameTexture_Watermark			= GameTextureID(6);
const GameTextureID GameTexture_StartGameTextures	= GameTextureID(7);	// this is where gameside textures ids start

class FrankRender
{
public:

	FrankRender();

	void InitDeviceObjects();
	void DestroyDeviceObjects();

	bool LoadTexture(const WCHAR* textureName, GameTextureID ti);
	void ReleaseTextures();
	
	// support for using texture tile sheets
	void SetTextureTile(GameTextureID tileSheetTi, const WCHAR* textureName, GameTextureID tileTi, const ByteVector2& tilePos, const ByteVector2& tileSize);
	GameTextureID GetTextureTileSheet(GameTextureID ti) const { ASSERT(ti < MAX_TEXTURE_COUNT); return textures[ti].tileSheetTi; }
	void GetTextureTileInfo(GameTextureID ti, ByteVector2& tilePos, ByteVector2& tileSize) const { ASSERT(ti < MAX_TEXTURE_COUNT); tilePos = textures[ti].tilePos; tileSize = textures[ti].tileSize; }

	LPDIRECT3DTEXTURE9 GetTexture(GameTextureID ti, bool deferredCheck = true) const;
	LPDIRECT3DTEXTURE9 GetTexture(const WCHAR* textureName);
	const WCHAR* GetTextureName(GameTextureID ti) const { return textures[ti].name; }
	IntVector2 GetTextureSize(GameTextureID ti) const;
	IntVector2 GetTextureSize(LPDIRECT3DTEXTURE9 texture) const;
	D3DFORMAT GetTextureFormat(LPDIRECT3DTEXTURE9 texture) const;
	
	static Vector2 WorldSpaceToScreenSpace(const Vector2& p);
	bool LoadPixelShader(const WCHAR* name, LPDIRECT3DPIXELSHADER9& shader, LPD3DXCONSTANTTABLE& constants);
	bool IsNormalMapShaderLoaded() const { return normalMapShader != NULL; }

	void SetFiltering();

public:	// fast rendering (no textures)

	void DrawPolygon(const Vector2* vertices, int vertexCount, const DWORD color = Color::White());
	void DrawPolygon(const XForm2& xf, const Vector2* vertices, int vertexCount, const DWORD color = Color::White());
	void DrawSolidPolygon(const Vector2* vertices, int vertexCount, const DWORD color = Color::White());
	void DrawSolidPolygon(const XForm2& xf, const Vector2* vertices, int vertexCount, const DWORD color = Color::White());
	void DrawOutlinedPolygon(const XForm2& xf, const Vector2* vertices, int vertexCount, const DWORD color = Color::Black(), const DWORD outlineColor = Color::White());

	void DrawCircle(const XForm2& xf, const Vector2& size, const DWORD color = Color::White(), int sides = 12);
	void DrawSolidCircle(const XForm2& xf, const Vector2& size, const DWORD color = Color::White(), int sides = 12);
	void DrawOutlinedCircle(const XForm2& xf, const Vector2& size, const DWORD color = Color::Black(), const DWORD outlineColor = Color::White(), int sides = 12);

	void DrawCircle(const XForm2& xf, float radius, const DWORD color = Color::White(), int sides = 12) { DrawCircle(xf, Vector2(radius), color, sides); }
	void DrawSolidCircle(const XForm2& xf, float radius, const DWORD color = Color::White(), int sides = 12)  { DrawSolidCircle(xf, Vector2(radius), color, sides); }
	void DrawOutlinedCircle(const XForm2& xf, float radius, const DWORD color = Color::Black(), const DWORD outlineColor = Color::White(), int sides = 12) { DrawOutlinedCircle(xf, Vector2(radius), color, sides); }

	void DrawCone(const XForm2& xf, float radius, float coneAngle, const DWORD color = Color::White(), int divisions = 12);

	void DrawSegment(const Vector2& p1, const Vector2& p2, const DWORD color = Color::White());
	void DrawSegment(const XForm2& xf, const Vector2& p1, const Vector2& p2, const DWORD color = Color::White());

	void DrawSegment(const Line2& l, const DWORD color = Color::White()) { DrawSegment(l.p1, l.p2, color); }
	void DrawSegment(const XForm2& xf, const Line2& l, const DWORD color = Color::White()) { DrawSegment(xf, l.p1, l.p2, color); }
	void DrawThickSegment(const Line2& l, float thickness = 0.1f, const DWORD color = Color::White());

	void DrawBox(const XForm2& xf, const Vector2& size, const DWORD color = Color::White());
	void DrawSolidBox(const XForm2& xf, const Vector2& size, const DWORD color = Color::White());
	void DrawOutlinedBox(const XForm2& xf, const Vector2& size, const DWORD color = Color::White(), const DWORD outlineColor = Color::Black());
	void DrawAxisAlignedBox(const Box2AABB& box, const DWORD color = Color::White());

	void DrawXForm(const XForm2& xf, float radius = 1, const DWORD color = Color::White());

public:	// lower level functions for fast rendering

	// call these at beginning and end of simper vert rendering
	void RenderSimpleVerts();
	void RenderSimpleLineVerts();
	void RenderSimpleTriVerts();

	void AddPointToLineVerts(const Vector2& v, DWORD color);
	void AddPointToTriVerts(const Vector2& v, DWORD color);

	// call these before and after you start a shape to cap it off
	void CapLineVerts(const Vector2& v) { AddPointToLineVerts(v, 0); }
	void CapTriVerts(const Vector2& v) { AddPointToTriVerts(v, 0); AddPointToTriVerts(v, 0); }

	// call thse functions to set the current render group simple verts to be additive
	// note: this will affect all simple verts that are rendered out for the current group!
	void SetSimpleVertsAreAdditive(bool additive);
	bool GetSimpleVertsAreAdditive() const			{ return simpleVertsAreAdditive; }

public:	// rendering functions

	static Matrix44 GetScreenSpaceMatrix(float x, float y, float sx, float sy, float r = 0) 
	{ 
		Matrix44 matrixR = Matrix44::BuildRotate(0, 0, -(float)r);
		Matrix44 matrix = Matrix44::BuildScale((float)sx, -(float)sy, 0);
		matrix += Vector3((float)x, (float)y, 0);
		return matrixR * matrix;
	}

	void RenderQuad
	(
		const XForm2& xf,
		const Vector2& size,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid,
		bool cameraCheck = true
	)
	{ 
		if (!cameraCheck || g_cameraBase->CameraTest(xf.position, fabs(size.x) + fabs(size.y)))
			Render(Matrix44::BuildScale(size) * Matrix44(xf), color, ti, primitiveQuad); 
	}

	void RenderQuad
	(
		const XForm2& xf,
		const Box2AABB& localBoundingBox,
		const Color& color,
		GameTextureID ti = GameTexture_Invalid,
		bool cameraCheck = true
	)
	{
		const Vector2 size = Vector2(localBoundingBox.upperBound - localBoundingBox.lowerBound) / 2;
		const Vector2 offset = localBoundingBox.lowerBound + size;
		
		if (!cameraCheck || g_cameraBase->CameraTest(xf.position, sqrtf(Max(localBoundingBox.upperBound.LengthSquared(), localBoundingBox.lowerBound.LengthSquared()))))
			Render(Matrix44::BuildScale(size) * Matrix44::BuildTranslate(offset.x, offset.y, 0) * Matrix44(xf), color, ti, primitiveQuad); 
	}
	
	void RenderQuadLine
	(
		const Line2& segment,
		float thickness = 0.1f,
		const Color& color = Color::White(),
		LPDIRECT3DTEXTURE9 texture = NULL
	);

	void RenderQuad
	(
		const XForm2& xf,
		const Vector2& size,
		const Color& color,
		LPDIRECT3DTEXTURE9 texture
	)
	{ 
		Render
		(
			Matrix44::BuildScale(size) * Matrix44(xf),
			color,
			texture,
			primitiveQuad.vb,
			primitiveQuad.primitiveCount,
			primitiveQuad.primitiveType,
			primitiveQuad.stride,
			primitiveQuad.fvf
		);
	}
	
	void RenderQuadSimple
	(
		const XForm2& xf,
		const Vector2& size
	)
	{ 
		Render
		(
			Matrix44::BuildScale(size) * Matrix44(xf),
			primitiveQuad.vb,
			primitiveQuad.primitiveCount,
			primitiveQuad.primitiveType,
			primitiveQuad.stride,
			primitiveQuad.fvf
		);
	}

	void RenderQuad
	(
		const Matrix44& matrix,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid
	)
	{ Render(matrix, color, ti, primitiveQuad); }
	
	void RenderScreenSpaceQuadOutline
	(
		const Vector2& position,
		const Vector2& size,
		const Color& color = Color::White()
	)
	{
		const Matrix44 matrix = GetScreenSpaceMatrix(position.x, position.y, size.x, size.y);
		RenderScreenSpaceQuadOutline(matrix, color);
	}
	
	void RenderScreenSpaceQuad
	(
		const Box2AABB& box2,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid,
		const Color& outlineColor = Color::Black(0)
	)
	{
		const Vector2 size = Vector2(box2.upperBound - box2.lowerBound)/2.0f;
		RenderScreenSpaceQuad
		(
			Vector2(box2.lowerBound + size),
			size,
			color,
			ti,
			outlineColor
		);
	}

	void RenderScreenSpaceQuad
	(
		const XForm2& xf,
		const Vector2& size,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid,
		const Color& outlineColor = Color::Black(0)
	)
	{
		const Matrix44 matrix = GetScreenSpaceMatrix(xf.position.x, xf.position.y, size.x, size.y, xf.angle);
		g_render->RenderScreenSpaceQuad(matrix, color, ti);

		if (outlineColor.a)
			RenderScreenSpace(matrix, outlineColor, GameTexture_Invalid, primitiveQuadOutline);
	}
	
	void RenderScreenSpaceQuadOutline
	(
		const Matrix44& matrix,
		const Color& color = Color::White()
	)
	{ RenderScreenSpace(matrix, color, GameTexture_Invalid, primitiveQuadOutline); }

	void RenderScreenSpaceQuad
	(
		const Matrix44& matrix,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid
	)
	{ RenderScreenSpace(matrix, color, ti, primitiveQuad); }
	
	void RenderScreenSpaceQuad
	(
		const Matrix44& matrix,
		const Color& color,
		LPDIRECT3DTEXTURE9 texture
	)
	{ 
		RenderScreenSpace
		(
			matrix,
			color,
			texture,
			primitiveQuad.vb,
			primitiveQuad.primitiveCount,
			primitiveQuad.primitiveType,
			primitiveQuad.stride,
			primitiveQuad.fvf
		);
	}
		
	void RenderCircle
	(
		const Matrix44& matrix,
		const Color& color = Color::White()
	)
	{ Render(matrix, color, GameTexture_Invalid, primitiveCircle); }

	void RenderCylinder
	(
		const Matrix44& matrix,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid
	)
	{ Render(matrix, color, ti, primitiveCylinder); }

	void RenderCube
	(
		const Matrix44& matrix,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid
	)
	{ Render(matrix, color, ti, primitiveCube); }

	struct RenderPrimitive
	{
		LPDIRECT3DVERTEXBUFFER9 vb;
		UINT primitiveCount;
		UINT vertexCount;
		D3DPRIMITIVETYPE primitiveType;
		UINT stride;
		DWORD fvf;

		RenderPrimitive() :
			vb(NULL),
			primitiveType(D3DPT_LINESTRIP),
			primitiveCount(0),
			vertexCount(0),
			stride(0),
			fvf(0)
		{}

		bool Create
		(
			UINT _primitiveCount,
			UINT _vertexCount,
			D3DPRIMITIVETYPE _primitiveType,
			UINT _stride,
			DWORD _fvf,
			bool dynamic = false
		)
		{
			ASSERT(!vb);

			primitiveCount = _primitiveCount;
			vertexCount = _vertexCount;
			primitiveType = _primitiveType;
			stride = _stride;
			fvf = _fvf;

			const D3DPOOL pool = D3DPOOL_DEFAULT;
			const DWORD usage = (dynamic? (D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC) : D3DUSAGE_WRITEONLY);

			return SUCCEEDED(DXUTGetD3D9Device()->CreateVertexBuffer(vertexCount*stride, usage, fvf, pool, &vb, NULL));
		}

		void SafeRelease() { SAFE_RELEASE(vb); }
	};

	void Render
	(
		const Matrix44& matrix,
		const Color& color,
		GameTextureID ti,
		const RenderPrimitive& rp
	);

	void RenderScreenSpace
	(
		const Matrix44& matrix,
		const Color& color,
		GameTextureID ti,
		const RenderPrimitive& rp
	);

	static int GetMaxGetCircleVertSides() { return primitiveCircleMaxSides; }
	const Vector2* GetCircleVerts(int sides) const { return primitiveCircleVerts[sides]; }

	void ResetTotalSimpleVertsRendered() { totalSimpleVertsRendered = 0; }
	int GetTotalSimpleVertsRendered() const { return totalSimpleVertsRendered; }

public: // tile rendering

	void RenderTile
	(
		const IntVector2& tilePos,
		const IntVector2& tileSize,
		const XForm2& xf,
		const Vector2& size,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid,
		bool cameraCheck = true
	)
	{
		if (!cameraCheck || g_cameraBase->CameraTest(xf.position, fabs(size.x) + fabs(size.y)))
			RenderTile(tilePos, tileSize, Matrix44::BuildScale(size) * Matrix44(xf), color, ti, primitiveQuad); 
	}
	
	void RenderScreenSpaceTile
	(
		const IntVector2& tilePos,
		const IntVector2& tileSize,
		const Vector2& position,
		const Vector2& size,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid
	)
	{
		const Matrix44 matrix = GetScreenSpaceMatrix(position.x, position.y, size.x, size.y);
		RenderScreenSpaceTile(tilePos, tileSize, matrix, color, ti);
	}
	
	void RenderScreenSpaceTile
	(
		const IntVector2& tilePos,
		const IntVector2& tileSize,
		const Matrix44& matrix,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid
	);
	
	void RenderTile
	(
		const IntVector2& tilePos,
		const IntVector2& tileSize,
		const Matrix44& matrix,
		const Color& color,
		GameTextureID ti,
		const RenderPrimitive& rp
	);
	
	void RenderScreenSpaceTile
	(
		const IntVector2& tilePos,
		const IntVector2& tileSize,
		const Box2AABB& box2,
		const Color& color = Color::White(),
		GameTextureID ti = GameTexture_Invalid
	)
	{
		const Vector2 size = Vector2(box2.upperBound - box2.lowerBound)/2.0f;
		RenderScreenSpaceTile
		(
			tilePos,
			tileSize,
			box2.lowerBound + size,
			size,
			color,
			ti
		);
	}
private:

	struct TextureWrapper
	{
		WCHAR name[FILENAME_STRING_LENGTH];
		LPDIRECT3DTEXTURE9 texture;
		GameTextureID tileSheetTi;
		ByteVector2 tilePos;
		ByteVector2 tileSize;
	};
	
	bool LoadTexture(const WCHAR* textureName, GameTextureID ti, TextureWrapper& t);
	bool LoadTextureFromFile(const WCHAR* textureName, GameTextureID ti, TextureWrapper& t);
	void ClearTextureList();
	
	void RenderInternal
	(
		const Matrix44& matrix,
		const Color& color,
		GameTextureID ti,
		const RenderPrimitive& rp
	);

	void Render
	(
		const Matrix44& matrix,
		const Color& color,
		LPDIRECT3DTEXTURE9 texture,
		LPDIRECT3DVERTEXBUFFER9 vb,
		int primitiveCount,
		D3DPRIMITIVETYPE primitiveType,
		UINT stride,
		DWORD fvf
	);

	void Render
	(
		const Matrix44& matrix,
		LPDIRECT3DVERTEXBUFFER9 vb,
		int primitiveCount,
		D3DPRIMITIVETYPE primitiveType,
		UINT stride,
		DWORD fvf
	);

	void RenderScreenSpace
	(
		const Matrix44& matrix,
		const Color& color,
		LPDIRECT3DTEXTURE9 texture,
		LPDIRECT3DVERTEXBUFFER9 vb,
		int primitiveCount,
		D3DPRIMITIVETYPE primitiveType,
		UINT stride,
		DWORD fvf
	);

	void BuildQuad();
	void BuildCircle();
	void BuildCylinder();
	void BuildCube();

    RenderPrimitive primitiveQuad;
    RenderPrimitive primitiveQuadOutline;
    RenderPrimitive primitiveCircle;
	RenderPrimitive primitiveCylinder;
	RenderPrimitive primitiveCube;
	RenderPrimitive primitiveLines;
	RenderPrimitive primitiveTris;
	
	LPDIRECT3DPIXELSHADER9 normalMapShader;
	LPD3DXCONSTANTTABLE normalMapConstantTable;

	static const int primitiveCircleMaxSides = 24;
	Vector2 primitiveCircleVerts[primitiveCircleMaxSides+1][primitiveCircleMaxSides+1];

	TextureWrapper textures[MAX_TEXTURE_COUNT];
	TextureWrapper textureNormals[MAX_TEXTURE_COUNT];
	TextureWrapper textureSpecular[MAX_TEXTURE_COUNT];
	TextureWrapper textureEmissive[MAX_TEXTURE_COUNT];

	bool wasInitialized;

private: // simple vert stuff

	struct SimpleVertex
	{
		Vector3 position;
		DWORD color;
	};

	static const int maxSimpleVerts = 2000;
	int simpleVertLineCount;
	int simpleVertTriCount;
	SimpleVertex simpleVertsLines[maxSimpleVerts];
	SimpleVertex simpleVertsTris[maxSimpleVerts];
	int totalSimpleVertsRendered;
	bool simpleVertsAreAdditive;
};

inline void FrankRender::AddPointToLineVerts(const Vector2& position, DWORD color)
{
	if (simpleVertLineCount >= maxSimpleVerts)
	{
		SimpleVertex vert1 = simpleVertsLines[maxSimpleVerts-1];
		RenderSimpleLineVerts();
		simpleVertsLines[simpleVertLineCount++] = vert1;
	}

	SimpleVertex& vert = simpleVertsLines[simpleVertLineCount++];
	vert.position = position;
	vert.color = color;
}

inline void FrankRender::AddPointToTriVerts(const Vector2& position, DWORD color)
{
	if (simpleVertTriCount >= maxSimpleVerts)
	{
		SimpleVertex vert1 = simpleVertsTris[maxSimpleVerts-2];
		SimpleVertex vert2 = simpleVertsTris[maxSimpleVerts-1];
		RenderSimpleTriVerts();
		simpleVertsTris[simpleVertTriCount++] = vert1;
		simpleVertsTris[simpleVertTriCount++] = vert2;
	}

	SimpleVertex& vert = simpleVertsTris[simpleVertTriCount++];
	vert.position = position;
	vert.color = color;
}

inline void FrankRender::Render
(
	const Matrix44& matrix,
	const Color& color,
	LPDIRECT3DTEXTURE9 texture,
	LPDIRECT3DVERTEXBUFFER9 vb,
	int primitiveCount,
	D3DPRIMITIVETYPE primitiveType,
	UINT stride,
	DWORD fvf
)
{
	if (primitiveCount <= 0)
		return;

	if (color.a == 0)
		return;	// skip if there is no alpha

	IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();

	// set the world transform
	pd3dDevice->SetTransform(D3DTS_WORLD, &matrix.GetD3DXMatrix());

	// set up the color
	const D3DMATERIAL9 material =
	{
		{0, 0, 0, color.a}, 
		{color.r, color.g, color.b, 1}, 
		{0.0f, 0.0f, 0.0f, 1},
		{0.0f, 0.0f, 0.0f, 1},
		0
	};
	pd3dDevice->SetMaterial(&material);

	// set up the texture
	pd3dDevice->SetTexture(0, texture);

	// render the primitive
    pd3dDevice->SetStreamSource(0, vb, 0, stride);
    pd3dDevice->SetFVF(fvf);
	pd3dDevice->DrawPrimitive(primitiveType, 0, primitiveCount);
}

#endif // FRANK_RENDER_H
