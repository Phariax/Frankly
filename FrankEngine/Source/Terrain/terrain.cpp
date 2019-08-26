////////////////////////////////////////////////////////////////////////////////////////
/*
	Terrain
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../terrain/terrainSurface.h"
#include "../terrain/terrain.h"
#include "../editor/objectEditor.h"
#include <fstream>

////////////////////////////////////////////////////////////////////////////////////////

// terrain settings
int Terrain::dataVersion			= 11;
int Terrain::fullSize				= 20;				// how many patches per terrain
int Terrain::patchSize				= 16;				// how many tiles per patch
int Terrain::patchLayers			= 2;				// how many layers per patch
int Terrain::physicsLayer			= 0;				// the layer to create physics for
int Terrain::windowSize				= 1;				// size of the tetrain stream window
int Terrain::renderWindowSize		= 1;				// size of the tetrain render window
Color Terrain::debugOutlineColor1	= Color::White();	// color used for lines around tiles when editing
Color Terrain::debugOutlineColor2	= Color::Black();	// color used for lines around tiles when editing
Vector2 Terrain::gravity			= Vector2::Zero();	// acceleartion due to gravity
ConsoleCommand(Terrain::gravity, gravity);
WCHAR Terrain::terrainFilename[256]	= defaultTerrainFilename;
float Terrain::restitution			= 0;				// how bouncy is the terrain
float Terrain::friction				= 0.3f;				// how much friction for terrain
int Terrain::maxProxies				= 10000;			// limit on how many terrain proxies can be made

// planet terrain config
bool Terrain::isCircularPlanet			= false;	// should terrain be treated like a circular planet?
float Terrain::planetRadius				= 150;		// radius or height if non-circular
float Terrain::planetGravityConstant	= 0;		// gravity constant or global downward gravity if non-circular
float Terrain::planetMinAltitude		= 1000;		// sea level, use to similate round planets
float Terrain::planetMaxAltitude		= 2000;		// atmosphere height, use to similate round planets
bool Terrain::planetAltitudeDamping	= false;	// fade off linear damping as we exit planet atmosphere

// tile sets
int Terrain::tileSetCount				= 0;				// how many tile sets there are
GameTextureID Terrain::tileSets[maxTileSets] = { GameTexture_Invalid };

// if it should use polys or edge loops for terrain physics
bool Terrain::usePolyPhysics = true;

// combine physics of tiles to reduce proxies
bool Terrain::combineTileShapes = true;

// stream a window around the player
bool Terrain::enableStreaming = true;
static bool streamDebug = false;

// set the handle to a large enough value to cover any objects we might create on startup
static const GameObjectHandle firstStartHandle = 10000; 

ConsoleCommand(Terrain::isCircularPlanet, isCircularPlanet);
ConsoleCommand(Terrain::planetRadius, planetRadius);
ConsoleCommand(Terrain::planetGravityConstant, planetGravityConstant);
ConsoleCommand(Terrain::planetMinAltitude, planetMinAltitude);
ConsoleCommand(Terrain::planetMaxAltitude, planetMaxAltitude);
ConsoleCommand(Terrain::terrainFilename, terrainFilename);
ConsoleCommand(Terrain::restitution, terrainRestitution);
ConsoleCommand(Terrain::friction, terrainFriction);
ConsoleCommand(Terrain::usePolyPhysics, terrainPolyPhysics);
ConsoleCommand(Terrain::combineTileShapes, combineTileShapes);
ConsoleCommand(Terrain::enableStreaming, enableStreaming);
ConsoleCommand(Terrain::maxProxies, maxTerrainProxies);
ConsoleCommand(streamDebug, streamDebug);

////////////////////////////////////////////////////////////////////////////////////////

Terrain::Terrain(const Vector2& pos) :
	GameObject(XForm2(pos)),
	streamWindowPatch(0, 0),
	streamWindowPatchLast(0, 0),
	streamWindow(Vector2::Zero(), Vector2::Zero())
{
	playerEditorStartPos = Vector2(0);
	SetRenderGroup(0); // terrain is on render 0

	patches = static_cast<TerrainPatch**>(malloc(sizeof(void*) * fullSize * fullSize));

	for(int x=0; x<fullSize; ++x)
	for(int y=0; y<fullSize; ++y)
	{
		const Vector2 patchPos = pos + patchSize * TerrainTile::GetSize() * Vector2((float)x, (float)y);
		patches[x + fullSize * y] = new TerrainPatch(patchPos);
	}

	// create terrain layers
	layerRenderArray = new TerrainLayerRender *[patchLayers];
	for (int i = 0; i < Terrain::patchLayers; ++i)
		layerRenderArray[i] = new TerrainLayerRender(pos, i);

	// assign first layer to our render group
	layerRenderArray[0]->SetRenderGroup(GetRenderGroup());

	startHandle = firstStartHandle;
	ResetStartHandle();

	// terrain layer render handles rendering
	SetVisible(false);
}

Terrain::~Terrain()
{
	for(int x=0; x<fullSize; ++x)
	for(int y=0; y<fullSize; ++y)
		delete GetPatch(x,y);

	free(patches);

	delete [] layerRenderArray;
}

void Terrain::GiveStubNewHandle(GameObjectStub& stub)
{
	stub.handle = GameObject::GetNextUniqueHandleValue();
	GameObject::SetNextUniqueHandleValue(stub.handle + 1);
	startHandle = GameObject::GetNextUniqueHandleValue();
}

void Terrain::ResetStartHandle()
{
	// reset the starting handle
	GameObject::SetNextUniqueHandleValue(startHandle);
}

void Terrain::Deactivate()
{
	for(int x=0; x<fullSize; ++x)
	for(int y=0; y<fullSize; ++y)
		GetPatch(x,y)->Deactivate();
}

void Terrain::ActivateAreaAroundPlayer()
{
	Deactivate();
	UpdateActiveWindow(true);
}

void Terrain::UpdateActiveWindow(bool init)
{
	const Vector2& pos = g_gameControlBase->GetUserPosition();
	streamWindowPatch = g_terrain->GetPatchOffset(pos);
	int x2 = streamWindowPatchLast.x;
	int y2 = streamWindowPatchLast.y;

	const bool windowMoved = streamWindowPatchLast != streamWindowPatch;
	streamWindowPatchLast = streamWindowPatch;

	// update stream window
	streamWindow = Box2AABB
	(
		GetPosWorld() + patchSize*TerrainTile::GetSize()*Vector2(streamWindowPatch - IntVector2(windowSize)),
		GetPosWorld() + patchSize*TerrainTile::GetSize()*Vector2(streamWindowPatch + IntVector2(1+windowSize))
	);

	//if (!init && (x == x2 && y == y2))
	//	return; // window has not moved

	if (!init && windowMoved && enableStreaming)
	{
		// stream out patches
		// go through patches that were in the window
		for(int i=x2-windowSize; i<=x2+windowSize; ++i)
		for(int j=y2-windowSize; j<=y2+windowSize; ++j)
		{
			TerrainPatch* patch = GetPatch(i,j);
			if (!patch)
				continue;

			// check if patch is no longer in the window
			if (i < streamWindowPatch.x-windowSize || i > streamWindowPatch.x+windowSize || 
					j < streamWindowPatch.y-windowSize || j > streamWindowPatch.y+windowSize)
			{
				patch->SetActivePhysics(false);
				patch->SetActiveObjects(false);
			}
		}
	}

	if (enableStreaming)
	{
		// make physics in the current window active
		for(int i=streamWindowPatch.x-windowSize; i<=streamWindowPatch.x+windowSize; ++i)
		for(int j=streamWindowPatch.y-windowSize; j<=streamWindowPatch.y+windowSize; ++j)
		{
			TerrainPatch* patch = GetPatch(i,j);
			if (!patch)
				continue;

			patch->SetActivePhysics(true);
			patch->SetActiveObjects(true, windowMoved);
		}
	}
	else if (init)
	{
		// load everything on startup if streaming is disabled
		for(int i=0; i<=fullSize; ++i)
		for(int j=0; j<=fullSize; ++j)
		{
			TerrainPatch* patch = GetPatch(i,j);
			if (!patch)
				continue;

			patch->SetActivePhysics(true);
			patch->SetActiveObjects(true);
		}
	}

	UpdateStreaming();
}

void Terrain::UpdatePost()
{
	// update physics or terrain that needs rebuild
	for(int i=streamWindowPatch.x-windowSize; i<=streamWindowPatch.x+windowSize; ++i)
	for(int j=streamWindowPatch.y-windowSize; j<=streamWindowPatch.y+windowSize; ++j)
	{
		TerrainPatch* patch = GetPatch(i,j);
		if (!patch)
			continue;

		if (patch->needsPhysicsRebuild)
		{
			g_terrainRender.RefereshCached(*patch);
			patch->SetActivePhysics(false);
			patch->SetActivePhysics(true);
		}
	}
}

void Terrain::UpdateStreaming()
{
	if (!enableStreaming)
		return;

	if (streamDebug)
		streamWindow.RenderDebug();

	// for all world objects
	const GameObjectHashTable& objects = g_objectManager.GetObjects();
	for (GameObjectHashTable::const_iterator it = objects.begin(); it != objects.end(); ++it)
	{
		GameObject* gameObject = it->second;

		if (gameObject->HasParent())
			continue;	// only stream out top level objects

		if (gameObject->IsPersistant())
			continue;	// skip persistant objects like the player, camera, some effects, etc
		
		if (gameObject->IsDestroyed())
			continue;

		const GameObjectType type = gameObject->GetType();
		const ObjectTypeInfo& objectTypeInfo = GameObjectStub::GetObjectInfo(type);

		if (objectTypeInfo.IsSerializable())
		{
			// serializable objects stream out when their aabb goes out of the stream window
			const Box2AABB stubAABB(gameObject->GetXFormWorld(), gameObject->GetStubSize());
			if (streamDebug)
				stubAABB.RenderDebug();
			if (streamWindow.FullyContains(stubAABB))
				continue;

			TerrainPatch* patch = g_terrain->GetPatch(gameObject->GetPosWorld());
			if (patch)
			{
				GameObjectStub stub = gameObject->Serialize();
				patch->AddStub(stub);
			}
		}
		else
		{
			if (gameObject->IsStatic())
			{
				// static objects stream when center is out of the window
				if (streamWindow.Contains(gameObject->GetPosWorld()))
					continue;
			}
			else if (gameObject->FullyContainedBy(streamWindow))
				continue;
		}

		gameObject->StreamOut();
	}
}





/*
void Terrain::Convert()
{
	int newfullSize = 25;
	int newpatchSize = 32;

	ofstream outTerrainFile("terrain.2dt", ios::out | ios::binary);
	if (outTerrainFile.fail())
		return;

	// write out the data version
	const char version = (char)(dataVersion);
	outTerrainFile.write(&version, 1);

	// save the next handle
	outTerrainFile.write((const char *)&startHandle, sizeof(startHandle));

	for(int x=0; x<newfullSize; ++x)
	for(int y=0; y<newfullSize; ++y)
	{
		//const TerrainPatch& patch = *patches[x][y];

		// write out the tile data
		for(int x2=0; x2<newpatchSize; ++x2)
		for(int y2=0; y2<newpatchSize; ++y2)
		{
			{
				int x3 = x * newpatchSize + x2;
				int y3 = y * newpatchSize + y2;

				int xo = x3 / patchSize;
				int xo2 = x3 % patchSize;

				int yo = y3 / patchSize;
				int yo2 = y3 % patchSize;

				ASSERT(xo < fullSize && yo < fullSize)
				ASSERT(xo2 < patchSize && yo2 < patchSize)

				TerrainTile& tile = patches[xo][yo]->tiles[xo2][yo2];
				outTerrainFile.write((const char *)&tile, sizeof(TerrainTile));
			}

			//outTerrainFile.write((const char *)&patch.tiles[x2][y2], sizeof(TerrainTile));
		}

		// save out the object stubs
		unsigned int stubCount = 0;
		outTerrainFile.write((const char *)&stubCount, sizeof(stubCount));
	}

	outTerrainFile.close();
}*/

