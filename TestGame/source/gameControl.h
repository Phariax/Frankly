////////////////////////////////////////////////////////////////////////////////////////
/*
	High Leve Game Control
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_CONTROLLER_H
#define GAME_CONTROLLER_H

#include "player.h"

class GameControl : public GameControlBase
{
public:

	GameControl();

	virtual void Reset();
	virtual void LoadTextures();
	virtual void LoadSounds();
	virtual void SetupInput();
	virtual void InitDeviceObjects();
	virtual void DestroyDeviceObjects();
	bool IsUsingGamepad() const { return usingGamepad; }
	
	virtual void UpdateFrame(float delta);
	virtual void RenderInterpolatedObjects();
	virtual void RenderPost();
	virtual void RenderDebugText();

public: // game stuff

	virtual const WCHAR*	GetGameTitle() const	{ return gameTitle; }
	virtual const WCHAR*	GetGameVersion() const	{ return gameVersion; }
	virtual GameObject*		GetPlayer()				{ return g_player; }
	bool usingGamepad;
};

#endif // GAME_CONTROLLER_H