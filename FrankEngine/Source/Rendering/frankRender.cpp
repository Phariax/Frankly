////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Rendering
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../rendering/frankRender.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Renderer Globals
*/
////////////////////////////////////////////////////////////////////////////////////////

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_TEX1)
struct CUSTOMVERTEX
{
    D3DXVECTOR3 position;		// vertex position
    FLOAT tu, tv;				// texture coordinates
};

#define D3DFVF_SimpleVertex (D3DFVF_XYZ|D3DFVF_DIFFUSE)

bool g_usePointFiltering = false;
ConsoleCommand(g_usePointFiltering, usePointFiltering);

// setting to scale uvs when using a tile mode to fix filtering issues
// this should scale depending on tile size
float g_tileSetUVFix = 1 - (1/32.0f);
ConsoleCommand(g_tileSetUVFix, tileSetUVFix);

// interpolation used for rendering
float g_interpolatePercent = 0;

////////////////////////////////////////////////////////////////////////////////////////
/*
	Frank Engine Renderer Member Functions
*/
////////////////////////////////////////////////////////////////////////////////////////
FrankRender::FrankRender() :
	wasInitialized(false),
	simpleVertLineCount(0),
	simpleVertTriCount(0),
	totalSimpleVertsRendered(0),
	simpleVertsAreAdditive(false)
{
	ClearTextureList();
}

void FrankRender::ClearTextureList()
{
	// clear the texture list
	for (int i = 0; i < MAX_TEXTURE_COUNT; i++)
	{
		textures[i].texture = NULL;
		textures[i].name[0] = L'\0';
		textureNormals[i].texture = NULL;
		textureNormals[i].name[0] = L'\0';
		textureSpecular[i].texture = NULL;
		textureSpecular[i].name[0] = L'\0';
		textureEmissive[i].texture = NULL;
		textureEmissive[i].name[0] = L'\0';
		textures[i].tileSheetTi = GameTexture_Invalid;
		textures[i].tilePos = ByteVector2(0);
		textures[i].tileSize = ByteVector2(0);
	}
}

void FrankRender::InitDeviceObjects()
{
	if (!wasInitialized)
	{
		wasInitialized = true;
		primitiveQuad.vb = NULL;
		primitiveQuadOutline.vb = NULL;
		primitiveCircle.vb = NULL;
		primitiveCylinder.vb = NULL;
		primitiveCube.vb = NULL;
		primitiveLines.vb = NULL;
		primitiveTris.vb = NULL;
		normalMapShader = NULL;
		normalMapConstantTable = NULL;
	}

	{
		// build circle verts
		for (int i = 1; i <= primitiveCircleMaxSides; i++)
		{
			const float coef = 2*PI/i;
			for (int j = 0; j < i; j++)
				primitiveCircleVerts[i][j] = Vector2::BuildFromAngle(j*coef);
		}
	}

	BuildQuad();
	BuildCircle();
	BuildCylinder();
	BuildCube();

	{
		primitiveLines.Create
		(
			0,								// primitiveCount
			maxSimpleVerts,					// vertexCount
			D3DPT_LINESTRIP,				// primitiveType
			sizeof(SimpleVertex),			// stride
			D3DFVF_SimpleVertex,			// fvf
			true							// dynamic
		);
		primitiveTris.Create
		(
			0,								// primitiveCount
			maxSimpleVerts,					// vertexCount
			D3DPT_TRIANGLESTRIP,			// primitiveType
			sizeof(SimpleVertex),			// stride
			D3DFVF_SimpleVertex,			// fvf
			true							// dynamic
		);
	}

	if (DeferredRender::normalMappingEnable)
	{
		if (!g_render->LoadPixelShader(L"data/shaders/normalMapShader.psh", normalMapShader, normalMapConstantTable))
		{
			if (!g_render->LoadPixelShader(L"normalMapShader.psh", normalMapShader, normalMapConstantTable))
			{
				g_debugMessageSystem.AddError(L"Normal map shader failed to create!   Normal mapping disabled.");
				DeferredRender::normalMappingEnable = false;
				return;
			}
		}
	}
}

void FrankRender::DestroyDeviceObjects()
{
	for (int i = 0; i < MAX_TEXTURE_COUNT; i++)
	{
		SAFE_RELEASE(textures[i].texture);
		textures[i].texture = NULL;
		textures[i].name[0] = 0;

		SAFE_RELEASE(textureNormals[i].texture);
		textureNormals[i].texture = NULL;
		textureNormals[i].name[0] = 0;

		SAFE_RELEASE(textureSpecular[i].texture);
		textureSpecular[i].texture = NULL;
		textureSpecular[i].name[0] = 0;

		SAFE_RELEASE(textureEmissive[i].texture);
		textureEmissive[i].texture = NULL;
		textureEmissive[i].name[0] = 0;
	}

	primitiveQuad.SafeRelease();
	primitiveQuadOutline.SafeRelease();
	primitiveCircle.SafeRelease();
	primitiveCylinder.SafeRelease();
	primitiveCube.SafeRelease();
	primitiveLines.SafeRelease();
	primitiveTris.SafeRelease();
	SAFE_RELEASE(normalMapConstantTable);
	SAFE_RELEASE(normalMapShader);

	ClearTextureList();
}

void FrankRender::SetFiltering()
{
	if (g_usePointFiltering)
	{
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_NONE );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_NONE );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT );
	}
	else
	{
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );
	}
}

LPDIRECT3DTEXTURE9 FrankRender::GetTexture(GameTextureID ti, bool deferredCheck) const
{ 
	if (textures[ti].tileSheetTi)
	{
		// tiled textures don't have their own textures, use their tile sheet's texture
		ti = textures[ti].tileSheetTi;
	}

	if (DeferredRender::GetRenderPassIsNormalMap() && deferredCheck)
		return textureNormals[ti].texture;
	else if (DeferredRender::GetRenderPassIsSpecular() && deferredCheck)
		return textureSpecular[ti].texture;
	else  if (DeferredRender::GetRenderPassIsEmissive() && deferredCheck)
		return textureEmissive[ti].texture;
	else
		return textures[ti].texture;
}

LPDIRECT3DTEXTURE9 FrankRender::GetTexture(const WCHAR* textureName)
{
	int ti;
	for (ti = 0; (ti < MAX_TEXTURE_COUNT && wcscmp(textureName, textures[ti].name)); ti++);

	if (ti != MAX_TEXTURE_COUNT)
	{
		
		if (textures[ti].tileSheetTi)
		{
			// tiled textures don't have their own textures, use their tile sheet's texture
			ti = textures[ti].tileSheetTi;
		}

		return textures[ti].texture;
	}
	else
		return NULL;
}