void Terrain::Save(const WCHAR* filename)
{
	ofstream outTerrainFile(filename, ios::out | ios::binary);
	if (outTerrainFile.fail())
		return;

	// write out the data version
	const char version = (char)(dataVersion);
	outTerrainFile.write(&version, 1);

	// save player position
	playerEditorStartPos = g_gameControlBase->GetPlayer()? g_gameControlBase->GetPlayer()->GetPosWorld() : Vector2(0);
	outTerrainFile.write((const char *)&playerEditorStartPos.x,	sizeof(float));
	outTerrainFile.write((const char *)&playerEditorStartPos.y,	sizeof(float));
	
	outTerrainFile.write((const char *)&fullSize, sizeof(fullSize));
	outTerrainFile.write((const char *)&patchSize, sizeof(patchSize));
	outTerrainFile.write((const char *)&patchLayers, sizeof(patchLayers));

	// save the next handle
	outTerrainFile.write((const char *)&startHandle, sizeof(startHandle));

	for(int x=0; x<fullSize; ++x)
	for(int y=0; y<fullSize; ++y)
	{
		const TerrainPatch& patch = *GetPatch(x,y);
		
		// fast write out the tile data
		outTerrainFile.write((const char *)patch.tiles, sizeof(TerrainTile) * patchSize * patchSize * patchLayers);

		// separate write out the tile data
		/*for(int l=0; l<patchLayers; ++l)
		for(int j=0; j<patchSize; ++j)
		for(int k=0; k<patchSize; ++k)
		{
			const TerrainTile& tile = patch.GetTileLocal(j, k, l);

			outTerrainFile << tile.GetEdgeData();
			outTerrainFile << tile.GetSurfaceData(0);
			outTerrainFile << tile.GetSurfaceData(1);
			outTerrainFile << tile.GetTileSet();
		}*/

		// save out the object stubs
		unsigned int stubCount = patch.objectStubs.size();
		outTerrainFile.write((const char *)&stubCount, sizeof(stubCount));
		for (list<GameObjectStub>::const_iterator it = patch.objectStubs.begin(); it != patch.objectStubs.end(); ++it) 
		{       
			const GameObjectStub& stub = *it;

			outTerrainFile.write((const char *)&stub.type,		sizeof(stub.type));
			outTerrainFile.write((const char *)&stub.xf,		sizeof(stub.xf));
			outTerrainFile.write((const char *)&stub.size,		sizeof(stub.size));
			outTerrainFile.write((const char *)&stub.handle,	sizeof(stub.handle));

			int attributesLength = strlen(stub.attributes) + 1;
			outTerrainFile.write((const char *)&attributesLength, sizeof(attributesLength));
			outTerrainFile.write((const char *)&stub.attributes, attributesLength);
		}
	}

	outTerrainFile.close();
}

