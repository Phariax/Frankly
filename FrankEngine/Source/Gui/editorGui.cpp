////////////////////////////////////////////////////////////////////////////////////////
/*
	Editor GUI
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../terrain/terrain.h"
#include "../editor/tileEditor.h"
#include "../editor/objectEditor.h"
#include "../gui/editorGui.h"

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Editor GUI Globals
*/
////////////////////////////////////////////////////////////////////////////////////////

float EditorGui::quickPickSize = 150;
int EditorGui::quickPickTileColumns = 16;
int EditorGui::quickPickObjectColumns = 8;

static const D3DCOLOR buttonToggleOffColor = D3DCOLOR_ARGB( 255, 0, 0, 0);
static const D3DCOLOR buttonToggleOnColor = D3DCOLOR_ARGB( 255, 255, 255, 255);

// manager for shared resources of dialogs
extern CDXUTDialogResourceManager g_dialogResourceManager; 

static void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );

static bool showHelp = true;
ConsoleCommand(showHelp, showHelpPopups);

enum ControlId
{
	ControlId_invalid = -1,
		
	ControlId_gameMode,
	ControlId_button_layer1,
	ControlId_button_layer2,
	ControlId_button_layer3,
	ControlId_help,
	ControlId_static_playerPos,

	ControlId_startControls,
		ControlId_button_editorSave,
		ControlId_button_editorLoad,
		ControlId_button_editorAutoSave,
		ControlId_button_editorShowGrid,
		ControlId_button_editorSnapToGrid,
		ControlId_button_editorBlockEdit,
		ControlId_button_randomizeTerrain,
		ControlId_button_editorHelp,
	ControlId_endControls,
		
	ControlId_startHiddenControls,
		ControlId_button_editorClear,
		ControlId_button_fullscreen,
	ControlId_endHiddenControls,

	ControlId_start_tileEdit,
		ControlId_static_surfaceName,
		ControlId_static_surfaceTexture,
		ControlId_static_tileSetName,
		ControlId_button_tileSetUp,
		ControlId_button_tileSetDown,
	ControlId_end_tileEdit,

	ControlId_start_objectEdit,
		ControlId_static_objectEditName,
		ControlId_static_objectEditTexture,
		ControlId_static_objectEditTextureSmall,
	ControlId_end_objectEdit,
		
	ControlId_start_objectEditSelected,
		ControlId_static_objectEdit_type,
		ControlId_static_objectEdit_handle,
		ControlId_static_objectEdit_xf,
		ControlId_edit_objectEdit_attributesBox,
		ControlId_static_objectEdit_objectInfoDisplay,
		ControlId_static_objectEdit_objectAttributesDisplay,
	ControlId_end_objectEditSelected,
};


////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Editor GUI Member Functions
*/
////////////////////////////////////////////////////////////////////////////////////////


void EditorGui::Init()
{
    mainDialog.Init( &g_dialogResourceManager );
    mainDialog.SetCallback( OnGUIEvent );
    mainDialog.SetFont( 0, L"Arial", 26, FW_NORMAL );
    mainDialog.SetFont( 1, L"Arial Bold", 32, FW_NORMAL );
    mainDialog.SetFont( 2, L"Arial Bold", 82, FW_NORMAL );

	{	
		mainDialog.AddStatic( ControlId_gameMode, L"Layer", 0, 0, 0, 0 );
		mainDialog.GetStatic( ControlId_gameMode )->SetFont( 1 );
		mainDialog.GetControl( ControlId_gameMode )->SetTextColor(D3DCOLOR_ARGB( 255, 255, 255, 255 ));
		mainDialog.GetControl( ControlId_gameMode )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_CENTER | DT_NOCLIP;
	}

	// main controls
	mainDialog.AddButton( ControlId_button_editorAutoSave, L"Auto Save", 0, 0, 0, 0 ); 
	mainDialog.AddButton( ControlId_button_editorLoad, L"Load", 0, 0, 0, 0 ); 
	mainDialog.AddButton( ControlId_button_editorSave, L"Save", 0, 0, 0, 0 ); 
	mainDialog.AddButton( ControlId_button_editorHelp, L"Help", 0, 0, 0, 0 ); 
	mainDialog.AddButton( ControlId_button_editorShowGrid, L"Grid", 0, 0, 0, 0 ); 
	mainDialog.AddButton( ControlId_button_editorSnapToGrid, L"Snap To Grid", 0, 0, 0, 0 ); 
	mainDialog.AddButton( ControlId_button_editorBlockEdit, L"Block Edit", 0, 0, 0, 0); 
	mainDialog.AddButton( ControlId_button_fullscreen, L"Full Screen", 0, 0, 0, 0 ); 
	mainDialog.AddButton( ControlId_button_randomizeTerrain, L"Randomize", 0, 0, 0, 0 ); 
	mainDialog.AddButton( ControlId_button_editorClear, L"Clear", 0, 0, 0, 0 ); 
	mainDialog.AddButton( ControlId_button_layer1, L"1", 0, 0, 0, 0 ); 
	mainDialog.GetButton( ControlId_button_layer1 )->SetFont( 2 );
	mainDialog.AddButton( ControlId_button_layer2, L"2", 0, 0, 0, 0 ); 
	mainDialog.GetButton( ControlId_button_layer2 )->SetFont( 2 );
	mainDialog.AddButton( ControlId_button_layer3, L"3", 0, 0, 0, 0 ); 
	mainDialog.GetButton( ControlId_button_layer3 )->SetFont( 2 );
	mainDialog.AddButton( ControlId_button_tileSetDown, L"\x25BC", 0, 0, 0, 0 ); 
	mainDialog.AddButton( ControlId_button_tileSetUp, L"\x25B2", 0, 0, 0, 0 ); 

	// help
	mainDialog.AddStatic( ControlId_help, L"", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_help )->SetVisible(false);
	mainDialog.GetControl( ControlId_help )->SetTextColor(D3DCOLOR_ARGB( 255, 255, 255, 255 ));
    mainDialog.GetControl( ControlId_help )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_CENTER | DT_NOCLIP;

	// level edit info
	mainDialog.AddStatic( ControlId_static_surfaceName, L"surface name", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_surfaceName )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_LEFT | DT_NOCLIP;
	mainDialog.AddStatic( ControlId_static_tileSetName, L"tile set name", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_tileSetName )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_LEFT | DT_NOCLIP;
	mainDialog.AddStatic( ControlId_static_surfaceTexture, L"", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_surfaceTexture )->SetVisible(false);

	// object edit info
	mainDialog.AddStatic( ControlId_static_objectEdit_objectInfoDisplay, L"Object Info", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_objectEdit_objectInfoDisplay )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_LEFT;
	mainDialog.AddStatic( ControlId_static_objectEdit_objectAttributesDisplay, L"Object Attributes", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_objectEdit_objectAttributesDisplay )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_LEFT;
	mainDialog.AddStatic( ControlId_static_objectEditName, L"name", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_objectEditName )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_LEFT | DT_NOCLIP;
	mainDialog.AddStatic( ControlId_static_objectEdit_handle, L"handle", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_objectEdit_handle )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_LEFT | DT_NOCLIP;
	mainDialog.AddStatic( ControlId_static_objectEdit_type, L"type", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_objectEdit_type )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_LEFT | DT_NOCLIP;
	mainDialog.AddStatic( ControlId_static_objectEdit_xf, L"xform", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_objectEdit_xf )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_LEFT | DT_NOCLIP;
	mainDialog.AddEditBox( ControlId_edit_objectEdit_attributesBox, L"", 0, 0, 600, 30 );
	mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->SetBorderWidth(1);
	mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->SetSpacing(1);
	mainDialog.AddStatic( ControlId_static_objectEditTexture, L"", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_objectEditTexture )->SetVisible(false);
	mainDialog.AddStatic( ControlId_static_objectEditTextureSmall, L"", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_objectEditTextureSmall )->SetVisible(false);
	mainDialog.AddStatic( ControlId_static_playerPos, L"", 0, 0, 0, 0 );
	mainDialog.GetControl( ControlId_static_playerPos )->GetElement( 0 )->dwTextFormat = DT_TOP | DT_LEFT | DT_NOCLIP;

	EditorGui::OnResetDevice();
}