bool FrankRender::LoadTextureFromFile(const WCHAR* textureName, GameTextureID ti, TextureWrapper& t)
{
	// check if the texture already exists
	if (t.texture)
		return true;

	WCHAR textureNamePng[FILENAME_STRING_LENGTH];
	wcsncpy_s(textureNamePng, FILENAME_STRING_LENGTH, textureName, _TRUNCATE );
	wcscat_s(textureNamePng, FILENAME_STRING_LENGTH, L".png");

	LPDIRECT3DTEXTURE9 texture;
	if
	(
		FAILED
		(
			D3DXCreateTextureFromFile(	
				DXUTGetD3D9Device(),
				textureNamePng,
				&texture
			)
		)
	)
	{
		// try loading from jpg
		WCHAR textureNameJpg[FILENAME_STRING_LENGTH];
		wcsncpy_s(textureNameJpg, FILENAME_STRING_LENGTH, textureName, _TRUNCATE );
		wcscat_s(textureNameJpg, FILENAME_STRING_LENGTH, L".jpg");
		if
		(
			FAILED
			(
				D3DXCreateTextureFromFile(	
					DXUTGetD3D9Device(),
					textureNameJpg,
					&texture
				)
			)
		)
			return(false);
		else
		{
			//g_debugMessageSystem.AddFormatted(L"Loaded local texture \"%s\"", textureNameJpg);
		}
	}
	else	
	{
		//g_debugMessageSystem.AddFormatted(L"Loaded local texture \"%s\"", textureNamePng);
	}

	wcsncpy_s(t.name, FILENAME_STRING_LENGTH, textureName, FILENAME_STRING_LENGTH);
	t.texture = texture;

	return(true);
}

bool FrankRender::LoadTexture(const WCHAR* textureName, GameTextureID ti, TextureWrapper& t)
{
	ASSERT(ti < MAX_TEXTURE_COUNT);

	// check if the texture already exists
	if (t.texture)
		return true;

	// try to load it from file first
	if (LoadTextureFromFile(textureName, ti, t))
		return true;

	{
		// hack: check data folder
		// todo: search subfolders, or way to add list of sub folders to search
		WCHAR textureName2[FILENAME_STRING_LENGTH];
		wcsncpy_s(textureName2, FILENAME_STRING_LENGTH, L"data/", _TRUNCATE );
		wcscat_s(textureName2, FILENAME_STRING_LENGTH, textureName);
		if (LoadTextureFromFile(textureName2, ti, t))
			return true;
	}

	LPDIRECT3DTEXTURE9 texture;
	if
	(
		FAILED
		(
			D3DXCreateTextureFromResource(
				DXUTGetD3D9Device(), 
				NULL,
				textureName, 
				&texture
			)
		)
	)
		return(false);

	wcsncpy_s(t.name, FILENAME_STRING_LENGTH, textureName, FILENAME_STRING_LENGTH);
	t.texture = texture;

	return(true);
}

void FrankRender::SetTextureTile(GameTextureID tileSheetTi, const WCHAR* textureName, GameTextureID tileTi, const ByteVector2& tilePos, const ByteVector2& tileSize)
{
	ASSERT(tileSheetTi < MAX_TEXTURE_COUNT);
	ASSERT(tileTi < MAX_TEXTURE_COUNT);
	ASSERT(!textures[tileTi].texture); // tile sheets should not have their own texture
	
	textures[tileTi].tileSheetTi = tileSheetTi;
	textures[tileTi].tilePos = tilePos;
	textures[tileTi].tileSize = tileSize;
	wcsncpy_s(textures[tileTi].name, FILENAME_STRING_LENGTH, textureName, FILENAME_STRING_LENGTH);
}

