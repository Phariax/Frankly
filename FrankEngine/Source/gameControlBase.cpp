////////////////////////////////////////////////////////////////////////////////////////
/*
	Base Class for High Leve Game Control
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"

////////////////////////////////////////////////////////////////////////////////////////
//
//	Game Globals
//
////////////////////////////////////////////////////////////////////////////////////////

InputControl*		g_input		= NULL;
Terrain*			g_terrain	= NULL;
SoundControl*		g_sound		= NULL;
	
bool GameControlBase::showDebugInfo = false;
ConsoleCommand(GameControlBase::showDebugInfo, debugInfoEnable);

bool GameControlBase::autoSaveTerrain = true;
ConsoleCommand(GameControlBase::autoSaveTerrain, autoSaveTerrain);

bool GameControlBase::editorMovePlayerToMouse = true;
ConsoleCommand(GameControlBase::editorMovePlayerToMouse, editorMovePlayerToMouse);

bool GameControlBase::useFrameDeltaSmoothing = true;
ConsoleCommand(GameControlBase::useFrameDeltaSmoothing, useFrameDeltaSmoothing);

bool GameControlBase::useRefreshRateAsFrameDelta = false;
ConsoleCommand(GameControlBase::useRefreshRateAsFrameDelta, useRefreshRateAsFrameDelta);

bool GameControlBase::updateWithoutWindowFocus = false;
ConsoleCommand(GameControlBase::updateWithoutWindowFocus, updateWithoutWindowFocus);

float GameControlBase::debugTimeScale = 1;
ConsoleCommand(GameControlBase::debugTimeScale, timeScale);

#ifdef DEBUG
bool GameControlBase::devMode = true;
#else
bool GameControlBase::devMode = false;
#endif // DEBUG
ConsoleCommand(GameControlBase::devMode, devMode);

IntVector2 GameControlBase::debugInfoPos(8, 40);
ConsoleCommand(GameControlBase::debugInfoPos, debugInfoPos);

static bool showFps = false;
ConsoleCommand(showFps, showFps);

////////////////////////////////////////////////////////////////////////////////////////
//
//	Game Control Member Functions
//
////////////////////////////////////////////////////////////////////////////////////////

GameControlBase::GameControlBase() :
	updateTimer(0),
	totalTimer(0),
	timeScale(1),
	timeScaleLast(1),
	gameMode(GameMode_Normal),
	lastResetMode(GameMode_Normal),
	paused(false),
	resetWorldXForms(false),
	renderFrameCount(0),
	updateFrameCount(0)
{
	ASSERT(!g_gameControlBase);
}

GameControlBase::~GameControlBase()
{
	// we must remove all objects from the world
	g_objectManager.RemoveAll();
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Init, reset and respawn functions
//
////////////////////////////////////////////////////////////////////////////////////////

void GameControlBase::Init() 
{ 
	Reset();
}

void GameControlBase::Reset()
{
	if (GetPlayer() && g_terrain)
		g_terrain->SetPlayerEditorStartPos(GetPlayer()->GetPosWorld());

	g_editor.ResetEditor();

	if (autoSaveTerrain && WasEditMode())
	{
		// auto save when exiting edit mode, and auto load when entering edit
		// save terrain automatically when exiting edit mode
		g_terrain->Save(Terrain::terrainFilename);
	}

	SetPaused(false);

	resetTime.Set();

	// reset the terrain
	// this is to make sure object handles are reset
	if (g_terrain)
		g_terrain->ResetStartHandle();

	// reset the world, removing all objects set to destroy on reset
	g_objectManager.Reset();
	
	// trigger reset of transforms
	resetWorldXForms = true;

	if (!g_terrain)
	{
		// create some terrain
		const Vector2 pos = -(Terrain::fullSize * Terrain::patchSize * TerrainTile::GetSize()/2.0f) * Vector2(1);
		g_terrain = new Terrain(pos);
		
		// terrain must be loaded before other objects start getting created
		g_terrain->Load(Terrain::terrainFilename);
	}
	else if (GameControlBase::autoSaveTerrain)
	{
		// reload terrain on reset
		g_terrain->Load(Terrain::terrainFilename);
	}

	static bool first = true;
	if (first)
	{
		// HACK: position the camera around the player on startup
		g_cameraBase->SetPosWorld(g_terrain->GetPlayerEditorStartPos());
		first = false;
	}

	lastResetMode = gameMode;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Updating
//
////////////////////////////////////////////////////////////////////////////////////////

void GameControlBase::Update(float delta)
{
	FrankProfilerEntryDefine(L"GameControlBase::Update()", Color::Yellow(), 1);

	// low frame rate test
	//float a; for(int i = 0; i < 400000; ++i) { a = sqrt((float)i); }
	
	// clamp min frame rate, anything below this will get slowed down
	static float minFrameRate = 15.0f;
	delta = Min(delta, 1/minFrameRate);

	if (DXUTIsVsyncEnabled() && DXUTGetRefreshRate() > 0)
	{
		if (useFrameDeltaSmoothing)
		{
			// show the actual delta in the output window
			//static WCHAR buffer[64];
			//swprintf_s( buffer, 64, L"%f\n",delta );
			//OutputDebugString(buffer);

			// this buffer keeps track of the extra bits of time
			static float deltaBuffer = 0;

			// add in whatever time we currently have saved in the buffer
			delta += deltaBuffer;

			// calculate how many frames the delta is telling us have passed
			const int refreshRate = DXUTGetRefreshRate();
			int frameCount = (int)(delta * refreshRate + 1);

			// if less then a full frame, increase delta to cover the extra
			if (frameCount <= 0)
				frameCount = 1;
			
			// save off the delta, we need it later to update the buffer
			const float oldDelta = delta;

			// re-calculate delta to be a frame rate multiple
			delta = frameCount / (float)refreshRate;

			// update delta buffer so we keep the same time
			deltaBuffer = oldDelta - delta;
		}

		if (useRefreshRateAsFrameDelta)
		{
			// the easy way works if we know we can render at the refersh rate
			const int fps = DXUTGetRefreshRate();
			delta = 1 / (float)fps;
		}
	}
	
	GamePauseTimer::UpdateGlobal(delta);
	GetDebugConsole().Update(delta);

	// apply time scale
	if (!paused)
		delta *= timeScale;
	delta *= debugTimeScale;

	// wipe out left over time at start of frame
	if (!paused)
		GameTimer::SetLeftOverGlobal(0);

	// step it in fixed increments
	updateTimer += delta;
	int updateCount = 0;
	while (updateTimer >= GAME_TIME_STEP) 
	{
		updateCount++;
		updateTimer -= GAME_TIME_STEP;
		delta -= GAME_TIME_STEP;
		if (!paused)
			GameTimer::UpdateGlobal(GAME_TIME_STEP);

		UpdateFrameInternal(GAME_TIME_STEP);
	}

	// update interpolation percent
	g_interpolatePercent = (IsGameplayMode()? CalculateInterpolationPercent() : 0);

	// update the sound listener
	g_sound->Update();
		
	// push out any simple verts left in the buffer
	//g_render->RenderSimpleVerts();
	
	// keep track of left over time, this will be used for interpolation during rendering
	if (!paused)
		GameTimer::SetLeftOverGlobal(float(updateTimer));

	const float timeScaleInterpolated = timeScaleLast + g_interpolatePercent * (timeScale*debugTimeScale - timeScaleLast);
	g_sound->UpdateTimeScale(timeScaleInterpolated);
	g_sound->GetMusicPlayer().SetFrequencyScale(timeScaleInterpolated);
	timeScaleLast = timeScale*debugTimeScale;
}

void GameControlBase::UpdateFrameInternal(float delta)
{
	// update the frame count
	++updateFrameCount;

	// disable interpolation during update
	g_interpolatePercent = 0;

	g_input->Update();

	if (devMode && g_input->WasJustPushed(GB_InfoDisplay))
		showDebugInfo = !showDebugInfo;

	// toggle editor
	if (g_input->WasJustPushed(GB_Editor))
	{
		if (IsGameplayMode())
		{
			if (devMode)
				SetGameMode(GameMode_Edit);
		}
		else
		{
			g_editorGui.Reset();

			if (editorMovePlayerToMouse)
			{
				// set player to cursor pos before returning to game
				const Vector2 pos = g_input->GetMousePosWorldSpace();

				// prevent moving player outside the physics world bounds
				if (g_physics->IsInWorld(pos))
					g_gameControlBase->GetPlayer()->SetXFormWorld(XForm2(pos, g_gameControlBase->GetPlayer()->GetPhysicsXForm().angle));
			}

			SetGameMode(GameMode_Normal);
		}
	}

	if (devMode && g_input->WasJustPushed(GB_RefreshResources))
	{
		DestroyDeviceObjects();
		InitDeviceObjects();

		g_debugMessageSystem.AddFormatted(L"Texture and sound resources have been reloaded.");
	}

	if (g_input->WasJustPushed(GB_Screenshot))
		g_cameraBase->SaveScreenshot();
	
	{
		static int gamepadScreenShotButton = -1;
		ConsoleCommand(gamepadScreenShotButton, gamepadScreenShotButton);
		if (gamepadScreenShotButton >= 0 && g_input->IsGamepadButtonDown(GamepadButtons(GAMEPAD_BUTTON_0 + gamepadScreenShotButton), 0))
		{
			g_cameraBase->SaveScreenshot();
		}
	}

	if (devMode && g_input->WasJustPushed(GB_Profiler))
		FrankProfiler::ToggleDisplay();

	{
		// setting to automatically take a screenshot every few seconds while playing
		static float autoScreenShotTime = 0;
		ConsoleCommand(autoScreenShotTime, autoScreenShotTime);
		if (autoScreenShotTime > 0 && !IsEditMode())
		{
			static GameTimer screenShotSaveTimer;
			if (!screenShotSaveTimer.IsValid() || screenShotSaveTimer > autoScreenShotTime)
			{
				g_cameraBase->SaveScreenshot();
				screenShotSaveTimer.Set();
			}
		}
	}

	if (devMode && g_input->WasJustPushed(GB_PhysicsDebug))
		g_physicsRender.SetDebugRender(!g_physicsRender.GetDebugRender());

	g_debugRender.Update(delta);

	if (!paused)
	{
		// update transforms after physics
		g_objectManager.UpdateTransforms();

		if (IsGameplayMode())
		{
			g_terrain->UpdateActiveWindow();
			g_physics->Update(delta);
		}
		
		if (IsEditMode())
			g_editor.Update();
		
		// update the camera window
		g_cameraBase->PrepForUpdate();
		
		if (IsGameplayMode())
			g_objectManager.Update();
	}
	
	UpdateFrame(delta);
	
	if (resetWorldXForms)
	{
		// reset all transforms when game is reset
		g_objectManager.UpdateTransforms();
		g_objectManager.SaveLastWorldTransforms();
		resetWorldXForms = false;

		g_terrainRender.ClearCache();
	}
	
	if (g_gameControlBase->IsGameplayMode())
		g_guiBase->Refresh();
	else
		g_editorGui.Refresh();
	
	g_debugMessageSystem.Update();

	// create the list of objects to be rendered sorted by render group
	g_objectManager.CreateRenderList();

	if (!paused && IsGameplayMode())
		g_terrain->UpdatePost();
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Rendering
//
////////////////////////////////////////////////////////////////////////////////////////

static bool enableFrameInterpolation = true;
ConsoleCommand(enableFrameInterpolation, enableFrameInterpolation);

float GameControlBase::CalculateInterpolationPercent()
{
	if (!enableFrameInterpolation)
		return 1;

	float interpolation = 1 - (float)(updateTimer * GAME_FPS);
	//interpolation = interpolation - floorf(interpolation);

	ASSERT(interpolation >= 0 && interpolation <= 1);
	return CapPercent(interpolation);
}

void GameControlBase::SetupRender()
{
	// update the render count
	++renderFrameCount;

	// update terrain rendering
	g_terrainRender.Update();

	// update lights
	DeferredRender::GlobalUpdate();
}

void GameControlBase::Render()
{
	FrankProfilerEntryDefine(L"GameControlBase::Render()", Color::Yellow(), 1);

	g_cameraBase->SetAspectRatio(g_aspectRatio);
	g_cameraBase->PrepareForRender();

	// render foreground objects
	// opt: skip render if it will be obscured by accumulator mode
	if (IsEditMode() || !DeferredRender::lightEnable || !DeferredRender::showFinalAccumulator)
		RenderInterpolatedObjects();

	// renderLights
	DeferredRender::GlobalRender();

	g_cameraBase->RenderPost();

	if (IsEditMode()) // render editors
	{
		g_editor.Render();
		g_render->RenderSimpleVerts();
	}

	if (IsGameplayMode() && g_physicsRender.GetDebugRender())
		g_physics->Render();

	g_debugRender.Render();
	g_debugMessageSystem.Render();

	DXUTGetD3D9Device()->SetRenderState( D3DRS_LIGHTING, TRUE);
	g_objectManager.RenderPost();
}

void GameControlBase::RenderPost()
{
	FrankProfilerEntryDefine(L"RenderPost()", Color::Yellow(), 0);
	RenderDebugText();
	FrankProfiler::Render();
	GetDebugConsole().Render();

	if (showFps)
	{
		g_textHelper->Begin();
		g_textHelper->SetInsertionPos( 4, 40 );
		g_textHelper->SetForegroundColor(Color::White(0.5f));
		g_textHelper->DrawFormattedTextLine( L"%.0f fps", DXUTGetFPS() );
		g_textHelper->End();
	}
}

void GameControlBase::RenderInterpolatedObjects()
{
	// render out the main object group
	g_objectManager.Render();
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Init & destroy device objects
//
////////////////////////////////////////////////////////////////////////////////////////

void GameControlBase::InitDeviceObjects()
{
	ASSERT(g_render);

	// init the rendering
	g_render->InitDeviceObjects();
	g_debugRender.InitDeviceObjects();
	g_terrainRender.InitDeviceObjects();
	DeferredRender::InitDeviceObjects();
	FrankFont::InitDeviceObjects();

	// load sounds
	LoadSounds();

	// load textures
	LoadTextures();

	// setup input
	static bool inputWasSetup = false;
	if (!inputWasSetup)
	{
		inputWasSetup = true;
		SetupInput();
	}
}

void GameControlBase::DestroyDeviceObjects()
{
	if (g_render)
		g_render->DestroyDeviceObjects();
	g_debugRender.DestroyDeviceObjects();
	g_terrainRender.DestroyDeviceObjects();
	DeferredRender::DestroyDeviceObjects();
	FrankFont::DestroyDeviceObjects();
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Game mode functions
//
////////////////////////////////////////////////////////////////////////////////////////

void GameControlBase::SetGameMode(GameMode _gameMode)
{
	gameMode = _gameMode;

	Reset();
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Setup and loading
//
////////////////////////////////////////////////////////////////////////////////////////

void GameControlBase::SetupInput()
{
	// basic controls
	g_input->SetButton(GB_MouseLeft,		VK_LBUTTON);
	g_input->SetButton(GB_MouseMiddle,		VK_MBUTTON);
	g_input->SetButton(GB_MouseRight,		VK_RBUTTON);
	g_input->SetButton(GB_Control,			VK_CONTROL);
	g_input->SetButton(GB_Shift,			VK_SHIFT);
	g_input->SetButton(GB_Alt,				VK_MENU);
	g_input->SetButton(GB_Tab,				VK_TAB);
	g_input->SetButton(GB_Space,			' ');
	g_input->SetButton(GB_Up,				VK_UP);
	g_input->SetButton(GB_Down,				VK_DOWN);
	g_input->SetButton(GB_Right,			VK_RIGHT);
	g_input->SetButton(GB_Left,				VK_LEFT);
	
	// engine controls
	g_input->SetButton(GB_InfoDisplay,		VK_F1);
	g_input->SetButton(GB_PhysicsDebug,		VK_F2);
	g_input->SetButton(GB_RefreshResources,	VK_F3);
	g_input->SetButton(GB_Editor,			VK_F4);
	g_input->SetButton(GB_Screenshot,		VK_F5);
	g_input->SetButton(GB_Profiler,			VK_F6);

	// editor controls
	g_input->SetButton(GB_Editor_Mode_Terrain1,		'1');
	g_input->SetButton(GB_Editor_Mode_Terrain2,		'2');
	g_input->SetButton(GB_Editor_Mode_Object,		'3');
	g_input->SetButton(GB_Editor_Cut,				'X');
	g_input->SetButton(GB_Editor_Copy,				'C');
	g_input->SetButton(GB_Editor_Paste,				'V');
	g_input->SetButton(GB_Editor_Undo,				'Z');
	g_input->SetButton(GB_Editor_Redo,				'Y');
	g_input->SetButton(GB_Editor_Add,				'Z');
	g_input->SetButton(GB_Editor_Remove,			'X');
	g_input->SetButton(GB_Editor_Insert,			VK_INSERT);
	g_input->SetButton(GB_Editor_Delete,			VK_DELETE);
	g_input->SetButton(GB_Editor_Up,				'W');
	g_input->SetButton(GB_Editor_Down,				'S');
	g_input->SetButton(GB_Editor_Left,				'A');
	g_input->SetButton(GB_Editor_Right,				'D');
	g_input->SetButton(GB_Editor_TileSetUp,			'E');
	g_input->SetButton(GB_Editor_TileSetDown,		'Q');
	g_input->SetButton(GB_Editor_MovePlayer,		'M');
	g_input->SetButton(GB_Editor_Save,				'S');
	g_input->SetButton(GB_Editor_Load,				'L');
	g_input->SetButton(GB_Editor_Grid,				'G');
	g_input->SetButton(GB_Editor_BlockEdit,			'B');
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Render debug text
//
////////////////////////////////////////////////////////////////////////////////////////

void GameControlBase::RenderDebugText()
{
	if (!showDebugInfo && !g_physicsRender.GetDebugRender())
		return;

	g_textHelper->Begin();
	g_textHelper->SetInsertionPos( debugInfoPos.x, debugInfoPos.y );
	{
		if (showDebugInfo || g_physicsRender.GetDebugRender())
		{
			g_textHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 0.8f ) );
			g_textHelper->DrawFormattedTextLine( L"%s v%s", GetGameTitle(), GetGameVersion() );
			g_textHelper->DrawFormattedTextLine( L"%s v%s", frankEngineName, frankEngineVersion );
			g_textHelper->DrawTextLine( L"Copyright © 2010 - Frank Force" );
		}

		if (showDebugInfo)
		{
			g_textHelper->DrawTextLine( L"------------------------" );
			g_textHelper->DrawFormattedTextLine( L"fps: %.0f", DXUTGetFPS() );

			g_textHelper->DrawFormattedTextLine( L"objects: %d", g_objectManager.GetObjectCount());
			g_textHelper->DrawFormattedTextLine( L"particles: %d / %d", ParticleEmitter::GetTotalEmitterCount(), ParticleEmitter::GetTotalParticleCount() );
			g_textHelper->DrawFormattedTextLine( L"simple / dynamic lights: %d / %d", DeferredRender::GetSimpleLightCount(), DeferredRender::GetDynamicLightCount());
			g_textHelper->DrawFormattedTextLine( L"simple verts: %d", g_render->GetTotalSimpleVertsRendered());
			g_textHelper->DrawFormattedTextLine( L"terrain batches: %d", g_terrainRender.renderedBatchCount);
			//g_textHelper->DrawFormattedTextLine( L"terrain prims/batches: %d / %d", g_terrainRender.renderedPrimitiveCount, g_terrainRender.renderedBatchCount);
			//g_textHelper->DrawFormattedTextLine( L"next handle: %d", GameObject::GetNextHandle() );
			//g_textHelper->DrawFormattedTextLine( L"start handle: %d", g_terrain->GetStartHandle() );
			//const Vector2 rightStick = g_input->GetGamepadRightStick(0);
			//const Vector2 leftStick = g_input->GetGamepadLeftStick(0);
			const float rightTrigger =  g_input->GetGamepadRightTrigger(0);
			const float leftTrigger = g_input->GetGamepadLeftTrigger(0);
			//g_textHelper->DrawFormattedTextLine( L"rightStick: %f %f %f", rightStick.x, rightStick.y, rightStick.Length() );
			//g_textHelper->DrawFormattedTextLine( L"leftStick: %f %f %f", leftStick.x, leftStick.y, leftStick.Length() );
			g_textHelper->DrawFormattedTextLine( L"triggers: %f %f", leftTrigger, rightTrigger );

			if (GetPlayer())
			{
				const Vector2 pos = GetUserPosition();
				g_textHelper->DrawFormattedTextLine( L"pos: (%.2f, %.2f)", pos.x, pos.y );

				if (g_terrain)
				{
					int x, y;
					g_terrain->GetTile(pos, x, y);
					g_textHelper->DrawFormattedTextLine( L"tile: (%d, %d)", x, y );

					const IntVector2 patchOffset = g_terrain->GetPatchOffset(pos);
					g_textHelper->DrawFormattedTextLine( L"patch: (%d, %d)", patchOffset.x, patchOffset.y );
				}

				if (IsGameplayMode())
					g_textHelper->DrawFormattedTextLine( L"speed: %.2f, %.2f°", GetPlayer()->GetSpeed(), GetPlayer()->GetAngularSpeed() );
			}
		}

		if (g_physicsRender.GetDebugRender())
		{
			g_textHelper->DrawTextLine( L"------------------------" );
			//g_textHelper->DrawFormattedTextLine( L"proxys: %d (%d)", g_physics->GetPhysicsWorld()->GetProxyCount(), b2_maxProxies );
			//g_textHelper->DrawFormattedTextLine( L"pairs: %d (%d)", g_physics->GetPhysicsWorld()->GetPairCount(), b2_maxPairs );
			g_textHelper->DrawFormattedTextLine( L"proxys: %d", g_physics->GetPhysicsWorld()->GetProxyCount() );
			//g_textHelper->DrawFormattedTextLine( L"pairs: %d", g_physics->GetPhysicsWorld()->GetPairCount() );
			g_textHelper->DrawFormattedTextLine( L"bodys: %d", g_physics->GetPhysicsWorld()->GetBodyCount() );
			g_textHelper->DrawFormattedTextLine( L"contacts: %d", g_physics->GetPhysicsWorld()->GetContactCount() );
			g_textHelper->DrawFormattedTextLine( L"joints: %d", g_physics->GetPhysicsWorld()->GetJointCount() );
			g_textHelper->DrawFormattedTextLine( L"raycasts: %d", g_physics->GetRaycastCount() );
			//g_textHelper->DrawFormattedTextLine( L"heap bytes: %d", b2_byteCount );
		}	
	}
	g_textHelper->End();
}

bool GameControlBase::IsObjectEditMode() const	
{ 
	return IsEditMode() && g_editor.IsObjectEdit(); 
}

bool GameControlBase::IsTileEditMode() const	
{ 
	return IsEditMode() && g_editor.IsTileEdit(); 
}

// return the positon of the current user
// in game mode this is the player
// in level edit mode this is the cursor
Vector2 GameControlBase::GetUserPosition() const
{
	return IsGameplayMode()? g_gameControlBase->GetPlayer()->GetPosWorld() : g_input->GetMousePosWorldSpace();
}

Vector2 GameControlBase::GetUserPositionInterpolated() const
{
	return IsGameplayMode()? g_gameControlBase->GetPlayer()->GetInterpolatedXForm().position : g_input->GetInterpolatedMousePosWorldSpace();
}
