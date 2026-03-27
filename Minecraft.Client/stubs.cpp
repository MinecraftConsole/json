#include "stdafx.h"

void glReadPixels(int,int, int, int, int, int, ByteBuffer *)
{
}

void glClearDepth(double)
{
}

void glVertexPointer(int, int, int, int)
{
}

void glVertexPointer(int, int, FloatBuffer *)
{
}

void glTexCoordPointer(int, int, int, int)
{
}

void glTexCoordPointer(int, int, FloatBuffer *)
{
}

void glNormalPointer(int, int, int)
{
}

void glNormalPointer(int, ByteBuffer *)
{
}

void glEnableClientState(int)
{
}

void glDisableClientState(int)
{
}

void glColorPointer(int, int, int, int)
{
}

void glColorPointer(int, bool, int, ByteBuffer *)
{
}

void glDrawArrays(int,int,int)
{
}

void glNormal3f(float,float,float)
{
}

void glGenQueriesARB(IntBuffer *)
{
}

void glBeginQueryARB(int,int)
{
}

void glEndQueryARB(int)
{
}

void glGetQueryObjectuARB(int,int,IntBuffer *)
{
}

void glShadeModel(int)
{
}

void glColorMaterial(int,int)
{
}

//1.8.2
void glClientActiveTexture(int)
{
}

void glActiveTexture(int)
{
}

void glFlush()
{
}

void glTexGeni(int,int,int)
{
}

bool Keyboard::m_bEnableKeyRepeat = false;
bool Keyboard::m_bKeyDown[Keyboard::MAX_KEYS] = { false };
bool Keyboard::m_bKeyPressed[Keyboard::MAX_KEYS] = { false };
bool Keyboard::m_bKeyReleased[Keyboard::MAX_KEYS] = { false };
std::deque<Keyboard::KeyboardEvent> Keyboard::m_keyEvents;
int Keyboard::m_iCurrentEventKey = Keyboard::KEY_NONE;
bool Keyboard::m_bCurrentEventDown = false;
wchar_t Keyboard::m_wCurrentEventCharacter = 0;

bool Mouse::m_bButtonDown[Mouse::MAX_BUTTONS] = { false };
bool Mouse::m_bButtonPressed[Mouse::MAX_BUTTONS] = { false };
std::deque<Mouse::MouseEvent> Mouse::m_mouseEvents;
bool Mouse::m_bCurrentEventDown = false;
int Mouse::m_iCurrentEventX = 0;
int Mouse::m_iCurrentEventY = 0;
int Mouse::m_iCurrentEventButton = -1;
int Mouse::m_iCurrentEventWheel = 0;
int Mouse::m_iX = 0;
int Mouse::m_iY = 0;
int Mouse::m_iDeltaX = 0;
int Mouse::m_iDeltaY = 0;
int Mouse::m_iWheelDelta = 0;

void Keyboard::enableRepeatEvents(bool enabled)
{
	m_bEnableKeyRepeat = enabled;
}

void Keyboard::tick()
{
	for (int i = 0; i < MAX_KEYS; ++i)
	{
		m_bKeyPressed[i] = false;
		m_bKeyReleased[i] = false;
	}
}

bool Keyboard::isKeyDown(int key)
{
	return key >= 0 && key < MAX_KEYS && m_bKeyDown[key];
}

bool Keyboard::isKeyPressed(int key)
{
	return key >= 0 && key < MAX_KEYS && m_bKeyPressed[key];
}

bool Keyboard::isKeyReleased(int key)
{
	return key >= 0 && key < MAX_KEYS && m_bKeyReleased[key];
}

bool Keyboard::next()
{
	if (m_keyEvents.empty())
	{
		return false;
	}

	KeyboardEvent event = m_keyEvents.front();
	m_keyEvents.pop_front();

	m_iCurrentEventKey = event.key;
	m_bCurrentEventDown = event.isDown;
	m_wCurrentEventCharacter = event.character;
	return true;
}

wstring Keyboard::getKeyName(int key)
{
	switch (key)
	{
	case KEY_ESCAPE: return L"Esc";
	case KEY_TAB: return L"Tab";
	case KEY_RETURN: return L"Enter";
	case KEY_SPACE: return L"Space";
	case KEY_BACK: return L"Backspace";
	case KEY_LSHIFT: return L"Left Shift";
	case KEY_RSHIFT: return L"Right Shift";
	case KEY_LCONTROL: return L"Left Ctrl";
	case KEY_RCONTROL: return L"Right Ctrl";
	case KEY_LEFT: return L"Left";
	case KEY_RIGHT: return L"Right";
	case KEY_UP: return L"Up";
	case KEY_DOWN: return L"Down";
	case KEY_F1: return L"F1";
	case KEY_F2: return L"F2";
	case KEY_F3: return L"F3";
	case KEY_F4: return L"F4";
	case KEY_F5: return L"F5";
	case KEY_F6: return L"F6";
	case KEY_F7: return L"F7";
	case KEY_F8: return L"F8";
	case KEY_F9: return L"F9";
	case KEY_F10: return L"F10";
	case KEY_F11: return L"F11";
	case KEY_F12: return L"F12";
	default:
		break;
	}

	if (key >= KEY_0 && key <= KEY_9)
	{
		return wstring(1, (wchar_t)key);
	}
	if (key >= KEY_A && key <= KEY_Z)
	{
		return wstring(1, (wchar_t)key);
	}

	wchar_t keyName[32];
	_snwprintf(keyName, 32, L"KEY_%d", key);
	keyName[31] = 0;
	return keyName;
}

