////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object Manager Class
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#include "frankEngine.h"
#include "../objects/gameObject.h"
#include "../objects/gameObjectManager.h"

GameObjectManager g_objectManager;			// singleton that contains all game objects in the world

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object Group Globals
*/
////////////////////////////////////////////////////////////////////////////////////////

bool GameObjectManager::lockDeleteObjects = true;		// to prevent improperly deleting objects

typedef pair<GameObjectHandle, class GameObject*> GameObjectHashPair;

////////////////////////////////////////////////////////////////////////////////////////
/*
	Game Object Group Member Functions
*/
////////////////////////////////////////////////////////////////////////////////////////

GameObject* GameObjectManager::GetObjectFromHandle(GameObjectHandle handle) 
{ 
	// look for matching handle
	GameObjectHashTable::iterator it = objects.find(handle);
	if (it != objects.end())
	{
		GameObject* object = ((*it).second);
		return object;
	}
	else
		return NULL;
}

void GameObjectManager::Add(GameObject& obj)
{
	ASSERT(!GetObjectFromHandle(obj.GetHandle())); // handle not unique!
	objects.insert(GameObjectHashPair(obj.GetHandle(), &obj));
}

void GameObjectManager::Remove(const GameObject& obj) 
{
	ASSERT(GetObjectFromHandle(obj.GetHandle())); // make sure object is in table
	objects.erase(obj.GetHandle());
}

void GameObjectManager::Update()
{
	for (GameObjectHashTable::iterator it = objects.begin(); it != objects.end(); ++it)
	{
		GameObject& obj = *((*it).second);
		if (obj.IsDestroyed() || obj.WasJustAdded())
			continue;
		
		obj.Update();
	}
}

// call this once per frame to clear out dead objects
void GameObjectManager::UpdateTransforms()
{
	// clear render objects because the objects may be deleted
	sortedRenderObjects.clear();

	lockDeleteObjects = false;
	for (GameObjectHashTable::iterator it = objects.begin(); it != objects.end();)
	{
		GameObject& obj = (*(*it).second);

		/*if (obj.wasJustAdded && obj.destroyThis)
		{
			g_debugMessageSystem.AddError( L"Object %d at (%0.2f, %0.2f) destroyed same frame as it was created.", 
				obj.GetHandle(), obj.GetXFormWorld().position.x, obj.GetXFormWorld().position.y );
		}*/

		if (obj.IsDestroyed())
		{	
			ASSERT(!obj.parent && obj.children.empty());
			it = objects.erase(it);
			delete &obj;
		} 
		else
		{
			if (!obj.HasParent())
				obj.UpdateTransforms();
			++it;
		}
	}
	lockDeleteObjects = true;
}

void GameObjectManager::SaveLastWorldTransforms()
{
	// save the last world transform for interpolation
	for (GameObjectHashTable::iterator it = objects.begin(); it != objects.end(); ++it)
	{
		GameObject& obj = *((*it).second);
		obj.xfWorldLast = obj.xfWorld;
	}
}

// sort objects by render order
bool GameObjectManager::RenderSortCompare(GameObject* first, GameObject* second)
{
	return (first->GetRenderGroup() < second->GetRenderGroup());
}

void GameObjectManager::CreateRenderList()
{
	sortedRenderObjects.clear();

	for (GameObjectHashTable::iterator it = objects.begin(); it != objects.end(); ++it)
	{
		GameObject& obj = *((*it).second);
		
		if (obj.WasJustAdded())
		{
			// render list is last thing to update, so clear just added flag
			obj.flags &= ~GameObject::ObjectFlag_JustAdded;
			//continue;
		}

		if (obj.IsVisible())
			sortedRenderObjects.push_back(&obj);
	}
	
	sortedRenderObjects.sort(RenderSortCompare);
}

void GameObjectManager::Render()
{
	int renderGroup = sortedRenderObjects.empty()? 0 : (*sortedRenderObjects.front()).GetRenderGroup();
	for (list<GameObject *>::iterator it = sortedRenderObjects.begin(); it != sortedRenderObjects.end(); ++it)
	{
		GameObject& obj = **it;
		if (obj.IsDestroyed())
			continue;

		if (renderGroup != obj.GetRenderGroup())
		{
			// always render simple verts and disable additive at the end of each group
			g_render->RenderSimpleVerts();
			g_render->SetSimpleVertsAreAdditive(false);
		}

		renderGroup = obj.GetRenderGroup();
		obj.Render();
	}

	g_render->RenderSimpleVerts();
	g_render->SetSimpleVertsAreAdditive(false);
}

void GameObjectManager::RenderPost()
{
	int renderGroup = sortedRenderObjects.empty()? 0 : (*sortedRenderObjects.front()).GetRenderGroup();
	for (list<GameObject *>::iterator it = sortedRenderObjects.begin(); it != sortedRenderObjects.end(); ++it)
	{
		GameObject& obj = **it;

		if (renderGroup != obj.GetRenderGroup())
		{
			// always render simple verts and disable additive at the end of each group
			g_render->RenderSimpleVerts();
			g_render->SetSimpleVertsAreAdditive(false);
		}

		renderGroup = obj.GetRenderGroup();

		obj.RenderPost();
	}

	g_render->RenderSimpleVerts();
	g_render->SetSimpleVertsAreAdditive(false);
}

void GameObjectManager::RemoveAll()
{
	// do a normal flush to get rid of all the game objects
	Reset();

	lockDeleteObjects = false;
	for (GameObjectHashTable::iterator it = objects.begin(); it != objects.end(); ++it)
		delete (*it).second;
	objects.clear();
	lockDeleteObjects = true;

	// clear render objects
	sortedRenderObjects.clear();
}

void GameObjectManager::Reset()
{
	// clear render objects because the objects may be deleted
	sortedRenderObjects.clear();
	
	// remove all objects
	lockDeleteObjects = false;
	for (GameObjectHashTable::iterator it = objects.begin(); it != objects.end();)
	{
		GameObject& obj = (*(*it).second);

		if (obj.DestroyOnWorldReset())
		{
			ASSERT(!obj.HasParent() || obj.GetParent()->DestroyOnWorldReset());
			obj.Destroy();
		}

		if (obj.IsDestroyed())
		{	
			ASSERT(!obj.parent && obj.children.empty());
			it = objects.erase(it);
			delete &obj;
		} 
		else
			++it;
	}
	lockDeleteObjects = true;
}


list<GameObject*> GameObjectManager::GetObjects(const Vector2& pos, float radius, bool skipChildern)
{
	list<GameObject*> results;

	const float radius2 = Square(radius);
	if (skipChildern)
	{
		for (GameObjectHashTable::iterator it = objects.begin(); it != objects.end(); ++it)
		{
			GameObject& obj = (*(*it).second);

			if (obj.WasJustAdded() || obj.GetParent())
				continue;

			if ((pos - obj.GetPosWorld()).MagnitudeSquared() < radius2)
				results.push_back(&obj);
		}
	}
	else
	{
		for (GameObjectHashTable::iterator it = objects.begin(); it != objects.end(); ++it)
		{
			GameObject& obj = (*(*it).second);

			if (obj.WasJustAdded())
				continue;
			
			if ((pos - obj.GetPosWorld()).MagnitudeSquared() < radius2)
				results.push_back(&obj);
		}
	}

	return results;
}