const WCHAR* EditorGui::GetHoverHelp()
{
	CDXUTControl* hoverControl = mainDialog.GetMouseOver();
	if (hoverControl)
	{
		const ControlId id = (ControlId)(hoverControl->GetID());
		switch (id)
		{
			case ControlId_button_editorAutoSave:
				return L"Toggle if auto saving is enabled.)";
			case ControlId_button_editorLoad:
				return L"Load terrain file from disk. (Ctrl+L)";
			case ControlId_button_editorSave:
				return L"Save terrain file to disk. (Ctrl+S)";
			case ControlId_button_editorShowGrid:
				return L"Toggle showing the grid overlay. (G)";
			case ControlId_button_editorSnapToGrid:
				return L"Toggle rounding position, size and angles. (G)";
			case ControlId_button_editorBlockEdit:
				return L"Toggle editing block style. (B)";
			case ControlId_button_editorHelp:
				return L"Open help information in browser.";
			case ControlId_button_layer1:
				return L"Select background tiles layer. (1)";
			case ControlId_button_layer2:
				return L"Select foreground tiles layer. (2)";
			case ControlId_button_layer3:
				return L"Select objects layer. (3)";
			case ControlId_edit_objectEdit_attributesBox:
				return L"Change object attributes.";
			case ControlId_button_editorClear:
				return L"Clear all tile and object data.";
		}
	}
	
	if (g_editor.IsObjectEdit() && GetObjectQuickPickId() >= 0)
		return L"Quick pick object type.";
	else if (g_editor.IsTileEdit() && GetTileQuickPickId() >= 0)
		return L"Quick pick tile type.";
	
	const Vector2 mousePos = g_input->GetMousePosScreenSpace();

	{
		int x, y, sx, sy;
		if (g_editor.IsObjectEdit() && g_editor.GetObjectEditor().HasOnlyOneSelected())
		{
			mainDialog.GetControl( ControlId_static_objectEditTextureSmall )->GetLocation(x, y);
			mainDialog.GetControl( ControlId_static_objectEditTextureSmall )->GetSize(sx, sy);
		}
		else
		{
			mainDialog.GetControl( ControlId_static_objectEditTexture )->GetLocation(x, y);
			mainDialog.GetControl( ControlId_static_objectEditTexture )->GetSize(sx, sy);
		}

		Box2AABB previewBox(IntVector2(x,y) - IntVector2(sx, sy), IntVector2(x,y) + IntVector2(sx, sy));
		if (previewBox.Contains(mousePos))
			return L"Displays preview of tile or object.";
	}

	{
		Box2AABB positionBox(Vector2(243, 17)-Vector2(244, 18), Vector2(243, 17)+Vector2(244, 18));
		if (positionBox.Contains(mousePos))
			return L"Displays current mouse position.";
	}

	if (g_debugMessageSystem.IsMouseOver())
		return L"Show output log window.";

	return L"";
}


