////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Input Processing
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

static int gamepadCount = 0;
static const int gamepadMaxCount = 2;

struct GamepadInfo
{
	LPDIRECTINPUTDEVICE8 gamepad;
	DIJOYSTATE2 state;
	DIDEVCAPS capabilities;
	bool wasPolled;

	ValueInterpolator<Vector2> leftStick;
	ValueInterpolator<Vector2> rightStick;
	ValueInterpolator<float> leftTrigger;
	ValueInterpolator<float> rightTrigger;
};

static LPDIRECTINPUT8 directInput = NULL;
static GamepadInfo gamepadInfo[gamepadMaxCount] = {NULL};

static float gamepadDeadzone = 0.3f;
ConsoleCommand(gamepadDeadzone, gamepadDeadzone);

static float gamepadUpperDeadzone = 0.75f;
ConsoleCommand(gamepadUpperDeadzone, gamepadUpperDeadzone);

static bool enableGamepad = true;
ConsoleCommand(enableGamepad, gamepadEnable);

bool InputControl::enableGamepadRightStickEmulation = true;
ConsoleCommand(InputControl::enableGamepadRightStickEmulation, enableGamepadRightStickEmulation);

bool InputControl::enableGamepadTriggerEmulation = true;
ConsoleCommand(InputControl::enableGamepadTriggerEmulation, enableGamepadTriggerEmulation);

float InputControl::gamepadRightStickEmulationSpeed = 0.1f;
ConsoleCommand(InputControl::gamepadRightStickEmulationSpeed, gamepadRightStickEmulationSpeed);

float InputControl::gamepadStickButtonMin = 0.9f;
ConsoleCommand(InputControl::gamepadStickButtonMin, gamepadStickButtonMin);

float InputControl::gamepadTriggerButtonMin = 0.1f;
ConsoleCommand(InputControl::gamepadTriggerButtonMin, gamepadTriggerButtonMin);

InputControl::InputControl() :
	wasCleared(false)
{
	ASSERT(!g_input);
	InitGamepad();
	Clear();
}

InputControl::~InputControl()
{
	for(int i = 0; i < gamepadMaxCount; ++i)
		SAFE_RELEASE(gamepadInfo[i].gamepad);
	SAFE_RELEASE(directInput);
}

void InputControl::Clear()
{
	// if we lost focus reset the input
	ZeroMemory(&mouseInput, sizeof(mouseInput));
	ZeroMemory(&keyboardInput, sizeof(keyboardInput));
	ZeroMemory(&keyboardInputSubFrame, sizeof(keyboardInputSubFrame));

	// clear all the input buttons
	for (int i = 0; i < GB_MaxCount; ++i)
		gameButtonInfo[i].Reset();

	wasCleared = true;
	
	for(int i = 0; i < gamepadMaxCount; ++i)
	{
		// update gamepad
		GamepadInfo& info = gamepadInfo[i];
		info.leftStick.Init(Vector2(0));
		info.rightStick.Init(Vector2(0));
		info.leftTrigger.Init(0);
		info.rightTrigger.Init(0);
	}
}