bool FrankRender::LoadTexture(const WCHAR* textureName, GameTextureID ti)
{
	ASSERT(ti < MAX_TEXTURE_COUNT);

	// check if the texture already exists
	if (textures[ti].texture)
		return true;
	
	// load diffuse texture
	LoadTexture(textureName, ti, textures[ti]);
	if (!textures[ti].texture)
		g_debugMessageSystem.AddError(L"Could not find texture \"%s\".", textureName);

	// load normal map
	wstring textureNameNormals = textureName + wstring(L"_n");
	LoadTexture(textureNameNormals.c_str(), ti,  textureNormals[ti]);

	// load specular map
	wstring textureNameSpecular = textureName + wstring(L"_s");
	LoadTexture(textureNameSpecular.c_str(), ti,  textureSpecular[ti]);

	// load emissive map
	wstring textureNameEmissive = textureName + wstring(L"_e");
	LoadTexture(textureNameEmissive.c_str(), ti,  textureEmissive[ti]);

	return(true);
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Primitive Building
*/
////////////////////////////////////////////////////////////////////////////////////////


void FrankRender::BuildCylinder()
{
	static const int divisions = 8;

	RenderPrimitive& rp = primitiveCylinder;

	if 
	(
		rp.Create
		(
			2*divisions,				// primitiveCount
			2*(divisions+1),			// vertexCount
			D3DPT_TRIANGLESTRIP,		// primitiveType
			sizeof(CUSTOMVERTEX),		// stride
			D3DFVF_CUSTOMVERTEX			// fvf
		)
	)
	{
		CUSTOMVERTEX* vertices;
		if
		(
			SUCCEEDED
			(
				rp.vb->Lock(0, 0, (VOID**)&vertices, 0)
			)
		)
		{
			for (int i = 0; i <= divisions; ++i)
			{
				const float percent = (float)i / (float)divisions;
				const float theta = percent * 2 * PI;
				const float cosTheta = cosf(theta);
				const float sinTheta = sinf(theta);

				const int offset = i*2;
				vertices[offset].position = D3DXVECTOR3( cosTheta, sinTheta, 1.0f );
				vertices[offset].tu = percent;
				vertices[offset].tv = 0.0f;
				vertices[offset + 1].position = D3DXVECTOR3( cosTheta, sinTheta, -1.0f );
				vertices[offset + 1].tu = percent;
				vertices[offset + 1].tv = 1.0f;
			}

			rp.vb->Unlock();
		}
	}
}


void FrankRender::BuildCircle()
{
	static const int sides = 10;
	RenderPrimitive& rp = primitiveCircle;
	if 
	(
		rp.Create
		(
			sides,					// primitiveCount
			sides+2,				// vertexCount
			D3DPT_TRIANGLEFAN,		// primitiveType
			sizeof(CUSTOMVERTEX),	// stride
			D3DFVF_CUSTOMVERTEX		// fvf
		)
	)
	{
		CUSTOMVERTEX* vertices;
		if
		(
			SUCCEEDED
			(
				rp.vb->Lock(0, 0, (VOID**)&vertices, 0)
			)
		)
		{
			const float coef = 2.0f*PI/sides;
			const float startAngle = 0; // fixed angle for now

			vertices[0].position = Vector3::Zero();
			vertices[0].tu = 0.5f;
			vertices[0].tv = 0.5f;
			++vertices;

			for (int i = 0; i < sides+1; i++)
			{
				const float angle = i*coef + startAngle;
				const Vector3 pos = Vector2::BuildFromAngle(angle);
				vertices[i].position = pos;
				vertices[i].tu = (1 + vertices[i].position.x)/2;
				vertices[i].tv = (1 + vertices[i].position.y)/2;
			}

			rp.vb->Unlock();
		}
	}
}
void FrankRender::BuildQuad()
{
	{
		RenderPrimitive& rp = primitiveQuad;

		if 
		(
			rp.Create
			(
				2,							// primitiveCount
				4,							// vertexCount
				D3DPT_TRIANGLESTRIP,		// primitiveType
				sizeof(CUSTOMVERTEX),		// stride
				D3DFVF_CUSTOMVERTEX			// fvf
			)
		)
		{
			CUSTOMVERTEX* vertices;
			if
			(
				SUCCEEDED
				(
					rp.vb->Lock(0, 0, (VOID**)&vertices, 0)
				)
			)
			{
				vertices[0].position = D3DXVECTOR3( -1.0f,  1.0f, 0.0f );
				vertices[0].tu = 0.0f;
				vertices[0].tv = 0.0f;
				vertices[1].position = D3DXVECTOR3(  1.0f,  1.0f, 0.0f );
				vertices[1].tu = 1.0f;
				vertices[1].tv = 0.0f;
				vertices[2].position = D3DXVECTOR3( -1.0f, -1.0f, 0.0f );
				vertices[2].tu = 0.0f;
				vertices[2].tv = 1.0f;
				vertices[3].position = D3DXVECTOR3(  1.0f, -1.0f, 0.0f );
				vertices[3].tu = 1.0f;
				vertices[3].tv = 1.0f;

				rp.vb->Unlock();
			}
		}
	}
	{
		RenderPrimitive& rp = primitiveQuadOutline;

		if 
		(
			rp.Create
			(
				4,							// primitiveCount
				5,							// vertexCount
				D3DPT_LINESTRIP,			// primitiveType
				sizeof(CUSTOMVERTEX),		// stride
				D3DFVF_CUSTOMVERTEX			// fvf
			)
		)
		{
			CUSTOMVERTEX* vertices;
			if
			(
				SUCCEEDED
				(
					rp.vb->Lock(0, 0, (VOID**)&vertices, 0)
				)
			)
			{
				vertices[0].position = D3DXVECTOR3( -1.0f,  1.0f, 0.0f );
				vertices[1].position = D3DXVECTOR3(  1.0f,  1.0f, 0.0f );
				vertices[2].position = D3DXVECTOR3(  1.0f, -1.0f, 0.0f );
				vertices[3].position = D3DXVECTOR3( -1.0f, -1.0f, 0.0f );
				vertices[4].position = D3DXVECTOR3( -1.0f,  1.0f, 0.0f );

				rp.vb->Unlock();
			}
		}
	}
}

void FrankRender::BuildCube()
{
	RenderPrimitive& rp = primitiveCube;
	if 
	(
		rp.Create
		(
			12,							// primitiveCount
			rp.primitiveCount * 3,		// vertexCount
			D3DPT_TRIANGLELIST,			// primitiveType
			sizeof(CUSTOMVERTEX),		// stride
			D3DFVF_CUSTOMVERTEX			// fvf
		)
	)
	{
		CUSTOMVERTEX* vertices;
		if
		(
			SUCCEEDED
			(
				rp.vb->Lock(0, 0, (VOID**)&vertices, 0)
			)
		)
		{
			static const float cubeSize = 1.0f;
			D3DXVECTOR3 cubeVerts[8];
			cubeVerts[0] = D3DXVECTOR3(  cubeSize,  cubeSize,  cubeSize ); // URF
			cubeVerts[1] = D3DXVECTOR3(  cubeSize, -cubeSize,  cubeSize ); // ULF
			cubeVerts[2] = D3DXVECTOR3( -cubeSize, -cubeSize,  cubeSize ); // DLF
			cubeVerts[3] = D3DXVECTOR3( -cubeSize,  cubeSize,  cubeSize ); // DRF
			cubeVerts[4] = D3DXVECTOR3(  cubeSize,  cubeSize, -cubeSize ); // URB
			cubeVerts[5] = D3DXVECTOR3(  cubeSize, -cubeSize, -cubeSize ); // ULB
			cubeVerts[6] = D3DXVECTOR3( -cubeSize, -cubeSize, -cubeSize ); // DLB
			cubeVerts[7] = D3DXVECTOR3( -cubeSize,  cubeSize, -cubeSize ); // DRB

			// todo: write an algorithm to do this
			const int vertexPositions[] = {
				5, 4, 6, 7, 6, 4,
				7, 4, 3, 0, 3, 4,
				4, 5, 0, 1, 0, 5,
				5, 6, 1, 2, 1, 6,
				6, 7, 2, 3, 2, 7,
				3, 0, 2, 1, 2, 0
			};

			for (UINT i = 0; i < rp.vertexCount; ++i)
				vertices[i].position = cubeVerts[vertexPositions[i]];

			//D3DXVECTOR3 cubeNormals[6];
			//cubeNormals[0] = D3DXVECTOR3(  0.0f,  0.0f, -1.0f );
			//cubeNormals[1] = D3DXVECTOR3(  0.0f,  1.0f,  0.0f );
			//cubeNormals[2] = D3DXVECTOR3(  1.0f,  0.0f,  1.0f );
			//cubeNormals[3] = D3DXVECTOR3(  0.0f, -1.0f,  0.0f );
			//cubeNormals[4] = D3DXVECTOR3( -1.0f,  0.0f,  0.0f );
			//cubeNormals[5] = D3DXVECTOR3(  0.0f,  0.0f,  1.0f );

			for ( DWORD j=0; j<6; j++ )
			{
				vertices[j*6+0].tu = 0.f;
				vertices[j*6+0].tv = 0.f;
				vertices[j*6+1].tu = 1.f;
				vertices[j*6+1].tv = 0.f;
				vertices[j*6+2].tu = 0.f;
				vertices[j*6+2].tv = 1.f;
				vertices[j*6+3].tu = 1.f;
				vertices[j*6+3].tv = 1.f;
				vertices[j*6+4].tu = 0.f;
				vertices[j*6+4].tv = 1.f;
				vertices[j*6+5].tu = 1.f;
				vertices[j*6+5].tv = 0.f;

				//for ( DWORD i=j*6; i<j*6+6; i++ )
				//	vertices[i].normal = cubeNormals[j];
			}

			rp.vb->Unlock();
		}
	}
}

void FrankRender::ReleaseTextures()
{
	for (int i = 0; i < MAX_TEXTURE_COUNT; i++)
	{
		SAFE_RELEASE(textures[i].texture);
		textures[i].texture = NULL;
		textures[i].name[0] = L'\0';
		SAFE_RELEASE(textureNormals[i].texture);
		textureNormals[i].texture = NULL;
		textureNormals[i].name[0] = L'\0';
		SAFE_RELEASE(textureSpecular[i].texture);
		textureSpecular[i].texture = NULL;
		textureSpecular[i].name[0] = L'\0';
		SAFE_RELEASE(textureEmissive[i].texture);
		textureEmissive[i].texture = NULL;
		textureEmissive[i].name[0] = L'\0';
	}
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Fast Rendering Building
*/
////////////////////////////////////////////////////////////////////////////////////////

void FrankRender::RenderSimpleVerts()
{
	RenderSimpleTriVerts();
	RenderSimpleLineVerts();
}

void FrankRender::RenderSimpleLineVerts()
{
	if (simpleVertLineCount <= 1)
	{
		simpleVertLineCount = 0;
		return;
	}

	IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();

	static DWORD savedAmbient = 0;
	if (simpleVertsAreAdditive)
	{
		// additive blend with full brightness
		pd3dDevice->GetRenderState( D3DRS_AMBIENT, &savedAmbient );
		pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x00FFFFFF );
	}

	Color color = Color::White();
	if (DeferredRender::GetRenderPassIsNormalMap())
	{
		color = Color(0.5f, 0.5f, 1.0f, color.a);
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 );
		pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );
	}
	else if (DeferredRender::GetRenderPassIsSpecular())
	{
		color = DeferredRender::SpecularRenderBlock::IsActive()? Color::White(color.a) : Color::Black(color.a);
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 );
		pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );
	}
	else if (DeferredRender::GetRenderPassIsEmissive())
	{
		// hack, turn off lighting only if we are additively blending to the emissive buffer
		DWORD blend;
		DXUTGetD3D9Device()->GetRenderState(D3DRS_DESTBLEND, &blend);
		if (blend == D3DBLEND_ONE)
			pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	}
	else if (!DeferredRender::GetRenderPassIsShadow())
	{
		// normally simple verts don't use directx lighting
		pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	}

	totalSimpleVertsRendered += simpleVertLineCount;
	SimpleVertex* lockedVerts;
	primitiveLines.vb->Lock(0, 0, (VOID**)&lockedVerts, D3DLOCK_DISCARD);
	for(int i = 0; i < simpleVertLineCount; ++i)
	{
		SimpleVertex& vert = simpleVertsLines[i];
		*lockedVerts = vert;
		++lockedVerts;
	}
	primitiveLines.vb->Unlock();
	primitiveLines.primitiveCount = simpleVertLineCount - 1;
	Render(Matrix44::Identity(), color, GameTexture_Invalid, primitiveLines);
	simpleVertLineCount = 0;
	
	if (DeferredRender::GetRenderPassIsNormalMap() || DeferredRender::GetRenderPassIsSpecular())
	{
		// set stuff back to what it was
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
		pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
	}
	
	pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

	if (simpleVertsAreAdditive)
	{
		// set stuff back to what it was
		pd3dDevice->SetRenderState( D3DRS_AMBIENT, savedAmbient );
		pd3dDevice->SetRenderState (D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	}
}

