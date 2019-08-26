////////////////////////////////////////////////////////////////////////////////////////
/*
	Base Class for High Leve Game Control
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_CONTROL_BASE_H
#define GAME_CONTROL_BASE_H

enum GameMode
{
	GameMode_First = 0,
	GameMode_Normal = GameMode_First,
	GameMode_Edit,
	GameMode_Count
};

class GameControlBase
{
public:

	GameControlBase();
	virtual ~GameControlBase();
	
	virtual void Init();
	virtual void Reset();
	virtual void LoadTextures() {}
	virtual void LoadSounds() {}
	virtual void SetupInput();

	virtual void Render();
	virtual void SetupRender();
	virtual void RenderPre() {};
	virtual void RenderPost();
	virtual void RenderInterpolatedObjects();
	virtual void RenderDebugText();

	virtual void Update(float delta);

	virtual void InitDeviceObjects();
	virtual void DestroyDeviceObjects();

	InputControl& GetInputControl() { return *g_input; }
	virtual Vector2 GetUserPosition() const;
	virtual Vector2 GetUserPositionInterpolated() const;

	UINT GetRenderFrameCount() const { return renderFrameCount; }
	UINT GetUpdateFrameCount() const { return updateFrameCount; }

	// gamemode functions
	void			SetGameMode(GameMode _gameMode);
	GameMode		GetGameMode() const			{ return gameMode; }
	bool			IsGameplayMode() const		{ return gameMode == GameMode_Normal; }
	bool			IsEditMode() const			{ return gameMode == GameMode_Edit; }
	bool			WasEditMode() const			{ return lastResetMode == GameMode_Edit; }
	bool			IsObjectEditMode() const;
	bool			IsTileEditMode() const;
	virtual bool	HideMouseCursor() const		{ return !IsPaused() && IsGameplayMode(); }

	float GetResetTime() const			{ return resetTime; }
	float GetTotalTime() const			{ return totalTimer; }
	float GetTimeScale() const			{ return timeScale; }
	float CalculateInterpolationPercent();
	
	bool IsPaused() const { return paused; }
	void SetPaused(bool _paused) { paused = _paused; }

	// hook from editor to generate random terrain
	virtual void RandomizeTerrain() {}

public: // bridge to gameside stuff

	virtual const WCHAR* GetGameTitle() const	{ return L"Untitled"; }
	virtual const WCHAR* GetGameVersion() const	{ return L"0.0"; }

	// access to game objects
	virtual GameObject*	GetPlayer() = 0;

protected:

	void UpdateFrameInternal(float delta);
	virtual void UpdateFrame(float delta) {}

	double updateTimer;			// store left over update time
	GameTimer resetTime;		// time since last reset
	GameTimer totalTimer;		// total ammount of time it's been running
	GameMode gameMode;			// current game/edit mode
	GameMode lastResetMode;		// last game mode
	float timeScale;			// time dialation factor
	float timeScaleLast;		// time dialation factor
	bool paused;				// is the game in a paused state?
	bool resetWorldXForms;		// will cause all transforms to be reset at end of frame
	UINT renderFrameCount;		// how many frames have been renderd
	UINT updateFrameCount;		// how many game updates have happened

public:

	static bool showDebugInfo;				// debug info is toggled by pressing F1
	static bool autoSaveTerrain;			// save terrain when exiting edit mode
	static bool editorMovePlayerToMouse;	// moves player to mouse when exiting edit mode
	static bool useFrameDeltaSmoothing;		// system to smooth frame rate when fullscreen
	static bool useRefreshRateAsFrameDelta;	// force use the refresh rate as the frame delta
	static bool devMode;					// allow engine function keys and level editor
	static bool updateWithoutWindowFocus;	// allow background updating
	static IntVector2 debugInfoPos;			// screen coords to show debug info
	static float debugTimeScale;			// time scale for debugging
};

#endif // GAME_CONTROL_BASE_H