void InputControl::Update()
{
	// handle mouse input
	POINT ptCursor;
	GetCursorPos( &ptCursor );
	ScreenToClient( DXUTGetHWND(), &ptCursor );
	
	// update mouse pos
	mouseInput.posLast = mouseInput.pos;
	mouseInput.pos = Vector2((float)(ptCursor.x), (float)(ptCursor.y));

	for(int i = 0; i < gamepadMaxCount; ++i)
	{
		// update gamepad
		PollGamepad(i);
		GamepadInfo& info = gamepadInfo[i];
		info.leftStick.SaveLast();
		info.rightStick.SaveLast();
		info.leftTrigger.SaveLast();
		info.rightTrigger.SaveLast();
		info.leftStick = PollGamepadLeftStick(i);
		info.rightStick = PollGamepadRightStick(i);
		info.leftTrigger = PollGamepadLeftTrigger(i);
		info.rightTrigger = PollGamepadRightTrigger(i);
	}

	if (wasCleared)
		mouseInput.posLast = mouseInput.pos;
	
	if (GetDebugConsole().IsOpen())
	{
		// clear all input buttons when console is up
		for (int i = 0; i < GB_MaxCount; ++i)
			gameButtonInfo[i].Reset();
	}
	else
	{
		// update all the input buttons
		for (int i = 0; i < GB_MaxCount; ++i)
		{
			GameButtonInfo& buttonInfo = gameButtonInfo[i];
			const bool wasDown = buttonInfo.isDown;
			buttonInfo.isDown = false;
			buttonInfo.wasJustPushed = false;
			buttonInfo.wasJustDoubleClicked = false;

			// check if any of the keys for this button are down
			for (list<UINT>::iterator it = buttonInfo.keys.begin(); it != buttonInfo.keys.end(); ++it) 
			{
				const UINT key = *it;
				if (key == VK_LBUTTON)
				{
					buttonInfo.isDown |= mouseInput.leftDown;
					buttonInfo.wasJustDoubleClicked |= mouseInput.leftDoubleClick;
					buttonInfo.wasJustPushed |= mouseInput.leftWentDownSubFrame;
				}
				else if (key == VK_MBUTTON)
				{
					buttonInfo.isDown |= mouseInput.middleDown;
					buttonInfo.wasJustDoubleClicked |= mouseInput.middleDoubleClick;
					buttonInfo.wasJustPushed |= mouseInput.middleWentDownSubFrame;
				}
				else if (key == VK_RBUTTON)
				{
					buttonInfo.isDown |= mouseInput.rightDown;
					buttonInfo.wasJustDoubleClicked |= mouseInput.rightDoubleClick;
					buttonInfo.wasJustPushed |= mouseInput.rightWentDownSubFrame;
				}
				else if (key < KEYBOARD_INPUT_SIZE)
				{
					buttonInfo.isDown |= IsKeyDown(key);

					// make sure it registered as just pushed if it went down during the sub frame
					buttonInfo.wasJustPushed |= !wasDown && keyboardInputSubFrame[key];
				}
				else if (key >= GAMEPAD_BUTTON_START && key < GAMEPAD_BUTTON_START + GAMEPAD_BUTTON_GROUP_COUNT*gamepadMaxCount)
				{
					UINT actualKey = key;
					UINT gamepadIndex = 0;
					GetGamepadKey(actualKey, gamepadIndex);

					GamepadInfo& info = gamepadInfo[gamepadIndex];
					if (!enableGamepad || !info.gamepad || !info.wasPolled)
						continue;

					buttonInfo.isDown |= IsGamepadButtonDown(GamepadButtons(actualKey), gamepadIndex);
				}
			}

			if (buttonInfo.wasJustDoubleClicked)
				buttonInfo.isDownFromDoubleClick = true;
			else if (!buttonInfo.isDown)
				buttonInfo.isDownFromDoubleClick = false;

			buttonInfo.wasJustPushed = buttonInfo.wasJustPushed || (!wasDown && buttonInfo.isDown);
			buttonInfo.wasJustReleased = wasDown && !buttonInfo.isDown;

			// handle holding keys buttons for ui stuff
			buttonInfo.isDownUI = false;
			if (buttonInfo.isDown)
			{
				if (buttonInfo.timeUI == 0)
				{
					static const int buttonHoldTimeStart = 20;
					static const int buttonHoldTimeNormal = 5;
					buttonInfo.isDownUI = true;
					buttonInfo.timeUI = buttonInfo.wasJustPushed? buttonHoldTimeStart : buttonHoldTimeNormal;
				}
				else
					--buttonInfo.timeUI;
			}
			else
				buttonInfo.timeUI = 0;
		}
	}

	// reset some of the mouse states
	mouseInput.leftWentDownSubFrame = false;
	mouseInput.middleWentDownSubFrame = false;
	mouseInput.rightWentDownSubFrame = false;
	mouseInput.leftDoubleClick = false;
	mouseInput.middleDoubleClick = false;
	mouseInput.rightDoubleClick = false;
	mouseInput.wheelLast = mouseInput.wheel;
	mouseInput.wheel = 0;
	
	// clear sub frame input
	ZeroMemory(&keyboardInputSubFrame, sizeof(keyboardInputSubFrame));
	wasCleared = false;
}
	