void FrankRender::RenderSimpleTriVerts()
{
	if (simpleVertTriCount <= 2)
	{
		simpleVertTriCount = 0;
		return;
	}
	
	IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();

	DWORD savedAmbient = 0;
	if (simpleVertsAreAdditive && !DeferredRender::GetRenderPassIsNormalMap())
	{
		// additive blend with full brightness
		pd3dDevice->GetRenderState( D3DRS_AMBIENT, &savedAmbient );
		pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
		pd3dDevice->SetRenderState( D3DRS_AMBIENT, 0x00FFFFFF );
	}

	Color color = Color::White();
	if (DeferredRender::GetRenderPassIsNormalMap())
	{
		color = Color(0.5f, 0.5f, 1.0f, color.a);
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 );
		pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );
	}
	else if (DeferredRender::GetRenderPassIsSpecular())
	{
		color = DeferredRender::SpecularRenderBlock::IsActive()? Color::White(color.a) : Color::Black(color.a);
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG2 );
		pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG2 );
	}
	else if (DeferredRender::GetRenderPassIsEmissive())
	{
		// hack, turn off lighting only if we are additively blending to the emissive buffer
		DWORD blend;
		DXUTGetD3D9Device()->GetRenderState(D3DRS_DESTBLEND, &blend);
		if (blend == D3DBLEND_ONE)
			pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	}
	else if (!DeferredRender::GetRenderPassIsShadow())
	{
		// normally simple verts don't use directx lighting
		pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	}

	totalSimpleVertsRendered += simpleVertTriCount;
	SimpleVertex* lockedVerts;
	primitiveTris.vb->Lock(0, 0, (VOID**)&lockedVerts, D3DLOCK_DISCARD);
	for(int i = 0; i < simpleVertTriCount; ++i)
	{
		SimpleVertex& vert = simpleVertsTris[i];
		*lockedVerts = vert;
		++lockedVerts;
	}
	primitiveTris.vb->Unlock();
	primitiveTris.primitiveCount = simpleVertTriCount - 2;
	Render(Matrix44::Identity(), color, GameTexture_Invalid, primitiveTris);
	simpleVertTriCount = 0;
	
	if (DeferredRender::GetRenderPassIsNormalMap() || DeferredRender::GetRenderPassIsSpecular())
	{
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
		pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
	}
	
	pd3dDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

	if (simpleVertsAreAdditive && !DeferredRender::GetRenderPassIsNormalMap())
	{
		pd3dDevice->SetRenderState( D3DRS_AMBIENT, savedAmbient );
		pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	}
}

void FrankRender::DrawPolygon(const Vector2* vertices, int vertexCount, const DWORD color)
{
	const Vector2 startPos = vertices[0];
	CapLineVerts(startPos);

	for (int32 i = 0; i < vertexCount; ++i)
		AddPointToLineVerts(vertices[i], color);

	AddPointToLineVerts(startPos, color);
	CapLineVerts(startPos);
}

void FrankRender::DrawPolygon(const XForm2& xf, const Vector2* vertices, int vertexCount, const DWORD color)
{
	static const int maxPolygonVertCount = 256;
	static Vector2 transformedVerts[maxPolygonVertCount];
	ASSERT(vertexCount <= maxPolygonVertCount);

	// transform all the verts
	for (int i = 0; i <  vertexCount; ++i)
		transformedVerts[i] = xf.TransformCoord(vertices[i]);

	DrawPolygon(transformedVerts, vertexCount, color);
}