void Terrain::Load(const WCHAR* filename)
{
	g_editor.ResetEditor();

	ifstream inTerrainFile(filename, ios::in | ios::binary);

	if (inTerrainFile.fail())
	{
		LoadFromResource(filename);
		return;
	}

	// check the data version
	char version;
	inTerrainFile.read(&version, 1);

	if (version != dataVersion)
	{
		g_debugMessageSystem.AddError(L"Local terrain file version mismatch.  Using built in terrain.");
		inTerrainFile.close();
		LoadFromResource(filename);
		return;
	}
	
	Vector2 playerPos(0);
	inTerrainFile.read((char *)&playerEditorStartPos.x, sizeof(float));
	inTerrainFile.read((char *)&playerEditorStartPos.y, sizeof(float));
	
	int fullSizeIn, patchSizeIn, patchLayersIn;
	inTerrainFile.read((char *)&fullSizeIn, sizeof(fullSizeIn));
	inTerrainFile.read((char *)&patchSizeIn, sizeof(patchSizeIn));
	inTerrainFile.read((char *)&patchLayersIn, sizeof(patchLayersIn));
	if (fullSizeIn != fullSize || patchSizeIn != patchSize || patchLayersIn != patchLayers)
	{
		g_debugMessageSystem.AddError(L"Local terrain file size mismatch.  Using built in terrain.");
		inTerrainFile.close();
		LoadFromResource(filename);
		return;
	}
	
	// clear out terrain
	Clear();

	// read in next handle
	inTerrainFile.read((char *)&startHandle, sizeof(startHandle));

	for(int x=0; x<fullSize; ++x)
	for(int y=0; y<fullSize; ++y)
	{
		TerrainPatch& patch = *GetPatch(x,y);
		
		// fast read in the tile data
		inTerrainFile.read((char *)patch.tiles, sizeof(TerrainTile) * patchSize * patchSize * patchLayers);

		// separate read in the tile data
		/*for(int l=0; l<patchLayers; ++l)
		for(int j=0; j<patchSize; ++j)
		for(int k=0; k<patchSize; ++k)
		{
			TerrainTile& tile = patch.GetTileLocal(j, k, l);

			BYTE edgeData = 0;
			BYTE surfaceData0 = 0;
			BYTE surfaceData1 = 0;
			BYTE tileSet = 0;
			
			inTerrainFile.read((char *)&edgeData,		1);
			inTerrainFile.read((char *)&surfaceData0,	1);
			inTerrainFile.read((char *)&surfaceData1,	1);
			inTerrainFile.read((char *)&tileSet,		1);

			tile.SetEdgeData(edgeData);
			tile.SetSurfaceData(0, surfaceData0);
			tile.SetSurfaceData(1, surfaceData1);
			tile.SetTileSet(tileSet);
		}*/

		if (inTerrainFile.eof())
			break; // error

		// read in the object stubs
		unsigned int stubCount;
		inTerrainFile.read((char*)&stubCount, sizeof(stubCount));
		for (unsigned int i = 0; i < stubCount; ++i) 
		{       
			GameObjectStub stub;

			inTerrainFile.read((char *)&stub.type,		sizeof(stub.type));
			inTerrainFile.read((char *)&stub.xf,		sizeof(stub.xf));
			inTerrainFile.read((char *)&stub.size,		sizeof(stub.size));
			inTerrainFile.read((char *)&stub.handle,	sizeof(stub.handle));

			// hack: cap small stub sizes
			if (fabs(stub.size.x) < 0.01f)
				stub.size.x = 0.01f;
			if (fabs(stub.size.y) < 0.01f)
				stub.size.y = 0.01f;

			int attributesLength = 0;
			inTerrainFile.read((char *)&attributesLength, sizeof(attributesLength));
			inTerrainFile.read((char *)&stub.attributes, attributesLength);
			
			patch.AddStub(stub);
			if (inTerrainFile.eof())
				break; // error
		}
	}

	inTerrainFile.close();
	ResetStartHandle();
}

void Terrain::LoadFromResource(const WCHAR* filename)
{
	// clear out terrain
	Clear();

	// Get pointer and size to resource
	HRSRC hRes = FindResource(0, filename, RT_RCDATA);
	HGLOBAL hMem = LoadResource(0, hRes);
	void* pMem = LockResource(hMem);
	DWORD size = SizeofResource(0, hRes);

	if (!pMem || size == 0)
		return;

	BYTE* dataPointer = (BYTE*)pMem;	

	// check the data version
	const BYTE version = *(dataPointer++);
	if (version != dataVersion)
	{
		g_debugMessageSystem.AddError(L"Built in terrain version mismatch.  Using clear terrain.");
		return;
	}
	
	playerEditorStartPos.x = *(float*)(dataPointer); dataPointer += sizeof(float);
	playerEditorStartPos.y = *(float*)(dataPointer); dataPointer += sizeof(float);
	
	int fullSizeIn, patchSizeIn, patchLayersIn;
	fullSizeIn = *(int*)(dataPointer);
	dataPointer += sizeof(fullSizeIn);
	patchSizeIn = *(int*)(dataPointer);
	dataPointer += sizeof(patchSizeIn);
	patchLayersIn = *(int*)(dataPointer);
	dataPointer += sizeof(patchLayersIn);
	if (fullSizeIn != fullSize || patchSizeIn != patchSize || patchLayersIn != patchLayers)
	{
		g_debugMessageSystem.AddError(L"Built in terrain file size mismatch.  Using clear terrain.");
		return;
	}

	// read in next handle
	startHandle = *(GameObjectHandle*)(dataPointer);
	dataPointer += sizeof(startHandle);

	for(int x=0; x<fullSize; ++x)
	for(int y=0; y<fullSize; ++y)
	{
		TerrainPatch& patch = *GetPatch(x,y);

		// read in the tile data
		const int dataSize = sizeof(TerrainTile) * patchSize * patchSize * patchLayers;
		memcpy(patch.tiles, dataPointer, dataSize);
		dataPointer += dataSize;

		// read in the object stubs
		unsigned int stubCount = *(unsigned int*)(dataPointer);
		dataPointer += sizeof(unsigned int);
		for (unsigned int i = 0; i < stubCount; ++i) 
		{
			GameObjectStub stub;
			stub.type = *(GameObjectType*)(dataPointer); dataPointer += sizeof(GameObjectType);
			stub.xf = *(XForm2*)(dataPointer); dataPointer += sizeof(XForm2);
			stub.size = *(Vector2*)(dataPointer); dataPointer += sizeof(Vector2);
			stub.handle = *(GameObjectHandle*)(dataPointer); dataPointer += sizeof(GameObjectHandle);

			// hack: cap small stub sizes
			if (fabs(stub.size.x) < 0.1f)
				stub.size.x = 0.1f;
			if (fabs(stub.size.y) < 0.1f)
				stub.size.y = 0.1f;
			
			char* attributes = NULL;
			int attributesLength = 0;
			attributesLength = *(int*)(dataPointer); dataPointer += sizeof(int);
			attributes = (char*)(dataPointer); dataPointer += attributesLength;
			strncpy_s(stub.attributes, attributes, sizeof(stub.attributes));
			
			patch.AddStub(stub);
		}
	}
	UnlockResource(hMem);
	FreeResource(hRes);
	ResetStartHandle();
}

