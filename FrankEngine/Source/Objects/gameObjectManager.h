////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object Manager Class
	Copyright 2013 Frank Force - http://www.frankforce.com

	- hash map of game objects using their unique handle as the hash
	- handles update and render of objects
	- protects against improper deleting of objects
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_OBJECT_MANAGER_H
#define GAME_OBJECT_MANAGER_H

#include <hash_map>

// global game object manager singleton
extern class GameObjectManager g_objectManager;

class GameObject;
typedef unsigned GameObjectHandle;
 
typedef stdext::hash_map<GameObjectHandle, class GameObject*> GameObjectHashTable;

class GameObjectManager
{
public:

	GameObjectManager() {}
	~GameObjectManager() 
	{
		ASSERT(GetObjectCount() == 0); // all objects should be removed by now
		RemoveAll(); 
	}

public:	// call these functions once per frame

	virtual void Update();
	virtual void SaveLastWorldTransforms();
	void CreateRenderList();
	virtual void Render();
	virtual void RenderPost();

public:	// functions to get / add / remove objects

	void Add(GameObject& obj);
	void Remove(const GameObject& obj);
	void RemoveAll();
	void Reset();
	virtual void UpdateTransforms();

	GameObjectHashTable& GetObjects() { return objects; }
	list<GameObject*> GetObjects(const Vector2& pos, float radius, bool skipChildern);
	GameObject* GetObjectFromHandle(GameObjectHandle handle);
	int GetObjectCount() { return objects.size(); }

	static bool GetLockDeleteObjects() { return lockDeleteObjects; }

private:

	static bool RenderSortCompare(GameObject* first, GameObject* second);

	GameObjectHashTable objects;			// hash map of all objects
	list<GameObject *> sortedRenderObjects;	// list of objects to render sorted by render group
	static bool lockDeleteObjects;			// to prevent improperly deleting objects
};

#endif // GAME_OBJECT_MANAGER_H