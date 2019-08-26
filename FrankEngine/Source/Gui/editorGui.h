////////////////////////////////////////////////////////////////////////////////////////
/*
	Editor GUI
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_GUI_H
#define EDITOR_GUI_H

#include "gui/guiBase.h"

class EditorGui : public GuiBase
{
public:

	virtual void Init();
	virtual void OnResetDevice();
	virtual HRESULT Render(float delta);
	virtual void Refresh();
	void NewObjectSelected();
	void Reset();
	void ClearFocus();
	
	const WCHAR* GetHoverHelp();

	bool IsEditBoxFocused();
	void ClearEditBox();
	LPCWSTR GetEditBoxText();

	int GetTileQuickPickRows() const;
	int GetObjectQuickPickRows() const;
	int GetTileQuickPickId();
	int GetObjectQuickPickId();
	static float quickPickSize;
	static int quickPickTileColumns;
	static int quickPickObjectColumns;

	void RenderObjectAttributes();

	GamePauseTimer hoverHelpTimer;
};

#endif // EDITOR_GUI_H