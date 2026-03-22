#pragma once
#include "Screen.h"
class Button;
class EditBox;
using namespace std;

class RenamePlayerScreen : public Screen
{
private:
	Screen *lastScreen;
    EditBox *nameEdit;

public:
	RenamePlayerScreen(Screen *lastScreen);
    virtual void tick();
    virtual void init() ;
    virtual void removed();
protected:
	virtual void buttonClicked(Button *button);
    virtual void keyPressed(wchar_t ch, int eventKey);
    virtual void mouseClicked(int x, int y, int buttonNum);
public:
	virtual void render(int xm, int ym, float a);
};
