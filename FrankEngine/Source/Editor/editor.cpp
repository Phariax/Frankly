////////////////////////////////////////////////////////////////////////////////////////
/*
	Editor
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../terrain/terrain.h"
#include "../editor/editor.h"

#define saveStateDirectory	L"temp/"
#define screenshotPrefix	L"terrain_"
#define screenshotFileType	L"2dt"

// the one and only game editor
Editor g_editor;

// size of render window for slower pcs, 0 indicates to render full terrain
int Editor::terrainRenderWindowSize = 0;
ConsoleCommand(Editor::terrainRenderWindowSize, editorTerrainRenderWindowSize)
	
bool Editor::showGrid = true;
ConsoleCommand(Editor::showGrid, showGrid);

Color Editor::gridPatchColor = Color::White(0.5f);
ConsoleCommand(Editor::gridPatchColor, gridPatchColor);

Color Editor::gridColor = Color::White(0.25f);
ConsoleCommand(Editor::gridColor, gridColor);

// optimzation to only cull stubs that are offscreen
bool Editor::stubCameraTest = false;
ConsoleCommand(Editor::stubCameraTest, stubCameraTest);

Editor::Editor()
{
	stateChanged = false;
	isTileEdit = true;
	isRectangleSelecting = false;
	saveStateStart = 0;
	saveStateEnd = 0;
	saveStatePosition = 0;
}

void Editor::ResetEditor()
{
	isRectangleSelecting = false;
	ClearSelection();
}

void Editor::Update()
{
	ASSERT(g_gameControlBase->IsEditMode());
	const Vector2 mousePos = g_input->GetMousePosWorldSpace();

	// move the camera
	if (g_input->IsDown(GB_MouseMiddle) || g_input->IsDown(GB_Tab) && g_input->IsDown(GB_MouseLeft))
		g_cameraBase->SetPosWorld(g_cameraBase->GetPosWorld() - g_input->GetMouseDeltaWorldSpace());
	
	if (g_input->IsDown(GB_Control) && g_input->IsDownUI(GB_Editor_Undo))
		Undo();
	else if (g_input->IsDown(GB_Control) && g_input->IsDownUI(GB_Editor_Redo))
		Redo();

	if (!g_editorGui.IsEditBoxFocused())
	{
		if (g_input->WasJustPushed(GB_Editor_Grid))
			showGrid = !showGrid;
		if (g_input->WasJustPushed(GB_Editor_BlockEdit))
			TileEditor::blockEdit = !TileEditor::blockEdit;
	}
	{
		if (g_input->IsDown(GB_Control) && g_input->WasJustPushed(GB_Editor_Save))
		{
			if (GetTileEditor().HasSelection())
				ClearSelection();
			g_terrain->Save(Terrain::terrainFilename);
			SaveState();
			g_debugMessageSystem.Add(L"Saved world file...", Color::Cyan());
			g_debugMessageSystem.Add(Terrain::terrainFilename, Color::Cyan());
		}
		else if (g_input->IsDown(GB_Control) && g_input->WasJustPushed(GB_Editor_Load))
		{
			g_terrain->Load(Terrain::terrainFilename);
			SaveState();
			g_debugMessageSystem.Add(L"Loaded world file...", Color::Cyan());
			g_debugMessageSystem.Add(Terrain::terrainFilename, Color::Cyan());
		}
	}

	if (!isRectangleSelecting && g_input->WasJustPushed(GB_MouseRight) && !g_input->IsDown(GB_Tab) && !g_input->IsDown(GB_Shift) && !g_input->IsDown(GB_Control))
	{
		// must not be in quick pick box
		if (!(IsObjectEdit() && g_editorGui.GetObjectQuickPickId() >= 0) && !(IsTileEdit() && g_editorGui.GetTileQuickPickId() >= 0))
		{
			if (!IsOnSelected(mousePos))
			{
				// rectangle selection tool
				ClearSelection();
				SaveState(true);
				isRectangleSelecting = true;
				isMultiLayerSelecting = true;
				rectangleSelectBox.lowerBound = mousePos;
			}
		}
	}

	if (isRectangleSelecting)
	{
		// update rectange select
		rectangleSelectBox.upperBound = mousePos;

		if (g_input->IsDown(GB_Alt))
			isMultiLayerSelecting = false;

		if (g_input->IsDown(GB_MouseLeft))
		{
			isRectangleSelecting = false;
		}
		else if (!g_input->IsDown(GB_MouseRight) || g_input->IsDown(GB_Tab))
		{
			// make the selection
			isRectangleSelecting = false;

			if (IsTileEdit() || isMultiLayerSelecting)
				tileEditor.Select(rectangleSelectBox, isMultiLayerSelecting);
			if (IsObjectEdit() || isMultiLayerSelecting)
				objectEditor.Select(rectangleSelectBox);

			// if box didn't select anything, pick the object under the mouse
			if (!HasSelection() && !isTileEdit)
			{
				// only selecte the stub if the box is fully on a stub
				GameObjectStub* stub1 = g_terrain->GetStub(mousePos);
				GameObjectStub* stub2 = g_terrain->GetStub(rectangleSelectBox.lowerBound);
				if (stub1 == stub2 && stub1 != NULL)
					objectEditor.SelectStub(mousePos);
			}
		}
	}
	else
	{
		if (!g_editorGui.IsEditBoxFocused())
		{
			if (HasSelection() && (g_input->IsDown(GB_Control) && g_input->WasJustPushed(GB_Editor_Cut)) || g_input->WasJustPushed(GB_Editor_Delete))
			{
				objectEditor.Cut();
				tileEditor.Cut();
				SaveState();
			}
			else if (g_input->IsDown(GB_Control) && g_input->WasJustPushed(GB_Editor_Copy))
			{
				if (g_input->IsDown(GB_Shift))
				{
					// select the current patch
					TerrainPatch* patch = g_terrain->GetPatch(mousePos);
					if (patch)
					{
						rectangleSelectBox.lowerBound = patch->GetTilePos(0,0);
						rectangleSelectBox.upperBound = patch->GetTilePos(Terrain::patchSize-1, Terrain::patchSize-1);

						tileEditor.Select(rectangleSelectBox, true);
						objectEditor.Select(rectangleSelectBox);
					}
				}
				if (HasSelection())
				{
					objectEditor.Copy();
					tileEditor.Copy();
				}
			}
			else if ((g_input->IsDown(GB_Control) && g_input->WasJustPushed(GB_Editor_Paste)) || g_input->WasJustPushed(GB_Editor_Insert))
			{
				if (g_input->IsDown(GB_Shift))
				{
					if (HasSelection())
					{
						// paste centered around the current patch
						Vector2 offset = g_terrain->GetPatch(mousePos)->GetPosWorld();
						offset += 0.5f*Vector2(TerrainTile::GetSize()*Terrain::patchSize);
						offset = tileEditor.GetPasteOffset(offset);
						objectEditor.Paste(offset);
						tileEditor.Paste(offset);
						SaveState();
					}
				}
				else
				{
					// figure out where to paste
					if (tileEditor.IsClipboardEmpty())
					{
						// center around stubs if there is no terrain
						const Vector2 offset = objectEditor.GetPasteOffset(mousePos);
						ClearSelection();
						objectEditor.Paste(offset);
						SaveState();
					}
					else
					{
						// center around terrain
						const Vector2 offset = tileEditor.GetPasteOffset(mousePos);
						if (HasSelection())
						{
							ClearSelection();
							SaveState();
						}
						objectEditor.Paste(offset);
						tileEditor.Paste(offset);
					}
				}
			}
		}

		if (g_input->WasJustPushed(GB_MouseRight) && !g_input->IsDown(GB_Tab))
		{
			if (g_input->IsDown(GB_Shift))
			{
				if (IsObjectEdit())
				{
					// add to selection
					GameObjectStub* newSelectedObjectStub = g_terrain->GetStub(mousePos);
					if (newSelectedObjectStub && !objectEditor.IsSelected(*newSelectedObjectStub))
					{
						objectEditor.AddToSelection(*newSelectedObjectStub);
					}
				}
			}
			else if (g_input->IsDown(GB_Control))
			{
				// remove from selection
				if (!objectEditor.HasOnlyOneSelected())
				{
					GameObjectStub* newSelectedObjectStub = g_terrain->GetStub(mousePos);
					if (newSelectedObjectStub)
						objectEditor.RemoveFromSelection(*newSelectedObjectStub);
				}
			}
			else if (HasSelection())
			{
				// check if user clicked off selected
				if (!IsOnSelected(mousePos))
					ClearSelection();
			}
		}
	}
	
	if (IsObjectEdit())
	{
		objectEditor.Update(true);
		tileEditor.Update(false);
	}
	else
	{
		tileEditor.Update(true);
		objectEditor.Update(false);
	}

	if (!GetIsInQuickPick() && g_input->WasJustPushed(GB_MouseLeft))
	{
		// check if user clicked off selected
		if (HasSelection() && !IsOnSelected(mousePos))
		{
			ClearSelection();
			SaveState(true);
		}
	}

	if (!g_editorGui.IsEditBoxFocused())
	{
		// handle edit mode controls
		if (g_input->WasJustPushed(GB_Editor_Mode_Object))
			isTileEdit = false;
		else if (g_input->WasJustPushed(GB_Editor_Mode_Terrain1))
		{
			isTileEdit = true;
			tileEditor.SetLayer(1);
		}
		else if (g_input->WasJustPushed(GB_Editor_Mode_Terrain2) && Terrain::patchLayers > 1)
		{
			isTileEdit = true;
			tileEditor.SetLayer(0);
		}
		if (g_input->IsDown(GB_Editor_MovePlayer))
		{
			g_gameControlBase->GetPlayer()->SetPosWorld(g_input->GetMousePosWorldSpace());
			g_terrain->SetPlayerEditorStartPos(g_gameControlBase->GetPlayer()->GetPosWorld());
		}
		else if (g_input->WasJustReleased(GB_Editor_MovePlayer))
		{
			SaveState();
			if (ObjectEditor::snapToGridSize)
			{
				// round player position
				const float gridSize = ObjectEditor::snapToGridSize * TerrainTile::GetSize();
				Vector2 pos = g_gameControlBase->GetPlayer()->GetPosWorld();
				pos.x = gridSize*floor(0.5f + pos.x/gridSize);
				pos.y = gridSize*floor(0.5f + pos.y/gridSize);
				g_gameControlBase->GetPlayer()->SetPosWorld(pos);
			}
		}
	}
	
	if (saveStateStart == 0 && saveStateEnd == 0)
		g_editor.SaveState(); // hack save on startup
	else if (!g_input->IsDown(GB_Tab) && (g_input->WasJustReleased(GB_MouseLeft)) || g_input->WasJustReleased(GB_MouseRight))
		g_editor.SaveState(true);
}

void Editor::SetEditLayer(int editLayer)
{
	if (editLayer >= Terrain::patchLayers)
		isTileEdit = false;
	else
	{
		isTileEdit = true;
		tileEditor.SetLayer(editLayer);
	}
}

void Editor::Render()
{
	ASSERT(g_terrain);
	
	const Vector2& pos = g_gameControlBase->GetUserPosition();
	const IntVector2 patchOffset = g_terrain->GetPatchOffset(pos);

	int i0, i1, j0, j1;
	GetTerrainRenderWindow(patchOffset.x, patchOffset.y, i0, i1, j0, j1);

	for(int i = i0; i <= i1; ++i)
	for(int j = j0; j <= j1; ++j)
		RenderPatch(*g_terrain->GetPatch(i,j));
	
	for(int i = i0; i <= i1; ++i)
	for(int j = j0; j <= j1; ++j)
		RenderStubs(*g_terrain->GetPatch(i,j));
	
	g_render->RenderSimpleVerts();
	
	tileEditor.Render();
	objectEditor.Render();

	if (isRectangleSelecting)
	{
		g_render->DrawAxisAlignedBox(rectangleSelectBox, isMultiLayerSelecting? Color::Yellow() : Color::Green());
	}
	
	if (showGrid)
	{
		// render a grid
		int step = 1;
		if (g_cameraBase->GetZoom() > 600)
		{
			// only draw grids around each tile once you zoom out enough
			step = Terrain::patchSize;
		}
		
		const int size = Terrain::fullSize*Terrain::patchSize;
		const Vector2 pos = g_terrain->GetPosWorld();
		for(int x=0; x<=size; x+=step)
		{
			Vector2 start = pos;
			Vector2 end = pos;
			start.x += x * TerrainTile::GetSize();
			end.x += x * TerrainTile::GetSize();
			end.y += size * TerrainTile::GetSize();

			Color color = gridPatchColor;
			if (step == 1 && x % Terrain::patchSize)
				color = gridColor;

			g_render->DrawSegment(start, end, color);
		}
		for(int y=0; y<=size; y+=step)
		{
			Vector2 start = pos;
			Vector2 end = pos;
			start.y += y * TerrainTile::GetSize();
			end.y += y * TerrainTile::GetSize();
			end.x += size * TerrainTile::GetSize();

			Color color = gridPatchColor;
			if (step == 1 && y % Terrain::patchSize)
				color = gridColor;

			g_render->DrawSegment(start, end, color);
		}
	}
}

void Editor::RenderStubs(const TerrainPatch& patch) const
{
	const Vector2 pos = patch.GetPosWorld();

	// render the stubs in this patch
	for (list<GameObjectStub>::const_iterator it = patch.objectStubs.begin(); it != patch.objectStubs.end(); ++it) 
	{       
		const GameObjectStub& stub = *it;

		// skip selected stubs so we can render them last
		if (objectEditor.IsSelected(stub))
			continue;
		
		if (!stubCameraTest || g_cameraBase->CameraTest(stub.xf.position, stub.size.Length()))
		{
			const float stubAlpha = isTileEdit? 0.5f : 1.0f;
			stub.Render(stubAlpha);
		}
		
		const Color c = Color::Blue(isTileEdit? 0.35f : 1.0f);
		g_render->DrawBox(stub.xf, stub.size, c);
		g_render->DrawSolidCircle(stub.xf, 0.1f*TerrainTile::GetSize(), c);
	}
}

void Editor::RenderPatch(const TerrainPatch& patch) const
{
	const Vector2 pos = patch.GetPosWorld();

	// do a camera test on the full patch
	const Vector2 minPatchPos = patch.GetTilePos(0,0);
	const Vector2 maxPatchPos = patch.GetTilePos(Terrain::patchSize,Terrain::patchSize);
	const Vector2 centerPatchPos = minPatchPos + 0.5f * (maxPatchPos - minPatchPos);
	const float patchRadius = TerrainTile::GetRadius() * Terrain::patchSize;

	if (g_cameraBase->CameraTest(centerPatchPos, patchRadius) && isTileEdit)
	{
		const int terrainLayer = tileEditor.GetLayer();

		// draw edge lines
		for(int x=0; x<Terrain::patchSize; ++x)
		for(int y=0; y<Terrain::patchSize; ++y)
		{
			const Vector2 tileOffset = patch.GetTilePos(x, y);
			const TerrainTile& tile = patch.GetTileLocal(x,y, terrainLayer);

			static const Vector2 halfOffset = Vector2(TerrainTile::GetSize()/2, TerrainTile::GetSize()/2);
			if (!tile.IsClear() && !tile.IsFull())
			{
				if (g_cameraBase->CameraTest(tileOffset + halfOffset, TerrainTile::GetRadius()))
				{
					// draw a dashed line to show the edge
					const Vector2 posStart = tileOffset + tile.GetPosA();
					const Vector2 posEnd = tileOffset + tile.GetPosB();
					const Vector2 deltaPos = posEnd - posStart;
					g_render->DrawSegment(posStart, posEnd, Terrain::debugOutlineColor1);
					int dashCount = (int)(8*deltaPos.Magnitude() / TerrainTile::GetSize());
					for (int i = 0; i < dashCount; ++i)
					{
						if (i % 2)
							continue;

						const Vector2 pos1 = posStart + deltaPos * (float)i / (float)dashCount;
						const Vector2 pos2 = posStart + deltaPos * (float)(i+1) / (float)dashCount;
						g_render->DrawSegment(pos1, pos2, Terrain::debugOutlineColor2);
					}
				}
			}
		}
	}
}

void Editor::GetTerrainRenderWindow(int x, int y, int& xStart, int& xEnd, int& yStart, int& yEnd) const
{
	if (terrainRenderWindowSize == 0)
	{
		xStart	= 0;
		xEnd	= Terrain::fullSize-1;
		yStart	= 0;
		yEnd	= Terrain::fullSize-1;
	}
	else
	{
		xStart	= Cap(x-terrainRenderWindowSize, 0, Terrain::fullSize-1);
		xEnd	= Cap(x+terrainRenderWindowSize, 0, Terrain::fullSize-1);
		yStart	= Cap(y-terrainRenderWindowSize, 0, Terrain::fullSize-1);
		yEnd	= Cap(y+terrainRenderWindowSize, 0, Terrain::fullSize-1);
	}
}

bool Editor::HasSelection() const
{
	return (objectEditor.HasSelection() || tileEditor.HasSelection());
}

void Editor::ClearSelection()
{
	if (objectEditor.HasSelection() && tileEditor.HasSelection())
	{
		// when pasting objects and terrain, clear out any stubs in that region that aren't selected
		objectEditor.DeleteUnselected(tileEditor.GetSelectionBox());
	}

	objectEditor.ClearSelection();
	tileEditor.ClearSelection();
}

void Editor::ClearClipboard()
{
	objectEditor.ClearClipboard();
	tileEditor.ClearClipboard();
}

bool Editor::IsOnSelected(const Vector2& pos) const
{
	return (objectEditor.IsOnSelected(pos) || tileEditor.IsOnSelected(pos));
}

void Editor::Undo()
{
	const bool hadTileSelection = tileEditor.HasSelection();
	ClearSelection();

	if (saveStatePosition == saveStateStart)
		return;

	if (!hadTileSelection)
	{
		saveStatePosition -= 1;
		if (saveStatePosition < 0)
			saveStatePosition = saveStatePosition + maxSaveStates;
	}

	if (saveStatePosition != saveStateStart)
	{
		saveStatePosition -= 1;
		if (saveStatePosition < 0)
			saveStatePosition = saveStatePosition + maxSaveStates;
	}

	WCHAR* filename = GetUndoFilename(saveStatePosition);
	g_terrain->Load(filename);
	++saveStatePosition;
	saveStatePosition = saveStatePosition%maxSaveStates;
}

void Editor::Redo()
{
	ClearSelection();
	
	if (saveStatePosition == saveStateEnd)
		return;

	WCHAR* filename = GetUndoFilename(saveStatePosition);
	g_terrain->Load(filename);
	++saveStatePosition;
	saveStatePosition = saveStatePosition%maxSaveStates;
}

void Editor::SaveState(bool onlyIfStateChanged)
{
	if (tileEditor.HasSelection() || (onlyIfStateChanged && !stateChanged))
		return;

	WCHAR* filename = GetUndoFilename(saveStatePosition);
	CreateDirectory(saveStateDirectory, NULL);
	g_terrain->Save(filename);

	++saveStatePosition;
	saveStatePosition = saveStatePosition%maxSaveStates;
	saveStateEnd = saveStatePosition;
	if (saveStateStart == saveStateEnd)
		saveStateStart = (saveStateEnd + 1)%maxSaveStates;
	stateChanged = false;
}

WCHAR* Editor::GetUndoFilename(int i)
{
	static WCHAR filename[256];
	swprintf_s(filename, 256, saveStateDirectory screenshotPrefix L"%d." screenshotFileType, saveStatePosition);
	return filename;
}

void Editor::ChangeDrawType(bool direction)
{
	if (isTileEdit)
		tileEditor.ChangeDrawType(direction);
	else
		objectEditor.ChangeDrawType(direction);
}

void Editor::ClearTempFolder()
{
	const WCHAR *filename = saveStateDirectory screenshotPrefix L"*";
	
	int failsafe = 0;
	WIN32_FIND_DATA findData;
	while (FindFirstFile(filename, &findData) != INVALID_HANDLE_VALUE && ++failsafe < 10000)
	{
		WCHAR filename[256];
		swprintf_s(filename, 256, saveStateDirectory L"%s", findData.cFileName);
		DeleteFile(filename);
	}
}