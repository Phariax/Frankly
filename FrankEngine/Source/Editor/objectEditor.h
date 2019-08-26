////////////////////////////////////////////////////////////////////////////////////////
/*
	Object Editor
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_EDITOR_H
#define OBJECT_EDITOR_H

#include "../terrain/terrain.h"
#include "../objects/gameObjectBuilder.h"

class ObjectEditor
{
public:

	ObjectEditor();

	void Update(bool isActive);
	void Render();
	void UpdateSelected(GameObjectStub* stub);
	GameObjectStub& MoveStub(GameObjectStub& stub, const Vector2& offset);
	void MoveSelectedStubs(const Vector2& offset);

	bool IsSelected(const GameObjectStub& stub) const;
	bool IsOnSelected(const Vector2& pos) const;
	void SelectStub(const Vector2& pos);
	bool HasSelection() const { return !selectedStubs.empty(); }
	bool HasOnlyOneSelected() const;
	void Select(const Box2AABB& box);
	void RemoveFromSelection(GameObjectStub& stub);
	void AddToSelection(GameObjectStub& stub);
	void ClearSelection();
	void ClearClipboard();
	GameObjectStub* GetOnlyOneSelectedStub() { return HasOnlyOneSelected()? selectedStubs.front() : NULL; }
	list<GameObjectStub*>& GetSelectedStubs() { return selectedStubs; }
	Vector2 GetPasteOffset(const Vector2& pos);
	void ChangeDrawType(bool direction);
	void DeleteUnselected(const Box2AABB& box);
	bool GetIsInQuickPick() const { return isInQuickPick; }
	
	const ObjectTypeInfo& GetNewStubInfo() const { return GameObjectStub::GetObjectInfo(newStubType); }
	GameObjectType GetNewStubType() const { return newStubType; }

	void Cut();
	void Copy();
	void Paste(const Vector2& offset);
	void RemoveSelected();

	static float minStubSize;
	static bool snapToGrid;
	static float snapToGridSize;
	static int snapToGridAngle;
	
private:
	
	void AddStub();

	list<GameObjectStub*> selectedStubs;
	list<GameObjectStub> copiedStubs;
	GameObjectType newStubType;
	bool isInQuickPick;
};

#endif // OBJECT_EDITOR_H