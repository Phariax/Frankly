////////////////////////////////////////////////////////////////////////////////////////
/*
	GUI Base
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GUI_BASE_H
#define GUI_BASE_H

class GuiBase
{
public:
	
	GuiBase() {ASSERT(!g_guiBase);}
	virtual ~GuiBase()	{}
	virtual void Init() {}
	virtual void OnResetDevice() {}
	virtual void OnLostDevice() {}
	virtual HRESULT Render(float delta) { return S_OK; }
	virtual void Refresh() {}

	virtual bool MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
	{ return mainDialog.MsgProc(hWnd, uMsg, wParam, lParam); }

	CDXUTDialog & GetDialog() { return mainDialog; }
	CDXUTDialog mainDialog;
};

#endif // GUI_BASE_H