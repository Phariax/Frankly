////////////////////////////////////////////////////////////////////////////////////////
/*
	High Level Game Control
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "gameGlobals.h"
#include "gameControl.h"
#include "gameObjects.h"

////////////////////////////////////////////////////////////////////////////////////////
//
//	Game Globals
//
////////////////////////////////////////////////////////////////////////////////////////

Player*			g_player = NULL;
FrankFont*		g_gameFont = NULL;
PopupMessage*	g_popupMessage = NULL;

////////////////////////////////////////////////////////////////////////////////////////
//
//	Game Control Member Functions
//
////////////////////////////////////////////////////////////////////////////////////////

GameControl::GameControl()
{
	// this example supports both tile sheet and non tile sheet rendering
	static const bool exampleUseTileSheets = false;
	if (exampleUseTileSheets)
	{
		// terrain tile set
		Terrain::tileSetCount = 2;
		g_usePointFiltering = true;
		Terrain::SetTileSetTexture(0, GameTexture_terrainTiles0);
		Terrain::SetTileSetTexture(1, GameTexture_tiles0);
	}
	{
		// physics settings
		Physics::defaultFriction			= 0.3f;		// how much friction an object has
		Physics::defaultRestitution			= 0.3f;		// how bouncy objects are
		Physics::defaultDensity				= 1.0f;		// how heavy objects are
		Physics::defaultLinearDamping		= 0.2f;		// how quickly objects come to rest
		Physics::defaultAngularDamping		= 0.1f;		// how quickly objects stop rotating
		Physics::worldSize					= 5000;		// maximum extents of physics world
	}
	{
		// terrain settings
		Terrain::patchLayers		= 2;	// how many layers there are
		Terrain::fullSize			= 32;	// how many patches per terrain
		Terrain::patchSize			= 16;	// how many tiles per patch
		Terrain::windowSize			= 1;	// size of the terrain stream window
		Terrain::renderWindowSize	= 1;	// size of the terrain render window
		TerrainTile::SetSize(1.0f);			// size in world space of each tile
		Terrain::gravity = Vector2(0,-10);	// acceleartion due to gravity
	}
	{
		// rendering settings
		DeferredRender::lightEnable				= true;
		DeferredRender::normalMappingEnable		= true;
		DeferredRender::emissiveBackgroundColor	= Color::Grey(1, 0.2f);
		//DeferredRender::ambientLightColor		= Color::Grey(1, 0.1f);
		g_backBufferClearColor = Color(0.3f, 0.5f, 1.0f, 1.0f);
	}
	{
		// particle system settings
		ParticleEmitter::defaultRenderGroup = RenderGroup_effect;
		ParticleEmitter::defaultAdditiveRenderGroup = RenderGroup_additiveEffect;
	}


	// enable dev mode for demo
	GameControlBase::devMode = true;
	usingGamepad = false;
}

void GameControl::SetupInput()
{
	GameControlBase::SetupInput();

	g_input->SetButton(GB_MoveUp,			'W');
	g_input->SetButton(GB_MoveUp,			VK_UP);
	g_input->SetButton(GB_MoveDown,			'S');
	g_input->SetButton(GB_MoveDown,			VK_DOWN);
	g_input->SetButton(GB_MoveRight,		'D');
	g_input->SetButton(GB_MoveRight,		VK_RIGHT);
	g_input->SetButton(GB_MoveLeft,			'A');
	g_input->SetButton(GB_MoveLeft,			VK_LEFT);
	g_input->SetButton(GB_Restart,			'R');

	g_input->SetButton(GB_Trigger1,			VK_LBUTTON);
	g_input->SetButton(GB_Trigger2,			VK_RBUTTON);
	g_input->SetButton(GB_Trigger2,			GAMEPAD_BUTTON_RIGHT_TRIGGER);

	g_input->SetButton(GB_TimeScale,		VK_MBUTTON);
	g_input->SetButton(GB_TimeScale,		GAMEPAD_BUTTON_0);
	g_input->SetButton(GB_TimeScale,		VK_SPACE);

	g_input->SetButton(GB_Pause,			VK_ESCAPE);
	g_input->SetButton(GB_Test,				'T');
}

void GameControl::LoadSounds()
{
	g_sound->LoadSound(L"sound_test",			SoundControl_test);
}

void GameControl::LoadTextures()
{
	// load the basic textures
	g_render->LoadTexture( L"watermark",		GameTexture_Watermark);
	g_render->LoadTexture( L"lightMask",		GameTexture_LightMask);
	g_render->LoadTexture( L"font1",			GameTexture_Font1);
	g_render->LoadTexture(L"tiles0",			GameTexture_tiles0);
	g_render->LoadTexture(L"terrainTiles0",		GameTexture_terrainTiles0);
	
	// load tile sheets
	g_render->LoadTexture( L"smoke",			GameTexture_Smoke);
	g_render->LoadTexture( L"circle",			GameTexture_Circle);
	g_render->LoadTexture( L"dot",				GameTexture_Dot);
	g_render->LoadTexture( L"arrow",			GameTexture_Arrow);
	g_render->LoadTexture( L"lightGel1",		GameTexture_LightGel1);

	// load tiles
	g_render->SetTextureTile(GameTexture_tiles0, L"crate",		GameTexture_Crate,		ByteVector2(4,0),	ByteVector2(8,8));

	if (Terrain::tileSetCount == 0)
	{
		// load terrain tiles
		g_render->LoadTexture( L"tile_test1",		GameTexture_tile_test1);
		g_render->LoadTexture( L"tile_test2",		GameTexture_tile_test2);
		g_render->LoadTexture( L"tile_test3",		GameTexture_tile_test3);
	}
}

void GameControl::InitDeviceObjects()
{
	GameControlBase::InitDeviceObjects();
	
	if (g_cameraBase)
	{
		// lock camera to anything between 16/9 and 16/10 aspect ratio
		float aspect = Cap(g_aspectRatio, 16/10.0f, 16/9.0f);
		g_cameraBase->SetLockedAspectRatio(aspect);
	}

	g_gameFont = new FrankFont(GameTexture_Font1, L"font1.fnt");
}

void GameControl::DestroyDeviceObjects()
{
	GameControlBase::DestroyDeviceObjects();
	
	SAFE_DELETE(g_gameFont);
}

void GameControl::UpdateFrame(float delta)
{
	if (g_input->WasJustPushed(GB_Pause))
	{
		// handle pausing
		SetPaused(!IsPaused());
	}
	
	if (!usingGamepad)
		usingGamepad = g_input->HasGamepadInput(0);
	else 
	{
		if (g_input->GetMouseDeltaLocalSpace().MagnitudeSquared() > 100)
			usingGamepad = false;
		if (g_input->WasJustPushed(GB_MouseLeft))
			usingGamepad = false;
		if (g_input->WasJustPushed(GB_MouseRight))
			usingGamepad = false;
		if (g_input->WasJustPushed(GB_MouseMiddle))
			usingGamepad = false;
	}

	if (g_input->IsDown(GB_Test) && !IsEditMode())
	{
		SimpleRaycastResult result;
		Line2 test(g_input->GetMousePosWorldSpace(), g_player->GetPosWorld());
		GameObject* hitObject = g_physics->RaycastSimple(test, &result, g_player);
		//if (hitObject)
		{
			Line2(result.point, test.p1).RenderDebug();
		}

		// throw down a random light when the test button is pressed
		//Light* light = new Light(g_input->GetMousePosWorldSpace(), NULL, RAND_BETWEEN(9, 15), Color::RandBetweenComponents(Color::Grey(1, 0.3f), Color::Grey(1, 0.8f)), true);
		//light->SetFadeTimer(20.0f);
		//light->SetHaloRadius(0.8f);
		//light->SetOverbrightRadius(0.8f);
		//light->SetRenderGroup(RenderGroup_high);
	}

	{
		// allow player to slow down time
		const float changeRate = 0.02f;
		if (g_input->IsDown(GB_TimeScale))
			timeScale -= changeRate;
		else
			timeScale += changeRate;
		if (g_gameControlBase->IsEditMode())
			timeScale = 1;
		timeScale = Cap(timeScale, 0.25f, 1.0f);
	}

	if (!IsEditMode() && g_input->WasJustPushed(GB_Restart))
	{
		Reset();
		return;
	}

	if (g_player->GetDeadTime() > 3.0f)
	{
		// automatically reset after player dies
		Reset();
		return;
	}
}

void GameControl::Reset()
{
	const bool wasEditMode = WasEditMode();

	// reload our suface info incase it changed when developing
	InitSurfaceInfoArray();

	// call the base level reset function
	// this clears all objects set to destroy on reset (basically everything except the camera and terrain)
	// it will also automatically save/load the terrain
	GameControlBase::Reset();

	new Player(g_terrain->GetPlayerEditorStartPos());

	// create a singleton popup message
	g_popupMessage = new PopupMessage(Vector2(0), NULL, "Test", 0.5f, 0, Color::White(0), false);

	// setup the game camera
	g_cameraBase->SetMinGameplayZoom(5);
	g_cameraBase->SetMaxGameplayZoom(15);
	if (!IsEditMode() && !wasEditMode)
	{
		// don't reset camera zoom if editing
		g_cameraBase->SetZoom(15);
	}

	// terrain is only active in gameplay mode
	if (IsGameplayMode())
		g_terrain->ActivateAreaAroundPlayer();

	// reset on screen text message
	g_gameGui->ResetOnScreenText();

	// pause music when editing
	g_sound->GetMusicPlayer().Pause(IsEditMode());

	{
		// tell directx what hot keys to handle
		const bool altEnterToToggleFullscreen = true;
		const bool escapeToQuit = IsEditMode();
		const bool pauseToToggleTimePause = false;
		DXUTSetHotkeyHandling(altEnterToToggleFullscreen, escapeToQuit, pauseToToggleTimePause);
	}
	
	g_terrain->GetLayerRender(0)->SetRenderGroup(RenderGroup_foregroundTerrain);
	g_terrain->GetLayerRender(1)->SetRenderGroup(RenderGroup_backgroundTerrain);
}

void GameControl::RenderInterpolatedObjects()
{
	if (!IsEditMode())
	{
		// render a simple time of day system by modifying background directional light settings
		static float orbitSpeed = 0.1f;
		ConsoleCommand(orbitSpeed, orbitSpeed);
		const Vector2 direction = Vector2::BuildFromAngle(-1.5f + GetResetTime()*orbitSpeed);
		const float timeOfDay = Percent(direction.y, -1.0f, 1.0f);
	
		// set background light direction
		const Vector2 lightDirectionSun = 1.0f*direction;
		const Vector2 lightDirectionMoon = -1.0f*direction;
		const Vector2 lightDirection = PercentLerp(timeOfDay, 0.3f, 0.7f, lightDirectionSun, lightDirectionMoon);
		DeferredRender::directionalLightDirection = lightDirection;
	
		// set bakground light color
		const float sunBackgroundColor = 1;
		const float moonBackgroundColor = 0.5f;
		const float lightBackgroundColor = Lerp(timeOfDay, sunBackgroundColor, moonBackgroundColor);
		DeferredRender::directionalLightColor = Color::Grey(1, lightBackgroundColor);

		// set background buffer clear color
		const Color sunClearColor(0.3f, 0.5f, 1.0f, 1.0f);
		const Color moonClearColor(0.03f, 0.0f, 0.2f, 1.0f);
		const Color lightClearColor = Lerp(timeOfDay, sunClearColor, moonClearColor);
		g_backBufferClearColor = lightClearColor;

		// pass some emissive through the background
		const Color sunEmissiveColor = Color::Grey(1, 0.15f);
		const Color moonEmissiveColor = Color::Grey(1, 0.05f);
		DeferredRender::emissiveBackgroundColor = Lerp(timeOfDay, sunEmissiveColor, moonEmissiveColor);

		{
			// draw sun and moon
			DeferredRender::EmissiveRenderBlock emissiveRenderBlock;
			DeferredRender::AdditiveRenderBlock additiveRenderBlock;

			const float radius = 9;
			const float horizonOffset = 2;
			const Vector2 centerPos = g_cameraBase->GetInterpolatedXForm().position - Vector2(0, horizonOffset);
			
			{
				// sun
				const Vector2 offset = -radius*direction;
				float alpha = Percent(timeOfDay, 0.8f, 0.0f);
				g_render->RenderQuad(centerPos + offset, Vector2(12), Color::White(0.2f*alpha), GameTexture_Dot);
				g_render->RenderQuad(centerPos + offset, Vector2(1.7f), Color::White(0.5f*alpha), GameTexture_Dot);
			}
			{

				// moon
				const Vector2 offset = radius*direction;
				float alpha = Percent(timeOfDay, 0.3f, 1.0f);
				g_render->RenderQuad(centerPos + offset, Vector2(5), Color::White(0.2f*alpha), GameTexture_Dot);
				g_render->RenderQuad(centerPos + offset, Vector2(1), Color::White(0.2f*alpha), GameTexture_Circle);
			}
		}

		/*{
			// draw static background image behind everything but in front of the sun/moon
			// this is where you would implement paralx backgrounds
			DeferredRender::EmissiveRenderBlock emissiveRenderBlock;
			Vector2 position = g_player->GetInterpolatedXForm().position;
			Vector2 size(5);
			g_render->RenderQuad(position, size, Color::White(), GameTexture_Crate );
		}*/
	}

	GameControlBase::RenderInterpolatedObjects();
}