void EditorGui::Refresh()
{
	WCHAR output[256];

	static bool wasEditBoxFocused = false;
	if (wasEditBoxFocused && !IsEditBoxFocused())
	{
		// save state when use clicks off edit box
		g_editor.SaveState();
	}
	wasEditBoxFocused = IsEditBoxFocused();

	if (g_gameControlBase->IsGameplayMode())
		mainDialog.GetStatic( ControlId_gameMode )->SetVisible(false);
	else
	{
		LPCWSTR modeName;
		if (g_editor.IsObjectEdit())
			modeName = L"Object Stubs";
		else
		{
			const int layer = g_editor.GetTileEditor().GetLayer();
			if (layer == 0)
				modeName = L"Foreground Tiles";
			else if (layer == 1)
				modeName = L"Background Tiles";
			else
			{
				static WCHAR buffer[64];
				swprintf_s( buffer, 64, L"Tile Layer %d", g_editor.GetTileEditor().GetLayer() );
				modeName = buffer;
			}
		}

		mainDialog.GetStatic( ControlId_gameMode )->SetText(modeName);
		mainDialog.GetStatic( ControlId_gameMode )->SetVisible(true);
	}
	
	mainDialog.GetButton( ControlId_button_editorAutoSave )->SetTextColor(GameControlBase::autoSaveTerrain? Color::White() : Color::Black());

	{
		// sloppy way of making the attributes brighter when it has focus to remind the user
		const D3DCOLOR editBoxColor = IsEditBoxFocused()? D3DCOLOR_ARGB( 255, 255, 255, 255 ) : D3DCOLOR_ARGB( 255, 170, 170, 170 );
		DXUTBlendColor blendColor;
		blendColor.Init(editBoxColor, editBoxColor, editBoxColor);
		DXUTBlendColor test = mainDialog.GetControl( ControlId_edit_objectEdit_attributesBox )->GetElement( 0 )->TextureColor = blendColor;
	}

	{
		// set up help
		if (showHelp)
		{
			// set up help
			const WCHAR* hoverHelp = GetHoverHelp();
			if (hoverHelp && hoverHelp[0] != 0)
			{
				mainDialog.GetStatic( ControlId_help )->SetText(hoverHelp);
				mainDialog.GetStatic( ControlId_help )->SetVisible(true);
				//Vector2 pos = g_input->GetMousePosScreenSpace();
				//pos.y -= 8;
				//const float a = Percent(float(hoverHelpTimer), 1.0f, 1.5f);
				//g_debugRender.RenderTextRaw(pos, hoverHelp, Color::White(a));
			}
			else
				mainDialog.GetStatic( ControlId_help )->SetVisible(false);
		}
		else
			mainDialog.GetStatic( ControlId_help )->SetVisible(false);
	}
	/*if (showHelp)
	{
		if (g_gameControlBase->IsObjectEditMode())
		{
			mainDialog.GetStatic( ControlId_help )->SetText(
				L"Mouse Left : Move Object\n"
				L" +Shift : Rotate Object\n"
				L" +Ctrl : Scale Object\n"
				L"Mouse Right : Rectangle Select\n"
				L" +Alt : Multi-Layer Select\n"
				L" +Shift : Add to Selection\n"
				L" +Ctrl : Remove from Selection\n"
				L"Mouse Middle : Move View\n"
				L"1,2,3 : Select Layer\n"
				L"Z, X : Add/Remove Object\n"
				L"Ctrl + X, C, V : Cut, Copy, Paste\n"
				L"Ctrl + Z, Y : Undo, Redo\n"
			);
		}
		else if (g_gameControlBase->IsTileEditMode())
		{
			mainDialog.GetStatic( ControlId_help )->SetText(
				L"Mouse Left : Free Draw\n"
				L" +Shift : Draw Line\n"
				L" +Ctrl : Eyedropper\n"
				L" +Alt : Block Draw\n"
				L" +Alt+Ctrl : Block Erase\n"
				L"Mouse Right : Rectangle Select\n"
				L" + Alt : Multi-Layer Select\n"
				L"Mouse Middle : Move View\n"
				L"1,2,3 : Select Layer\n"
				L"Z, X : Flood Fill/Erase\n"
				L"Ctrl + X, C, V : Cut, Copy, Paste\n"
				L"Ctrl + Z, Y : Undo, Redo\n"
			);
		}
	}*/
	
	mainDialog.GetButton( ControlId_button_fullscreen )->SetTextColor
	(
		!DXUTIsWindowed()? buttonToggleOnColor : buttonToggleOffColor
	);

	mainDialog.GetButton( ControlId_button_editorShowGrid )->SetTextColor
	(
		Editor::showGrid? buttonToggleOnColor : buttonToggleOffColor
	);

	mainDialog.GetButton( ControlId_button_editorSnapToGrid )->SetTextColor
	(
		ObjectEditor::snapToGrid? buttonToggleOnColor : buttonToggleOffColor
	);
	
	mainDialog.GetButton( ControlId_button_editorBlockEdit )->SetTextColor
	(
		TileEditor::blockEdit? buttonToggleOnColor : buttonToggleOffColor
	);

	// update tile info
	if (g_gameControlBase->IsTileEditMode())
	{
		const BYTE surfaceIndex = g_editor.GetTileEditor().GetDrawSurfaceIndex();
		const GameSurfaceInfo& surface = g_editor.GetTileEditor().GetDrawSurfaceInfo();

		swprintf_s(output, 256, L"%s (%d)", surface.name? surface.name : L"N/A", surfaceIndex);
		mainDialog.GetStatic( ControlId_static_surfaceName )->SetText(output);

		swprintf_s(output, 256, L"Tile Set %d", g_editor.GetTileEditor().GetTileSet());
		mainDialog.GetStatic( ControlId_static_tileSetName )->SetText(output);
	}	
	
	{
		// hide terrain edit mode controls when not editing
		const bool visible = g_gameControlBase->IsTileEditMode();
		for (int id = ControlId_start_tileEdit+1; id < ControlId_end_tileEdit; ++id)
			mainDialog.GetControl( id )->SetVisible(visible);

		bool showTileset = (visible && Terrain::tileSetCount > 0);
		{
			mainDialog.GetControl( ControlId_button_tileSetUp )->SetVisible(showTileset);
			mainDialog.GetControl( ControlId_button_tileSetDown )->SetVisible(showTileset);
			mainDialog.GetControl( ControlId_static_tileSetName )->SetVisible(showTileset);
		}
	}
	
	{
		// show the mouse position
		const Vector2 mousePos = g_input->GetMousePosWorldSpace();
		
		IntVector2 tilePos;
		TerrainTile* tile = g_terrain->GetTile(mousePos, tilePos.x, tilePos.y);
		if (tile)
		{
			const Vector2 pos = mousePos;
			const IntVector2 patchPos = tilePos / Terrain::patchSize;
			swprintf_s(output, 256, L"Pos(%.0f, %.0f) Tile(%d, %d) Patch(%d, %d)", pos.x, pos.y, tilePos.x, tilePos.y, patchPos.x, patchPos.y);
		}
		else
		{
			const IntVector2 pos = mousePos;
			swprintf_s(output, 256, L"Pos(%d, %d)", pos.x, pos.y);
		}
		mainDialog.GetStatic( ControlId_static_playerPos )->SetText(output);
	}

	// update object edit display
	if (g_gameControlBase->IsObjectEditMode())
	{
		const ObjectTypeInfo newTypeInfo = g_editor.GetObjectEditor().GetNewStubInfo();
		swprintf_s(output, 256, L"%s (%d)", newTypeInfo.GetName(), newTypeInfo.GetType());
		mainDialog.GetStatic( ControlId_static_objectEditName )->SetText(output);

		if (g_editor.GetObjectEditor().HasOnlyOneSelected())
		{
			GameObjectStub& stub = *g_editor.GetObjectEditor().GetOnlyOneSelectedStub();
		
			swprintf_s(output, 256, L"Type: %s (%d)", stub.GetObjectInfo().GetName(), stub.type);
			mainDialog.GetStatic( ControlId_static_objectEdit_type )->SetText(output);
			
			swprintf_s(output, 256, L"%s", stub.GetObjectInfo().GetDescription());
			mainDialog.GetStatic( ControlId_static_objectEdit_objectInfoDisplay )->SetText(output);
			
			// show attributes via special attributes display method
			//swprintf_s(output, 256, L"%s", stub.GetObjectInfo().GetAttributesDescription());
			//mainDialog.GetStatic( ControlId_static_objectEdit_objectAttributesDisplay )->SetText(output);
			mainDialog.GetStatic( ControlId_static_objectEdit_objectAttributesDisplay )->SetText(L"");

			if (stub.handle)
				swprintf_s(output, 256, L"ID: %d", stub.handle);
			else
				swprintf_s(output, 256, L"ID: n/a");
			mainDialog.GetStatic( ControlId_static_objectEdit_handle )->SetText(output);

			swprintf_s(output, 256, L"XForm: (%.2f, %.2f) %.2f°  Size: (%.2f, %.2f)", stub.xf.position.x, stub.xf.position.y, RadiansToDegrees(stub.xf.angle), stub.size.x, stub.size.y);
			mainDialog.GetStatic( ControlId_static_objectEdit_xf )->SetText(output);
		}
		else
		{
			const ObjectTypeInfo newStubInfo = g_editor.GetObjectEditor().GetNewStubInfo();
		
			swprintf_s(output, 256, L"%s (%d)", newStubInfo.GetName(), g_editor.GetObjectEditor().GetNewStubType());
			mainDialog.GetStatic( ControlId_static_objectEditName )->SetText(output);
			mainDialog.GetStatic( ControlId_static_objectEdit_type )->SetText(L"");
			mainDialog.GetStatic( ControlId_static_objectEdit_objectInfoDisplay )->SetText(L"");
			mainDialog.GetStatic( ControlId_static_objectEdit_objectAttributesDisplay )->SetText(L"");
			mainDialog.GetStatic( ControlId_static_objectEdit_handle )->SetText(L"");
			mainDialog.GetStatic( ControlId_static_objectEdit_xf )->SetText(L"");
			mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->SetText(L"");
		}
	}
	
	const int gap = 4;
	{
		const int rows = GetTileQuickPickRows();
		const float sy2 = quickPickSize / quickPickTileColumns;
		int x = g_backBufferWidth - gap; 
		int y = g_backBufferHeight - int(2*sy2*rows) - 76;
		
		y -= 2;

		{
			// tile set buttons
			const int height = 26;
			const int width = 30;
			x -= width + gap;
			mainDialog.GetControl( ControlId_button_tileSetUp )->SetLocation( x, y );
			mainDialog.GetControl( ControlId_button_tileSetUp )->SetSize( width, height );
			x -= width + gap;
			mainDialog.GetControl( ControlId_button_tileSetDown )->SetLocation( x, y );
			mainDialog.GetControl( ControlId_button_tileSetDown )->SetSize( width, height );
			x -= gap;
		}
			
		x = g_backBufferWidth - gap;
		mainDialog.GetControl( ControlId_static_tileSetName )->SetLocation( x-292, y );
		mainDialog.GetControl( ControlId_static_tileSetName )->SetSize(0, 0);

		// hide some stuff when not in tile set mode
		if (Terrain::tileSetCount > 0)
			y -= 26;

		mainDialog.GetControl( ControlId_static_surfaceName )->SetLocation( x-292, y );
	}
	{
		const int rows = GetObjectQuickPickRows();
		const float sy2 = quickPickSize / quickPickObjectColumns;
		const int x = g_backBufferWidth - gap-292; 
		const int y = g_backBufferHeight - int(2*sy2*rows) - 78;
		mainDialog.GetControl( ControlId_static_objectEditName )->SetLocation( x, y );
		mainDialog.GetControl( ControlId_static_objectEditName )->SetSize(0, 0);
	}

	{
		// hide object edit mode controls when not editing
		const bool visible = g_gameControlBase->IsObjectEditMode();
		for (int id = ControlId_start_objectEdit+1; id < ControlId_end_objectEdit; ++id)
			mainDialog.GetControl( id )->SetVisible(visible);
	}

	{
		// only show object info if object is selected
		const bool visible = g_gameControlBase->IsObjectEditMode() && g_editor.GetObjectEditor().HasOnlyOneSelected();
		for (int id = ControlId_start_objectEditSelected+1; id < ControlId_end_objectEditSelected; ++id)
			mainDialog.GetControl( id )->SetVisible(visible);
	}

	{
		// layer buttons
		const Color onColor = Color::Red();
		const Color offColor = Color::Grey(0.5f);
		int editLayer = 3;
		if (g_editor.IsTileEdit())
			editLayer = g_editor.GetTileEditor().GetLayer() == 0? 2 : 1;
		mainDialog.GetControl( ControlId_button_layer1 )->SetTextColor(editLayer == 1? onColor : offColor);
		mainDialog.GetControl( ControlId_button_layer2 )->SetTextColor(editLayer == 2? onColor : offColor);
		mainDialog.GetControl( ControlId_button_layer3 )->SetTextColor(editLayer == 3? onColor : offColor);
	}
}