bool InputControl::IsGamepadButtonDown(GamepadButtons actualKey, UINT gamepadIndex)
{
	ASSERT(actualKey >= GAMEPAD_BUTTON_START && actualKey < GAMEPAD_BUTTON_END);

	GamepadInfo& info = gamepadInfo[gamepadIndex];
	if (!enableGamepad || !info.gamepad || !info.wasPolled)
		return false;

	if (actualKey < GAMEPAD_BUTTON_LAST)
	{
		// get the gamepad button input
		const UINT button = actualKey - GAMEPAD_BUTTON_START;
		return (info.state.rgbButtons[button] & 0x80) != 0;
	}
	else if (actualKey >= GAMEPAD_BUTTON_POV_UP && actualKey <= GAMEPAD_BUTTON_POV_LEFT)
	{
		// get the pov (dpad) input
		DWORD pov = info.state.rgdwPOV[0];
		if (pov != -1)
		{
			pov /= 100;
			bool isDown = false;
			if (actualKey == GAMEPAD_BUTTON_POV_UP)
				isDown = pov > 270 || pov < 90;
			else if (actualKey == GAMEPAD_BUTTON_POV_DOWN)
				isDown = pov < 270 && pov > 90;
			else if (actualKey == GAMEPAD_BUTTON_POV_RIGHT)
				isDown = pov > 0 && pov < 180;
			else if (actualKey == GAMEPAD_BUTTON_POV_LEFT)
				isDown = pov < 360 && pov > 180;
			return isDown;
		}
	}
	else
	{
		// treat the stick input as a button press
		switch (actualKey)
		{
			case GAMEPAD_BUTTON_UP:
				return GetGamepadLeftStick(gamepadIndex).y > gamepadStickButtonMin; break;
			case GAMEPAD_BUTTON_DOWN:
				return GetGamepadLeftStick(gamepadIndex).y < -gamepadStickButtonMin; break;
			case GAMEPAD_BUTTON_RIGHT:
				return GetGamepadLeftStick(gamepadIndex).x > gamepadStickButtonMin; break;
			case GAMEPAD_BUTTON_LEFT:
				return GetGamepadLeftStick(gamepadIndex).x < -gamepadStickButtonMin; break;
			case GAMEPAD_BUTTON_LEFT_TRIGGER:
				return GetGamepadLeftTrigger(gamepadIndex) > gamepadTriggerButtonMin; break;
			case GAMEPAD_BUTTON_RIGHT_TRIGGER:
				return GetGamepadRightTrigger(gamepadIndex) > gamepadTriggerButtonMin; break;
		}
	}

	return false;
}

void InputControl::OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	keyboardInput[nChar] = bKeyDown;
	if (bKeyDown)
	{
		keyboardInputSubFrame[nChar] = true;
		GetDebugConsole().OnKeyboard(nChar);
	}
}

void InputControl::SetMousePosScreenSpace(const Vector2& mousePos)
{
	// reset the mouse pos
	mouseInput.pos = mousePos;
	POINT pos = {(long)mousePos.x, (long)mousePos.y};
	ClientToScreen(DXUTGetHWND(), &pos);
	SetCursorPos(pos.x, pos.y);
}

void InputControl::OnMouseMessage(UINT uMsg, WPARAM wParam)
{
	switch( uMsg )
	{
		case WM_MOUSEWHEEL:
		{
			mouseInput.wheel += (short)HIWORD(wParam);
			break;
		}

		case WM_LBUTTONDOWN:
		{
			mouseInput.leftDown = true;
			mouseInput.leftWentDownSubFrame = true;
			break;
		}

		case WM_LBUTTONUP:
		{
			mouseInput.leftDown = false;
			break;
		}

		case WM_LBUTTONDBLCLK:
		{
			mouseInput.leftDoubleClick = true;
			mouseInput.leftDown = true;
			break;
		}

		case WM_MBUTTONDOWN:
		{
			mouseInput.middleDown = true;
			mouseInput.middleWentDownSubFrame = true;
			break;
		}

		case WM_MBUTTONUP:
		{
			mouseInput.middleDown = false;
			break;
		}

		case WM_MBUTTONDBLCLK:
		{
			mouseInput.middleDoubleClick = true;
			mouseInput.middleDown = true;
			break;
		}

		case WM_RBUTTONDOWN:
		{
			mouseInput.rightDown = true;
			mouseInput.rightWentDownSubFrame = true;
			break;
		}

		case WM_RBUTTONUP:
		{
			mouseInput.rightDown = false;
			break;
		}

		case WM_RBUTTONDBLCLK:
		{
			mouseInput.rightDoubleClick = true;
			mouseInput.rightDown = true;
			break;
		}

		default:
		{
			ASSERT(true); // this was not a mouse message
			break;
		}
	}
	
	if (g_lostFocusTimer.IsValid() && g_lostFocusTimer < 0.25f && g_gameControlBase->IsEditMode())
	{
		// ignore mouse input just after we regain focus when editing so user doesnt mess up their work
		mouseInput.leftDown = false;
		mouseInput.leftWentDownSubFrame = false;
		mouseInput.leftDoubleClick = false;
		mouseInput.rightDown = false;
		mouseInput.rightWentDownSubFrame = false;
		mouseInput.rightDoubleClick = false;
	}
}

