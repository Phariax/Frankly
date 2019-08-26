////////////////////////////////////////////////////////////////////////////////////////
/*
	Tile Editor
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef TILE_EDITOR_H
#define TILE_EDITOR_H

#include "../terrain/terrainSurface.h"
#include "../terrain/terrain.h"

class TileEditor
{
public:

	TileEditor();
	~TileEditor();

	void Update(bool isActive);
	void Render();
	
	bool HasSelection() const { return selectedTiles != NULL; }
	bool IsOnSelected(const Vector2& pos) const;
	int GetDrawSurfaceIndex() const { return drawSurface; }
	const GameSurfaceInfo& GetDrawSurfaceInfo() const { return GameSurfaceInfo::Get(drawSurface); }
	int GetLayer() const { return terrainLayer; }
	void SetLayer(int layer)  { terrainLayer = layer; }
	void Select(Box2AABB& box, bool allLayers);
	void ClearSelection(bool anchor = true);
	void ClearClipboard();
	bool IsClipboardEmpty() const { return copiedTiles == NULL; }
	Vector2 GetPasteOffset(const Vector2& pos);
	Vector2 GetRoundingOffset();
	Box2AABB GetSelectionBox() const;

	TerrainTile& GetSelectedTile(int x, int y, int layer)
	{
		ASSERT(selectedTiles && selectedTilesSize.x >= 0 && selectedTilesSize.y >= 0);
		ASSERT(x >= 0 && x < selectedTilesSize.x && y >= 0 && y < selectedTilesSize.y && layer >= 0 && layer < selectedLayerCount);
		return selectedTiles[selectedTilesSize.x*selectedTilesSize.y*layer + selectedTilesSize.y*x + y];
	}
	
	int GetTileSet() const { return tileSet; }
	void SetTileSet(int layer, int set) { tileSet = set; }
	
	void Cut();
	void Copy();
	void Paste(const Vector2& offset);
	void ChangeDrawType(bool direction);
	void ChangeTileSet(bool direction);
	bool GetIsInQuickPick() const { return isInQuickPick; }
	void RemoveSelected();

	static bool enableCrossLayerPaste;
	static bool blockEdit;
	static bool pasteToCorner;

private:

	void FloodFill();
	void FloodErase();
	
	void Resurface(const Vector2& posA, const Vector2& posB, BYTE surfaceData);
	void Resurface(TerrainPatch& patch, const Vector2& posA, const Vector2& posB, BYTE surfaceData);

	void FloodFill(TerrainPatch& patch, const Vector2& testPos, BYTE surfaceData);
	void FloodFillDirection(TerrainPatch& patch, int x, int y, int direction, BYTE surfaceData, BYTE startSurfaceData, int surfaceSide);
	void FloodFillInternal(TerrainPatch& patch, int x, int y, BYTE surfaceData, BYTE startSurfaceData, int surfaceSide);
	
	Vector2 GetSnapLinePoint(const Vector2& pos, bool snapToGrid = false);

	int drawSurface;
	Vector2 mousePosLineStart;
	Vector2 worldCenter;

	int terrainLayer;
	int tileSet;
	IntVector2 selectedTilesSize;
	int selectedLayerCount;
	int firstSelectedLayer;
	TerrainTile *selectedTiles;
	Vector2 selectedTilesPos;

	TerrainTile *copiedTiles;
	IntVector2 copiedTileSize;
	Vector2 copiedTilesPos;
	int copiedLayerCount;
	bool isInQuickPick;
};

#endif // TILE_EDITOR_H