int EditorGui::GetTileQuickPickRows() const
{
	return int(ceilf(GameSurfaceInfo::Count() / float(quickPickTileColumns)));
}

int EditorGui::GetObjectQuickPickRows() const
{
	return int(ceilf((ObjectTypeInfo::GetMaxType()+1) / float(quickPickObjectColumns)));
}

void EditorGui::Reset()
{
	ClearFocus();
}

void EditorGui::ClearFocus()
{
	mainDialog.ClearFocus();
}

void EditorGui::NewObjectSelected()
{
	if (g_editor.GetObjectEditor().HasOnlyOneSelected())
	{
		const GameObjectStub& stub = *g_editor.GetObjectEditor().GetOnlyOneSelectedStub();
	
		// Convert to a wchar_t*
		const size_t stringSize = strlen(stub.attributes) + 1;
		const size_t textBoxSize = 256;
		wchar_t wcstring[textBoxSize];
		mbstowcs_s(NULL, wcstring, stringSize, stub.attributes, _TRUNCATE);
		mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->SetText(wcstring);
	}
	else
	{
		mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->SetText(L"");
		
		if (!g_editor.GetObjectEditor().HasSelection())
		{
			if (mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->HasFocus())
				mainDialog.ClearFocus();
		}
	}
}

void EditorGui::OnResetDevice()
{
    mainDialog.SetLocation( 0, 0 );
    mainDialog.SetSize( g_backBufferWidth, g_backBufferHeight );

	int x, y;
	int buttonHeight = 40;
	int buttonWidth = 100;
	int textHeight = 24;
	const int gap = 4;
	{
		// game mode buttons
		const int width = 80;
		const int height = 80;
		x = g_backBufferWidth - gap - width;
		y = gap;
		mainDialog.GetControl( ControlId_button_layer3 )->SetLocation( x, y );
		mainDialog.GetControl( ControlId_button_layer3 )->SetSize( width, height );
		x -= gap + width;
		mainDialog.GetControl( ControlId_button_layer2 )->SetLocation( x, y );
		mainDialog.GetControl( ControlId_button_layer2 )->SetSize( width, height );
		mainDialog.GetControl( ControlId_gameMode )->SetLocation( x+width/2, y + 80 );

		x -= gap + width;
		mainDialog.GetControl( ControlId_button_layer1 )->SetLocation( x, y );
		mainDialog.GetControl( ControlId_button_layer1 )->SetSize( width, height );
	}

	{
		{
			// main control buttons
			const int buttonCount = ControlId_endControls - ControlId_startControls - 1;
			
			Vector2 buttonSize( g_backBufferWidth / float(buttonCount), float(buttonHeight) );
			buttonSize.x = Min(buttonSize.x, 240.0f);

			int x, y;
			y = (int)(g_backBufferHeight - buttonSize.y/2 - gap);
			
			for (int id = ControlId_startControls+1; id < ControlId_endControls; ++id)
			{
				x = (int)(g_backBufferWidth * (0.5f + id - (ControlId_startControls+1)) / buttonCount);
				mainDialog.GetControl( id )->SetLocation( (int)(x - buttonSize.x / 2), (int)(y - buttonSize.y / 2) );
				mainDialog.GetControl( id )->SetSize( (int)(buttonSize.x), (int)(buttonSize.y) );
			}
		}
		
		int xStart = g_backBufferWidth - buttonWidth - gap;
		int x = xStart;
		int y = g_backBufferHeight - buttonHeight - gap;
		int offset = buttonWidth + gap;

		{
			// player pos
			x = gap;
			y = gap;
			mainDialog.GetControl( ControlId_static_playerPos )->SetLocation( x, y );
			mainDialog.GetControl( ControlId_static_playerPos )->SetTextColor( 0xFFFFFFFF );
		}

		{
			// help
			int y = 42;
			int x = g_backBufferWidth/2;
			mainDialog.GetControl( ControlId_help )->SetLocation( x, y );
		}

		const int textureSize = 160;
		const int previewX = 0 + gap;
		const int previewY = g_backBufferHeight - textureSize - gap - buttonHeight - gap - 2;

		// terrain edit info
		{
			x = previewX;
			y = previewY;
			mainDialog.GetControl( ControlId_static_surfaceTexture )->SetLocation( x +  textureSize/2, y + textureSize/2 );
			mainDialog.GetControl( ControlId_static_surfaceTexture )->SetSize( textureSize/2, textureSize/2 );
		}
		
		// object edit info
		{
			x = previewX;
			y = previewY;
			mainDialog.GetControl( ControlId_static_objectEditTexture )->SetLocation( x +  textureSize/2, y + textureSize/2 );
			mainDialog.GetControl( ControlId_static_objectEditTexture )->SetSize( textureSize/2, textureSize/2 );
			
			const float smallPreviewScale = 0.23f;
			const int smallPreviewSize = int(textureSize * smallPreviewScale);
			mainDialog.GetControl( ControlId_static_objectEditTextureSmall )->SetLocation( x + smallPreviewSize, y + smallPreviewSize );
			mainDialog.GetControl( ControlId_static_objectEditTextureSmall )->SetSize( smallPreviewSize, smallPreviewSize );
			
			y += 2;

			x += gap;
			mainDialog.GetControl( ControlId_static_objectEdit_handle )->SetLocation( x + gap + 2*smallPreviewSize, y );
			y += textHeight;
			
			mainDialog.GetControl( ControlId_static_objectEdit_type )->SetLocation( x + gap + 2*smallPreviewSize, y );
			y += textHeight;

			mainDialog.GetControl( ControlId_static_objectEdit_xf )->SetLocation( x + gap + 2*smallPreviewSize, y );
			y += textHeight;

			mainDialog.GetControl( ControlId_static_objectEdit_objectInfoDisplay )->SetLocation( x, y );
			y += textHeight;

			mainDialog.GetControl( ControlId_static_objectEdit_objectAttributesDisplay )->SetLocation( x, y );
			y += textHeight + gap;

			mainDialog.GetControl( ControlId_edit_objectEdit_attributesBox )->SetLocation( x, y );

			// make attributes stretch length of screen
			int width = g_backBufferWidth - 16 - 2*int(quickPickSize);
			mainDialog.GetEditBox(ControlId_edit_objectEdit_attributesBox)->SetSize(width, 30);
			mainDialog.GetStatic(ControlId_static_objectEdit_objectInfoDisplay)->SetSize(width, 30);
			mainDialog.GetStatic(ControlId_static_objectEdit_objectAttributesDisplay)->SetSize(width, 30);
		}
	}
}