Vector2 InputControl::GetMousePosLocalSpace() const
{
	// FF: code was adapted DirectX SDK Pick Sample

    // store the curser point and projection matrix
    const Vector2& mousePos = mouseInput.pos;
    const D3DXMATRIX matrixProjection = g_cameraBase->GetMatrixProjection().GetD3DXMatrix();

    // compute the vector of the pick ray in local space
    Vector2 mousePosLocalSpace
	(
		 (((2.0f * mousePos.x) / g_backBufferWidth) - 1) / matrixProjection._11,
		-(((2.0f * mousePos.y) / g_backBufferHeight) - 1) / matrixProjection._22
	);

	return mousePosLocalSpace;
}

Vector2 InputControl::GetLastMousePosLocalSpace() const
{
	// FF: code was adapted DirectX SDK Pick Sample

    // store the curser point and projection matrix
    const Vector2& mousePos = mouseInput.posLast;
    const D3DXMATRIX matrixProjection = g_cameraBase->GetMatrixProjection().GetD3DXMatrix();

    // compute the vector of the pick ray in screen space
    Vector2 mousePosLocalSpace
	(
		 (((2.0f * mousePos.x) / g_backBufferWidth) - 1) / matrixProjection._11,
		-(((2.0f * mousePos.y) / g_backBufferHeight) - 1) / matrixProjection._22
	);

	return mousePosLocalSpace;
}

Vector2 InputControl::GetMousePosWorldSpace() const
{
	return g_cameraBase->GetXFormWorld().TransformCoord(GetMousePosLocalSpace());
}

Vector2 InputControl::GetLastMousePosWorldSpace() const
{
	return g_cameraBase->GetXFormWorld().TransformCoord(GetLastMousePosLocalSpace());
}

Vector2 InputControl::GetMouseDeltaWorldSpace() const
{
	return GetMousePosWorldSpace() - GetLastMousePosWorldSpace();
}

Vector2 InputControl::GetInterpolatedMousePosWorldSpace() const
{
	return Lerp(g_interpolatePercent, GetMousePosWorldSpace(), GetLastMousePosWorldSpace());
}

Vector2 InputControl::GetInterpolatedMousePosLocalSpace() const
{
	return Lerp(g_interpolatePercent, GetMousePosLocalSpace(), GetLastMousePosLocalSpace());
}

////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
	GamepadInfo& info = gamepadInfo[gamepadCount];

    // Obtain an interface to the enumerated gamepad.
    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if (FAILED(directInput->CreateDevice(instance->guidInstance, &info.gamepad, NULL)))
        return DIENUM_CONTINUE;

	++gamepadCount;
	if (gamepadCount < gamepadMaxCount)
		return DIENUM_CONTINUE;

    // Stop enumeration. Note: we're just taking the first gamepad we get. You
    // could store all the enumerated gamepads and let the user pick.
    return DIENUM_STOP;
}

void InputControl::InitGamepad()
{
	for(int i = 0; i < gamepadMaxCount; ++i)
	{
		gamepadInfo[i].gamepad = NULL;
		gamepadInfo[i].wasPolled = false;
	}

	gamepadCount = 0;

	// Create a DirectInput device
	if (FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&directInput, NULL)))
		return;

	// Look for the first simple gamepad we can find.
	if (FAILED(directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, enumCallback, NULL, DIEDFL_ATTACHEDONLY)))
		return;

	// Make sure we got a gamepad
	for(int i = 0; i < gamepadMaxCount; ++i)
	{
		LPDIRECTINPUTDEVICE8 gamepad = gamepadInfo[i].gamepad;
		if (!gamepad)
			continue;

		// Set the data format to "simple gamepad" - a predefined data format 
		//
		// A data format specifies which controls on a device we are interested in,
		// and how they should be reported. This tells DInput that we will be
		// passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
		if (FAILED(gamepad->SetDataFormat(&c_dfDIJoystick2)))
		{
			gamepad->Unacquire();
			gamepad = NULL;
			return;
		}

		// Set the cooperative level to let DInput know how this device should
		// interact with the system and with other DInput applications.
		if (FAILED(gamepad->SetCooperativeLevel(DXUTGetHWND(), DISCL_EXCLUSIVE | DISCL_FOREGROUND)))
		{
			gamepad->Unacquire();
			gamepad = NULL;
			return;
		}

		// Determine how many axis the joystick has (so we don't error out setting
		// properties for unavailable axis)
		DIDEVCAPS& gamepadCapabilities = gamepadInfo[i].capabilities;
		gamepadCapabilities.dwSize = sizeof(DIDEVCAPS);
		if (FAILED(gamepad->GetCapabilities(&gamepadCapabilities)))
		{
			gamepad->Unacquire();
			gamepad = NULL;
			return;
		}
	}
}

