////////////////////////////////////////////////////////////////////////////////////////
/*
	Debug Command Console
	Copyright 2013 Frank Force - http://www.frankforce.com

	text console where user can tweak variables or call functions
	- simple to expose a variable or callback with provided macro
	- transforms raw keyboard input into plain text
	- uses stl string stream to process user input
	- user can flip through previous input lines or search commands
	- renders via directx and dxut
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H

#include <list>
#include <vector>
#include <queue>
#include <sstream>

////////////////////////////////////////////////////////////////////////////////////////

// use this macro to add console commands
#define ConsoleCommand(variable, name) static DebugConsole::Command name##_CONSOLE_COMMAND(variable, L#name);

// global access to the debug console
class DebugConsole& GetDebugConsole();

////////////////////////////////////////////////////////////////////////////////////////

class DebugConsole
{
public:

	DebugConsole();
	void Update(float delta);
	void Render();
	void Clear()			{ lines.clear(); }
	bool IsOpen() const		{ return isOpen; }
	void ParseFile(const wstring& filename, bool showFileNotFound = true);
	void Init();

	void AddLine(const wstring& text = wstring(L""));
	void AddLine(const wstring& text, const Color& color);
	void AddError(const wstring& text = wstring(L""));
	void AddFormatted(const wstring messageFormat, ...);

	void OnKeyboard(UINT nChar);

public:	// commands

	// nornaly this class should only be called via provided macros
	struct Command
	{
		enum Type
		{
			Type_invalid,
			Type_bool,
			Type_int8,
			Type_uint8,
			Type_int16,
			Type_uint16,
			Type_int32,
			Type_uint32,
			Type_float,
			Type_double,
			Type_intvector2,
			Type_vector2,
			Type_vector3,
			Type_color,
			Type_string,
			Type_wstring,
			Type_function
		};

		typedef void (*CallbackFunction)(const wstring&);

		Command(void* _value,		const wstring& _name, Type _type)	: name(_name), value(_value), type(_type)				{ GetDebugConsole().AddCommand(*this); }
		Command(bool& _value,		const wstring& _name)			: name(_name), value((void*)&_value), type(Type_bool)		{ GetDebugConsole().AddCommand(*this); }
		Command(int8& _value,		const wstring& _name)			: name(_name), value((void*)&_value), type(Type_int8)		{ GetDebugConsole().AddCommand(*this); }
		Command(uint8& _value,		const wstring& _name)			: name(_name), value((void*)&_value), type(Type_uint8)		{ GetDebugConsole().AddCommand(*this); }
		Command(int16& _value,		const wstring& _name)			: name(_name), value((void*)&_value), type(Type_int16)		{ GetDebugConsole().AddCommand(*this); }
		Command(uint16& _value,		const wstring& _name)			: name(_name), value((void*)&_value), type(Type_uint16)		{ GetDebugConsole().AddCommand(*this); }
		Command(int32& _value,		const wstring& _name)			: name(_name), value((void*)&_value), type(Type_int32)		{ GetDebugConsole().AddCommand(*this); }
		Command(uint32& _value,		const wstring& _name)			: name(_name), value((void*)&_value), type(Type_uint32)		{ GetDebugConsole().AddCommand(*this); }
		Command(float& _value,		const wstring& _name)			: name(_name), value((void*)&_value), type(Type_float)		{ GetDebugConsole().AddCommand(*this); }
		Command(double& _value,		const wstring& _name)			: name(_name), value((void*)&_value), type(Type_double)		{ GetDebugConsole().AddCommand(*this); }
		Command(IntVector2& _value,	const wstring& _name)			: name(_name), value((void*)&_value), type(Type_intvector2)	{ GetDebugConsole().AddCommand(*this); }
		Command(Vector2& _value,	const wstring& _name)			: name(_name), value((void*)&_value), type(Type_vector2)	{ GetDebugConsole().AddCommand(*this); }
		Command(Vector3& _value,	const wstring& _name)			: name(_name), value((void*)&_value), type(Type_vector3)	{ GetDebugConsole().AddCommand(*this); }
		Command(Color& _value,		const wstring& _name)			: name(_name), value((void*)&_value), type(Type_color)		{ GetDebugConsole().AddCommand(*this); }
		Command(char* _value,		const wstring& _name)			: name(_name), value((void*)_value), type(Type_string)		{ GetDebugConsole().AddCommand(*this); }
		Command(WCHAR* _value,		const wstring& _name)			: name(_name), value((void*)_value), type(Type_wstring)	
		{ 
			GetDebugConsole().AddCommand(*this); 
		}
		Command(CallbackFunction _function,	const wstring& _name)	: name(_name), value((void*)_function), type(Type_function)	{ GetDebugConsole().AddCommand(*this); }
		~Command()	{ ASSERT(true); GetDebugConsole().RemoveCommand(*this); }

		void Run(wstringstream& inputStream);
		void SkipWhitespace(wstringstream& inputStream);

		wstring GetValueString() const;

		wstring name;
		void* value;
		Type type;
	};

	Command* FindCommand(const wstring& name);
	void AddCommand(Command& command);
	void RemoveCommand(Command& command);
	void Save() const;
	void Load();

	list<Command*> commandList;

private:	// input processing

	void ProcessInput();
	void ParseInputLine(const wstring& input);
	void UpdateSkiGame();

	bool ReadNextKey(UINT& key) 
	{ 
		if (keyBuffer.empty())
			return false;
		key = keyBuffer.front();
		keyBuffer.pop();
		return true;
	}

	// keep a buffer each time a key is pushed
	queue<UINT> keyBuffer;

private:	// console lines

	struct ConsoleLine
	{
		ConsoleLine(const wstring& _text, const Color& _color = Color::White()) : text(_text), color(_color) {}

		wstring text;
		Color color;
	};

	static const int maxLines = 32;
	vector<ConsoleLine> lines;			// list of all the input lines in the buffer
	wstring inputLine;					// current input line being typed by the user

	vector<Command*> findMatches;		// all matches found when user pressed search (tab)
	vector<wstring> lineMemory;			// list of user's previous commands
	unsigned int lineMemoryPos;			// user can flip through previous commands

	bool isOpen;						// is console curretly open
	float openPercent;					// used to move console into view when it is opened

public:

	static bool enable;					// allow use of the console
};

#endif // DEBUG_CONSOLE_H