int EditorGui::GetTileQuickPickId()
{
	const int rows = GetTileQuickPickRows();
	const float sx2 = quickPickSize / quickPickTileColumns;
	const float sy2 = quickPickSize / quickPickTileColumns;
	const float sx = quickPickSize;
	const float sy = rows*sy2;
	const float x = g_backBufferWidth - quickPickSize - 4;
	const float y = float(g_backBufferHeight - sy2*rows) - 50;

	// tile quick pick
	Vector2 tilePos = g_input->GetMousePosScreenSpace();
	tilePos -= Vector2(x, y);
	tilePos /= Vector2(sx, sy);
	tilePos = Vector2(0.5f) + tilePos / 2;
	tilePos.x *= float(quickPickTileColumns);
	tilePos.y *= float(rows);
	int tileX = int(floor(tilePos.x));
	int tileY = int(floor(tilePos.y));

	if (tileX >= 0 && tileX < quickPickTileColumns && tileY >= 0 && tileY < rows)
		return tileX + quickPickTileColumns * tileY;

	return -1;
}

int EditorGui::GetObjectQuickPickId()
{
	const int rows = GetObjectQuickPickRows();
	const float sx2 = quickPickSize / quickPickObjectColumns;
	const float sy2 = quickPickSize / quickPickObjectColumns;
	const float sx = quickPickSize;
	const float sy = rows*sy2;
	const float x = g_backBufferWidth - quickPickSize - 4;
	const float y = float(g_backBufferHeight - sy2*rows) - 50;

	// object quick pick
	Vector2 tilePos = g_input->GetMousePosScreenSpace();
	tilePos -= Vector2(x, y);
	tilePos /= Vector2(sx, sy);
	tilePos = Vector2(0.5f) + tilePos / 2;
	tilePos.x *= float(quickPickObjectColumns);
	tilePos.y *= float(rows);
	int tileX = int(floor(tilePos.x));
	int tileY = int(floor(tilePos.y));

	if (tileX >= 0 && tileX < quickPickObjectColumns && tileY >= 0 && tileY < rows)
		return tileX + quickPickObjectColumns * tileY;

	return -1;
}