bool InputControl::HasGamepad(UINT gamepadIndex) const
{ 
	return gamepadInfo[gamepadIndex].gamepad != NULL; 
}

int InputControl::GetGamepadAxesCount(UINT gamepadIndex) const
{
	return gamepadInfo[gamepadIndex].capabilities.dwAxes;
}

int InputControl::GetGamepadButtonCount(UINT gamepadIndex) const
{
	return gamepadInfo[gamepadIndex].capabilities.dwButtons;
}

bool InputControl::GetIsUsingSnesStyleGamepad(UINT gamepadIndex) const
{
	GamepadInfo& info = gamepadInfo[gamepadIndex];
	return (info.gamepad && enableGamepad && info.wasPolled && info.capabilities.dwAxes == 2 && info.capabilities.dwButtons == 8);
}

void InputControl::PollGamepad(UINT gamepadIndex)
{
	GamepadInfo& info = gamepadInfo[gamepadIndex];
	if (!enableGamepad || !info.gamepad)
		return;

	// Poll the device to read the current state
	if (FAILED(info.gamepad->Poll()))
	{
		// DInput is telling us that the input stream has been
		// interrupted. We aren't tracking any state between polls, so
		// we don't have any special reset that needs to be done. We
		// just re-acquire and try again.
		info.gamepad->Acquire();
	}

	// Get the input's device state
	if (FAILED(info.gamepad->GetDeviceState(sizeof(DIJOYSTATE2), &info.state)))
		return;

	info.wasPolled = true;
}

Vector2 InputControl::PollGamepadLeftStick(UINT gamepadIndex) const
{
	GamepadInfo& info = gamepadInfo[gamepadIndex];
	if (!enableGamepad || !info.gamepad || !info.wasPolled || info.capabilities.dwAxes < 2)
		return Vector2(0,0);

	/*const float max = 0xFFFF;
	const Vector2 input( gamepadState.lX / (max / 2) - 1, -gamepadState.lY / (max / 2) - 1);
	if (!input.IsZero())
		return input.Normalize() * Percent(input.Magnitude(), gamepadDeadzone, 1.0f);
	else
		return Vector2(0);*/

	const float max = 0xFFFF;
	float x = info.state.lX / (max / 2) - 1;
	float y = info.state.lY / (max / 2) - 1;

	if (x > 0)
	{
		x -= gamepadDeadzone;
		x /= (1 - gamepadDeadzone);
		x = Cap(x, 0.0f, 1.0f);
	}
	else
	{
		x += gamepadDeadzone;
		x /= (1 - gamepadDeadzone);
		x = Cap(x, -1.0f, 0.0f);
	}

	if (y > 0)
	{
		y -= gamepadDeadzone;
		y /= (1 - gamepadDeadzone);
		y = Cap(y, 0.0f, 1.0f);
	}
	else
	{
		y += gamepadDeadzone;
		y /= (1 - gamepadDeadzone);
		y = Cap(y, -1.0f, 0.0f);
	}
	
	Vector2 result = Vector2(x, -y);
	if (result.Length() > gamepadUpperDeadzone)
		result = result.Normalize();
	else
		result /= gamepadUpperDeadzone;
	return result;
}

