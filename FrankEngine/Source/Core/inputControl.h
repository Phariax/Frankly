////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Input Processing
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef INPUT_CONTROL_H
#define INPUT_CONTROL_H

#define KEYBOARD_INPUT_SIZE			(256)
#define GAMEPAD_BUTTON_GROUP_COUNT	(100)

enum GamepadButtons
{
	GAMEPAD_BUTTON_START = KEYBOARD_INPUT_SIZE,
	GAMEPAD_BUTTON_0 = GAMEPAD_BUTTON_START,
	GAMEPAD_BUTTON_1,
	GAMEPAD_BUTTON_2,
	GAMEPAD_BUTTON_3,
	GAMEPAD_BUTTON_4,
	GAMEPAD_BUTTON_5,
	GAMEPAD_BUTTON_6,
	GAMEPAD_BUTTON_7,
	GAMEPAD_BUTTON_8,
	GAMEPAD_BUTTON_9,
	GAMEPAD_BUTTON_LAST = GAMEPAD_BUTTON_9,
	GAMEPAD_BUTTON_UP,
	GAMEPAD_BUTTON_DOWN,
	GAMEPAD_BUTTON_RIGHT,
	GAMEPAD_BUTTON_LEFT,
	GAMEPAD_BUTTON_POV_UP,
	GAMEPAD_BUTTON_POV_DOWN,
	GAMEPAD_BUTTON_POV_RIGHT,
	GAMEPAD_BUTTON_POV_LEFT,
	GAMEPAD_BUTTON_LEFT_TRIGGER,
	GAMEPAD_BUTTON_RIGHT_TRIGGER,
	GAMEPAD_BUTTON_END,

	// aliases for XBOX 360 controller
	GAMEPAD_XBOX_A			= GAMEPAD_BUTTON_0,
	GAMEPAD_XBOX_B			= GAMEPAD_BUTTON_1,
	GAMEPAD_XBOX_X			= GAMEPAD_BUTTON_2,
	GAMEPAD_XBOX_Y			= GAMEPAD_BUTTON_3,
	GAMEPAD_XBOX_LB			= GAMEPAD_BUTTON_4,
	GAMEPAD_XBOX_RB			= GAMEPAD_BUTTON_5,
	GAMEPAD_XBOX_BACK		= GAMEPAD_BUTTON_6,
	GAMEPAD_XBOX_START		= GAMEPAD_BUTTON_7,
	GAMEPAD_XBOX_LTHUMB		= GAMEPAD_BUTTON_8,
	GAMEPAD_XBOX_RTHUMB		= GAMEPAD_BUTTON_9,
	GAMEPAD_XBOX_DPAD_UP	= GAMEPAD_BUTTON_POV_UP,
	GAMEPAD_XBOX_DPAD_DOWN	= GAMEPAD_BUTTON_POV_DOWN,
	GAMEPAD_XBOX_DPAD_RIGHT	= GAMEPAD_BUTTON_POV_RIGHT,
	GAMEPAD_XBOX_DPAD_LEFT	= GAMEPAD_BUTTON_POV_LEFT,
};

