#pragma once

#include "UIScene.h"

class UIComponent_Logo : public UIScene
{
	UIControl m_menuTitle;
	UIControl m_menuTitleSmall;

public:
	UIComponent_Logo(int iPad, void *initData, UILayer *parentLayer);

	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT(m_menuTitle, "MenuTitle")
		UI_MAP_ELEMENT(m_menuTitleSmall, "MenuTitleSmall")
	UI_END_MAP_ELEMENTS_AND_NAMES()

protected:
	// TODO: This should be pure virtual in this class
	virtual wstring getMoviePath();

public:
	virtual EUIScene getSceneType() { return eUIComponent_Logo;}

	// Returns true if this scene handles input
	virtual bool stealsFocus() { return false; }

	// Returns true if this scene has focus for the pad passed in
	virtual bool hasFocus(int iPad) { return false; }

	// Returns true if lower scenes in this scenes layer, or in any layer below this scenes layers should be hidden
	virtual bool hidesLowerScenes() { return false; }

	virtual void render(S32 width, S32 height, C4JRender::eViewportType viewport);
};