Vector2 InputControl::PollGamepadRightStick(UINT gamepadIndex) const
{
	GamepadInfo& info = gamepadInfo[gamepadIndex];
	if (!enableGamepad || !info.gamepad || !info.wasPolled)
		return Vector2(0,0);

	if (enableGamepadRightStickEmulation && GetIsUsingSnesStyleGamepad(gamepadIndex))
	{
		// emulate right stick on snes style controler (2 axes & 8 buttons)
		static Vector2 result(0, 0);
		Vector2 direction(0, 0);
		const float decay = 0.95f;

		if (info.state.rgbButtons[0] & 0x80)
			direction.x -= 1;
		if (info.state.rgbButtons[1] & 0x80)
			direction.y += 1;
		if (info.state.rgbButtons[2] & 0x80)
			direction.y -= 1;
		if (info.state.rgbButtons[3] & 0x80)
			direction.x += 1;
		
		// move towards direction
		result = result + gamepadRightStickEmulationSpeed*direction;
		result.x = Cap(result.x, -1.0f, 1.0f);
		result.y = Cap(result.y, -1.0f, 1.0f);
		
		// decay
		if (direction.x == 0)
			result.x *= decay;
		if (direction.y == 0)
			result.y *= decay;

		if (fabs(result.y) < 0.01f)
			result.y = 0;
		if (fabs(result.x) < 0.01f)
			result.x = 0;
		
		return result;
	}

	if (info.capabilities.dwAxes < 4)
		return Vector2(0,0);

	const float max = 0xFFFF;

	float x = info.state.lRx / (max / 2) - 1;
	float y = info.state.lRy / (max / 2) - 1;

	if (x > 0)
	{
		x -= gamepadDeadzone;
		x /= (1 - gamepadDeadzone);
		x = Cap(x, 0.0f, 1.0f);
	}
	else
	{
		x += gamepadDeadzone;
		x /= (1 - gamepadDeadzone);
		x = Cap(x, -1.0f, 0.0f);
	}

	if (y > 0)
	{
		y -= gamepadDeadzone;
		y /= (1 - gamepadDeadzone);
		y = Cap(y, 0.0f, 1.0f);
	}
	else
	{
		y += gamepadDeadzone;
		y /= (1 - gamepadDeadzone);
		y = Cap(y, -1.0f, 0.0f);
	}

	Vector2 result = Vector2(x, -y);
	if (result.Length() > gamepadUpperDeadzone)
		result = result.Normalize();
	else
		result /= gamepadUpperDeadzone;

	return result;
}

float InputControl::PollGamepadLeftTrigger(UINT gamepadIndex) const
{
	GamepadInfo& info = gamepadInfo[gamepadIndex];
	if (!enableGamepad || !info.gamepad || !info.wasPolled)
		return 0;
	
	if (GetIsUsingSnesStyleGamepad(gamepadIndex))
	{
		// emulate trigger on snes style controler (2 axes & 8 buttons)
		return (info.state.rgbButtons[4] & 0x80)? 1.0f : 0.0f;
	}

	if (info.capabilities.dwAxes < 5)
		return 0;

	const float max = 0xFFFF;	
	float z = info.state.lZ / (max / 2) - 1;

	if (z > 0)
	{
		z -= gamepadDeadzone;
		z /= (1 - gamepadDeadzone);
		z = CapPercent(z);
	}
	else
		z = 0;

	if (z > gamepadUpperDeadzone)
		z = 1.0f;
	else if (z < -gamepadUpperDeadzone)
		z = -1.0f;
	else
		z /= gamepadUpperDeadzone;
	return z;
}

float InputControl::PollGamepadRightTrigger(UINT gamepadIndex) const
{
	GamepadInfo& info = gamepadInfo[gamepadIndex];
	if (!enableGamepad || !info.gamepad || !info.wasPolled)
		return 0;
	
	if (GetIsUsingSnesStyleGamepad(gamepadIndex))
	{
		// emulate trigger on snes style controler (2 axes & 8 buttons)
		return (info.state.rgbButtons[5] & 0x80)? 1.0f : 0.0f;
	}

	if (info.capabilities.dwAxes < 5)
		return 0;

	const float max = 0xFFFF;	
	float z = -(info.state.lZ / (max / 2) - 1);

	if (z > 0)
	{
		z -= gamepadDeadzone;
		z /= (1 - gamepadDeadzone);
		z = CapPercent(z);
	}
	else
		z = 0;

	if (z > gamepadUpperDeadzone)
		z = 1.0f;
	else if (z < -gamepadUpperDeadzone)
		z = -1.0f;
	else
		z /= gamepadUpperDeadzone;
	return z;
}