HRESULT EditorGui::Render(float delta)
{
	const Color backgroundColor = Color::Black(0.7f);
	const Color outlineColor = Color::White();
	
	{
		// draw backgrounds

		// position
		g_render->RenderScreenSpaceQuad(Vector2(243, 17), Vector2(244, 18), backgroundColor, GameTexture_Invalid, outlineColor);
		
		// layer buttons
		g_render->RenderScreenSpaceQuad(Vector2(g_backBufferWidth - 130.0f, 61), IntVector2(130, 62), backgroundColor, GameTexture_Invalid, outlineColor);

		// help popup
		if (mainDialog.GetStatic( ControlId_help )->GetVisible())
			g_render->RenderScreenSpaceQuad(Vector2(g_backBufferWidth/2.0f, 56), IntVector2(220, 15), backgroundColor, GameTexture_Invalid, outlineColor);
	
		// buttons bar
		g_render->RenderScreenSpaceQuad(Vector2(g_backBufferWidth/2.0f-1, g_backBufferHeight - 25.0f), IntVector2(g_backBufferWidth/2+2, 25), backgroundColor, GameTexture_Invalid, outlineColor);
		
		if (g_gameControlBase->IsTileEditMode())
		{
			const int rows = GetTileQuickPickRows();
			float quickPickSizeY = quickPickSize * rows / 16;

			// quick pick
			float x = g_backBufferWidth - quickPickSize - 4;
			float y = g_backBufferHeight - quickPickSizeY - 50;
			float sx = quickPickSize;
			float sy = quickPickSizeY;
			g_render->RenderScreenSpaceQuad(Vector2(x, y - 28), Vector2(sx, sy + 28), backgroundColor, GameTexture_Invalid, outlineColor);
		}
		else
		{
			if (g_editor.GetObjectEditor().HasOnlyOneSelected())
			{
				// object attributes
				Box2AABB box(Vector2(2, float(g_backBufferHeight) - 186 - 24), Vector2(float(g_backBufferWidth) - 304, float(g_backBufferHeight) - 50));
				g_render->RenderScreenSpaceQuad(box, backgroundColor, GameTexture_Invalid, outlineColor);
			}

			const int rows = GetObjectQuickPickRows();
			float quickPickSizeY = quickPickSize * rows / 8;

			// quick pick
			float x = g_backBufferWidth - quickPickSize - 4;
			float y = g_backBufferHeight - quickPickSizeY - 50;
			float sx = quickPickSize;
			float sy = quickPickSizeY;
			g_render->RenderScreenSpaceQuad(Vector2(x, y -15), Vector2(sx, sy + 15), backgroundColor, GameTexture_Invalid, outlineColor);
		}
	}
	
	mainDialog.OnRender(delta);

	DXUTGetD3D9Device()->SetRenderState(D3DRS_LIGHTING, TRUE);

	// render the terrain edit texture
	if (g_gameControlBase->IsTileEditMode())
	{
		const int surfaceIndex = g_editor.GetTileEditor().GetDrawSurfaceIndex();
		const GameSurfaceInfo& surface = g_editor.GetTileEditor().GetDrawSurfaceInfo();

		int x, y, sx, sy;
		mainDialog.GetControl( ControlId_static_surfaceTexture )->GetLocation(x, y);
		mainDialog.GetControl( ControlId_static_surfaceTexture )->GetSize(sx, sy);
		Matrix44 matrix = FrankRender::GetScreenSpaceMatrix((float)x, (float)y, (float)sx, (float)sy);
		g_render->RenderScreenSpaceQuad( matrix, backgroundColor );
		
		if (Terrain::tileSetCount == 0)
			g_render->RenderScreenSpaceQuad( matrix, surface.color, surface.ti );
		else
		{
			const GameTextureID ti = Terrain::GetTileSetTexture(g_editor.GetTileEditor().GetTileSet());
			const IntVector2 tilePos( surfaceIndex % 16, surfaceIndex / 16 );
			g_render->RenderScreenSpaceTile( tilePos, Vector2(16,16), matrix, surface.color, ti );
		}
		g_render->RenderScreenSpaceQuadOutline( matrix, Color::White() );

		{
			// quick pick
			float x = g_backBufferWidth - quickPickSize - 4;
			float y = g_backBufferHeight - quickPickSize - 50;
			float sx = quickPickSize;

			float sy = quickPickSize;

			if (Terrain::tileSetCount > 0)
			{
				// draw the terrain quick pick
				const Matrix44 matrix = FrankRender::GetScreenSpaceMatrix(x, y, sx, sy);
				g_render->RenderScreenSpaceQuad( matrix, Color::Grey(0.5f) );
				const GameTextureID ti = Terrain::GetTileSetTexture(g_editor.GetTileEditor().GetTileSet());
				g_render->RenderScreenSpaceQuad( matrix, Color::White(), ti);
				g_render->RenderScreenSpaceQuadOutline( matrix, Color::White() );
			}
			else
			{
				const int rows = GetTileQuickPickRows();
				const float sx2 = sx / quickPickTileColumns;
				const float sy2 = sy / quickPickTileColumns;

				y = float(g_backBufferHeight - sy2*rows) - 50;
				sy = rows*sy2;

				// draw the terrain quick pick
				const Matrix44 matrix = FrankRender::GetScreenSpaceMatrix(x, y, sx, sy);
				g_render->RenderScreenSpaceQuad( matrix, Color::Grey(0.5f) );

				// draw each tile
				for(int j = 0; j < rows; ++j)
				for(int i = 0; i < quickPickTileColumns; ++i)
				{
					const int surfaceIndex = i + quickPickTileColumns*j;
					if (surfaceIndex >= GameSurfaceInfo::Count())
						break;
						
					const float x2 = x + sx2 - sx + 2*i * sx2;
					const float y2 = y + sy2 - sy + 2*j * sy2;
					const Matrix44 matrix = FrankRender::GetScreenSpaceMatrix(x2, y2, sx2, sy2);
					const GameSurfaceInfo& si = GameSurfaceInfo::Get(surfaceIndex);
					g_render->RenderScreenSpaceQuad(matrix, si.color, si.ti);
				}
				g_render->RenderScreenSpaceQuadOutline( matrix, Color::White() );
			}

			{
				// highlight selected tile in quick pick
				const int drawSurface = g_editor.GetTileEditor().GetDrawSurfaceIndex();
				const IntVector2 drawPos(drawSurface % quickPickTileColumns, drawSurface / quickPickTileColumns);
				const float sx2 = quickPickSize / quickPickTileColumns;
				const float sy2 = quickPickSize / quickPickTileColumns;
				const float x2 = x + sx2 - sx + 2*drawPos.x * sx2;
				const float y2 = y + sy2 - sy + 2*drawPos.y * sy2;
				const Matrix44 matrix = FrankRender::GetScreenSpaceMatrix((float)x2, (float)y2, (float)sx2, (float)sy2);
				g_render->RenderScreenSpaceQuadOutline( matrix, Color::Green() );
			}
		}
	}
	else if (g_gameControlBase->IsObjectEditMode())
	{
		int x, y, sx, sy;
		
		if (g_editor.GetObjectEditor().HasOnlyOneSelected())
		{
			mainDialog.GetControl( ControlId_static_objectEditTextureSmall )->GetLocation(x, y);
			mainDialog.GetControl( ControlId_static_objectEditTextureSmall )->GetSize(sx, sy);
		}
		else
		{
			mainDialog.GetControl( ControlId_static_objectEditTexture )->GetLocation(x, y);
			mainDialog.GetControl( ControlId_static_objectEditTexture )->GetSize(sx, sy);
		}

		const ObjectTypeInfo& info = g_editor.GetObjectEditor().HasOnlyOneSelected()?  g_editor.GetObjectEditor().GetOnlyOneSelectedStub()->GetObjectInfo(): g_editor.GetObjectEditor().GetNewStubInfo();
		Matrix44 matrix = FrankRender::GetScreenSpaceMatrix((float)x, (float)y, (float)sx, (float)sy);
		g_render->RenderScreenSpaceQuad( matrix, backgroundColor );
		g_render->RenderScreenSpaceQuad( matrix, info.GetColor(), info.GetTexture() );
		g_render->RenderScreenSpaceQuadOutline( matrix, Color::White() );

		{
			// quick pick
			const int rows = GetObjectQuickPickRows();
			const float sx2 = quickPickSize / quickPickObjectColumns;
			const float sy2 = quickPickSize / quickPickObjectColumns;;
			const float sx = quickPickSize;
			const float sy = rows*sy2;
			const float x = g_backBufferWidth - quickPickSize - 4;
			const float y = float(g_backBufferHeight - sy2*rows) - 50;

			const Matrix44 matrix = FrankRender::GetScreenSpaceMatrix(x, y, sx, sy);
			g_render->RenderScreenSpaceQuad( matrix, Color::Black(0.5f) );

			// draw each object type
			for(int j = 0; j < rows; ++j)
			for(int i = 0; i < quickPickObjectColumns; ++i)
			{
				const GameObjectType objectType = GameObjectType(i + quickPickObjectColumns*j);
				if (objectType > ObjectTypeInfo::GetMaxType())
					break;

				if (!GameObjectStub::HasObjectInfo(objectType))
					continue;
						
				const float x2 = x + sx2 - sx + 2*i * sx2;
				const float y2 = y + sy2 - sy + 2*j * sy2;
				const Matrix44 matrix = FrankRender::GetScreenSpaceMatrix(x2, y2, sx2, sy2);
				const ObjectTypeInfo& info = GameObjectStub::GetObjectInfo(objectType);
				g_render->RenderScreenSpaceQuad(matrix, info.GetColor(), info.GetTexture());
				g_render->RenderScreenSpaceQuadOutline( matrix, Color::White(0.25f) );
			}
			
			g_render->RenderScreenSpaceQuadOutline( matrix, Color::White() );

			{
				// highlight selected object in quick pick
				const GameObjectType objectType = g_editor.GetObjectEditor().GetNewStubType();
				const IntVector2 drawPos(objectType % quickPickObjectColumns, objectType / quickPickObjectColumns);
				const float sx2 = quickPickSize / quickPickObjectColumns;
				const float sy2 = quickPickSize / quickPickObjectColumns;
				const float x2 = x + sx2 - sx + 2*drawPos.x * sx2;
				const float y2 = y + sy2 - sy + 2*drawPos.y * sy2;
				const Matrix44 matrix = FrankRender::GetScreenSpaceMatrix((float)x2, (float)y2, (float)sx2, (float)sy2);
				g_render->RenderScreenSpaceQuadOutline( matrix, Color::Green() );
			}
		}
	}
	
	if (g_editor.GetObjectEditor().HasOnlyOneSelected())
		RenderObjectAttributes();

	{
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR );
		DXUTGetD3D9Device()->SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR );

		// draw watermark
		float sx = 120, sy = 30;
		float x = float(g_backBufferWidth/2);
		float y = 20;

		Matrix44 matrix = FrankRender::GetScreenSpaceMatrix(x, y, sx, sy);
		if (g_render->GetTexture(GameTexture_Watermark))
			g_render->RenderScreenSpaceQuad( matrix, Color::White(), GameTexture_Watermark);
		else
		{
			g_debugRender.ImmediateRenderTextRaw(Vector2(x+2,y+2), L"FRANK ENGINE", Color::Black());
			g_debugRender.ImmediateRenderTextRaw(Vector2(x,y), L"FRANK ENGINE");
		}
		g_render->SetFiltering();
	}

	return S_OK;
}

