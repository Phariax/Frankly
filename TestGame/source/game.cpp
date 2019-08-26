////////////////////////////////////////////////////////////////////////////////////////
/*
	Startup for Frank Engine
	Copyright 2013 - Frank Force
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "gameGlobals.h"

////////////////////////////////////////////////////////////////////////////////////////
// Game Globals
GameGui*		g_gameGui		= NULL;
GameControl*	g_gameControl	= NULL;

////////////////////////////////////////////////////////////////////////////////////////
// Main Entry Point
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	// startup the frank engine
	const int startWidth = 1280;
	const int startHeight = 720;
	FrankEngineStartup(gameTitle);
	
	// init frank engine with custom objects
	g_gameControl = new GameControl();
	g_gameGui = new GameGui();
	Camera* camera = new Camera();
	FrankEngineInit(startWidth, startHeight, g_gameControl, g_gameGui, camera);

	// frank engine main loop
	FrankEngineLoop();

	// shutdown frank engine
	FrankEngineShutdown();

	// exit the program
	return DXUTGetExitCode();
}