bool InputControl::HadAnyInput() const
{
	// check if anything was down or is down
	for (GameButtonIndex i = GB_Invalid; i < GB_MaxCount; i = (GameButtonIndex)(i + 1))
	{
		const GameButtonInfo& buttonInfo = gameButtonInfo[i];
		bool hadInput = false;
		// check if any of the keys for this button are down
		for (list<UINT>::const_iterator it = buttonInfo.keys.begin(); it != buttonInfo.keys.end(); ++it) 
		{
			const UINT key = *it;
			if (key == VK_LBUTTON)
			{
				hadInput |= mouseInput.leftDown;
				hadInput |= mouseInput.leftDoubleClick;
				hadInput |= mouseInput.leftWentDownSubFrame;
			}
			else if (key == VK_MBUTTON)
			{
				hadInput |= mouseInput.middleDown;
				hadInput |= mouseInput.middleDoubleClick;
				hadInput |= mouseInput.middleWentDownSubFrame;
			}
			else if (key == VK_RBUTTON)
			{
				hadInput |= mouseInput.rightDown;
				hadInput |= mouseInput.rightDoubleClick;
				hadInput |= mouseInput.rightWentDownSubFrame;
			}
			else if (key < KEYBOARD_INPUT_SIZE)
			{
				hadInput |= IsKeyDown(key);
				hadInput |= IsKeyDownSubFrame(key);
			}
			else if (key >= GAMEPAD_BUTTON_START && key < GAMEPAD_BUTTON_END)
			{
				UINT actualKey = key;
				UINT gamepadIndex = 0;
				GetGamepadKey(actualKey, gamepadIndex);

				GamepadInfo& info = gamepadInfo[gamepadIndex];
				if (!enableGamepad || !info.gamepad || !info.wasPolled)
					continue;

				if (actualKey < GAMEPAD_BUTTON_LAST)
				{
					// get the gamepad button input
					const UINT button = actualKey - GAMEPAD_BUTTON_START;
					hadInput |= (info.state.rgbButtons[button] & 0x80) != 0;
				}
				else if (actualKey >= GAMEPAD_BUTTON_POV_UP && actualKey <= GAMEPAD_BUTTON_POV_LEFT)
				{
					// get the pov (dpad) input
					DWORD pov = info.state.rgdwPOV[0];
					hadInput |= (pov != -1);
				}
				else
				{
					// treat the stick input as a button press
					if (actualKey == GAMEPAD_BUTTON_UP)
						hadInput |= GetGamepadLeftStick(gamepadIndex).y > 0;
					else if (actualKey == GAMEPAD_BUTTON_DOWN)
						hadInput |= GetGamepadLeftStick(gamepadIndex).y < 0;
					else if (actualKey == GAMEPAD_BUTTON_RIGHT)
						hadInput |= GetGamepadLeftStick(gamepadIndex).x > 0;
					else if (actualKey == GAMEPAD_BUTTON_LEFT)
						hadInput |= GetGamepadLeftStick(gamepadIndex).x < 0;
					else if (actualKey == GAMEPAD_BUTTON_LEFT_TRIGGER)
						hadInput |= GetGamepadLeftTrigger(gamepadIndex) > 0;
					else if (actualKey == GAMEPAD_BUTTON_RIGHT_TRIGGER)
						hadInput |= GetGamepadRightTrigger(gamepadIndex) > 0;
				}
			}
		}

		if (hadInput)
			return true;
	}

	return false;
}

void InputControl::UpdateSubFrame()
{
	// for updating mouse pos and other stuff when game is running in slow motion

	// handle mouse input
	POINT ptCursor;
	GetCursorPos( &ptCursor );
	ScreenToClient( DXUTGetHWND(), &ptCursor );
	
	// update mouse pos
	mouseInput.pos = Vector2((float)(ptCursor.x), (float)(ptCursor.y));
	mouseInput.posLast = mouseInput.pos;
	
	for(int i = 0; i < gamepadMaxCount; ++i)
	{
		PollGamepad(i);
		GamepadInfo& info = gamepadInfo[i];
		info.leftStick = PollGamepadLeftStick(i);
		info.rightStick = PollGamepadRightStick(i);
		info.leftTrigger = PollGamepadLeftTrigger(i);
		info.rightTrigger = PollGamepadRightTrigger(i);
	}
}

DWORD InputControl::GetPOVRaw(UINT gamepadIndex, UINT index) const
{
	ASSERT(index >= 0 && index < 4);
	return gamepadInfo[gamepadIndex].state.rgdwPOV[index];
}