// special function to highlight the attribute the caret is over for the selcted object
void EditorGui::RenderObjectAttributes()
{
	if (!g_gameControlBase->IsObjectEditMode() || !g_editor.GetObjectEditor().HasOnlyOneSelected())
		return;	

	GameObjectStub* stub = g_editor.GetObjectEditor().GetOnlyOneSelectedStub();
	
	int x, y, w, h;
	mainDialog.GetStatic( ControlId_static_objectEdit_objectAttributesDisplay )->GetLocation(x, y);
	mainDialog.GetStatic( ControlId_static_objectEdit_objectAttributesDisplay )->GetSize(w, h);
	RECT r = {x, y, x+w, y+h};

	if (!IsEditBoxFocused())
	{
		mainDialog.BeginSprite();
		mainDialog.DrawText( stub->GetObjectInfo().GetAttributesDescription(), 0, r, Color::White(), DT_TOP | DT_LEFT);
		mainDialog.EndSprite();
		g_render->SetFiltering();
		DXUTGetD3D9Device()->SetRenderState(D3DRS_LIGHTING, TRUE);
		return;
	}

	// get caret pos
	const int caretPos = Max(mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->GetCaretPos(), 0);
	LPCWSTR editText = mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->GetText();

	// figure out which word/parameter the cursor is in by looking at white space
	int valuePos = 0;
	int stringCount = 0;
	{
		FrankUtil::AttributesStringParser parser(stub->attributes);
					
		parser.skipWhiteSpace();
		if (*parser.getString() == '#')
			++stringCount;

		while (parser.skipToNextValue('#'))
		{
			++valuePos;
			if (parser.getOffset() > caretPos)
				break;

			if (*parser.getString() == '#')
				++stringCount;
		}

		if (parser.getOffset() <= caretPos)
		{
			// hack: check if we ended on white space and move on to the next value
			parser.moveBack();
			const char* parserEndString = parser.getString();
			if (*parserEndString == ' ')
				++valuePos;
		}
	}
	
	mainDialog.BeginSprite();
	{	
		const WCHAR* wideDescription = stub->GetObjectInfo().GetAttributesDescription();
		char description[256];
		wcstombs_s(NULL, description, 256, wideDescription, 256);
					
		FrankUtil::AttributesStringParser parser(description);
					
		if (stringCount == 0)
		{
			int pos = 0;
			while (++pos < valuePos && parser.skipToNextValue('#'));
		}
		else
		{
			int pos = 0;
			bool foundMarker = false;
			while (++pos <= stringCount)
			{
				foundMarker = parser.skipToMarker('#');
				if (!foundMarker)
					break;
			}
			if (foundMarker)
				parser.moveBack();
		}

		WCHAR string[256];
		int startOffset = 0;
		int endOffset = parser.getOffset();
		if (startOffset != endOffset)
		{
			wcsncpy_s(string, &wideDescription[startOffset], endOffset - startOffset);

			r.left = x;
			r.top = y;
			mainDialog.DrawText( string, 0, r, Color::White(), DT_TOP | DT_LEFT);
			x += mainDialog.GetTextLength(string, 0) + 6;
			startOffset = endOffset;
		}

		parser.skipToNextValue('#');
		endOffset = parser.getOffset();
		wcsncpy_s(string, &wideDescription[startOffset], endOffset - startOffset);
		r.left = x;
		r.top = y;
		mainDialog.DrawText( string, 0, r, Color::Red(), DT_TOP | DT_LEFT );
		x += mainDialog.GetTextLength(string, 0) + 6;
		startOffset = endOffset;

		parser.skipToNextValue('#');
		endOffset = strlen(parser.getStringStart());
		wcsncpy_s(string, &wideDescription[startOffset], endOffset - startOffset);
		r.left = x;
		r.top = y;
		mainDialog.DrawText( string, 0, r, Color::White(), DT_TOP | DT_LEFT);
		x += mainDialog.GetTextLength(string, 0) + 6;
	}
	mainDialog.EndSprite();
	g_render->SetFiltering();
	DXUTGetD3D9Device()->SetRenderState(D3DRS_LIGHTING, TRUE);
}