// hack because we want some game buttons accessable in the editor
enum GameButtonIndex;
const GameButtonIndex GB_Invalid				= GameButtonIndex(0);
const GameButtonIndex GB_MouseLeft				= GameButtonIndex(1);
const GameButtonIndex GB_MouseMiddle			= GameButtonIndex(2);
const GameButtonIndex GB_MouseRight				= GameButtonIndex(3);
const GameButtonIndex GB_Control				= GameButtonIndex(4);
const GameButtonIndex GB_Shift					= GameButtonIndex(5);
const GameButtonIndex GB_Alt					= GameButtonIndex(6);
const GameButtonIndex GB_Tab					= GameButtonIndex(7);
const GameButtonIndex GB_Space					= GameButtonIndex(8);
const GameButtonIndex GB_Editor					= GameButtonIndex(9);
const GameButtonIndex GB_InfoDisplay			= GameButtonIndex(10);
const GameButtonIndex GB_PhysicsDebug			= GameButtonIndex(11);
const GameButtonIndex GB_RefreshResources		= GameButtonIndex(12);
const GameButtonIndex GB_Screenshot				= GameButtonIndex(13);
const GameButtonIndex GB_Profiler				= GameButtonIndex(14);
const GameButtonIndex GB_Restart				= GameButtonIndex(15);
const GameButtonIndex GB_Up						= GameButtonIndex(16);
const GameButtonIndex GB_Down					= GameButtonIndex(17);
const GameButtonIndex GB_Right					= GameButtonIndex(18);
const GameButtonIndex GB_Left					= GameButtonIndex(19);
const GameButtonIndex GB_Editor_Mode_Object		= GameButtonIndex(20);
const GameButtonIndex GB_Editor_Mode_Terrain1	= GameButtonIndex(21);
const GameButtonIndex GB_Editor_Mode_Terrain2	= GameButtonIndex(22);
const GameButtonIndex GB_Editor_Cut				= GameButtonIndex(23);
const GameButtonIndex GB_Editor_Copy			= GameButtonIndex(24);
const GameButtonIndex GB_Editor_Paste			= GameButtonIndex(25);
const GameButtonIndex GB_Editor_Undo			= GameButtonIndex(26);
const GameButtonIndex GB_Editor_Redo			= GameButtonIndex(27);
const GameButtonIndex GB_Editor_MovePlayer		= GameButtonIndex(28);
const GameButtonIndex GB_Editor_Add				= GameButtonIndex(29);
const GameButtonIndex GB_Editor_Remove			= GameButtonIndex(30);
const GameButtonIndex GB_Editor_Up				= GameButtonIndex(31);
const GameButtonIndex GB_Editor_Down			= GameButtonIndex(32);
const GameButtonIndex GB_Editor_Left			= GameButtonIndex(33);
const GameButtonIndex GB_Editor_Right			= GameButtonIndex(34);
const GameButtonIndex GB_Editor_RunSifteo		= GameButtonIndex(35);
const GameButtonIndex GB_Editor_RotateCW		= GameButtonIndex(36);
const GameButtonIndex GB_Editor_RotateCCW		= GameButtonIndex(37);
const GameButtonIndex GB_Editor_TileSetUp		= GameButtonIndex(38);
const GameButtonIndex GB_Editor_TileSetDown		= GameButtonIndex(39);
const GameButtonIndex GB_Editor_Save			= GameButtonIndex(40);
const GameButtonIndex GB_Editor_Load			= GameButtonIndex(41);
const GameButtonIndex GB_Editor_Insert			= GameButtonIndex(42);
const GameButtonIndex GB_Editor_Delete			= GameButtonIndex(43);
const GameButtonIndex GB_Editor_Grid			= GameButtonIndex(44);
const GameButtonIndex GB_Editor_BlockEdit		= GameButtonIndex(45);
const GameButtonIndex GB_StartGameButtons		= GameButtonIndex(46);	// this is where gameside button ids start
const GameButtonIndex GB_MaxCount				= GameButtonIndex(256);	// total amount of allowable buttons

class InputControl
{
public:

	InputControl();
	~InputControl();

	void Update();
	void Clear();
	bool HadAnyInput() const;

	void SetButton(GameButtonIndex buttonIndex, UINT key);
	void SetButton(GameButtonIndex buttonIndex, UINT key, UINT gamepadIndex);
	bool IsDown(GameButtonIndex gbi) const;
	bool WasJustPushed(GameButtonIndex gbi) const;
	bool WasJustReleased(GameButtonIndex gbi) const;
	bool IsDownFromDoubleClick(GameButtonIndex gbi) const;
	bool WasJustDoubleClicked(GameButtonIndex gbi) const;
	bool IsDownUI(GameButtonIndex gbi) const;
	void ClearInput(GameButtonIndex gbi);

	// gamepad input
	void InitGamepad();
	bool HasGamepad(UINT gamepadIndex) const;
	void PollGamepad(UINT gamepadIndex);
	bool HasGamepadInput(UINT gamepadIndex);

	Vector2 GetGamepadLeftStick(UINT gamepadIndex) const;
	Vector2 GetGamepadRightStick(UINT gamepadIndex) const;
	Vector2 GetGamepadDPad(UINT gamepadIndex) const;
	float GetGamepadLeftTrigger(UINT gamepadIndex) const;
	float GetGamepadRightTrigger(UINT gamepadIndex) const;
	
	Vector2 GetGamepadLeftStickInterpolated(UINT gamepadIndex) const;
	Vector2 GetGamepadRightStickInterpolated(UINT gamepadIndex) const;
	float GetGamepadLeftTriggerInterpolated(UINT gamepadIndex) const;
	float GetGamepadRightTriggerInterpolated(UINT gamepadIndex) const;

	Vector2 PollGamepadLeftStick(UINT gamepadIndex) const;
	Vector2 PollGamepadRightStick(UINT gamepadIndex) const;
	float PollGamepadLeftTrigger(UINT gamepadIndex) const;
	float PollGamepadRightTrigger(UINT gamepadIndex) const;

	int GetGamepadAxesCount(UINT gamepadIndex) const;
	int GetGamepadButtonCount(UINT gamepadIndex) const;
	bool GetIsUsingSnesStyleGamepad(UINT gamepadIndex) const;

	// mouse input
	float GetMouseWheel() const;
	Vector2 GetMousePosScreenSpace() const { return mouseInput.pos; }
	void SetMousePosScreenSpace(const Vector2& mousePos);

