////////////////////////////////////////////////////////////////////////////////////////
/*
	Terrain Tile Surface
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef TERRAIN_SURFACE_H
#define TERRAIN_SURFACE_H

#include "../terrain/terrainTile.h"

enum GameMaterialIndex;
const GameMaterialIndex GMI_Invalid = GameMaterialIndex(0);
const GameMaterialIndex GMI_Start = GameMaterialIndex(1);
struct GameSurfaceInfo;

enum GameSurfaceInfoFlags
{
	GSI_None				= 0x00,		// no flags
	GSI_Collision			= 0x01,		// build collision for tile
	GSI_Destructible		= 0x02,		// tile can be destroyed
	GSI_ForegroundOcclude	= 0x04,		// forground tile is solid and occlude background
};

typedef void (*TileCreateCallback)(const GameSurfaceInfo& tileInfo, const Vector2& pos, int layer);

struct GameSurfaceInfo
{
	GameSurfaceInfo
	(
		WCHAR *_name = NULL,
		GameTextureID _ti = GameTexture_Invalid,
		GameMaterialIndex _materialIndex = GMI_Invalid,
		const Color& _color = Color::White(),
		const Color& _backgroundColor = Color::Grey(1, 0.3f),
		UINT _flags = GSI_Collision,
		const Color& _shadowColor = Color::Black(),
		const Color& _emissiveColor = Color::Black(),
		TileCreateCallback _tileCreateCallback = NULL
	) :
		name(_name),
		ti(_ti),
		materialIndex(_materialIndex),
		color(_color),
		backgroundColor(_backgroundColor),
		flags(_flags),
		shadowColor(_shadowColor),
		emissiveColor(_emissiveColor),
		tileCreateCallback(_tileCreateCallback),
		textureWrapSize(1),
		id(-1)
	{}

	static void Init(GameSurfaceInfo* gameSurfaceInfoArray);
	static bool NeedsEdgeCollision(const TerrainTile& tile);

	static int Count() { return surfaceInfoCount; }
	static const GameSurfaceInfo& Get(BYTE surfaceIndex) 
	{ 
		ASSERT(gameSurfaceInfoArray);
		return gameSurfaceInfoArray[surfaceIndex]; 
	}

	bool HasCollision() const		{ return (flags & GSI_Collision) != 0; }
	bool IsDestructible() const		{ return (flags & GSI_Destructible) != 0; }
	
	void SetID(int _id)				{ id = _id; }
	const int GetID() const			{ return id; }

	bool IsSameRenderGroup(const GameSurfaceInfo& other) const 
	{ 
		return
		(
			ti == other.ti && 
			color == other.color && 
			backgroundColor == other.backgroundColor && 
			shadowColor == other.shadowColor && 
			emissiveColor == other.emissiveColor && 
			textureWrapSize == other.textureWrapSize
		);
	}


public: // data

	WCHAR *name;
	GameTextureID ti;
	GameMaterialIndex materialIndex;
	Color color;
	Color backgroundColor;
	UINT flags;
	Color shadowColor;
	Color emissiveColor;
	TileCreateCallback tileCreateCallback;
	Vector2 textureWrapSize;
	int id;

private:
	
	static int surfaceInfoCount;
	static GameSurfaceInfo* gameSurfaceInfoArray;
};



#endif // TERRAIN_SURFACE_H