//--------------------------------------------------------------------------------------
bool GetSaveFileName() 
{ 
	const int BUFSIZE = 1024;
	WCHAR buffer[BUFSIZE] = L"";
	wcsncpy_s(buffer, Terrain::terrainFilename, BUFSIZE);
	OPENFILENAME ofns = {0};
	ofns.lStructSize = sizeof( ofns );
	ofns.lpstrFilter = L"Frank engine data file (*.2dt)\0*.2dt\0All files (*.*)\0*.*\0\0";
	ofns.lpstrFile = buffer;
	ofns.nMaxFile = BUFSIZE;
	ofns.lpstrTitle = L"Save file as...";

	// hack: set current diretory
	WCHAR currentDirectory[1024];
	GetCurrentDirectory(1024, currentDirectory);
	ofns.lpstrInitialDir = currentDirectory;

	ofns.Flags = OFN_NOCHANGEDIR;
	if (!GetSaveFileName(&ofns))
		return false;

	wcsncpy_s(Terrain::terrainFilename, buffer, 256);
	return true;
}

//--------------------------------------------------------------------------------------
bool GetLoadFileName() 
{ 
	const int BUFSIZE = 1024;
	WCHAR buffer[BUFSIZE] = L"";
	wcsncpy_s(buffer, Terrain::terrainFilename, BUFSIZE);
	OPENFILENAME ofns = {0};
	ofns.lStructSize = sizeof( ofns );
	ofns.lpstrFilter = L"Frank engine data file (*.2dt)\0*.2dt\0All files (*.*)\0*.*\0\0";
	
	// hack: set current diretory
	WCHAR currentDirectory[1024];
	GetCurrentDirectory(1024, currentDirectory);
	ofns.lpstrInitialDir = currentDirectory;

	ofns.lpstrFile = buffer;
	ofns.nMaxFile = BUFSIZE;
	ofns.lpstrTitle = L"Load file...";
	ofns.Flags = OFN_NOCHANGEDIR;
	if (!GetOpenFileName(&ofns))
		return false;

	wcsncpy_s(Terrain::terrainFilename, buffer, 256);
	return true;
}

//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
static void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
	// block GUI events when in debug console
	//if (GetDebugConsole().IsOpen())
	//	return;

	switch( nControlID )
    {
		case ControlId_button_layer1:
		{
			g_editor.SetEditLayer(1);
			break;
		}
		case ControlId_button_layer2:
		{
			g_editor.SetEditLayer(0);
			break;
		}
		case ControlId_button_layer3:
		{
			g_editor.SetEditLayer(2);
			break;
		}
		case ControlId_button_editorClear:
		{
			g_editor.ClearSelection();
			g_terrain->Clear();
			g_editor.SaveState();
			break;
		}
		case ControlId_button_editorHelp:
		{	
			// open the webpage
			SHELLEXECUTEINFOW shellInfo;
			ZeroMemory (&shellInfo, sizeof (SHELLEXECUTEINFO));
			shellInfo.cbSize = sizeof(shellInfo);
			shellInfo.fMask = SEE_MASK_ASYNCOK;
			shellInfo.lpVerb = L"open";
			shellInfo.lpFile = L"http://frankengine.3d2k.com/";
			shellInfo.nShow = SW_SHOWNORMAL;
			ShellExecuteEx(&shellInfo);
            break;
		}
		case ControlId_button_editorShowGrid:
		{
			Editor::showGrid = !Editor::showGrid;
			break;
		}
		case ControlId_button_editorSnapToGrid:
		{
			ObjectEditor::snapToGrid = !ObjectEditor::snapToGrid;
			break;
		}
		case ControlId_button_editorBlockEdit:
		{
			TileEditor::blockEdit = !TileEditor::blockEdit;
			break;
		}
		case ControlId_button_editorAutoSave:
		{
			GameControlBase::autoSaveTerrain = !GameControlBase::autoSaveTerrain;
			break;
		}
		case ControlId_button_randomizeTerrain:
		{	
			g_gameControlBase->RandomizeTerrain();
			break;
		}
		case ControlId_button_fullscreen:
		{	
			DXUTToggleFullScreen();
			break;
		}
		case ControlId_button_tileSetUp:
		{
			g_editor.GetTileEditor().ChangeTileSet(true);
			break;
		}
		case ControlId_button_tileSetDown:
		{
			g_editor.GetTileEditor().ChangeTileSet(false);
			break;
		}
		case ControlId_button_editorSave:
		{
			if (!g_terrain)
				break;

			if (!GetSaveFileName())
			{
				//g_debugMessageSystem.AddError(L"Saved canceled.");
				break;
			}

			if (g_editor.GetTileEditor().HasSelection())
				g_editor.ClearSelection();
			g_terrain->Save(Terrain::terrainFilename);
			g_editor.SaveState();
			g_debugMessageSystem.Add(L"Saved world file...", Color::Cyan());
			g_debugMessageSystem.Add(Terrain::terrainFilename, Color::Cyan());
			break;
		}
		case ControlId_button_editorLoad:
		{
			if (!g_terrain)
				break;

			if (!GetLoadFileName())
			{
				//g_debugMessageSystem.AddError(L"Load canceled.");
				break;
			}

			g_terrain->Load(Terrain::terrainFilename);
			g_editor.SaveState();
			g_debugMessageSystem.Add(L"Loaded world file...", Color::Cyan());
			g_debugMessageSystem.Add(Terrain::terrainFilename, Color::Cyan());
			break;
		}
		default:
			break;
    }
}

bool EditorGui::IsEditBoxFocused()
{
	return mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->HasFocus();
}

void EditorGui::ClearEditBox()
{
	return mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->SetText(L"");
}

LPCWSTR EditorGui::GetEditBoxText()
{
	return mainDialog.GetEditBox( ControlId_edit_objectEdit_attributesBox )->GetText();
}