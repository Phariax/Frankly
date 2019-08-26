////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Globals
	Copyright 2013 Frank Force - http://www.frankforce.com

	All files in the game framework should include this file.
	This file should be set as the precomiled header.

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_GLOBALS_H
#define GAME_GLOBALS_H

#define gameTitle		(L"Frank Engine Test")
#define gameVersion		(L"0.13")
#define gameWebsite		(L"http://frankengine.frankforce.com")

#include "../../FrankEngine/source/frankEngine.h"

extern class GameControl*	g_gameControl;
extern class GameGui*		g_gameGui;
extern class Player*		g_player;
extern class PopupMessage*	g_popupMessage;
extern FrankFont*			g_gameFont;

////////////////////////////////////////////////////////////////////////////////////////
// object settings

enum GameObjectType
{
	GOT_Invalid = 0,
	GOT_First,
	GOT_Light = GOT_First,
	GOT_Enemy,
	GOT_HelpSwitch,
	GOT_ObjectSpawner,
	GOT_Crate,
	GOT_TriggerBox,
	GOT_TextDecal,
	GOT_MusicBox,
	
	// add new game object types here
	// this array is used to save out object stub ids
	// removing object types from this enum will throw eventhing below that off!

	GOT_Count
};

////////////////////////////////////////////////////////////////////////////////////////
// game resources

enum GameTextureID
{
	// GameTexture_Invalid
	GameTexture_StartGameTextures_ = GameTexture_StartGameTextures,
	GameTexture_Font1,
	GameTexture_LightGel1,
	GameTexture_Crate,
	
	// tile sheets
	GameTexture_tiles0,		// main tile sheet for objects
	GameTexture_terrainTiles0,	// terrain tile sheet

	// terrain tiles
	GameTexture_tile_test1,
	GameTexture_tile_test2,
	GameTexture_tile_test3,
	// add new terrain tiles here

	GameTexture_Count
};

enum SoundControl_ID
{
	// SoundControl_Invalid
	SoundControl_test = SoundControl_Start,
	
	// add new sounds here

	SoundControl_Count
};

////////////////////////////////////////////////////////////////////////////////////////
// game buttons

enum GameButtonIndex
{
	// GB_Invalid
	GB_Reset = GB_StartGameButtons,
	GB_MoveUp,
	GB_MoveDown,
	GB_MoveRight,
	GB_MoveLeft,
	GB_Trigger1,
	GB_Trigger2,
	GB_Pause,
	GB_TimeScale,
	GB_Test,
	
	// add new game buttons here

	GB_Count,
};

////////////////////////////////////////////////////////////////////////////////////////
// terrain settings

enum GameMaterialIndex
{
	// GMI_Invalid
	GMI_Test = GMI_Start,

	// add new game material indexes here

	GMI_Count
};

void InitSurfaceInfoArray();
extern GameSurfaceInfo surfaceInfoArray[TERRAIN_UNIQUE_TILE_COUNT];

////////////////////////////////////////////////////////////////////////////////////////
// teams

enum GameTeam
{
	GameTeam_none = 0,
	GameTeam_enemy,
	GameTeam_player
};

////////////////////////////////////////////////////////////////////////////////////////
// physics groups
// object will not collide with others in the same group
// exept for group 0 which collides with everything

enum PhysicsGroup
{
	PhysicsGroup_none = 0,
	PhysicsGroup_player,
	PhysicsGroup_count
};

////////////////////////////////////////////////////////////////////////////////////////
// damage type

enum GameDamageType
{
	//GameDamageType_Default
	GameDamageType_Explosion = 1
};

////////////////////////////////////////////////////////////////////////////////////////
// renderGroup

enum RenderGroup
{
	RenderGroup_superLow = -10000,
	RenderGroup_backgroundTerrain = -1000,
	
	RenderGroup_low = -500,
	RenderGroup_effect = -200,
	RenderGroup_additiveEffect = -100,
	RenderGroup_default = 1,
	RenderGroup_foregroundEffect = 200,
	RenderGroup_foregroundAdditiveEffect = 300,
	RenderGroup_foregroundTerrain = 1000,
	RenderGroup_superHigh = 10000
};

////////////////////////////////////////////////////////////////////////////////////////

// global game includes
#include "gameControl.h"
#include "player.h"
#include "gameGui.h"

#endif // GAME_GLOBALS_H