void Terrain::Clear()
{
	for(int x=0; x<fullSize; ++x)
	for(int y=0; y<fullSize; ++y)
	{
		TerrainPatch& patch = *GetPatch(x,y);
		patch.Deactivate();
		patch.Clear();
	}

	startHandle = firstStartHandle;
	ResetStartHandle();
}

IntVector2 Terrain::GetTileOffset(const Vector2& pos) const
{
	Vector2 offset = (pos - GetPosWorld()) / (TerrainTile::GetSize());

	const int tileX = (int)floorf(offset.x);
	const int tileY = (int)floorf(offset.y);

	return IntVector2(tileX, tileY);
}

TerrainTile* Terrain::GetTile(const Vector2& pos, int& x, int& y, int layer) const
{
	const IntVector2 tileOffset = GetTileOffset(pos);

	if (tileOffset.x < 0 || tileOffset.y < 0 || tileOffset.x >= patchSize*fullSize || tileOffset.y >= patchSize*fullSize)
		return NULL;

	x = tileOffset.x;
	y = tileOffset.y;

	return GetTile(x, y, layer);
}

TerrainTile* Terrain::GetTile(int x, int y, int layer) const
{
	// seperate patch and tile offset
	const int patchX = x / patchSize;
	const int patchY = y / patchSize;

	const int tileX = x - patchX*patchSize;
	const int tileY = y - patchY*patchSize;

	if (patchX < 0 || patchX >= fullSize || patchY < 0 || patchY >= fullSize)
		return NULL;
	if (tileX < 0 || tileX >= patchSize || tileY < 0 || tileY >= patchSize)
		return NULL;

	return &GetPatch(patchX, patchY)->GetTileLocal(tileX, tileY, layer);
}

TerrainTile* Terrain::GetTile(const Vector2& pos, int layer) const
{
	int x, y;
	return GetTile(pos, x, y, layer);
}

int Terrain::GetSurfaceSide(const Vector2& pos, int layer) const
{
	int x, y;
	TerrainTile* tile = GetTile(pos, x, y, layer);
	if (tile)
	{
		const Vector2 offset = pos - GetTilePos(x, y);
		return tile->GetSurfaceSide(offset);
	}
	return false;
}

BYTE Terrain::GetSurfaceIndex(const Vector2& pos, int layer) const
{
	int x, y;
	TerrainTile* tile = GetTile(pos, x, y, layer);
	if (tile)
	{
		const Vector2 offset = pos - GetTilePos(x, y);
		return tile->GetSurfaceData(offset);
	}
	return 0;
}

void Terrain::SetSurfaceIndex(const Vector2& pos, BYTE surface, int layer)
{
	int x, y;
	TerrainTile* tile = GetTile(pos, x, y, layer);
	if (tile)
	{
		const Vector2 offset = pos - GetTilePos(x, y);
		tile->SetSurfaceData(offset, surface);
	}
}

TerrainTile* Terrain::GetConnectedTileA(int x, int y, int &x2, int &y2, int layer)
{
	const TerrainTile* tile = GetTile(x, y, layer);
	if (!tile || tile->IsClear() || tile->IsFull())
		return NULL;

	BYTE xa, ya;
	tile->GetXYA(xa, ya);

	TerrainTile* tileNeighbor = NULL;

	if (xa == 0 && ya == 0 || xa == 0 && ya == 4 || xa == 4 && ya == 0 || xa == 4 && ya == 4)
	{
		int xOffset = 0;
		int yOffset = 0;
		if (xa == 0 && ya == 0)
			yOffset = -1;
		else if (xa == 4 && ya == 4)
			yOffset = 1;
		else if (xa == 0 && ya == 4)
			xOffset = -1;
		else if (xa == 4 && ya == 0)
			xOffset = 1;
		
		// check around the corner until we find a connection
		for (int i = 0; i < 3; ++i)
		{
			x2 = x + xOffset;
			y2 = y + yOffset;
			tileNeighbor = GetTile(x2, y2);
			if (tileNeighbor && GameSurfaceInfo::NeedsEdgeCollision(*tileNeighbor))
			{
				if (TerrainTile::IsConnectedAB(*tile, *tileNeighbor, xOffset, yOffset))
					return tileNeighbor;
			}
			TerrainTile::RotateOffset(xOffset, yOffset, 1);

		}
		return NULL;
	}

	int xOffset = 0;
	int yOffset = 0;
	if (xa == 0)
		xOffset = -1;
	else if (xa == 4)
		xOffset = 1;
	else if (ya == 0)
		yOffset = -1;
	else if (ya == 4)
		yOffset = 1;

	x2 = x + xOffset;
	y2 = y + yOffset;
	tileNeighbor = GetTile(x2, y2);
	if (tileNeighbor && GameSurfaceInfo::NeedsEdgeCollision(*tileNeighbor))
	{
		if (!TerrainTile::IsConnectedAB(*tile, *tileNeighbor, xOffset, yOffset))
			tileNeighbor = NULL;
	}

	return tileNeighbor;

}

