#include "stdafx.h"
#include "RenamePlayerScreen.h"
#include "EditBox.h"
#include "Button.h"
#include "..\..\Minecraft.World\net.minecraft.locale.h"
#include "User.h"
#include "UserData_Info.h"

RenamePlayerScreen::RenamePlayerScreen(Screen *lastScreen)
{
	nameEdit = NULL;
    this->lastScreen = lastScreen;
}

void RenamePlayerScreen::tick()
{
	nameEdit->tick();
}

void RenamePlayerScreen::init() 
{
    Language *language = Language::getInstance();

    Keyboard::enableRepeatEvents(true);
    buttons.clear();
    buttons.push_back(new Button(0, width / 2 - 100, height / 4 + 24 * 4 + 12, L"Aceptar")); // Accept
    buttons.push_back(new Button(1, width / 2 - 100, height / 4 + 24 * 5 + 12, L"Cancelar")); // Cancel

    wstring currentName = UserData_Info::GetPlayerName();

    nameEdit = new EditBox(this, font, width / 2 - 100, 60, 200, 20, currentName);
    nameEdit->focus(true);
    nameEdit->setMaxLength(16);
}

void RenamePlayerScreen::removed()
{
	Keyboard::enableRepeatEvents(false);
}

void RenamePlayerScreen::buttonClicked(Button *button)
{
    if (!button->active) return;
    if (button->id == 1)
	{
		// Cancel
        minecraft->setScreen(lastScreen);
    }
	else if (button->id == 0)
	{
		// Accept
		wstring originalName = nameEdit->getValue();
		wstring filteredName = L"";

		// Filter only alphanumeric characters
		for (size_t i = 0; i < originalName.length(); i++)
		{
			wchar_t c = originalName[i];
			if ((c >= L'A' && c <= L'Z') || 
				(c >= L'a' && c <= L'z') || 
				(c >= L'0' && c <= L'9'))
			{
				filteredName += c;
			}
		}

		if (filteredName.length() > 0)
		{
			UserData_Info::SetPlayerName((wchar_t*)filteredName.c_str());
			UserData_Info::Save();

			if (minecraft != NULL && minecraft->user != NULL)
			{
				minecraft->user->name = UserData_Info::GetPlayerName();
			}
		}

        minecraft->setScreen(lastScreen);
    }
}

void RenamePlayerScreen::keyPressed(wchar_t ch, int eventKey)
{
    nameEdit->keyPressed(ch, eventKey);

	wstring currentValue = nameEdit->getValue();
	bool hasValidChars = false;
	for (size_t i = 0; i < currentValue.length(); ++i)
	{
		wchar_t c = currentValue[i];
		if ((c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9'))
		{
			hasValidChars = true;
			break;
		}
	}

    buttons[0]->active = hasValidChars;

    if (ch == 13) // Enter
	{
        buttonClicked(buttons[0]);
    }
	else if (eventKey == Keyboard::KEY_ESCAPE) // Escape
	{
		buttonClicked(buttons[1]);
	}
}

void RenamePlayerScreen::mouseClicked(int x, int y, int buttonNum)
{
    Screen::mouseClicked(x, y, buttonNum);

    nameEdit->mouseClicked(x, y, buttonNum);
}

void RenamePlayerScreen::render(int xm, int ym, float a)
{
    renderBackground();

    drawCenteredString(font, L"Cambiar Nombre de Jugador", width / 2, height / 4 - 60 + 20, 0xffffff);
    drawString(font, L"Escribe tu nuevo nombre (letras y numeros):", width / 2 - 100, 47, 0xa0a0a0);

    nameEdit->render();

    Screen::render(xm, ym, a);
}
