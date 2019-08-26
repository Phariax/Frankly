////////////////////////////////////////////////////////////////////////////////////////
/*
	Terrain
	Copyright 2013 Frank Force - http://www.frankforce.com
	
	- static terrain to form the world
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef TERRAIN_H
#define TERRAIN_H

#include "../objects/gameObject.h"
#include "../terrain/terrainTile.h"
#include "../terrain/terrainSurface.h"

////////////////////////////////////////////////////////////////////////////////////////
// terrain defines

class TerrainLayerRender;

class TerrainPatch : public GameObject
{
public:

	TerrainPatch(const Vector2& pos);
	~TerrainPatch();

	TerrainTile& GetTileLocal(int x, int y, int layer = 0) const;
	static bool IsTileIndexValid(int x, int y, int layer = 0);
	bool GetTileLocalIsSolid(int x, int y) const;
	Vector2 GetTilePos(int x, int y) const { return GetPosWorld() + TerrainTile::GetSize() * Vector2((float)x, (float)y); }
	void RebuildPhysics() { needsPhysicsRebuild = true; }
	Vector2 GetCenter() const;
	Box2AABB GetAABB() const;

	GameObjectStub* GetStub(GameObjectHandle handle);
	bool RemoveStub(GameObjectHandle handle);
	GameObjectStub* GetStub(const Vector2& pos);
	list<GameObjectStub>& GetStubs() { return objectStubs; }
	
	void Clear();
	void ClearTileData();
	void ClearTileData(int layer);
	void ClearObjectStubs();

	TerrainTile* GetTile(const Vector2& testPos, int layer = 0);
	TerrainTile* GetTile(const Vector2& testPos, int& x, int& y, int layer = 0);
	TerrainTile* GetTile(int x, int y, int layer = 0);

	virtual bool IsTerrain() const { return true; }

	virtual bool ShouldCollide(const GameObject& otherObject, const b2Shape* myShape, const b2Shape* otherShape) const { return !otherObject.IsTerrain(); }

public: // internal engine functions

	bool HasActivePhysics() const { return activePhysics; }
	bool HasActiveObjects() const { return activeObjects; }

	void Deactivate()
	{
		SetActivePhysics(false);
		SetActiveObjects(false);
	}

	void SetActivePhysics(bool _activePhysics);
	void SetActiveObjects(bool _activeObjects, bool windowMoved = false);

	void CreatePolyPhysicsBody(const Vector2 &pos);
	void CreateEdgePhysicsBody(const Vector2 &pos);

	GameObjectStub* AddStub(const GameObjectStub& stub) 
	{ 
		objectStubs.push_back(stub); 
		return &objectStubs.back();
	}

	bool RemoveStub(GameObjectStub* stub) 
	{ 
		for (list<GameObjectStub>::iterator it = objectStubs.begin(); it != objectStubs.end(); ++it) 
		{       
			if (stub == &(*it))
			{
				objectStubs.erase(it);
				return true;
			}
		}

		return false;
	}

public: // data members

	TerrainTile *tiles;
	list<GameObjectStub> objectStubs;
	
	bool activePhysics;
	bool activeObjects;
	bool needsPhysicsRebuild;
};

class Terrain : public GameObject
{
public:

	friend class TileEditor;
	friend class ObjectEditor;

	Terrain(const Vector2& pos);
	~Terrain();

	void ResetStartHandle();

	void Deactivate();
	void ActivateAreaAroundPlayer();

	TerrainLayerRender* GetLayerRender(int i) { ASSERT(i >= 0 && i < patchLayers); return layerRenderArray[i]; }

	IntVector2 GetPatchOffset(const Vector2& pos) const
	{
		const Vector2 offset = (pos - GetPosWorld()) / (patchSize*TerrainTile::GetSize());
		return IntVector2((int)floorf(offset.x), (int)floorf(offset.y));
	}

	Vector2 GetPatchFloatOffset(const Vector2& pos) const
	{ return (pos - GetPosWorld()) / (patchSize*TerrainTile::GetSize()); }
	
	GameObjectStub* GetStub(GameObjectHandle handle);
	bool RemoveStub(GameObjectHandle handle, TerrainPatch* patch = NULL);
	GameObjectStub* GetStub(const Vector2& pos);

	TerrainPatch* GetPatch(const Vector2& pos) const
	{
		const IntVector2 offset = GetPatchOffset(pos);
		return (IsPatchIndexInvalid(offset.x, offset.y) ? NULL : GetPatch(offset.x, offset.y));
	}

	TerrainPatch* GetPatch(int x, int y) const
	{
		if (IsPatchIndexInvalid(x,y))
			return NULL;
		else
			return patches[x + fullSize * y];
	}

	Box2AABB GetStreamWindow() const { return streamWindow; }
	void UpdateActiveWindow(bool init = false);
	void UpdatePost();
	
	IntVector2 GetTileOffset(const Vector2& pos) const;
	Vector2 GetTilePos(int x, int y) const { return GetPosWorld() + TerrainTile::GetSize() * Vector2((float)x, (float)y); }
	TerrainTile* GetTile(const Vector2& pos, int& x, int& y, int layer = 0) const;
	TerrainTile* GetTile(const Vector2& pos, int layer = 0) const;
	TerrainTile* GetTile(int x, int y, int layer = 0) const;
	int GetSurfaceSide(const Vector2& pos, int layer = 0) const;
	BYTE GetSurfaceIndex(const Vector2& pos, int layer = 0) const;
	void SetSurfaceIndex(const Vector2& pos, BYTE surface, int layer = 0);

	void Deform(const Vector2& pos, float radius);
	void DeformTile(const Vector2& startPos, const Vector2& direction, const GameObject* ignoreObject = NULL, GameMaterialIndex gmi = GMI_Invalid, bool clear = false, float distance = 3);

	// quick test if there is a given area is totally clear or not
	bool IsClear(const Vector2& pos, int layer = 0)
	{
		TerrainTile* tile = GetTile(pos);
		return (!tile || tile->IsClear());
	}

	static bool IsPatchIndexInvalid(int x, int y) { return (x<0 || x>=fullSize || y<0 || y>=fullSize); }

	void Save(const WCHAR* filename);
	void Load(const WCHAR* filename);

	void Clear();

	TerrainTile* GetConnectedTileA(int x, int y, int &x2, int &y2, int layer = 0);
	TerrainTile* GetConnectedTileB(int x, int y, int &x2, int &y2, int layer = 0);

	TerrainTile* GetConnectedTileA(int x, int y)
	{
		int x2, y2;
		return GetConnectedTileA(x, y, x2, y2);
	}
	TerrainTile* GetConnectedTileB(int x, int y)
	{
		int x2, y2;
		return GetConnectedTileB(x, y, x2, y2);
	}
	
	Vector2 GetPlayerEditorStartPos() const { return playerEditorStartPos; }
	void SetPlayerEditorStartPos(const Vector2& pos) { playerEditorStartPos = pos; }

	bool IsPersistant() const { return true; }
	bool DestroyOnWorldReset() const { return false; }

	// what handle to use for new objects
	GameObjectHandle GetStartHandle() const { return startHandle; }
	
	static void SetTileSetTexture(int i, GameTextureID tileSetTi) { ASSERT(i >= 0 && i < maxTileSets); tileSets[i] = tileSetTi; }
	static GameTextureID GetTileSetTexture(int i) { ASSERT(i >= 0 && i < maxTileSets); return tileSets[i]; }

	void GiveStubNewHandle(GameObjectStub& stub);
	void CheckForErrors();

	virtual bool IsTerrain() const { return true; }

public: // settings

	static int dataVersion;					// used to prevent old version from getting loaded
	static int fullSize;					// how many patches per terrain
	static int patchSize;					// how many tiles per patch
	static int patchLayers;					// how many layers patch
	static int physicsLayer;				// the layer to create physics for
	static int windowSize;					// size of the terrain stream window
	static int renderWindowSize;			// size of the terrain render window
	static Color debugOutlineColor1;		// color used for lines around tiles when editing
	static Color debugOutlineColor2;		// color used for lines around tiles when editing
	static Vector2 gravity;					// acceleartion due to gravity
	static WCHAR terrainFilename[256];		// filename used for save/load terrain operations
	static float restitution;				// restitution for terrain physics
	static float friction;					// friction for terrain physics
	static bool usePolyPhysics;				// should polygons be used instead of edge shapes for collision
	static bool combineTileShapes;			// optimization to combine physics shapes for tiles
	static bool enableStreaming;			// streaming of objects and physics for the window around the player
	static int maxProxies;					// limit on how many terrain proxies can be made

	// circular planet config
	static bool isCircularPlanet;			// should terrain be treated like a circular planet?
	static float planetRadius;				// average radius of planet
	static float planetGravityConstant;		// gravity constant only used for circular planets
	static float planetMinAltitude;			// sea level, use to similate round planets
	static float planetMaxAltitude;			// atmosphere height, use to similate round planets
	static bool planetAltitudeDamping;	// fade off linear damping as we exit planet atmosphere
	
	// tile sheets
	static int tileSetCount;						// how many tile sets there are
	static const int maxTileSets = 256;				// how many tile sets max
	static GameTextureID tileSets[maxTileSets];	// map of which tile set references which texture

private:

	// returns trajectory angles for a deltaPos with gravity in the -y direction
	// returns false if it is not possible to hit the target
	bool GetTrajectoryAngle(const Vector2& deltaPos, float speed, float gravity, float& angle1, float& angle2);
	
	void UpdateStreaming();
	void LoadFromResource(const WCHAR* filename);
	
	IntVector2 streamWindowPatch;
	IntVector2 streamWindowPatchLast;
	Box2AABB streamWindow;
	Vector2 playerEditorStartPos;
	TerrainPatch **patches;
	GameObjectHandle startHandle;
	TerrainLayerRender** layerRenderArray;

	friend class TerrainRender;
};

inline Box2AABB TerrainPatch::GetAABB() const 
{ 
	return Box2AABB(GetTilePos(0,0), GetTilePos(Terrain::patchSize,Terrain::patchSize)); 
}

inline Vector2 TerrainPatch::GetCenter() const
{
	const Vector2 minPatchPos = GetTilePos(0, 0);
	const Vector2 maxPatchPos = GetTilePos(Terrain::patchSize, Terrain::patchSize);
	return minPatchPos + 0.5f * (maxPatchPos - minPatchPos);
}

inline bool TerrainPatch::IsTileIndexValid(int x, int y, int l)
{ 
	return (x >= 0 && x < Terrain::patchSize && y >= 0 && y < Terrain::patchSize && l >= 0 && l < Terrain::patchLayers); 
}

inline TerrainTile& TerrainPatch::GetTileLocal(int x, int y, int layer) const
{
	ASSERT(IsTileIndexValid(x,y,layer));
	return tiles[Terrain::patchSize*Terrain::patchSize*layer + Terrain::patchSize * x + y];
}

#endif // TERRAIN_H