////////////////////////////////////////////////////////////////////////////////////////
/*
	Game GUI
	Copyright 2013 Frank Force - http://www.frankforce.com
*/
////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_GUI_H
#define GAME_GUI_H

class GameGui : public GuiBase
{
public:

	virtual void Init();

	virtual void OnResetDevice();

	virtual HRESULT Render(float delta);

	virtual void Refresh();
	
	void ResetOnScreenText() { onScreenTextTime.Invalidate(); }
	void SetOnScreenText(wstring _text) { onScreenTextString = _text; onScreenTextTime.Set(); }

private:

	void RenderFadeOverlay();

	wstring onScreenTextString;
	GameTimer onScreenTextTime;
};

#endif // GAME_GUI_H