void FrankRender::DrawSolidPolygon(const Vector2* vertices, int vertexCount, const DWORD color)
{
	const Vector2 startPos = vertices[0];

	CapTriVerts(startPos);
	AddPointToTriVerts(startPos, color);

	int leftPos = vertexCount-1;
	int rightPos = 1;

	while (1)
	{
		AddPointToTriVerts(vertices[rightPos++], color);
		if (rightPos > leftPos)
			break;

		AddPointToTriVerts(vertices[leftPos--], color);
		if (leftPos < rightPos)
			break;
	}

	const int endVert = vertexCount/2 + vertexCount % 2;
	CapTriVerts(vertices[endVert]);
}

void FrankRender::DrawSolidPolygon(const XForm2& xf, const Vector2* vertices, int vertexCount, const DWORD color)
{
	static const int maxPolygonVertCount = 256;
	static Vector2 transformedVerts[maxPolygonVertCount];
	ASSERT(vertexCount <= maxPolygonVertCount);

	// transform all the verts
	for (int i = 0; i <  vertexCount; ++i)
		transformedVerts[i] = xf.TransformCoord(vertices[i]);

	DrawSolidPolygon(transformedVerts, vertexCount, color);
}

void FrankRender::DrawOutlinedPolygon(const XForm2& xf, const Vector2* vertices, int vertexCount, const DWORD color, const DWORD outlineColor)
{
	static const int maxPolygonVertCount = 256;
	static Vector2 transformedVerts[maxPolygonVertCount];
	ASSERT(vertexCount <= maxPolygonVertCount);

	// transform all the verts
	for (int i = 0; i <  vertexCount; ++i)
		transformedVerts[i] = xf.TransformCoord(vertices[i]);

	DrawSolidPolygon(transformedVerts, vertexCount, color);
	DrawPolygon(transformedVerts, vertexCount, outlineColor);
}

void FrankRender::DrawCone(const XForm2& xf, float radius, float coneAngle, const DWORD color, int divisions)
{
	CapLineVerts(xf.position);
	AddPointToLineVerts(xf.position, color);

	for(int i = 0; i <= divisions; ++i)
	{
		float p = float(i) / float(divisions);
		float angle = Lerp(p, -coneAngle, coneAngle);
		const Vector2 point = xf.position + radius*Vector2::BuildFromAngle(xf.angle + angle);	
		AddPointToLineVerts(point, color);
	}
	
	AddPointToLineVerts(xf.position, color);
	CapLineVerts(xf.position);
}

void FrankRender::DrawCircle(const XForm2& xf, const Vector2& size, const DWORD color, int sides)
{
	ASSERT(sides <= primitiveCircleMaxSides);

	const Vector2 startPos = xf.TransformCoord(primitiveCircleVerts[sides][0]*size);
	CapLineVerts(startPos);
	AddPointToLineVerts(startPos, color);

	for (int i = 1; i < sides; i++)
		AddPointToLineVerts(xf.TransformCoord(primitiveCircleVerts[sides][i]*size), color);

	AddPointToLineVerts(startPos, color);
	CapLineVerts(startPos);
}

void FrankRender::DrawSolidCircle(const XForm2& xf, const Vector2& size, const DWORD color, int sides)
{
	ASSERT(sides <= primitiveCircleMaxSides);

	const Vector2 startPos = xf.TransformCoord(primitiveCircleVerts[sides][0]*size);
	CapTriVerts(startPos);
	AddPointToTriVerts(startPos, color);

	int leftPos = sides-1;
	int rightPos = 1;
	while (1)
	{
		AddPointToTriVerts(xf.TransformCoord(primitiveCircleVerts[sides][rightPos++]*size), color);
		if (rightPos > leftPos)
			break;

		AddPointToTriVerts(xf.TransformCoord(primitiveCircleVerts[sides][leftPos--]*size), color);
		if (leftPos < rightPos)
			break;
	}

	const Vector2 endPos = xf.TransformCoord(primitiveCircleVerts[sides][sides/2]*size);
	CapTriVerts(endPos);
}

void FrankRender::DrawOutlinedCircle(const XForm2& xf, const Vector2& size, const DWORD color, const DWORD outlineColor, int sides)
{
	DrawSolidCircle(xf, size, color, sides);
	DrawCircle(xf, size, outlineColor, sides);
}

void FrankRender::DrawThickSegment(const Line2& l, float thickness, const DWORD color)
{
	Vector2 center = 0.5f*l.p1 + 0.5f*l.p2;
	Vector2 direction = (l.p2 - l.p1);
	float length = direction.MagnitudeAndNormalize();
	XForm2 xf(center, direction.GetAngle());
	DrawSolidBox(xf, Vector2(thickness, length*0.5f), color);
}

void FrankRender::RenderQuadLine
(
	const Line2& l,
	float thickness,
	const Color& color,
	LPDIRECT3DTEXTURE9 texture
)
{
	Vector2 center = 0.5f*l.p1 + 0.5f*l.p2;
	Vector2 direction = (l.p2 - l.p1);
	float length = direction.MagnitudeAndNormalize();
	XForm2 xf(center, direction.GetAngle());
	RenderQuad(xf, Vector2(thickness, length*0.5f), color, texture);
}

void FrankRender::DrawSegment(const Vector2& p1, const Vector2& p2, const DWORD color)
{
	CapLineVerts(p1);
	AddPointToLineVerts(p1, color);
	AddPointToLineVerts(p2, color);
	CapLineVerts(p2);
}

void FrankRender::DrawSegment(const XForm2& xf, const Vector2& p1, const Vector2& p2, const DWORD color)
{
	const Vector2 p1t = xf.TransformCoord(p1);
	const Vector2 p2t = xf.TransformCoord(p2);

	CapLineVerts(p1t);
	AddPointToLineVerts(p1t, color);
	AddPointToLineVerts(p2t, color);
	CapLineVerts(p2t);
}

void FrankRender::DrawXForm(const XForm2& xf, float radius, const DWORD color) 
{ 
	DrawSegment(xf, Vector2(0,0), Vector2(radius,0), color);
	DrawSegment(xf, Vector2(0,0), Vector2(0,radius), color);
}

void FrankRender::DrawBox(const XForm2& xf, const Vector2& size, const DWORD color)
{
	const Vector2 vertices[4] =
	{
		xf.TransformCoord(Vector2(size.x, size.y)),
		xf.TransformCoord(Vector2(size.x, -size.y)),
		xf.TransformCoord(Vector2(-size.x, -size.y)),
		xf.TransformCoord(Vector2(-size.x, size.y)),
	};
	DrawPolygon(vertices, 4, color);
}