	Vector2 GetMousePosLocalSpace() const;
	Vector2 GetLastMousePosLocalSpace() const;
	Vector2 GetMousePosWorldSpace() const;
	Vector2 GetLastMousePosWorldSpace() const;
	Vector2 GetMouseDeltaLocalSpace() const { return mouseInput.pos - mouseInput.posLast; }
	Vector2 GetMouseDeltaWorldSpace() const;
	Vector2 GetInterpolatedMousePosWorldSpace() const;
	Vector2 GetInterpolatedMousePosLocalSpace() const;

	// check directly if key is down
	bool IsKeyDown(UINT key) const
	{ 
		ASSERT(key < KEYBOARD_INPUT_SIZE);
		return keyboardInput[key];
	}

	// check directly if key went down between input frames
	// note this will still return true if a button went down and up during a sub frame
	bool IsKeyDownSubFrame(UINT key) const
	{ 
		ASSERT(key < KEYBOARD_INPUT_SIZE);
		return keyboardInputSubFrame[key];
	}
	
	void UpdateSubFrame();

	DWORD GetPOVRaw(UINT gamepadIndex, UINT index) const;
	
	void GetGamepadKey(UINT& key, UINT& gamepadIndex) const;
	bool IsGamepadButtonDown(GamepadButtons button, UINT gamepadIndex);

public:

	void OnMouseMessage(UINT uMsg, WPARAM wParam);
	void OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );

	static bool enableGamepadRightStickEmulation;
	static bool enableGamepadTriggerEmulation;
	static float gamepadRightStickEmulationSpeed;
	static float gamepadStickButtonMin;
	static float gamepadTriggerButtonMin;

private:

	struct MouseInput
	{
		bool rightDown;
		bool rightDoubleClick;
		bool middleDown;
		bool middleDoubleClick;
		bool leftDown;
		bool leftDoubleClick;

		bool rightWentDownSubFrame;
		bool middleWentDownSubFrame;
		bool leftWentDownSubFrame;

		Vector2 pos;
		Vector2 posLast;
		float wheelLast;
		float wheel;
	};

	struct GameButtonInfo
	{
		GameButtonInfo() { Reset(); }
		void Reset() 
		{
			isDown = false;
			wasJustPushed = false;
			wasJustReleased = false;
			isDownFromDoubleClick = false;
			wasJustDoubleClicked = false;
			isDownUI = false; 
			timeUI = 0; 
		}

		list<UINT> keys;

		bool isDown;
		bool wasJustPushed;
		bool wasJustReleased;
		bool isDownFromDoubleClick;
		bool wasJustDoubleClicked;
		bool isDownUI;
		int timeUI;
	};

	MouseInput mouseInput;
	bool keyboardInput[KEYBOARD_INPUT_SIZE];
	bool keyboardInputSubFrame[KEYBOARD_INPUT_SIZE];
	GameButtonInfo gameButtonInfo[GB_MaxCount];
	bool wasCleared;
};

// inline functions

inline void InputControl::ClearInput(GameButtonIndex gbi)
{
	ASSERT(gbi < GB_MaxCount);
	gameButtonInfo[gbi].Reset();
}

inline void InputControl::SetButton(GameButtonIndex gbi, UINT key)
{
	ASSERT(gbi < GB_MaxCount);
	gameButtonInfo[gbi].keys.push_back(key);
}

inline bool InputControl::IsDown(GameButtonIndex gbi) const
{
	ASSERT(gbi < GB_MaxCount);
	return gameButtonInfo[gbi].isDown;
}

inline bool InputControl::IsDownUI(GameButtonIndex gbi) const
{
	ASSERT(gbi < GB_MaxCount);
	return gameButtonInfo[gbi].isDownUI || WasJustPushed(gbi);
}

inline bool InputControl::WasJustPushed(GameButtonIndex gbi) const
{
	ASSERT(gbi < GB_MaxCount);
	return gameButtonInfo[gbi].wasJustPushed;
}

inline bool InputControl::WasJustReleased(GameButtonIndex gbi) const
{
	ASSERT(gbi < GB_MaxCount);
	return gameButtonInfo[gbi].wasJustReleased;
}

inline bool InputControl::IsDownFromDoubleClick(GameButtonIndex gbi) const
{
	ASSERT(gbi < GB_MaxCount);
	return gameButtonInfo[gbi].isDownFromDoubleClick;
}

inline bool InputControl::WasJustDoubleClicked(GameButtonIndex gbi) const
{
	ASSERT(gbi < GB_MaxCount);
	return gameButtonInfo[gbi].wasJustDoubleClicked;
}

inline float InputControl::GetMouseWheel() const 
{ 
	return mouseInput.wheelLast; 
}

#endif // INPUT_CONTROL_H