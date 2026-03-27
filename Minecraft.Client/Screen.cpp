#include "stdafx.h"
#include "Screen.h"
#include "Button.h"
#include "GuiParticles.h"
#include "Tesselator.h"
#include "Textures.h"
#include "..\Minecraft.World\SoundTypes.h"



Screen::Screen()	// 4J added
{
	minecraft = NULL;
	width = 0;
    height = 0;
	passEvents = false;
	font = NULL;
	particles = NULL;
	clickedButton = NULL;
	selectedButton = -1;
}

void Screen::render(int xm, int ym, float a)
{
	AUTO_VAR(itEnd, buttons.end());
	for (AUTO_VAR(it, buttons.begin()); it != itEnd; it++)
	{
        Button *button = *it; //buttons[i];
        button->render(minecraft, xm, ym);
    }
}

void Screen::keyPressed(wchar_t eventCharacter, int eventKey)
{
	if (eventKey == Keyboard::KEY_ESCAPE)
	{
		minecraft->setScreen(NULL);
//    minecraft->grabMouse();	// 4J - removed
		return;
	}

	if (eventKey == Keyboard::KEY_TAB)
	{
		tabPressed();
		return;
	}

	if (eventKey == Keyboard::KEY_UP || eventKey == Keyboard::KEY_LEFT || eventKey == Keyboard::KEY_DOWN || eventKey == Keyboard::KEY_RIGHT)
	{
		if (buttons.empty())
		{
			return;
		}

		int dir = (eventKey == Keyboard::KEY_UP || eventKey == Keyboard::KEY_LEFT) ? -1 : 1;
		int idx = selectedButton;
		if (idx < 0 || idx >= (int)buttons.size())
		{
			idx = (dir > 0) ? -1 : 0;
		}

		for (unsigned int i = 0; i < buttons.size(); ++i)
		{
			idx += dir;
			if (idx < 0) idx = (int)buttons.size() - 1;
			if (idx >= (int)buttons.size()) idx = 0;

			Button *button = buttons[idx];
			if (button != NULL && button->visible && button->active)
			{
				selectedButton = idx;
				break;
			}
		}
		return;
	}

	if (eventKey == Keyboard::KEY_RETURN)
	{
		int idx = selectedButton;
		if (idx < 0 || idx >= (int)buttons.size() || buttons[idx] == NULL || !buttons[idx]->visible || !buttons[idx]->active)
		{
			idx = -1;
			for (unsigned int i = 0; i < buttons.size(); ++i)
			{
				if (buttons[i] != NULL && buttons[i]->visible && buttons[i]->active)
				{
					idx = (int)i;
					break;
				}
			}
			if (idx < 0)
			{
				return;
			}
			selectedButton = idx;
		}

		Button *button = buttons[idx];
		clickedButton = button;
		minecraft->soundEngine->playUI(eSoundType_RANDOM_CLICK, 1, 1);
		buttonClicked(button);
		clickedButton = NULL;
	}
}

wstring Screen::getClipboard()
{
	// 4J - removed
	return NULL;
}

void Screen::setClipboard(const wstring& str)
{
	// 4J - removed
}

void Screen::mouseClicked(int x, int y, int buttonNum)
{
    if (buttonNum == 0)
	{
		for (int i = 0; i < (int)buttons.size(); ++i)
		{
			Button *button = buttons[i];
            if (button->clicked(minecraft, x, y))
			{
                clickedButton = button;
				selectedButton = i;
                minecraft->soundEngine->playUI(eSoundType_RANDOM_CLICK, 1, 1);
                buttonClicked(button);
            }
        }
    }
}

void Screen::mouseReleased(int x, int y, int buttonNum)
{
    if (clickedButton!=NULL && buttonNum==0)
	{
        clickedButton->released(x, y);
        clickedButton = NULL;
    }
}

void Screen::buttonClicked(Button *button)
{
}

void Screen::init(Minecraft *minecraft, int width, int height)
{
    particles = new GuiParticles(minecraft);
    this->minecraft = minecraft;
    this->font = minecraft->font;
    this->width = width;
    this->height = height;
    buttons.clear();
	selectedButton = -1;
    init();

	for (int i = 0; i < (int)buttons.size(); ++i)
	{
		if (buttons[i] != NULL && buttons[i]->visible && buttons[i]->active)
		{
			selectedButton = i;
			break;
		}
	}
}

void Screen::setSize(int width, int height)
{
    this->width = width;
    this->height = height;
}

void Screen::init()
{
}

void Screen::updateEvents()
{
    while (Mouse::next())
	{
        mouseEvent();
    }

    while (Keyboard::next())
	{
        keyboardEvent();
    }

}

void Screen::mouseEvent()
{
	int button = Mouse::getEventButton();
	if (button < 0)
	{
		return;
	}

	int xm = Mouse::getEventX() * width / minecraft->width;
	int ym = height - Mouse::getEventY() * height / minecraft->height - 1;

    if (Mouse::getEventButtonState())
	{
        mouseClicked(xm, ym, button);
    }
	else
	{
        mouseReleased(xm, ym, button);
    }
}

void Screen::keyboardEvent()
{
    if (Keyboard::getEventKeyState())
	{
        keyPressed(Keyboard::getEventCharacter(), Keyboard::getEventKey());
    }
}

void Screen::tick()
{
}

void Screen::removed()
{
}

void Screen::renderBackground()
{
	renderBackground(0);
}

void Screen::renderBackground(int vo)
{
	if (minecraft->level != NULL)
	{
		fillGradient(0, 0, width, height, 0xc0101010, 0xd0101010);
	}
	else
	{
		renderDirtBackground(vo);
	}
}

void Screen::renderDirtBackground(int vo)
{
	// 4J Unused
#if 0
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    Tesselator *t = Tesselator::getInstance();
    glBindTexture(GL_TEXTURE_2D, minecraft->textures->loadTexture(L"/gui/background.png"));
    glColor4f(1, 1, 1, 1);
    float s = 32;
    t->begin();
    t->color(0x404040);
    t->vertexUV((float)(0), (float)( height), (float)( 0), (float)( 0), (float)( height / s + vo));
    t->vertexUV((float)(width), (float)( height), (float)( 0), (float)( width / s), (float)( height / s + vo));
    t->vertexUV((float)(width), (float)( 0), (float)( 0), (float)( width / s), (float)( 0 + vo));
    t->vertexUV((float)(0), (float)( 0), (float)( 0), (float)( 0), (float)( 0 + vo));
    t->end();
#endif
}

bool Screen::isPauseScreen()
{
	return true;
}

void Screen::confirmResult(bool result, int id)
{
}

void Screen::tabPressed()
{
}