TerrainTile* Terrain::GetConnectedTileB(int x, int y, int &x2, int &y2, int layer)
{
	const TerrainTile* tile = GetTile(x, y, layer);
	if (!tile || tile->IsClear() || tile->IsFull())
		return NULL;

	BYTE xb, yb;
	tile->GetXYB(xb, yb);

	TerrainTile* tileNeighbor = NULL;

	if (xb == 0 && yb == 0 || xb == 0 && yb == 4 || xb == 4 && yb == 0 || xb == 4 && yb == 4)
	{
		int xOffset = 0;
		int yOffset = 0;
		if (xb == 0 && yb == 0)
			xOffset = -1;
		else if (xb == 4 && yb == 4)
			xOffset = 1;
		else if (xb == 0 && yb == 4)
			yOffset = 1;
		else if (xb == 4 && yb == 0)
			yOffset = -1;
		
		// check around the corner until we find a connection
		for (int i = 0; i < 3; ++i)
		{
			x2 = x + xOffset;
			y2 = y + yOffset;
			tileNeighbor = GetTile(x2, y2);
			if (tileNeighbor && GameSurfaceInfo::NeedsEdgeCollision(*tileNeighbor))
			{
				if (TerrainTile::IsConnectedAB(*tileNeighbor, *tile, xOffset, yOffset))
					return tileNeighbor;
			}
			TerrainTile::RotateOffset(xOffset, yOffset, -1);

		}
		return NULL;
	}

	int xOffset = 0;
	int yOffset = 0;
	if (xb == 0)
		xOffset = -1;
	else if (xb == 4)
		xOffset = 1;
	else if (yb == 0)
		yOffset = -1;
	else if (yb == 4)
		yOffset = 1;

	x2 = x + xOffset;
	y2 = y + yOffset;
	tileNeighbor = GetTile(x2, y2);
	if (tileNeighbor && GameSurfaceInfo::NeedsEdgeCollision(*tileNeighbor))
	{
		if (!TerrainTile::IsConnectedAB(*tileNeighbor, *tile, xOffset, yOffset))
			tileNeighbor = NULL;
	}

	return tileNeighbor;
}

void Terrain::Deform(const Vector2& pos, float radius)
{
	int centerX, centerY;
	TerrainTile* tile = GetTile(pos, centerX, centerY, 1);

	const float tileSize = TerrainTile::GetSize();
	const Vector2 centerPos = GetPosWorld() + (Vector2(0.5f)+Vector2(float(centerX), float(centerY)))*tileSize;
	//centerPos.RenderDebug(Color::Red(0.3f), radius);

	// round radius to neraest tile
	int tileRadius = int(radius / tileSize);
	radius = ((float)tileRadius + 0.5f) * tileSize;
	const float tileRadiusSquared = ((float)tileRadius + 0.5f)*((float)tileRadius + 0.5f);

	for(int i=-tileRadius; i <= tileRadius; ++i)
	{
		for(int j=-tileRadius; j <= tileRadius; ++j)
		{
			Vector2 tilePos((float)i, (float)j);

			const int x = centerX + i;
			const int y = centerY + j;
			const int patchX = x / patchSize;
			const int patchY = y / patchSize;

			TerrainPatch* patch = GetPatch(patchX, patchY);
			if (!patch)
				continue;

			// get the tile
			const int patchTileX = x - patchX*patchSize;
			const int patchTileY = y - patchY*patchSize;

			if (!patch->IsTileIndexValid(patchTileX, patchTileY))
				continue;

			TerrainTile& tile = patch->GetTileLocal(patchTileX, patchTileY, Terrain::physicsLayer);
				
			if (tile.IsClear())
				continue;

			// calculate tangents
			const Vector2 tanDirection = tilePos.Normalize().RotateRightAngle();
			const Vector2 tanPos = centerPos + tilePos.Normalize()*radius;
			const Vector2 tanPos1 = tanPos + tanDirection*2*tileSize;
			const Vector2 tanPos2 = tanPos - tanDirection*2*tileSize;
			const Vector2 tileOffset = patch->GetTilePos(patchTileX, patchTileY);
			//Line2(tanPos, tanPos2).RenderDebug(Color::White(), 2);
			//Box2AABB(tileOffset, tileOffset + Vector2(tileSize)).RenderDebug(Color::White(0.1f), 3);

			// try to resurface
			const bool wasFull = (tile.IsFull());
			if (tile.Resurface(tanPos1 - tileOffset, tanPos2 - tileOffset))
			{
				// only full tiles can be resurfaced
				if (!wasFull)
					tile.MakeClear();
			}
			else
			{
				// clear tile if inside the circle
				if (tilePos.LengthSquared() < tileRadiusSquared)
					tile.MakeClear();
			}

			patch->RebuildPhysics();
		}
	}
}

void Terrain::DeformTile(const Vector2& startPos, const Vector2& direction, const GameObject* ignoreObject, GameMaterialIndex gmi, bool clear, float distance)
{
	const Line2 testLine(startPos - distance*direction, startPos + distance*direction);
	//testLine.RenderDebug(Color::Black());

	SimpleRaycastResult raycastResult;
	GameObject* hitObject = g_physics->RaycastSimple(testLine, &raycastResult, ignoreObject);
	if (!hitObject || !hitObject->IsStatic())
		return;

	Vector2 pos = raycastResult.point + 0.01f * (testLine.p2 - testLine.p1).Normalize();
	//pos.RenderDebug();

	int x, y;
	TerrainTile* tile = GetTile(pos, x, y);
	if (!tile)
		return;

	GameSurfaceInfo gsi = GameSurfaceInfo::Get(GetSurfaceIndex(pos));
	if (!gsi.IsDestructible() || (gmi != GMI_Invalid && gsi.materialIndex != gmi))
		return;

	const Vector2 tilePos = GetTilePos(x, y);
	TerrainPatch* patch = GetPatch(pos);
	if (patch)
	{
		if (clear)
		{
			const int side = g_terrain->GetSurfaceSide(pos);
			const GameSurfaceInfo& surfaceInfo = GameSurfaceInfo::Get(tile->GetSurfaceData(side));
			if (surfaceInfo.IsDestructible())
				tile->SetSurfaceData(side, 0);
		}
		else
		{
			tile->MakeSmaller(pos - tilePos, direction);
			
			// clear the tile if there is only a tiny bit left
			const GameSurfaceInfo& tile0Info = GameSurfaceInfo::Get(tile->GetSurfaceData(0));
			const GameSurfaceInfo& tile1Info = GameSurfaceInfo::Get(tile->GetSurfaceData(1));

			if (tile0Info.IsDestructible() && !tile1Info.HasCollision() && tile->GetSurfaceArea(0) < 0.1f)
				tile->MakeClear();
			else if (tile1Info.IsDestructible() && !tile0Info.HasCollision() && tile->GetSurfaceArea(1) < 0.1f)
				tile->MakeClear();
		}
		patch->RebuildPhysics();
		g_terrainRender.RefereshCached(*patch);
	}
}

GameObjectStub* Terrain::GetStub(GameObjectHandle handle)
{
	// check every patch
	for(int x=0; x<fullSize; ++x)
	for(int y=0; y<fullSize; ++y)
	{
		GameObjectStub* stub = patches[x + fullSize * y]->GetStub(handle);
		if (stub)
			return stub;
	}
	return NULL;
}

bool Terrain::RemoveStub(GameObjectHandle handle, TerrainPatch* patch)
{
	if (patch)
	{
		// check the passed in patch first
		if (patch->RemoveStub(handle))
			return true;
	}

	// check every patch
	for(int x=0; x<fullSize; ++x)
	for(int y=0; y<fullSize; ++y)
	{
		if (patches[x + fullSize * y]->RemoveStub(handle))
			return true;
	}
	return false;
}