void FrankRender::DrawSolidBox(const XForm2& xf, const Vector2& size, const DWORD color)
{
	const Vector2 vertices[4] =
	{
		xf.TransformCoord(Vector2(size.x, size.y)),
		xf.TransformCoord(Vector2(size.x, -size.y)),
		xf.TransformCoord(Vector2(-size.x, -size.y)),
		xf.TransformCoord(Vector2(-size.x, size.y)),
	};
	DrawSolidPolygon(vertices, 4, color);
}

void FrankRender::DrawOutlinedBox(const XForm2& xf, const Vector2& size, const DWORD color, const DWORD outlineColor)
{
	const Vector2 vertices[4] =
	{
		xf.TransformCoord(Vector2(size.x, size.y)),
		xf.TransformCoord(Vector2(size.x, -size.y)),
		xf.TransformCoord(Vector2(-size.x, -size.y)),
		xf.TransformCoord(Vector2(-size.x, size.y)),
	};
	DrawSolidPolygon(vertices, 4, color);
	DrawPolygon(vertices, 4, outlineColor);
}

void FrankRender::DrawAxisAlignedBox(const Box2AABB& box, const DWORD color)
{
	const Vector2 vertices[4] =
	{
		Vector2(box.lowerBound.x, box.lowerBound.y),
		Vector2(box.upperBound.x, box.lowerBound.y),
		Vector2(box.upperBound.x, box.upperBound.y),
		Vector2(box.lowerBound.x, box.upperBound.y),
	};
	DrawPolygon(vertices, 4, color);
}

IntVector2 FrankRender::GetTextureSize(GameTextureID ti) const
{
	LPDIRECT3DTEXTURE9 texture = GetTexture(ti);
	if (!texture)
		return IntVector2(1, 1);	// null texture
	return GetTextureSize(texture);
}

IntVector2 FrankRender::GetTextureSize(LPDIRECT3DTEXTURE9 texture) const
{
	ASSERT(texture);
	D3DSURFACE_DESC desc;
	texture->GetLevelDesc(0, &desc);
	return IntVector2(desc.Width, desc.Height);
}

D3DFORMAT FrankRender::GetTextureFormat(LPDIRECT3DTEXTURE9 texture) const
{
	ASSERT(texture);
	D3DSURFACE_DESC desc;
	texture->GetLevelDesc(0, &desc);
	return desc.Format;
}

void FrankRender::RenderScreenSpace
(
	const Matrix44& matrix,
	const Color& color,
	GameTextureID ti,
	const RenderPrimitive& rp
)
{
	const TextureWrapper& textureWrapper = textures[ti];
	if (textureWrapper.tileSheetTi != GameTexture_Invalid)
	{
		// handle tiled textures
		DWORD oldTT0, oldTT1;
		IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();
		pd3dDevice->GetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, &oldTT0);
		pd3dDevice->GetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, &oldTT1);
		pd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
		pd3dDevice->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

		{
			// set up the texture transform
			const Vector2 scale = 1.0f / Vector2(textureWrapper.tileSize);
			Matrix44 matrix = Matrix44::BuildScale(scale.x, scale.y, 0);
			D3DMATRIX& d3dmatrix = matrix.GetD3DXMatrix();
		
			const Vector2 tilePosFloat = Vector2(textureWrapper.tilePos) * scale;
			d3dmatrix._31 = tilePosFloat.x;
			d3dmatrix._32 = tilePosFloat.y;
			pd3dDevice->SetTransform(D3DTS_TEXTURE0, &d3dmatrix);
			pd3dDevice->SetTransform(D3DTS_TEXTURE1, &d3dmatrix);
		}

		RenderScreenSpace
		(
			matrix,
			color,
			GetTexture(textureWrapper.tileSheetTi),
			rp.vb,
			rp.primitiveCount,
			rp.primitiveType,
			rp.stride,
			rp.fvf
		);

		pd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, oldTT0);
		pd3dDevice->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, oldTT1);
	}
	else
	{
		RenderScreenSpace
		(
			matrix,
			color,
			GetTexture(ti),
			rp.vb,
			rp.primitiveCount,
			rp.primitiveType,
			rp.stride,
			rp.fvf
		);
	}
}

void FrankRender::RenderScreenSpace
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

	// save the camera transforms
	D3DXMATRIX viewMatrixOld, projectionMatrixOld;
	pd3dDevice->GetTransform(D3DTS_VIEW, &viewMatrixOld);
	pd3dDevice->GetTransform(D3DTS_PROJECTION, &projectionMatrixOld);

	// set camera transforms to identity
	pd3dDevice->SetTransform(D3DTS_VIEW, &Matrix44::Identity().GetD3DXMatrix());
	pd3dDevice->SetTransform(D3DTS_PROJECTION, &Matrix44::Identity().GetD3DXMatrix());

	// set the world transform
	Matrix44 matrixDeviceBufferOffset = Matrix44::BuildScale(2.0f / g_backBufferWidth, -2.0f / g_backBufferHeight, 0);
	matrixDeviceBufferOffset += Vector3(-1.0f, 1.0f, 0);
	const Matrix44 matrixScreenSpace = matrix * matrixDeviceBufferOffset;
	pd3dDevice->SetTransform(D3DTS_WORLD, &matrixScreenSpace.GetD3DXMatrix());

	// set up the texture
	pd3dDevice->SetTexture(0, texture);

	// set up the color
	const D3DMATERIAL9 material =
	{
		{color.r, color.g, color.b, color.a}, 
		{color.r, color.g, color.b, 1.0f}, 
		{0.0f, 0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f, 0.0f},
		0
	};
	pd3dDevice->SetMaterial(&material);

	// render the primitive
    pd3dDevice->SetStreamSource(0, vb, 0, stride);
    pd3dDevice->SetFVF(fvf);
	pd3dDevice->DrawPrimitive(primitiveType, 0, primitiveCount);

	// set the camera transforms back to what they were
	pd3dDevice->SetTransform(D3DTS_VIEW, &viewMatrixOld);
	pd3dDevice->SetTransform(D3DTS_PROJECTION, &projectionMatrixOld);
}

