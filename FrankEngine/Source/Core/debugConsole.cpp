////////////////////////////////////////////////////////////////////////////////////////
/*
	Debug Command Console
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include <fstream>
#include "../core/debugConsole.h"

#define CONSOLE_COLOR_INPUT		(Color(0.5f, 0.5f, 1.0f, 1.0f))
#define CONSOLE_COLOR_NORMAL	(Color(1.0f, 1.0f, 1.0f, 1.0f))
#define CONSOLE_COLOR_ERROR		(Color(1.0f, 0.5f, 0.5f, 1.0f))
#define CONSOLE_COLOR_SUCCESS	(Color(0.5f, 1.0f, 0.5f, 1.0f))
#define CONSOLE_COLOR_SPECIAL	(Color(1.0f, 1.0f, 0.5f, 1.0f))
#define CONSOLE_COLOR_BACK		(Color(0.0f,0.05f, 0.0f, 0.8f))
#define CONSOLE_COLOR_BORDER	(Color(0.0f, 0.0f, 0.0f, 0.5f))

bool DebugConsole::enable = true;
ConsoleCommand(DebugConsole::enable, enableConsole);

static const wstring consoleMemoryfilename = L"consoleMemory.txt";

////////////////////////////////////////////////////////////////////////////////////////

DebugConsole& GetDebugConsole()
{
	static DebugConsole console;
	return console;
}

DebugConsole::DebugConsole() :
	openPercent(0),
	isOpen(false),
	lineMemoryPos(0)
{
	wstring s = wstring(L"Booting ") + frankEngineName + wstring(L" ") + frankEngineVersion + wstring(L"...");
	AddLine(s, CONSOLE_COLOR_SPECIAL);
	AddLine();
}

void DebugConsole::Init()
{
	wstring s = frankEngineName + wstring(L" ") + frankEngineVersion;
	AddLine(s, CONSOLE_COLOR_SPECIAL);
	AddLine(L"Type 'help' for more info about the debug console.");
	AddLine();

	Load();
}

void DebugConsole::Update(float delta)
{
	if (!enable && isOpen)
	{
		// ~ key toggles console 
		isOpen = !isOpen;
		findMatches.clear();
		lineMemoryPos = 0;
		g_editorGui.ClearFocus();
		return;
	}

	const float openSpeed = 10;
	openPercent += (isOpen? 1 : -1)*delta * openSpeed;
	openPercent = CapPercent(openPercent);
	
	UpdateSkiGame();
	ProcessInput();

	while (lines.size() > maxLines)
	{
		// get rid of excess lines
		lines.pop_back();
	}
}

void DebugConsole::Save() const
{
	if (lineMemory.empty())
		return;

	ofstream outFile(consoleMemoryfilename);
	if (outFile.fail())
	{
		outFile.close();
		return;
	}

	static const int maxSaveLines = 100;
	int skipLines = lineMemory.size() - maxSaveLines;
	for (vector<wstring>::const_iterator it = lineMemory.begin(); it != lineMemory.end(); ++it)
	{
		if (skipLines-- > 0)
			continue;

		const wstring& wline = *it;
		char line[256];
		wcstombs_s(NULL, line, 256, wline.c_str(), 255);
		outFile << line << endl;
	}
		
	outFile.close();
}

void DebugConsole::Load()
{
	ifstream inFile(consoleMemoryfilename);
	while (!inFile.eof() && !inFile.fail())
	{
		char buffer[1024];
		inFile.getline(buffer, 1024);

		if (strlen(buffer) == 0)
			continue;
		
		string line(buffer);
		std::wstring wline;
		wline.assign(line.begin(), line.end());
		lineMemory.push_back(wline);
	}
		
	inFile.close();
}

void DebugConsole::ParseInputLine(const wstring& input)
{
	wstringstream inputStream(input);
	wstring name;
	inputStream >> name;

	if (name.empty())
		return;

	// search for this command
	Command* command = FindCommand(name);
	if (command)
		command->Run(inputStream);
	else
		AddError(wstring(L"Command not found"));
}

DebugConsole::Command* DebugConsole::FindCommand(const wstring& name)
{
	// do a case insensitive lookup for this command
	wstring nameLower( name );
	transform( nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower );

	for (list<Command*>::iterator it = commandList.begin(); it != commandList.end(); ++it) 
	{
		Command& command = **it;

		wstring commandNameLower( command.name );
		transform( commandNameLower.begin(), commandNameLower.end(), commandNameLower.begin(), ::tolower );
		if (nameLower == commandNameLower)
			return &command;
	}
	return NULL;
}

void DebugConsole::AddCommand(Command& command) 
{ 
	ASSERT(!FindCommand(command.name));
	commandList.push_back(&command); 

	// keep commands sorted by name
	struct SortWrapper
	{
		static bool CommandListCompare(DebugConsole::Command* first, DebugConsole::Command* second) { return (first->name < second->name); }
	};
	commandList.sort(SortWrapper::CommandListCompare);
}

void DebugConsole::RemoveCommand(Command& command)
{
	for (list<Command*>::iterator it = commandList.begin(); it != commandList.end(); ++it) 
	{
		Command& testCommand = **it;
		if (testCommand.name == command.name)
		{
			commandList.erase(it);
			return;
		}
	}
}

wstring DebugConsole::Command::GetValueString() const
{
	wstringstream output;
	switch (type)
	{
		case Type_bool:
		{
			output << name << L" = " << *(bool*)value;
			break;
		}
		case Type_uint8:
		{
			output << name << L" = " << *(uint8*)value;
			break;
		}
		case Type_int8:
		{
			output << name << L" = " << *(int8*)value;
			break;
		}
		case Type_int16:
		{
			output << name << L" = " << *(int16*)value;
			break;
		}
		case Type_uint16:
		{
			output << name << L" = " << *(uint16*)value;
			break;
		}
		case Type_int32:
		{
			output << name << L" = " << *(int32*)value;
			break;
		}
		case Type_uint32:
		{
			output << name << L" = " << *(uint32*)value;
			break;
		}
		case Type_float:
		{
			output << name << L" = " << *(float*)value;
			break;
		}
		case Type_double:
		{
			output << name << L" = " << *(double*)value;
			break;
		}
		case Type_intvector2:
		{
			IntVector2& vector = *(IntVector2*)value;
			output << name << L" = (" << vector.x << ", " << vector.y << ")";
			break;
		}
		case Type_vector2:
		{
			Vector2& vector = *(Vector2*)value;
			output << name << L" = (" << vector.x << ", " << vector.y << ")";
			break;
		}
		case Type_vector3:
		{
			Vector3& vector = *(Vector3*)value;
			output << name << L" = (" << vector.x << ", " << vector.y << ", "<< vector.z << ")";
			break;
		}
		case Type_color:
		{
			Color& color = *(Color*)value;
			output << name << L" = (" << color.r << ", " << color.g << ", " << color.b << ", " << color.a << ")";
			break;
		}
		case Type_string:
		{
			char* string = (char*)value;
			output << name << L" = \"" << string << L"\"";
			break;
		}
		case Type_wstring:
		{
			WCHAR* wstring = (WCHAR*)value;
			output << name << L" = \"" << wstring << L"\"";
			break;
		}
		case Type_function:
		{
			output << name;
			output << " ";
			break;
		}
		default:
		{
			output << name << L" = (N/A)";
			break;
		}
	}
	return output.str();
}

void DebugConsole::Command::SkipWhitespace(wstringstream& inputStream)
{
	// skip characters that can be ignored by the debug console
	while(true)
	{
		const WCHAR c = inputStream.peek();
		if (c == L' ' || c == '=' || c == '(' || c == ',' || c == '\t')
			inputStream.get();
		else
			break;
	}
}

void DebugConsole::Command::Run(wstringstream& inputStream)
{
	if (type != Type_function)
		SkipWhitespace(inputStream);

	bool error = false;
	wstringstream output;
	switch (type)
	{
		case Type_bool:
		{
			inputStream >> *(bool*)value;
			break;
		}
		case Type_uint8:
		{
			uint16 temp = *(uint8*)value;
			inputStream >> temp;
			*(uint8*)value = (uint8)temp;
			break;
		}
		case Type_int8:
		{
			int16 temp = *(int8*)value;
			inputStream >> temp;
			*(int8*)value = (int8)temp;
			break;
		}
		case Type_int16:
		{
			inputStream >> *(int16*)value;
			break;
		}
		case Type_uint16:
		{
			inputStream >> *(uint16*)value;
			break;
		}
		case Type_int32:
		{
			inputStream >> *(int32*)value;
			break;
		}
		case Type_uint32:
		{
			inputStream >> *(uint32*)value;
			break;
		}
		case Type_float:
		{
			inputStream >> *(float*)value;
			break;
		}
		case Type_double:
		{
			inputStream >> *(double*)value;
			break;
		}
		case Type_intvector2:
		{
			IntVector2& vector = *(IntVector2*)value;
			inputStream >> vector.x;
			SkipWhitespace(inputStream);
			inputStream >> vector.y;
			break;
		}
		case Type_vector2:
		{
			Vector2& vector = *(Vector2*)value;
			inputStream >> vector.x;
			SkipWhitespace(inputStream);
			inputStream >> vector.y;
			break;
		}
		case Type_vector3:
		{
			Vector3& vector = *(Vector3*)value;
			inputStream >> vector.x;
			SkipWhitespace(inputStream);
			inputStream >> vector.y;
			SkipWhitespace(inputStream);
			inputStream >> vector.z;
			break;
		}
		case Type_color:
		{
			Color& color = *(Color*)value;
			inputStream >> color.r;
			SkipWhitespace(inputStream);
			inputStream >> color.g;
			SkipWhitespace(inputStream);
			inputStream >> color.b;
			SkipWhitespace(inputStream);
			inputStream >> color.a;
			break;
		}
		case Type_string:
		case Type_wstring:
		{
			wstring buffer;
			inputStream >> buffer;
			int startPos = buffer.find('\"');
			if (startPos >= 0 && buffer.length() > 1)
			{
				int quotePos = buffer.find('\"', startPos+1);
				if (quotePos > 0)
					buffer[quotePos] = '\0';

				++startPos;
			}
			else
				startPos = 0;

			if (type == Type_wstring)
			{
				WCHAR* string = (WCHAR*)value;
				wcsncpy_s(string, 256, &buffer[startPos], _TRUNCATE );
			}
			else
			{
				char* string = (char*)value;
				wcstombs_s(NULL, string, 256, &buffer[startPos], 256);
			}
			break;
		}
		case Type_function:
		{
			wstring buffer;
			inputStream >> buffer;
			((Command::CallbackFunction)(value))(buffer);
			break;
		}
		default:
		{
			error = true;
			break;
		}
	}

	if (type != Type_function)
		output << GetValueString();

	if (error)
		GetDebugConsole().AddError(output.str());
	else if (type != Type_function)
		GetDebugConsole().AddLine(output.str(), CONSOLE_COLOR_SUCCESS);
}

void DebugConsole::ProcessInput()
{
	UINT keyCheck = 0;
	while (ReadNextKey(keyCheck))
	{
		if (keyCheck >= VK_F1 && keyCheck <= VK_F12 || keyCheck == VK_ESCAPE)
			continue; // skip special keys

		if (!isOpen)
			continue;	// only need to process input when open

		if (keyCheck == VK_RETURN)
		{
			if (!inputLine.empty())// && (lineMemory.empty() || *lineMemory.begin() != inputLine))
				lineMemory.push_back(inputLine);
			lineMemoryPos = lineMemory.size();
			findMatches.clear();
			AddLine(inputLine, CONSOLE_COLOR_INPUT);
			ParseInputLine(inputLine);
			inputLine.clear();
			continue;
		}
		else if (keyCheck == VK_DELETE || keyCheck == VK_BACK)
		{
			// delete the last char
			if (!inputLine.empty())
				inputLine.erase(--inputLine.end());
			continue;
		}
		else if (keyCheck == VK_TAB)
		{
			findMatches.clear();

			wstring inputLower( inputLine );
			transform( inputLower.begin(), inputLower.end(), inputLower.begin(), ::tolower );

			for (list<Command*>::iterator it = commandList.begin(); it != commandList.end(); ++it) 
			{
				Command& command = **it;

				wstring commandNameLower( command.name );
				transform( commandNameLower.begin(), commandNameLower.end(), commandNameLower.begin(), ::tolower );

				if (commandNameLower.find(inputLower) != string::npos)
				{
					// input matches this command, add to the match list
					findMatches.push_back(&command);
				}
			}

			lineMemoryPos = 0;
			if (!findMatches.empty())
			{
				// set first match to input
				inputLine = (*findMatches.begin())->GetValueString();
			}

			continue;
		}
		else if (keyCheck == VK_DOWN)
		{
			
			if (!findMatches.empty())
			{
				// go to previous line
				if (lineMemoryPos < findMatches.size() - 1)
					inputLine = findMatches[++lineMemoryPos]->GetValueString();
				else
					inputLine = findMatches[lineMemoryPos = 0]->GetValueString();
			}
			else if (!lineMemory.empty())
			{
				// go to previous line
				if (lineMemoryPos < lineMemory.size() - 1)
					inputLine = lineMemory[++lineMemoryPos];
				else
					inputLine = lineMemory[lineMemoryPos = 0];
			}
			continue;
		}
		else  if (keyCheck == VK_UP)
		{
			if (!findMatches.empty())
			{
				// go to next line
				if (lineMemoryPos > 0)
					inputLine = findMatches[--lineMemoryPos]->GetValueString();
				else
					inputLine = findMatches[lineMemoryPos = findMatches.size() - 1]->GetValueString();
			}
			else if (!lineMemory.empty())
			{
				// go to next line
				if (lineMemoryPos > 0)
					inputLine = lineMemory[--lineMemoryPos];
				else
					inputLine = lineMemory[lineMemoryPos = lineMemory.size() - 1];
			}
			continue;
		}

		UINT key = 0;

		if 
		(
			keyCheck >= 'A' && keyCheck <= 'Z' || 
			keyCheck >= 'a' && keyCheck <= 'z' ||
			keyCheck >= '0' && keyCheck <= '9' ||
			keyCheck == ' '
		)
		{
			// normal ascii key
			key = keyCheck;
		}

		switch(keyCheck)
		{
			// do windows virtual keys stuff
			case VK_OEM_MINUS:	key = '-'; break;
			case VK_OEM_PLUS:	key = '='; break;
			case VK_OEM_4:		key = '['; break;
			case VK_OEM_6:		key = ']'; break;
			case VK_OEM_1:		key = ';'; break;
			case VK_OEM_COMMA:	key = ','; break;
			case VK_OEM_PERIOD:	key = '.'; break;
			case VK_OEM_2:		key = '/'; break;
			case VK_SPACE:		key = ' '; break;
			case VK_OEM_5:		key = '\\'; break;
			case VK_OEM_7:		key = '\''; break;
		}

		if (!(GetKeyState(VK_CAPITAL) & 0x0001) && key >= 'A' && key <= 'Z')
		{
			// handle caps lock
			key += 'a' - 'A';
		}

		if ((GetKeyState(VK_SHIFT) & ~0x0001))
		{
			// handle shift
			if (key >= 'A' && key <= 'Z')
				key += 'a' - 'A';
			else if (key >= 'a' && key <= 'z')
				key -= 'a' - 'A';
			else
			{
				switch(key)
				{
					// convert key to shifted version
					case '1': key = '!'; break;
					case '2': key = '@'; break;
					case '3': key = '#'; break;
					case '4': key = '$'; break;
					case '5': key = '%'; break;
					case '6': key = '^'; break;
					case '7': key = '&'; break;
					case '8': key = '*'; break;
					case '9': key = '('; break;
					case '0': key = ')'; break;	
					case '-': key = '_'; break;
					case '=': key = '+'; break;
					case '[': key = '{'; break;
					case ']': key = '}'; break;
					case ';': key = ':'; break;
					case ',': key = '<'; break;
					case '.': key = '>'; break;
					case '/': key = '?'; break;
					case '\\': key = '|'; break;
					case '\'': key = '"'; break;
				}
			}
		}

		if (key)
		{
			// add key to end of line if it was valid
			inputLine += key;
		}
	}
}

void DebugConsole::Render()
{
	if (openPercent == 0)
		return;

	const float minY = 0;
	float maxY = 200;
	if (maxY > g_backBufferHeight)
		maxY = (float)g_backBufferHeight;
	
	float posY = Lerp(openPercent, minY, maxY) - (float)g_textHelper->GetLineHeight();

	{
		// draw background
		const float w = (float)g_backBufferWidth;
		const float h = (float)g_backBufferHeight;
		const float l = (float)g_textHelper->GetLineHeight();
		const Vector2 pos(w/2, posY - h/4 + l + 4);
		const Vector2 size(w/2, h/4);
		DXUTGetD3D9Device()->SetRenderState(D3DRS_LIGHTING, TRUE);
		g_render->RenderScreenSpaceQuad(pos, size, CONSOLE_COLOR_BACK);

		Vector2 bottomPos = pos + Vector2(0, size.y+2);
		Vector2 bottomSize(size.x, 2);
		g_render->RenderScreenSpaceQuad(bottomPos, bottomSize, CONSOLE_COLOR_BORDER);
	}
	
	{
		// print the text
		g_textHelper->Begin();
		g_textHelper->SetInsertionPos( 20, (int)posY );

		// draw the input line
		wstring output = inputLine;
		if ((int)(3*GamePauseTimer::GetTimeGlobal()) % 2)
		{
			// add the blinking cursor
			output += '_';
		}
		g_textHelper->SetForegroundColor( CONSOLE_COLOR_INPUT );
		g_textHelper->DrawTextLine( output.c_str(), true );

		// print all the lines in the text buffer
		for (vector<ConsoleLine>::iterator it = lines.begin(); it != lines.end(); ++it) 
		{
			const ConsoleLine& line = *it;
			g_textHelper->SetForegroundColor( line.color );
			g_textHelper->DrawTextLine( line.text.c_str(), true );
		}
	
		g_textHelper->End();
	}

	if (!findMatches.empty())
	{
		//vector<wstring>& showLines = findMatches.empty()? lineMemory : findMatches;

		// show the found matches
		posY += (float)g_textHelper->GetLineHeight() + 4;

		if (findMatches.size() > 1)
		{
			// draw background
			const float w = 500;
			const float h = (findMatches.size()-1) * (float)g_textHelper->GetLineHeight();
			const Vector2 pos(30 + w/2, posY + h/2 + 4);
			const Vector2 size(w/2 + 6, h/2 + 4);
			DXUTGetD3D9Device()->SetRenderState(D3DRS_LIGHTING, TRUE);
			g_render->RenderScreenSpaceQuad(pos, size, CONSOLE_COLOR_BACK);
		}

		{
			g_textHelper->Begin();
			g_textHelper->SetForegroundColor( Color::Grey(1.0f, 0.8f) );
			g_textHelper->SetInsertionPos( 30, (int)posY );

			for (unsigned int i = lineMemoryPos+1; i < findMatches.size(); ++i) 
			{
				const wstring& text = findMatches[i]->GetValueString();
				g_textHelper->DrawTextLine( text.c_str(), false );
			}

			for (unsigned int i = 0; i < lineMemoryPos; ++i) 
			{
				const wstring& text = findMatches[i]->GetValueString();
				g_textHelper->DrawTextLine( text.c_str(), false );
			}

			g_textHelper->End();
		}
	}
}

void DebugConsole::AddLine(const wstring& text, const Color& color)
{
	lines.emplace(lines.begin(), ConsoleLine(text, color));
}

void DebugConsole::AddLine(const wstring& text)
{
	AddLine(text, CONSOLE_COLOR_NORMAL);
}

void DebugConsole::AddError(const wstring& text)
{
	AddLine(text, CONSOLE_COLOR_ERROR);
}

void DebugConsole::AddFormatted(const wstring messageFormat, ...)
{
	// fill in the message
	va_list args;
	va_start(args, messageFormat);
	WCHAR output[256];
	StringCchVPrintf( output, 256, messageFormat.c_str(), args );
	output[255] = L'\0';
	va_end(args);

	AddLine(output, CONSOLE_COLOR_NORMAL);
}

void DebugConsole::OnKeyboard(UINT nChar)	
{ 
	if (enable && nChar == VK_OEM_3)
	{
		// ~ key toggles console 
		isOpen = !isOpen;
		findMatches.clear();
		lineMemoryPos = 0;
		g_editorGui.ClearFocus();
		return;
	}

	if (g_editorGui.IsEditBoxFocused())
		return;

	if (keyBuffer.size() < 256) 
		keyBuffer.push(nChar); 
}

void DebugConsole::ParseFile(const wstring& filename, bool showFileNotFound)
{		
	ifstream inFile(filename);
	
	if (inFile.fail())
	{
		if (showFileNotFound)
			AddLine(wstring(L"File \"") + filename + wstring(L"\" not found"), CONSOLE_COLOR_ERROR);
		return;
	}
	
	AddLine(wstring(L"Parsing file \"") + filename + wstring(L"\""));

	while (!inFile.eof())
	{
		char buffer[1024];
		inFile.getline(buffer, 1024);
		
		string line(buffer);
		std::wstring wline;
		wline.assign(line.begin(), line.end());
	
		static const WCHAR commentChar =  L'#';
		if (wline.size() > 0 && wline[0] != commentChar)
		{
			AddLine(wline);
			ParseInputLine(wline);
		}
	}
	
	inFile.close();
	AddLine(wstring(L"Finished parsing file \"") + filename + wstring(L"\""));
	AddLine();
}

///////////////////////////////////////////////////////////////////////////////////////////

// show some help
static void ConsoleCallback_help(const wstring& text)
{
	GetDebugConsole().AddLine();
	GetDebugConsole().AddLine(L"Press ~ anytime to bring up the debug console.");
	GetDebugConsole().AddLine(L"While the console is active it will block all keyboard input to the game.");
	GetDebugConsole().AddLine(L"Use up and down to cycle through previous commands.");
	GetDebugConsole().AddLine(L"To search for commands just type a sub string and press tab.");
	GetDebugConsole().AddLine(L"When searching you can press up and down to cycle through matches.");
	GetDebugConsole().AddLine(L"You can also just press tab on a blank line and cycle through all commands.");
	GetDebugConsole().AddLine();
}
ConsoleCommand(ConsoleCallback_help, help);

// list all commands
static void ConsoleCommandCallback_list(const wstring& text)
{
	GetDebugConsole().AddLine();
	for (list<DebugConsole::Command*>::iterator it = GetDebugConsole().commandList.begin(); it != GetDebugConsole().commandList.end(); ++it) 
		GetDebugConsole().AddLine((**it).name);
	GetDebugConsole().AddLine();
}
ConsoleCommand(ConsoleCommandCallback_list, list);

// clear console
static void ConsoleCommandCallback_clear(const wstring& text)
{
	GetDebugConsole().Clear();
}
ConsoleCommand(ConsoleCommandCallback_clear, clear);

// parse a file
static void ConsoleCommandCallback_parseFile(const wstring& text)
{
	GetDebugConsole().ParseFile(text.c_str());
}
ConsoleCommand(ConsoleCommandCallback_parseFile, parseFile);

///////////////////////////////////////////////////////////////////////////////////////////

static bool startSkiGame = false;
static void ConsoleCommandCallback_startSkiGame(const wstring& text) { startSkiGame = true; }
ConsoleCommand(ConsoleCommandCallback_startSkiGame, ski);

void DebugConsole::UpdateSkiGame()
{
	static bool skiGameActive = false;
	static bool skiGameWaitToStart;
	static int wallPos, gapSize, direction, lineCount, playerPos, sizeDirection;
	static queue<pair<int, int>> wallsQueue;
	const int width = 180;
	const wstring playerString[3] = { L"/°/", L"l°l", L"\\°\\" };

	if (startSkiGame)
	{
		skiGameActive = true;
		startSkiGame = false;
		skiGameWaitToStart = true;
		wallPos = 20;
		direction = 1;
		sizeDirection = -1;
		gapSize = 150;
		lineCount = 0;
		playerPos = wallPos + gapSize/2;
		while (!wallsQueue.empty()) wallsQueue.pop();
		AddLine(L"*ASPEN GOLD 2.0* - Use left/right keys to move and start skiing!");
	}
	if (!skiGameActive)
		return;
	else if (skiGameWaitToStart)
	{
		if (!g_input->IsKeyDown(VK_LEFT) && !g_input->IsKeyDown(VK_RIGHT) && !g_input->IsKeyDown(VK_SPACE))
			return;
		skiGameWaitToStart = false;
	}

	++lineCount;
	if ((lineCount % 10 == 0 || lineCount < 50) && gapSize > 1)
	{
		if (gapSize < 30 && sizeDirection < 0 && RAND_DIE(gapSize - 15) || gapSize > 30 && sizeDirection > 0 && RAND_DIE(50 - gapSize))
			sizeDirection *= -1;
		if (RAND_DIE(2) && wallPos > 0)
			wallPos -= sizeDirection;
		gapSize += sizeDirection;
	}

	wstringstream buffer;
	int cursorPos = 0;
	for (int i = 0; i < wallPos-2; ++i, ++cursorPos)
		buffer << (RAND_DIE(200)? L"^" : L"·");
	buffer << L"O";
	for (int i = 0; i < gapSize+1; ++i, ++cursorPos)
		buffer << L" ";
	buffer << L"O";
	if (lineCount % 50 <= 5)
		buffer << L" " << lineCount << L" "; 

	while (buffer.str().length() <= unsigned(width+10))
		buffer << (RAND_DIE(200)? L"^" : L"·");
	buffer << L"\n";
	AddLine(buffer.str());
		
	{
		// update player
		wstring& stringBuffer1 = lines[Min(6, int(lines.size())-1)].text;
		while (stringBuffer1.length() <= unsigned(width))
			stringBuffer1 += L" ";
		for(unsigned i = 0; i < playerString[0].length(); ++i)
			stringBuffer1[playerPos-1+i] = ' '; // clear old player

		const int moveDirection = (g_input->IsKeyDown(VK_LEFT)? -1 : 0) + (g_input->IsKeyDown(VK_RIGHT)? 1 : 0);
		playerPos = Cap(playerPos+moveDirection, 1, width-1);
		wstring& stringBuffer2 = lines[Min(5, int(lines.size())-1)].text;
		while (stringBuffer2.length() <= unsigned(width))
			stringBuffer2 += L" ";
		for(unsigned i = 0; i < playerString[0].length(); ++i)
			stringBuffer2[playerPos-1+i] = playerString[1+moveDirection][i]; // draw new player
	}

	wallPos += direction;
	if (wallsQueue.size() > 6)
		wallsQueue.pop();
	wallsQueue.push(pair<int, int>(wallPos, wallPos + gapSize));

	if (RAND_DIE(200) || lineCount < 20)
		direction = 0;
	else if (RAND_DIE(50))
		direction = RAND_SIGN;
	if (wallPos + gapSize > width && direction > 0 || wallPos < 0 && direction < 0)
		direction *= -1;
	
	pair<int, int> walls = wallsQueue.front();
	if (playerPos + 1 < walls.first || playerPos + 1 > walls.second)
	{
		skiGameActive = false;
		AddFormatted(L"Game over! Score: %d meters", lineCount-5);
	}

	Sleep(10);
}