GameObjectStub* Terrain::GetStub(const Vector2& pos)
{
	GameObjectStub* bestStub = NULL;
	float bestDistance = FLT_MAX;

	// pick out the stub that we are nearest to the edge of, so we can pick out overlapping stubs
	for(int x=0; x<fullSize; ++x)
	for(int y=0; y<fullSize; ++y)
	{
		list<GameObjectStub>& objectStubs = patches[x + fullSize * y]->GetStubs();
		for (list<GameObjectStub>::iterator it = objectStubs.begin(); it != objectStubs.end(); ++it) 
		{       
			GameObjectStub& stub = *it;
			if (stub.TestPosition(pos))
			{
				const Vector2 posLocal = stub.xf.Inverse().TransformCoord(pos);
				const float distance = Min(stub.size.x - fabs(posLocal.x), stub.size.y - fabs(posLocal.y));
				if (distance > bestDistance)
					continue;

				bestDistance = distance;
				bestStub = &stub;
			}
		}
	}
	return bestStub;
}

void Terrain::CheckForErrors()
{
	// check for duplicate handles
	for(int x=0; x<Terrain::fullSize; ++x)
	for(int y=0; y<Terrain::fullSize; ++y)
	{
		TerrainPatch& patch = *(GetPatch(x,Terrain::fullSize-1-y));

		for (list<GameObjectStub>::iterator it = patch.objectStubs.begin(); it != patch.objectStubs.end(); ++it) 
		{
			GameObjectStub& stub = *it;
			for(int i=0; i<Terrain::fullSize; ++i)
			for(int j=0; j<Terrain::fullSize; ++j)
			{
				TerrainPatch& patch = *(GetPatch(i,Terrain::fullSize-1-j));

				// save out the object stubs
				for (list<GameObjectStub>::iterator it = patch.objectStubs.begin(); it != patch.objectStubs.end(); ++it) 
				{
					GameObjectStub& stub2 = *it;
					if (stub.handle > GameObject::GetNextUniqueHandleValue())
					{
						ASSERT(false); // duplicate handles should not happen
						stub.handle = GameObject::GetNextUniqueHandleValue();
						GameObject::SetNextUniqueHandleValue(stub.handle + 1);
					}
					if (stub.handle == stub2.handle && &stub != &stub2)
					{
						ASSERT(false); // duplicate handles should not happen
						stub.handle = GameObject::GetNextUniqueHandleValue();
						GameObject::SetNextUniqueHandleValue(stub.handle + 1);
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Terrain Patch
*/
////////////////////////////////////////////////////////////////////////////////////////

// Terrain patch constructor
// note: terrain patches are not be added to the world!
TerrainPatch::TerrainPatch(const Vector2& pos) :
	GameObject(pos, NULL, GameObjectType(0), false),
	activePhysics(false),
	activeObjects(false),
	needsPhysicsRebuild(false)
{
	tiles = new TerrainTile[Terrain::patchLayers * Terrain::patchSize * Terrain::patchSize];

	Clear();

	// terrain layer render handles rendering
	SetVisible(false);
}

TerrainPatch::~TerrainPatch()
{
	delete [] tiles;
}

void TerrainPatch::Clear()
{
	Deactivate();
	ClearTileData();
	ClearObjectStubs();
}

void TerrainPatch::ClearTileData(int layer)
{
	ASSERT(layer < Terrain::patchLayers);

	// wipe out all the tile data
	for(int x=0; x<Terrain::patchSize; ++x)
	for(int y=0; y<Terrain::patchSize; ++y)
	{
		// reset tiles
		GetTileLocal(x, y, layer).MakeClear();
	}
}
	

void TerrainPatch::ClearTileData()
{
	// wipe out all the tile data
	for(int x=0; x<Terrain::patchSize; ++x)
	for(int y=0; y<Terrain::patchSize; ++y)
	for(int l=0; l<Terrain::patchLayers; ++l)
	{
		// reset tiles
		GetTileLocal(x, y, l).MakeClear();
	}
}

void TerrainPatch::ClearObjectStubs()
{
	// clear the object stub list
	objectStubs.clear();
}

void TerrainPatch::SetActivePhysics(bool _activePhysics)
{
	if (activePhysics == _activePhysics)
		return;
	activePhysics = _activePhysics;

	if (activePhysics)
	{
		ASSERT(!GetPhysicsBody());
		if (Terrain::usePolyPhysics)
			CreatePolyPhysicsBody(GetPosWorld());
		else
			CreateEdgePhysicsBody(GetPosWorld());
	}
	else
	{
		ASSERT(GetPhysicsBody());
		DestroyPhysicsBody();
	}
}

void TerrainPatch::SetActiveObjects(bool _activeObjects, bool windowMoved)
{
	if (_activeObjects && !activeObjects || windowMoved)
	{
		// update serialize objects when window moves or patch first becomes active
		const Box2AABB streamWindowAABB = g_terrain->GetStreamWindow();
		for (list<GameObjectStub>::iterator it = objectStubs.begin(); it != objectStubs.end(); ) 
		{       
			const GameObjectStub& stub = *it;
			list<GameObjectStub>::iterator itLast = it;
			++it;

			// check if it already exists
			if (g_objectManager.GetObjectFromHandle(stub.handle))
				continue;

			const ObjectTypeInfo& objectInfo = stub.GetObjectInfo();
			if (!objectInfo.IsSerializable())
				continue;

			// only load seralizable if fully in the stream window
			const Box2AABB stubAABB = stub.GetAABB();
			if (!streamWindowAABB.FullyContains(stubAABB))
				continue;

			// create the object from the stub
			stub.BuildObject();
			
			// seralizeable objects are removed as they are spawned
			objectStubs.erase(itLast);
		}
	}

	if (activeObjects == _activeObjects)
		return;
	activeObjects = _activeObjects;

	if (activeObjects)
	{
		const Box2AABB streamWindowAABB = g_terrain->GetStreamWindow();

		// create all the objects in the stub list
		for (list<GameObjectStub>::iterator it = objectStubs.begin(); it != objectStubs.end(); ++it) 
		{       
			const GameObjectStub& stub = *it;

			// check if it already exists
			if (g_objectManager.GetObjectFromHandle(stub.handle))
				continue;

			// serializible objecs update every time stream window moves
			const ObjectTypeInfo& objectInfo = stub.GetObjectInfo();
			if (objectInfo.IsSerializable())
				continue;

			// create the object from the stub
			stub.BuildObject();
		}

		// do tile create callbacks
		for(int x=0; x<Terrain::patchSize; ++x)
		for(int y=0; y<Terrain::patchSize; ++y)
		{
			for(int l=0; l<Terrain::patchLayers; ++l)
			{
				const TerrainTile& tile = GetTileLocal(x, y, l);
				const GameSurfaceInfo& tile0Info = GameSurfaceInfo::Get(tile.GetSurfaceData(false));
				const GameSurfaceInfo& tile1Info = GameSurfaceInfo::Get(tile.GetSurfaceData(true));
				const Vector2 tileOffset = TerrainTile::GetSize() * Vector2((float)x, (float)y);

				// call the tile create callbacks if it has any
				if (tile0Info.tileCreateCallback && tile.GetSurfaceHasArea(false))
					(tile0Info.tileCreateCallback)(tile0Info, GetPosWorld() + tileOffset + 0.5f*Vector2(TerrainTile::GetSize()), l);
				if (tile1Info.tileCreateCallback && tile.GetSurfaceHasArea(true))
					(tile1Info.tileCreateCallback)(tile1Info, GetPosWorld() + tileOffset + 0.5f*Vector2(TerrainTile::GetSize()), l);
			}
		}
	}
}

bool TerrainPatch::GetTileLocalIsSolid(int x, int y) const
{
	const TerrainTile& tile = GetTileLocal(x, y, Terrain::physicsLayer);

	if (tile.GetSurfaceHasArea(false) && GameSurfaceInfo::Get(tile.GetSurfaceData(false)).HasCollision())
		return true;
	
	if (tile.GetSurfaceHasArea(true) && GameSurfaceInfo::Get(tile.GetSurfaceData(true)).HasCollision())
		return true;
		
	return false;
}

void TerrainPatch::CreateEdgePhysicsBody(const Vector2 &pos)
{
	ASSERT(!GetPhysicsBody());
	ASSERT(!HasParent());
	ASSERT(!HasPhysics());
	ASSERT(g_terrain);
	
	needsPhysicsRebuild = false;
	GameObject::CreatePhysicsBody(XForm2(pos), b2_staticBody);

	for(int x=0; x<Terrain::patchSize; ++x)
	for(int y=0; y<Terrain::patchSize; ++y)
	{
		const TerrainTile& tile = GetTileLocal(x, y, Terrain::physicsLayer);
		if (tile.IsClear() || tile.IsFull() || !GameSurfaceInfo::NeedsEdgeCollision(tile))
			continue;

		// protect against creating way too many proxies
		const int proxyCount = g_physics->GetPhysicsWorld()->GetProxyCount();
		if (proxyCount > Terrain::maxProxies)
			break;

		const GameSurfaceInfo& tile0Info = GameSurfaceInfo::Get(tile.GetSurfaceData(0));
		const GameSurfaceInfo& tile1Info = GameSurfaceInfo::Get(tile.GetSurfaceData(1));
		const Vector2 tileOffset = TerrainTile::GetSize() * Vector2((float)x, (float)y);

		{
			// create edge shape terrain
			b2EdgeShape shapeDef;
			shapeDef.Set(tile.GetPosA() + tileOffset, tile.GetPosB() + tileOffset);
		
			b2FixtureDef fixtureDef;
			fixtureDef.shape = &shapeDef;
			fixtureDef.userData = (void*)((tile0Info.HasCollision() && tile.GetSurfaceHasArea(0))? tile.GetSurfaceData(0) : tile.GetSurfaceData(1));
			fixtureDef.friction = Terrain::friction;
			fixtureDef.restitution = Terrain::restitution;
			GetPhysicsBody()->CreateFixture(&fixtureDef);
		}
	}
}

void TerrainPatch::CreatePolyPhysicsBody(const Vector2 &pos)
{
	ASSERT(!GetPhysicsBody());
	ASSERT(!HasParent());
	ASSERT(!HasPhysics());
	ASSERT(g_terrain);
	
	needsPhysicsRebuild = false;
	GameObject::CreatePhysicsBody(XForm2(pos), b2_staticBody);
	
	bool* solidTileArray = static_cast<bool*>(malloc(sizeof(bool) * Terrain::patchSize * Terrain::patchSize));
	for(int x=0; x<Terrain::patchSize; ++x)
	for(int y=0; y<Terrain::patchSize; ++y)
		solidTileArray[x + y*Terrain::patchSize] = false;
	
	for(int y=0; y<Terrain::patchSize; ++y)
	for(int x=0; x<Terrain::patchSize; ++x)
	{
		TerrainTile tile = GetTileLocal(x, y, Terrain::physicsLayer);
		if (tile.IsClear())
			continue;

		// just flip tile if its all surface 1 so surface 0 is the dominate surface
		if (!tile.GetSurfaceHasArea(0))
			tile = tile.Invert();

		bool& solidTileCheck = solidTileArray[x + y*Terrain::patchSize];
		if (solidTileCheck)
			continue;

		// protect against creating way too many proxies
		const int proxyCount = g_physics->GetPhysicsWorld()->GetProxyCount();
		if (proxyCount > Terrain::maxProxies)
			break;

		if (tile.HasFullCollision() && Terrain::combineTileShapes)
		{
			const GameSurfaceInfo& tileInfo = GameSurfaceInfo::Get(tile.GetSurfaceData(0));
			const GameMaterialIndex materialIndex = tileInfo.materialIndex;

			// try to create large block for connected tiles
			int right = Terrain::patchSize;
			int bottom = Terrain::patchSize;

			// get the width first
			for(int x2=x; x2<Terrain::patchSize; ++x2)
			{
				const TerrainTile& tile2 = GetTileLocal(x2, y, Terrain::physicsLayer);
				const GameSurfaceInfo& tile2Info = GameSurfaceInfo::Get(tile2.GetSurfaceData(0));
				if (!tile2.HasFullCollision() || tile2Info.materialIndex != materialIndex || solidTileArray[x2 + y*Terrain::patchSize])
				{
					right = x2;
					break;
				}
			}

			// get the height
			for(int y2=y+1; y2<bottom; ++y2)
			for(int x2=x; x2<right; ++x2)
			{
				const TerrainTile& tile2 = GetTileLocal(x2, y2, Terrain::physicsLayer);
				const GameSurfaceInfo& tile2Info = GameSurfaceInfo::Get(tile2.GetSurfaceData(0));
				if (!tile2.HasFullCollision() || tile2Info.materialIndex != materialIndex)
				{
					bottom = y2;
					break;
				}
			}
			
			// mark off solid areas
			for(int x2=x; x2<right; ++x2)	
			for(int y2=y; y2<bottom; ++y2)
				solidTileArray[x2 + y2*Terrain::patchSize] = true;

			// create a box to fit that size
			b2PolygonShape shapeDef;
			const float width  = (right  - x)*0.5f*TerrainTile::GetSize();
			const float height = (bottom - y)*0.5f*TerrainTile::GetSize();
			const Vector2 center = TerrainTile::GetSize() * (Vector2(float(x), float(y)) + Vector2(width, height));
			shapeDef.SetAsBox(width, height, center, 0);
				
			b2FixtureDef fixtureDef;
			fixtureDef.shape = &shapeDef;
			fixtureDef.userData = (void*)(tile.GetSurfaceData(0));
			fixtureDef.friction = Terrain::friction;
			fixtureDef.restitution = Terrain::restitution;
			GetPhysicsBody()->CreateFixture(&fixtureDef);
			continue;
		}

		const GameSurfaceInfo& tile0Info = GameSurfaceInfo::Get(tile.GetSurfaceData(0));
		const GameSurfaceInfo& tile1Info = GameSurfaceInfo::Get(tile.GetSurfaceData(1));
		const Vector2 tileOffset = TerrainTile::GetSize() * Vector2((float)x, (float)y);

		if (tile.HasFullCollision())
		{
			// use full box
			b2PolygonShape shapeDef;
			const Vector2 *edgeVerts = NULL;
			int vertexCount;
			TerrainTile::GetVertList(edgeVerts, vertexCount, 0);
			shapeDef.SetCW(edgeVerts, vertexCount, tileOffset);
				
			b2FixtureDef fixtureDef;
			fixtureDef.shape = &shapeDef;
			fixtureDef.userData = (void*)((tile0Info.HasCollision() && tile.GetSurfaceHasArea(0))? tile.GetSurfaceData(0) : tile.GetSurfaceData(1));
			fixtureDef.friction = Terrain::friction;
			fixtureDef.restitution = Terrain::restitution;
			GetPhysicsBody()->CreateFixture(&fixtureDef);
			continue;
		}

		if (tile.GetSurfaceHasArea(0) && tile0Info.HasCollision())
		{
			// use tile vert list to make the collision
			b2PolygonShape shapeDef;
			const Vector2 *edgeVerts = NULL;
			int vertexCount;
			BYTE edgeData = tile.GetEdgeData();

			TerrainTile::GetVertList(edgeVerts, vertexCount, edgeData);
			shapeDef.SetCW(edgeVerts, vertexCount, tileOffset);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &shapeDef;
			fixtureDef.userData = (void*)(tile.GetSurfaceData(0));
			fixtureDef.friction = Terrain::friction;
			fixtureDef.restitution = Terrain::restitution;
			GetPhysicsBody()->CreateFixture(&fixtureDef);
		}
		
		if (tile.GetSurfaceHasArea(1) && tile1Info.HasCollision())
		{
			// use tile vert list to make the collision
			b2PolygonShape shapeDef;
			const Vector2 *edgeVerts = NULL;
			int vertexCount;
			BYTE edgeData = tile.GetInvertedEdgeData();

			TerrainTile::GetVertList(edgeVerts, vertexCount, edgeData);
			shapeDef.SetCW(edgeVerts, vertexCount, tileOffset);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &shapeDef;
			fixtureDef.userData = (void*)(tile.GetSurfaceData(1));
			fixtureDef.friction = Terrain::friction;
			fixtureDef.restitution = Terrain::restitution;
			GetPhysicsBody()->CreateFixture(&fixtureDef);
		}
	}
	free(solidTileArray);
}

// returns true if position is in this terrain patch, false if it is not
// sets x and y to the tile array location
TerrainTile* TerrainPatch::GetTile(int x, int y, int layer)
{
	if (x < 0 || y < 0 || x >= Terrain::patchSize || y >= Terrain::patchSize)
		return NULL;
	
	return &GetTileLocal(x, y, layer);
}

// returns true if position is in this terrain patch, false if it is not
// sets x and y to the tile array location
TerrainTile* TerrainPatch::GetTile(const Vector2& testPos, int& x, int& y, int layer)
{
	const Vector2 pos = testPos - GetPosWorld();
	x = (int)(floorf(pos.x / TerrainTile::GetSize()));
	y = (int)(floorf(pos.y / TerrainTile::GetSize()));

	return GetTile(x, y, layer);
}

TerrainTile* TerrainPatch::GetTile(const Vector2& testPos, int layer)
{
	int x, y;
	return GetTile(testPos, x, y, layer);
}

GameObjectStub* TerrainPatch::GetStub(const Vector2& pos)
{
	GameObjectStub* bestStub = NULL;
	float bestDistance = FLT_MAX;

	// pick out the stub that we are nearest to the edge of, so we can pick out overlapping stubs
	for (list<GameObjectStub>::iterator it = objectStubs.begin(); it != objectStubs.end(); ++it) 
	{       
		GameObjectStub& stub = *it;
		if (stub.TestPosition(pos))
		{
			const Vector2 posLocal = stub.xf.Inverse().TransformCoord(pos);
			const float distance = Min(stub.size.x - fabs(posLocal.x), stub.size.y - fabs(posLocal.y));
			if (distance > bestDistance)
				continue;

			bestDistance = distance;
			bestStub = &stub;
		}
	}
	return bestStub;
}

GameObjectStub* TerrainPatch::GetStub(GameObjectHandle handle)
{
	for (list<GameObjectStub>::iterator it = objectStubs.begin(); it != objectStubs.end(); ++it) 
	{       
		GameObjectStub& stub = *it;
		if (stub.handle == handle)
			return &stub;
	}
	return NULL;
}

bool TerrainPatch::RemoveStub(GameObjectHandle handle)
{
	for (list<GameObjectStub>::iterator it = objectStubs.begin(); it != objectStubs.end(); ++it) 
	{       
		GameObjectStub& stub = *it;
		if (stub.handle != handle)
			continue;

		objectStubs.erase(it);
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
// Helpful console commands
//

static void ConsoleCallback_saveTerrain(const wstring& text)
{
	if (!g_terrain)
		return;

	g_terrain->Save(text.c_str());
	GetDebugConsole().AddFormatted(L"Terrain saved to '%s'", text.c_str());
}
ConsoleCommand(ConsoleCallback_saveTerrain, saveTerrain);

static void ConsoleCallback_clearTerrain(const wstring& text)
{
	if (!g_terrain)
		return;

	g_editor.ClearSelection();
	g_terrain->Clear();
	g_editor.SaveState();

	GetDebugConsole().AddLine(L"Terrain cleared.");
}
ConsoleCommand(ConsoleCallback_clearTerrain, clearTerrain);

static void ConsoleCallback_replaceTile(const wstring& text)
{
	if (!g_terrain)
		return;

	int oldTileID = -1;
	int newTileID = -1;
	int count = 1;
	swscanf_s(text.c_str(), L"%d %d %d", oldTileID, newTileID, count);

	if (oldTileID == -1 && newTileID == -1)
	{
		GetDebugConsole().AddLine(L"syntax: replaceTile oldTileId newTileId count.");
		return;
	}

	int replaceCount = 0;
	for(int x=0; x<Terrain::fullSize*Terrain::patchSize; ++x)
	for(int y=0; y<Terrain::fullSize*Terrain::patchSize; ++y)
	for(int l=0; l<Terrain::patchLayers; ++l)
	{
		TerrainTile* tile = g_terrain->GetTile(x,y, l);

		const int s1 = tile->GetSurfaceData(false);
		if (s1 >= oldTileID && s1 < oldTileID + count)
		{
			++replaceCount;
			tile->SetSurfaceData(false, s1 + (newTileID - oldTileID));
		}
		const int s2 = tile->GetSurfaceData(true);
		if (s2 >= oldTileID && s2 < oldTileID + count)
		{
			++replaceCount;
			tile->SetSurfaceData(true, s2 + (newTileID - oldTileID));
		}

	}

	g_editor.SaveState();

	GetDebugConsole().AddFormatted(L"Replaced %d tiles %d with %d.", replaceCount, oldTileID, newTileID);
}
ConsoleCommand(ConsoleCallback_replaceTile, replaceTile);