void FrankRender::RenderTile
(
	const IntVector2& tilePos,
	const IntVector2& tileSize,
	const Matrix44& matrix,
	const Color& color,
	GameTextureID ti,
	const RenderPrimitive& rp
)
{
	ASSERT(textures[ti].tileSheetTi == GameTexture_Invalid); // tile's sheet should not be a tile in another sheet

	IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();

	pd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	pd3dDevice->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

	{
		// fix issue with texture filtering
		const Vector2 inverseTileSize = 1.0f / Vector2(tileSize);
		const Vector2 uvFix = (g_tileSetUVFix == 0)? Vector2(0) : Vector2(1 - g_tileSetUVFix) * inverseTileSize;

		// set up the texture transform
		const Vector2 scale = inverseTileSize - uvFix;
		Matrix44 matrix = Matrix44::BuildScale(scale.x, scale.y, 0);
		D3DMATRIX& d3dmatrix = matrix.GetD3DXMatrix();
		
		const Vector2 tilePosFloat = Vector2(tilePos) *inverseTileSize;
		d3dmatrix._31 = tilePosFloat.x + uvFix.x/2;
		d3dmatrix._32 = tilePosFloat.y + uvFix.y/2;
		pd3dDevice->SetTransform(D3DTS_TEXTURE0, &d3dmatrix);
		pd3dDevice->SetTransform(D3DTS_TEXTURE1, &d3dmatrix);
	}

	RenderInternal(matrix, color, ti, rp); 
		
	pd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
	pd3dDevice->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
}

void FrankRender::RenderScreenSpaceTile
(
	const IntVector2& tilePos,
	const IntVector2& tileSize,
	const Matrix44& matrix,
	const Color& color,
	GameTextureID ti
)
{
	// handle tiled textures
	DWORD oldTT0, oldTT1;
	IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();
	pd3dDevice->GetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, &oldTT0);
	pd3dDevice->GetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, &oldTT1);
	pd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);
	pd3dDevice->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2);

	{
		// fix issue with texture filtering
		const Vector2 inverseTileSize = 1.0f / Vector2(tileSize);
		const Vector2 uvFix = (g_tileSetUVFix == 0)? Vector2(0) : Vector2(1 - g_tileSetUVFix) * inverseTileSize;

		// set up the texture transform
		const Vector2 scale = inverseTileSize - uvFix;
		Matrix44 matrix = Matrix44::BuildScale(scale.x, scale.y, 0);
		D3DMATRIX& d3dmatrix = matrix.GetD3DXMatrix();
		
		const Vector2 tilePosFloat = Vector2(tilePos) *inverseTileSize;
		d3dmatrix._31 = tilePosFloat.x + uvFix.x/2;
		d3dmatrix._32 = tilePosFloat.y + uvFix.y/2;
		pd3dDevice->SetTransform(D3DTS_TEXTURE0, &d3dmatrix);
		pd3dDevice->SetTransform(D3DTS_TEXTURE1, &d3dmatrix);
	}

	const RenderPrimitive& rp = primitiveQuad;
	RenderScreenSpace
	(
		matrix,
		color,
		GetTexture(ti),
		rp.vb,
		rp.primitiveCount,
		rp.primitiveType,
		rp.stride,
		rp.fvf
	);

	pd3dDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, oldTT0);
	pd3dDevice->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, oldTT1);
}

void FrankRender::Render
(
	const Matrix44& matrix,
	LPDIRECT3DVERTEXBUFFER9 vb,
	int primitiveCount,
	D3DPRIMITIVETYPE primitiveType,
	UINT stride,
	DWORD fvf
)
{
	if (primitiveCount <= 0)
		return;

	IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();

	// set the world transform
	pd3dDevice->SetTransform(D3DTS_WORLD, &matrix.GetD3DXMatrix());

	// render the primitive
    pd3dDevice->SetStreamSource(0, vb, 0, stride);
    pd3dDevice->SetFVF(fvf);
	pd3dDevice->DrawPrimitive(primitiveType, 0, primitiveCount);
}

void FrankRender::Render
(
	const Matrix44& matrix,
	const Color& color,
	GameTextureID ti,
	const RenderPrimitive& rp
)
{
	const TextureWrapper& textureWrapper = textures[ti];
	if (textureWrapper.tileSheetTi != GameTexture_Invalid)
		RenderTile(textureWrapper.tilePos, textureWrapper.tileSize, matrix, color, textureWrapper.tileSheetTi, rp);
	else
		RenderInternal(matrix, color, ti, rp); 
}