void GameControl::RenderPost()
{
	if (!usingGamepad && IsGameplayMode() && !IsPaused())
	{
		// special display for the mouse cursor
		static float mouseAlpha = 0.5f;
		ConsoleCommand(mouseAlpha, mouseAlpha);
		static float mouseSize = .1f;
		ConsoleCommand(mouseSize, mouseSize);
		static float mouseLinesSize = 20;
		ConsoleCommand(mouseLinesSize, mouseLinesSize);
		
		const XForm2 xf = XForm2(g_input->GetInterpolatedMousePosLocalSpace()) * g_cameraBase->GetInterpolatedXForm();
		g_render->RenderQuad(xf, Vector2(mouseSize*3), Color(1,1,1,mouseAlpha), GameTexture_Circle);
		g_render->RenderQuad(xf, Vector2(mouseSize*1.5f), Color(1,1,1,mouseAlpha), GameTexture_Circle);

		if (mouseLinesSize > 0)
		{
			const float scale = g_cameraBase->GetZoomInterpoalted() / 100;
			const Vector2 p0t = xf.position;
			const Vector2 p1t = xf.TransformCoord(scale*Vector2::YAxis() * mouseLinesSize);
			const Vector2 p2t = xf.TransformCoord(-scale*Vector2::YAxis() * mouseLinesSize);
			const Vector2 p3t = xf.TransformCoord(scale*Vector2::XAxis() * mouseLinesSize);
			const Vector2 p4t = xf.TransformCoord(-scale*Vector2::XAxis() * mouseLinesSize);

			g_render->CapLineVerts(p1t);
			g_render->AddPointToLineVerts(p1t, Color(1,1,1,0));
			g_render->AddPointToLineVerts(p0t, Color(1,1,1,mouseAlpha));
			g_render->AddPointToLineVerts(p2t, Color(1,1,1,0));
			g_render->CapLineVerts(p2t);
			g_render->CapLineVerts(p3t);
			g_render->AddPointToLineVerts(p3t, Color(1,1,1,0));
			g_render->AddPointToLineVerts(p0t, Color(1,1,1,mouseAlpha));
			g_render->AddPointToLineVerts(p4t, Color(1,1,1,0));
			g_render->CapLineVerts(p4t);
		}
	}

	GameControlBase::RenderPost();
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	Render debug text
//
////////////////////////////////////////////////////////////////////////////////////////

void GameControl::RenderDebugText()
{
	GameControlBase::RenderDebugText();

	g_textHelper->Begin();
	if (showDebugInfo)
	{
		g_textHelper->SetForegroundColor( Color( 1.0f, 1.0f, 1.0f, 0.8f ) );
		g_textHelper->DrawTextLine( L"------------------------" );
		g_textHelper->DrawFormattedTextLine( L"Health: %.1f / %.1f", g_player->GetHealth(),  g_player->GetMaxHealth());
		g_textHelper->DrawFormattedTextLine( L"Time: %.2f", g_player->GetLifeTime());

		//float lightValue = 0;
		//DeferredRender::GetLightValue(g_input->GetMousePosWorldSpace(), lightValue);
		//g_textHelper->DrawFormattedTextLine( L"Light Value Test: %.2f", lightValue);
	}
	g_textHelper->End();
}