bool InputControl::HasGamepadInput(UINT gamepadIndex)
{
	GamepadInfo& info = gamepadInfo[gamepadIndex];
	if (!enableGamepad || !info.gamepad || !info.wasPolled)
		return false;

	bool hadInput = false;
	{
		// get the pov (dpad) input
		DWORD pov = info.state.rgdwPOV[0];
		hadInput |= (pov != -1);
	}
	{
		// treat the stick input as a button press
		const Vector2 leftStick = GetGamepadLeftStick(gamepadIndex);
		hadInput |= leftStick.y > 0;
		hadInput |= leftStick.y < 0;
		hadInput |= leftStick.x > 0;
		hadInput |= leftStick.x < 0;
		const Vector2 rightStick = GetGamepadRightStick(gamepadIndex);
		hadInput |= rightStick.y > 0;
		hadInput |= rightStick.y < 0;
		hadInput |= rightStick.x > 0;
		hadInput |= rightStick.x < 0;
		hadInput |= GetGamepadLeftTrigger(gamepadIndex) > 0;
		hadInput |= GetGamepadRightTrigger(gamepadIndex) > 0;
	}
	
	for (int i = 0; i <= GAMEPAD_BUTTON_LAST - GAMEPAD_BUTTON_START; ++i)
		hadInput |= (info.state.rgbButtons[i] & 0x80) != 0;

	return hadInput;
}

Vector2 InputControl::GetGamepadDPad(UINT gamepadIndex) const					
{ 
	GamepadInfo& info = gamepadInfo[gamepadIndex];
	if (!enableGamepad || !info.gamepad || !info.wasPolled)
		return Vector2(0);
	
	DWORD pov = info.state.rgdwPOV[0];
	if (pov == -1)
		return Vector2(0);

	float angle = float(pov);
	angle *= PI / 18000;

	Vector2 v = Vector2::BuildFromAngle(angle);
	v *= Vector2(-1, 1); // flip the sign
	return v;
}

Vector2 InputControl::GetGamepadLeftStick(UINT gamepadIndex) const					
{ return gamepadInfo[gamepadIndex].leftStick.Get(); }

Vector2 InputControl::GetGamepadRightStick(UINT gamepadIndex) const				
{ return gamepadInfo[gamepadIndex].rightStick.Get(); }

float InputControl::GetGamepadLeftTrigger(UINT gamepadIndex) const					
{ return gamepadInfo[gamepadIndex].leftTrigger.Get(); }

float InputControl::GetGamepadRightTrigger(UINT gamepadIndex) const		
{ return gamepadInfo[gamepadIndex].rightTrigger.Get(); }
	
Vector2 InputControl::GetGamepadLeftStickInterpolated(UINT gamepadIndex) const		
{ return gamepadInfo[gamepadIndex].leftStick.GetInterpolated(g_interpolatePercent); }

Vector2 InputControl::GetGamepadRightStickInterpolated(UINT gamepadIndex) const	
{ return gamepadInfo[gamepadIndex].rightStick.GetInterpolated(g_interpolatePercent); }

float InputControl::GetGamepadLeftTriggerInterpolated(UINT gamepadIndex) const		
{ return gamepadInfo[gamepadIndex].leftTrigger.GetInterpolated(g_interpolatePercent); }

float InputControl::GetGamepadRightTriggerInterpolated(UINT gamepadIndex) const	
{ return gamepadInfo[gamepadIndex].rightTrigger.GetInterpolated(g_interpolatePercent); }

void InputControl::SetButton(GameButtonIndex gbi, UINT key, UINT gamepadIndex)
{
	ASSERT(gbi < GB_MaxCount);
	ASSERT(key >= GAMEPAD_BUTTON_START && key < GAMEPAD_BUTTON_END);
	ASSERT(gamepadIndex < gamepadMaxCount);
	gameButtonInfo[gbi].keys.push_back(key+GAMEPAD_BUTTON_GROUP_COUNT*gamepadIndex);
}

void InputControl::GetGamepadKey(UINT& key, UINT& gamepadIndex) const
{
	ASSERT(key >= GAMEPAD_BUTTON_START);

	key -= GAMEPAD_BUTTON_START;
	gamepadIndex = (key / GAMEPAD_BUTTON_GROUP_COUNT);
	key -= GAMEPAD_BUTTON_GROUP_COUNT * gamepadIndex;
	key += GAMEPAD_BUTTON_START;

	ASSERT(key >= GAMEPAD_BUTTON_START && key < GAMEPAD_BUTTON_END);
	ASSERT(gamepadIndex < gamepadMaxCount);
}