void Keyboard::onKeyDown(int key, bool repeat, wchar_t character)
{
	if (key < 0 || key >= MAX_KEYS)
	{
		return;
	}

	bool wasDown = m_bKeyDown[key];
	if (!wasDown)
	{
		m_bKeyPressed[key] = true;
	}
	m_bKeyDown[key] = true;

	if (!repeat || m_bEnableKeyRepeat || !wasDown)
	{
		KeyboardEvent event;
		event.key = key;
		event.isDown = true;
		event.character = character;
		m_keyEvents.push_back(event);
		if (m_keyEvents.size() > 2048)
		{
			m_keyEvents.pop_front();
		}
	}
}

void Keyboard::onKeyUp(int key)
{
	if (key < 0 || key >= MAX_KEYS)
	{
		return;
	}

	bool wasDown = m_bKeyDown[key];
	m_bKeyDown[key] = false;

	if (wasDown)
	{
		m_bKeyReleased[key] = true;
		KeyboardEvent event;
		event.key = key;
		event.isDown = false;
		event.character = 0;
		m_keyEvents.push_back(event);
		if (m_keyEvents.size() > 2048)
		{
			m_keyEvents.pop_front();
		}
	}
}

void Keyboard::onChar(wchar_t ch)
{
	KeyboardEvent event;
	event.key = KEY_NONE;
	event.isDown = true;
	event.character = ch;
	m_keyEvents.push_back(event);
	if (m_keyEvents.size() > 2048)
	{
		m_keyEvents.pop_front();
	}
}

void Mouse::tick()
{
	for (int i = 0; i < MAX_BUTTONS; ++i)
	{
		m_bButtonPressed[i] = false;
	}
	m_iDeltaX = 0;
	m_iDeltaY = 0;
	m_iWheelDelta = 0;
}

bool Mouse::isButtonDown(int button)
{
	return button >= 0 && button < MAX_BUTTONS && m_bButtonDown[button];
}

bool Mouse::isButtonPressed(int button)
{
	return button >= 0 && button < MAX_BUTTONS && m_bButtonPressed[button];
}

bool Mouse::next()
{
	if (m_mouseEvents.empty())
	{
		return false;
	}

	MouseEvent event = m_mouseEvents.front();
	m_mouseEvents.pop_front();

	m_iCurrentEventX = event.x;
	m_iCurrentEventY = event.y;
	m_iCurrentEventButton = event.button;
	m_bCurrentEventDown = event.isDown;
	m_iCurrentEventWheel = event.wheel;
	return true;
}

void Mouse::onMouseMove(int x, int y)
{
	m_iDeltaX += (x - m_iX);
	m_iDeltaY += (y - m_iY);
	m_iX = x;
	m_iY = y;
}

void Mouse::onMouseDelta(int dx, int dy)
{
	m_iDeltaX += dx;
	m_iDeltaY += dy;
}

void Mouse::onMouseButton(int button, bool isDown, int x, int y)
{
	onMouseMove(x, y);

	if (button >= 0 && button < MAX_BUTTONS)
	{
		if (isDown && !m_bButtonDown[button])
		{
			m_bButtonPressed[button] = true;
		}
		m_bButtonDown[button] = isDown;
	}

	MouseEvent event;
	event.x = m_iX;
	event.y = m_iY;
	event.button = button;
	event.isDown = isDown;
	event.wheel = 0;
	m_mouseEvents.push_back(event);
	if (m_mouseEvents.size() > 2048)
	{
		m_mouseEvents.pop_front();
	}
}

void Mouse::onMouseWheel(int delta, int x, int y)
{
	onMouseMove(x, y);

	int wheelSteps = 0;
#ifdef WHEEL_DELTA
	if (delta != 0)
	{
		wheelSteps = delta / WHEEL_DELTA;
		if (wheelSteps == 0)
		{
			wheelSteps = (delta > 0) ? 1 : -1;
		}
	}
#else
	wheelSteps = (delta > 0) ? 1 : (delta < 0 ? -1 : 0);
#endif

	if (wheelSteps != 0)
	{
		m_iWheelDelta += wheelSteps;
		MouseEvent event;
		event.x = m_iX;
		event.y = m_iY;
		event.button = -1;
		event.isDown = false;
		event.wheel = wheelSteps;
		m_mouseEvents.push_back(event);
		if (m_mouseEvents.size() > 2048)
		{
			m_mouseEvents.pop_front();
		}
	}
}

#ifdef _XBOX
// 4J Stu - Added these to stop us needing to pull in loads of media libraries just to use Qnet
#include <xcam.h>
DWORD XCamInitialize(){ return 0; }
VOID XCamShutdown() {}
 
DWORD XCamCreateStreamEngine(
         CONST XCAM_STREAM_ENGINE_INIT_PARAMS *pParams,
         PIXCAMSTREAMENGINE *ppEngine
		 ) { return 0; }
 
DWORD XCamSetView(
         XCAMZOOMFACTOR ZoomFactor,
         LONG XCenter,
         LONG YCenter,
         PXOVERLAPPED pOverlapped
) { return 0; }
 
XCAMDEVICESTATE XCamGetStatus() { return XCAMDEVICESTATE_DISCONNECTED; }
#endif