inline void FrankRender::RenderInternal
(
	const Matrix44& matrix,
	const Color& color,
	GameTextureID ti,
	const RenderPrimitive& rp
)
{
	IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();
	LPDIRECT3DTEXTURE9 texture = textures[ti].texture;

	if (DeferredRender::GetRenderPassIsNormalMap() && (&rp != &primitiveLines) && (&rp != &primitiveTris))
	{
		if (!texture)
		{
			// if there is no texture just render z facing normals
			Render
			(
				matrix,
				Color(0.5f, 0.5f, 1.0f, color.a),
				texture,
				rp.vb,
				rp.primitiveCount,
				rp.primitiveType,
				rp.stride,
				rp.fvf
			);
			return;
		}
		
		if (!textureNormals[ti].texture)
		{
			// if there is no normals just render z facing normals with texture's alpha
			pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );

			Render
			(
				matrix,
				Color(0.5f, 0.5f, 1.0f, color.a),
				texture,
				rp.vb,
				rp.primitiveCount, 
				rp.primitiveType,
				rp.stride,
				rp.fvf
			);

			pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
			return;
		}
		
		// build the normal map transform
		{
			// get rid of position and fix z scale
			D3DXMATRIX m = matrix.GetD3DXMatrix();
			m._33 = 1;
			m._41 = 0;
			m._42 = 0;

			// invert to get local transform
			D3DXMATRIX m2;
			D3DXMatrixInverse(&m2, NULL, &m);

			// get rid of scale component
			const Vector2 v1 = Vector2(m2._11, m2._12).Normalize();
			m2._11 = v1.x;
			m2._12 = v1.y;
			const Vector2 v2 = Vector2(m2._21, m2._22).Normalize();
			m2._21 = v2.x;
			m2._22 = v2.y;
			
			// check for flipped texture
			const bool flip = Vector2::IsClockwise(Vector2(matrix.GetRight()), Vector2(matrix.GetUp()));
			if (flip)
			{
				m2._12 = -m2._12;
				m2._21 = -m2._21;
			}

			normalMapConstantTable->SetMatrix(pd3dDevice, "transformMatrix", &m2);
		}
		
		// use all white with diffuse alpha
		const Color c = Color(1.0f, 1.0f, 1.0f, color.a);

		const D3DXVECTOR4 colorVector = c;
		normalMapConstantTable->SetVector(pd3dDevice, "diffuseColor", &colorVector);
		
		float diffuseNormalAlphaPercent = 1;
		if (DeferredRender::DiffuseNormalAlphaRenderBlock::IsActive())
			diffuseNormalAlphaPercent = DeferredRender::DiffuseNormalAlphaRenderBlock::GetAlphaScale();

		normalMapConstantTable->SetFloat(pd3dDevice, "diffuseNormalAlphaPercent", diffuseNormalAlphaPercent);

		pd3dDevice->SetPixelShader(normalMapShader);
		pd3dDevice->SetTexture(1, textureNormals[ti].texture);

		Render
		(
			matrix,
			Color::White(),
			texture,
			rp.vb,
			rp.primitiveCount,
			rp.primitiveType,
			rp.stride,
			rp.fvf
		);

		pd3dDevice->SetPixelShader(NULL);
		pd3dDevice->SetTexture(1, NULL);
		return;
	}
	else if (DeferredRender::GetRenderPassIsSpecular())
	{
		const bool isSpecularBlock = DeferredRender::SpecularRenderBlock::IsActive();

		if (!texture)
		{
			// if there is no texture render all black
			Render
			(
				matrix,
				isSpecularBlock? Color::White(color.a) : Color::Black(color.a),
				texture,
				rp.vb,
				rp.primitiveCount,
				rp.primitiveType,
				rp.stride,
				rp.fvf
			);
			return;
		}

		// if there is no specular map use black, otherwise just use alpha
		const Color c = (!textureSpecular[ti].texture && !isSpecularBlock)? Color(0.0f, 0.0f, 0.0f, color.a) : color;
		
		pd3dDevice->SetTexture(1, textureSpecular[ti].texture);
		pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_CURRENT );

		Render
		(
			matrix,
			c,
			texture,
			rp.vb,
			rp.primitiveCount,
			rp.primitiveType,
			rp.stride,
			rp.fvf
		);

		pd3dDevice->SetTexture(1, NULL);
		pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
		pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
		pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		return;
	}
	else if (DeferredRender::GetRenderPassIsEmissive() && texture && textureEmissive[ti].texture)
	{
		DWORD oldAmbient;
		DXUTGetD3D9Device()->GetRenderState( D3DRS_AMBIENT, &oldAmbient );
		if (oldAmbient != 0x00FFFFFF)
		{
			// hack: if we weren't using an emissive override, use the emissive texture if we have one
			DXUTGetD3D9Device()->SetRenderState( D3DRS_AMBIENT, 0x00FFFFFF );
		
			pd3dDevice->SetTexture(1, textureEmissive[ti].texture);
			pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
			pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );

			Render
			(
				matrix,
				color,
				textures[ti].texture,
				rp.vb,
				rp.primitiveCount,
				rp.primitiveType,
				rp.stride,
				rp.fvf
			);
			
			pd3dDevice->SetTexture(1, NULL);
			pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_MODULATE );
			pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP, D3DTOP_MODULATE );
			DXUTGetD3D9Device()->SetRenderState( D3DRS_AMBIENT, oldAmbient );
			return;
		}
	}
	else if (DeferredRender::GetRenderPassIsShadow() && (DeferredRender::GetRenderPassIsDirectionalShadow() || !DeferredRender::TransparentRenderBlock::IsActive()))
	{
		// hack: don't allow transparent render block for directional shadow pass

		// make it use the shadow color rather then the texture by default
		Color shadowColor = DeferredRender::defaultShadowColor;
		shadowColor.a *= color.a;
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_SELECTARG2 );
		Render
		(
			matrix,
			shadowColor,
			texture,
			rp.vb,
			rp.primitiveCount,
			rp.primitiveType,
			rp.stride,
			rp.fvf
		);
		pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		return;
	}

	Render
	(
		matrix,
		color,
		texture,
		rp.vb,
		rp.primitiveCount,
		rp.primitiveType,
		rp.stride,
		rp.fvf
	);
}

bool FrankRender::LoadPixelShader(const WCHAR* name, LPDIRECT3DPIXELSHADER9& shader, LPD3DXCONSTANTTABLE& constants)
{
	// try to load pixel shader from disk
	LPD3DXBUFFER code = NULL;
	HRESULT result = D3DXCompileShaderFromFile
	(
		name,				//filepath
		NULL,				//defines         
		NULL,				//includes           
		"ps_main",			//main function      
		"ps_2_0",			//shader profile     
		0,					//flags              
		&code,				//compiled operations
		NULL,				//errors
		&constants			//constants
	);

	if (!SUCCEEDED(result))
	{
		// load from resource
		result = D3DXCompileShaderFromResource
		(
			NULL,
			name,
			NULL,			//defines         
			NULL,			//includes           
			"ps_main",		//main function      
			"ps_2_0",		//shader profile     
			0,				//flags              
			&code,			//compiled operations
			NULL,			//errors
			&constants		//constants
		);
	}
	else
	{
		//g_debugMessageSystem.AddFormatted(L"Loaded local pixel shader \"%s\"", name);
	}
	D3DXERR_INVALIDDATA;

	if (SUCCEEDED(result))
	{
		DXUTGetD3D9Device()->CreatePixelShader((DWORD*)code->GetBufferPointer(), &shader);
		code->Release();
		return true;
	}
	else
		return false;
}

void FrankRender::SetSimpleVertsAreAdditive(bool additive)
{
	if (simpleVertsAreAdditive == additive)
		return;

	RenderSimpleVerts();

	simpleVertsAreAdditive = additive;
}

Vector2 FrankRender::WorldSpaceToScreenSpace(const Vector2& p)
{
	IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();
	Matrix44 matrixView;
	Matrix44 matrixProjection;
	pd3dDevice->GetTransform( D3DTS_VIEW, &matrixView.GetD3DXMatrix() );
	pd3dDevice->GetTransform( D3DTS_PROJECTION, &matrixProjection.GetD3DXMatrix() );

	const Matrix44 matrixFinal = matrixView * matrixProjection;
	Vector2 localPos(matrixFinal.TransformCoord(p));
	localPos += Vector2(1, -1);
	localPos *= Vector2(0.5f*g_backBufferWidth, -0.5f*g_backBufferHeight);
	return localPos;
	
	/*IDirect3DDevice9* pd3dDevice = DXUTGetD3D9Device();
	const Camera* camera = g_cameraBase;

	// set the view transform
	const XForm2 xfViewInterp = camera->GetInterpolatedXForm().Inverse();
	const Matrix44 matrixView = Matrix44::BuildXFormZ(xfViewInterp.position, xfViewInterp.angle, 10.0f);

    // set the projection matrix
	const float height = camera->GetZoomInterpoalted() * camera->GetAspectFix();
	const float width = height * camera->GetAspectRatio();
	Matrix44 matrixProjection;
	D3DXMatrixOrthoLH( &matrixProjection.GetD3DXMatrix(), width, height, 1.0f, 1000.0f );

	const Matrix44 matrixFinal = matrixView * matrixProjection;
	Vector2 localPos(matrixFinal.TransformCoord(p));
	localPos *= Vector2(-1, 1);
	localPos *= Vector2(0.5f*g_backBufferWidth, 0.5f*g_backBufferHeight);
	return localPos;*/
}
