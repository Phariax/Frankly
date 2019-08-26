////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Globals
	Copyright 2013 Frank Force - http://www.frankforce.com

	All files in the game framework should include this file.
	This file should be set as the precomiled header.
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "gameGlobals.h"

////////////////////////////////////////////////////////////////////////////////////////
// customizable surface info

// array of all the terrain surface infos
GameSurfaceInfo surfaceInfoArray[TERRAIN_UNIQUE_TILE_COUNT];

// make background tiles a little darker by default
const Color backgroundColor = Color::Grey(1, 0.8f);

// surfaces can have a special callback for when a tile is created
void CreateLightTileCallback(const GameSurfaceInfo& tileInfo, const Vector2& pos, int layer)
{
	// automatically spawn a light whenever this tile type gets created
	// note: be frugal with this, only a few lights are necessary to light a scene
	Light* light = new Light(pos, NULL, 10, Color::White(0.75f), true);
	light->SetOverbrightRadius(0.8f);
	light->SetHaloRadius(0.8f);

	//TerrainTile* tile = g_terrain->GetTile(pos);
	//pos.RenderDebug(Color::Red(), 0.5f, 5);
}

void InitSurfaceInfoArray()
{
	// setup all the surface infos for the terrain
	// todo: make this read from xml

	if (Terrain::tileSetCount > 0)
	{
		// tile set style terrain

		int id = 0;
		for(int j = 0; j < 16; ++j)
		for(int i = 0; i < 16; ++i)
		{
			ASSERT(id < TERRAIN_UNIQUE_TILE_COUNT);
			GameSurfaceInfo& info = surfaceInfoArray[id];

			info.name = L"Unnamed Tile Type";
			//info.ti = GameTexture_terrainTiles0;//GameTextureID(GameTexture_tile_terrainStart + id);
			info.color = Color::White();
			info.backgroundColor = Color::White();
			info.emissiveColor = Color::White();
			info.flags = GSI_Collision;
			id++;
		}
	
		{
			// default transparent colors on the bottom row
			int i = 7;
			surfaceInfoArray[i  ].name = L"Red Transparent";
			surfaceInfoArray[i++].shadowColor = Color::Red();
			surfaceInfoArray[i  ].name = L"Yellow Transparent";
			surfaceInfoArray[i++].shadowColor = Color::Yellow();
			surfaceInfoArray[i  ].name = L"Green Transparent";
			surfaceInfoArray[i++].shadowColor = Color::Green();
			surfaceInfoArray[i  ].name = L"Blue Transparent";
			surfaceInfoArray[i++].shadowColor = Color::Blue();
			surfaceInfoArray[i  ].name = L"Clear Transparent";
			surfaceInfoArray[i++].shadowColor = Color::White(0);
		}

		{
			// automatically create a light for this tile type
			GameSurfaceInfo& info = surfaceInfoArray[12];
			info.name = L"Auto Light";
			info.tileCreateCallback = CreateLightTileCallback;
		}
	}
	else
	{
		// non-tile set style terrain

		int id = 0;
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Clear";
			info.ti = GameTexture_Invalid;
			info.color = Color::White();
			info.backgroundColor = Color::Grey(1, 0.5f);
			info.flags = GSI_None;
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Tile Example";
			info.ti = GameTexture_tile_test1;
			info.color = Color(0.6f, 0.3f, 0.1f);
			info.backgroundColor = Color(0.6f, 0.3f, 0.1f);
			//info.textureWrapSize = Vector2(0.5f);
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Tile Example Background";
			info.ti = GameTexture_tile_test1;
			info.color = Color(0.8f, 0.75f, 0.65f);
			info.backgroundColor = Color(0.8f, 0.75f, 0.65f);
			//info.textureWrapSize = Vector2(0.5f);
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Tile Example 2";
			info.ti = GameTexture_tile_test2;
			info.color = Color::Grey(1, 0.5f);
			info.backgroundColor = backgroundColor;
			info.shadowColor = Color::Black();
			info.emissiveColor = Color::Black();
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Black";
			info.ti = GameTexture_tile_test3;
			info.color = Color::Black();
			info.backgroundColor = Color::Black();
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Grey";
			info.ti = GameTexture_tile_test3;
			info.color = Color::Grey();
			info.backgroundColor = Color::Grey(1, 0.3f);
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"White";
			info.ti = GameTexture_tile_test3;
			info.color = Color::White();
			info.backgroundColor = Color::White();
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Red Transparent";
			info.ti = GameTexture_tile_test3;
			info.color = Color::Red(0.5f);
			info.backgroundColor = Color::Red(0.5f);
			info.shadowColor = Color::Red();
			info.emissiveColor = Color::Black(0.5f);
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Yellow Transparent";
			info.ti = GameTexture_tile_test3;
			info.color = Color::Yellow(0.5f);
			info.backgroundColor = Color::Yellow(0.5f);
			info.shadowColor = Color::Yellow();
			info.emissiveColor = Color::Black(0.5f);
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Green Transparent";
			info.ti = GameTexture_tile_test3;
			info.color = Color::Green(0.5f);
			info.backgroundColor = Color::Green(0.5f);
			info.shadowColor = Color::Green();
			info.emissiveColor = Color::Black(0.5f);
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Blue Transparent";
			info.ti = GameTexture_tile_test3;
			info.color = Color::Blue(0.5f);
			info.backgroundColor = Color::Blue(0.5f);
			info.shadowColor = Color::Blue();
			info.emissiveColor = Color::Blue(0.5f);
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Clear Transparent";
			info.color = Color::White(0.2f);
			info.backgroundColor = Color::White(0.5f);
			info.shadowColor = Color::White(0);
			info.emissiveColor = Color::Black(0);
		}
		{
			GameSurfaceInfo& info = surfaceInfoArray[id++];
			info.name = L"Auto Light";
			info.color = Color::White();
			info.backgroundColor = Color::White();
			info.shadowColor = Color::Black();
			info.emissiveColor = Color::Black();
			info.tileCreateCallback = CreateLightTileCallback;
		}
	}
	// add new surface infos here
	// note: order is important in this array!
	// todo: add way to reorder stuff for the editor

	// init the surface infos
	GameSurfaceInfo::Init(surfaceInfoArray);
}