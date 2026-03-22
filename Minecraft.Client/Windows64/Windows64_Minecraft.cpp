// Minecraft.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include <assert.h>
#include "GameConfig\Minecraft.spa.h"
#include "..\MinecraftServer.h"
#include "..\LocalPlayer.h"
#include "..\..\Minecraft.World\ItemInstance.h"
#include "..\..\Minecraft.World\MapItem.h"
#include "..\..\Minecraft.World\Recipes.h"
#include "..\..\Minecraft.World\Recipy.h"
#include "..\..\Minecraft.World\Language.h"
#include "..\..\Minecraft.World\StringHelpers.h"
#include "..\..\Minecraft.World\AABB.h"
#include "..\..\Minecraft.World\Vec3.h"
#include "..\..\Minecraft.World\Level.h"
#include "..\..\Minecraft.World\net.minecraft.world.level.tile.h"
#include "..\UserData_Info.h"

#include "..\ClientConnection.h"
#include "..\User.h"
#include "..\..\Minecraft.World\Socket.h"
#include "..\..\Minecraft.World\ThreadName.h"
#include "..\..\Minecraft.Client\StatsCounter.h"
#include "..\ConnectScreen.h"
//#include "Social\SocialManager.h"
//#include "Leaderboards\LeaderboardManager.h"
//#include "XUI\XUI_Scene_Container.h"
//#include "NetworkManager.h"
#include "..\..\Minecraft.Client\Tesselator.h"
#include "..\..\Minecraft.Client\Options.h"
#include "Sentient\SentientManager.h"
#include "..\..\Minecraft.World\IntCache.h"
#include "..\Textures.h"
#include "Resource.h"
#include "..\..\Minecraft.World\compression.h"
#include "..\..\Minecraft.World\OldChunkStorage.h"

// Force dedicated GPU on NVIDIA Optimus and AMD Dual Graphics systems.
// When pAdapter is NULL in D3D11CreateDeviceAndSwapChain, Windows picks the
// default adapter which is usually the integrated GPU on laptops. These exports
// are read by the NVIDIA and AMD drivers at load time and override that default.
extern "C" { __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }
extern "C" { __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1; }

HINSTANCE hMyInst;
LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
char chGlobalText[256];
uint16_t ui16GlobalText[256];

#define THEME_NAME		"584111F70AAAAAAA"
#define THEME_FILESIZE	2797568

#define FIFTY_ONE_MB (1000000*51)

#define NUM_PROFILE_VALUES	5
#define NUM_PROFILE_SETTINGS 4
DWORD dwProfileSettingsA[NUM_PROFILE_VALUES]=
{
#ifdef _XBOX
	XPROFILE_OPTION_CONTROLLER_VIBRATION,
	XPROFILE_GAMER_YAXIS_INVERSION,
	XPROFILE_GAMER_CONTROL_SENSITIVITY,
	XPROFILE_GAMER_ACTION_MOVEMENT_CONTROL,
	XPROFILE_TITLE_SPECIFIC1,
#else
	0,0,0,0,0
#endif
};

BOOL g_bWidescreen = TRUE;

int g_iScreenWidth = 1920;
int g_iScreenHeight = 1080;

void DefineActions(void)
{
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_A,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_B,							_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_X,							_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_Y,							_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OK,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_CANCEL,						_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_UP,							_360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_DOWN,						_360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_LEFT,						_360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_RIGHT,						_360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_PAGEUP,						_360_JOY_BUTTON_LT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_PAGEDOWN,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_RIGHT_SCROLL,				_360_JOY_BUTTON_RB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_LEFT_SCROLL,					_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_PAUSEMENU,					_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_STICK_PRESS,					_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OTHER_STICK_PRESS,			_360_JOY_BUTTON_RTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OTHER_STICK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OTHER_STICK_DOWN,			_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OTHER_STICK_LEFT,			_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,ACTION_MENU_OTHER_STICK_RIGHT,			_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_JUMP,					_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_FORWARD,				_360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_BACKWARD,				_360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LEFT,					_360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_RIGHT,					_360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LOOK_LEFT,				_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LOOK_RIGHT,				_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LOOK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LOOK_DOWN,				_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_USE,					_360_JOY_BUTTON_LT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_ACTION,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_RIGHT_SCROLL,			_360_JOY_BUTTON_RB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_LEFT_SCROLL,			_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_INVENTORY,				_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_PAUSEMENU,				_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_DROP,					_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_SNEAK_TOGGLE,			_360_JOY_BUTTON_RTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_CRAFTING,				_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_RENDER_THIRD_PERSON,	_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_GAME_INFO,				_360_JOY_BUTTON_BACK);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_DPAD_LEFT,				_360_JOY_BUTTON_DPAD_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_DPAD_RIGHT,				_360_JOY_BUTTON_DPAD_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_DPAD_UP,				_360_JOY_BUTTON_DPAD_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_0,MINECRAFT_ACTION_DPAD_DOWN,				_360_JOY_BUTTON_DPAD_DOWN);

	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_A,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_B,							_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_X,							_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_Y,							_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OK,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_CANCEL,						_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_UP,							_360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_DOWN,						_360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_LEFT,						_360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_RIGHT,						_360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_PAGEUP,						_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_PAGEDOWN,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_RIGHT_SCROLL,				_360_JOY_BUTTON_RB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_LEFT_SCROLL,					_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_PAUSEMENU,					_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_STICK_PRESS,					_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OTHER_STICK_PRESS,			_360_JOY_BUTTON_RTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OTHER_STICK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OTHER_STICK_DOWN,			_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OTHER_STICK_LEFT,			_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,ACTION_MENU_OTHER_STICK_RIGHT,			_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_JUMP,					_360_JOY_BUTTON_RB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_FORWARD,				_360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_BACKWARD,				_360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LEFT,					_360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_RIGHT,					_360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LOOK_LEFT,				_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LOOK_RIGHT,				_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LOOK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LOOK_DOWN,				_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_USE,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_ACTION,					_360_JOY_BUTTON_LT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_RIGHT_SCROLL,			_360_JOY_BUTTON_DPAD_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_LEFT_SCROLL,			_360_JOY_BUTTON_DPAD_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_INVENTORY,				_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_PAUSEMENU,				_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_DROP,					_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_SNEAK_TOGGLE,			_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_CRAFTING,				_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_RENDER_THIRD_PERSON,	_360_JOY_BUTTON_RTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_GAME_INFO,				_360_JOY_BUTTON_BACK);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_DPAD_LEFT,				_360_JOY_BUTTON_DPAD_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_DPAD_RIGHT,				_360_JOY_BUTTON_DPAD_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_DPAD_UP,				_360_JOY_BUTTON_DPAD_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_1,MINECRAFT_ACTION_DPAD_DOWN,				_360_JOY_BUTTON_DPAD_DOWN);

	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_A,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_B,							_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_X,							_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_Y,							_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OK,							_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_CANCEL,						_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_UP,							_360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_DOWN,						_360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_LEFT,						_360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_RIGHT,						_360_JOY_BUTTON_DPAD_RIGHT | _360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_PAGEUP,						_360_JOY_BUTTON_DPAD_UP | _360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_PAGEDOWN,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_RIGHT_SCROLL,				_360_JOY_BUTTON_RB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_LEFT_SCROLL,					_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_JUMP,					_360_JOY_BUTTON_LT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_FORWARD,				_360_JOY_BUTTON_LSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_BACKWARD,				_360_JOY_BUTTON_LSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LEFT,					_360_JOY_BUTTON_LSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_RIGHT,					_360_JOY_BUTTON_LSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LOOK_LEFT,				_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LOOK_RIGHT,				_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LOOK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LOOK_DOWN,				_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_USE,					_360_JOY_BUTTON_RT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_ACTION,					_360_JOY_BUTTON_A);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_RIGHT_SCROLL,			_360_JOY_BUTTON_DPAD_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_LEFT_SCROLL,			_360_JOY_BUTTON_DPAD_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_INVENTORY,				_360_JOY_BUTTON_Y);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_PAUSEMENU,				_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_DROP,					_360_JOY_BUTTON_B);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_SNEAK_TOGGLE,			_360_JOY_BUTTON_LB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_CRAFTING,				_360_JOY_BUTTON_X);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_RENDER_THIRD_PERSON,	_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_GAME_INFO,				_360_JOY_BUTTON_BACK);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_PAUSEMENU,					_360_JOY_BUTTON_START);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_STICK_PRESS,					_360_JOY_BUTTON_LTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OTHER_STICK_PRESS,			_360_JOY_BUTTON_RTHUMB);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OTHER_STICK_UP,				_360_JOY_BUTTON_RSTICK_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OTHER_STICK_DOWN,			_360_JOY_BUTTON_RSTICK_DOWN);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OTHER_STICK_LEFT,			_360_JOY_BUTTON_RSTICK_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,ACTION_MENU_OTHER_STICK_RIGHT,			_360_JOY_BUTTON_RSTICK_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_DPAD_LEFT,				_360_JOY_BUTTON_DPAD_LEFT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_DPAD_RIGHT,				_360_JOY_BUTTON_DPAD_RIGHT);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_DPAD_UP,				_360_JOY_BUTTON_DPAD_UP);
	InputManager.SetGameJoypadMaps(MAP_STYLE_2,MINECRAFT_ACTION_DPAD_DOWN,				_360_JOY_BUTTON_DPAD_DOWN);
}

#if 0
HRESULT InitD3D( IDirect3DDevice9 **ppDevice, D3DPRESENT_PARAMETERS *pd3dPP ) { return S_OK; }
#endif

//#define MEMORY_TRACKING

#ifdef MEMORY_TRACKING
void ResetMem();
void DumpMem();
void MemPixStuff();
#else
void MemSect(int sect) {}
#endif

HINSTANCE               g_hInst = NULL;
HWND                    g_hWnd = NULL;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = NULL;
ID3D11DeviceContext*    g_pImmediateContext = NULL;
IDXGISwapChain*         g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;
ID3D11DepthStencilView* g_pDepthStencilView = NULL;
ID3D11Texture2D*		g_pDepthStencilBuffer = NULL;
static bool             g_bMouseLocked = false;
static int              g_iMouseLockCenterX = 0;
static int              g_iMouseLockCenterY = 0;
static float            g_fMouseDeltaRemainderX = 0.0f;
static float            g_fMouseDeltaRemainderY = 0.0f;

static void UpdateMouseLockCenter()
{
	if (g_hWnd == NULL) return;
	RECT clientRect;
	GetClientRect(g_hWnd, &clientRect);
	g_iMouseLockCenterX = (clientRect.right - clientRect.left) / 2;
	g_iMouseLockCenterY = (clientRect.bottom - clientRect.top) / 2;
}

static void ConvertClientToRenderCoords(HWND hWnd, int &x, int &y)
{
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	const int clientW = clientRect.right - clientRect.left;
	const int clientH = clientRect.bottom - clientRect.top;
	if (clientW <= 0 || clientH <= 0) return;
	if (x < 0) x = 0; if (y < 0) y = 0;
	if (x > clientW) x = clientW; if (y > clientH) y = clientH;
	x = (x * g_iScreenWidth) / clientW;
	y = (y * g_iScreenHeight) / clientH;
	if (x < 0) x = 0; if (y < 0) y = 0;
	if (x >= g_iScreenWidth) x = g_iScreenWidth - 1;
	if (y >= g_iScreenHeight) y = g_iScreenHeight - 1;
}

static void SetCursorVisible(bool visible)
{
	if (visible) { while (ShowCursor(TRUE) < 0) {} }
	else         { while (ShowCursor(FALSE) >= 0) {} }
}

static void SetMouseClip(bool locked)
{
	if (!locked) { ClipCursor(NULL); return; }
	if (g_hWnd == NULL) return;
	RECT clientRect;
	if (!GetClientRect(g_hWnd, &clientRect)) return;
	POINT topLeft = { clientRect.left, clientRect.top };
	POINT bottomRight = { clientRect.right, clientRect.bottom };
	ClientToScreen(g_hWnd, &topLeft);
	ClientToScreen(g_hWnd, &bottomRight);
	RECT clipRect = { topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
	ClipCursor(&clipRect);
}

static void SetMouseLocked(bool locked)
{
	if (g_hWnd == NULL) { g_bMouseLocked = locked; return; }
	if (locked)
	{
		UpdateMouseLockCenter();
		POINT center = { g_iMouseLockCenterX, g_iMouseLockCenterY };
		ClientToScreen(g_hWnd, &center);
		SetCursorPos(center.x, center.y);
		int centerX = g_iMouseLockCenterX;
		int centerY = g_iMouseLockCenterY;
		ConvertClientToRenderCoords(g_hWnd, centerX, centerY);
		Mouse::onMouseMove(centerX, centerY);
	}
	if (g_bMouseLocked == locked) return;
	g_bMouseLocked = locked;
	if (locked) SetCapture(g_hWnd);
	else if (GetCapture() == g_hWnd) ReleaseCapture();
	SetMouseClip(locked);
	SetCursorVisible(!locked);
}

static int TranslateWinKey(WPARAM wParam, LPARAM lParam)
{
	int key = (int)wParam;
	if (key == VK_SHIFT)
	{
		UINT scanCode = (UINT)((lParam >> 16) & 0xFF);
		int translated = (int)MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);
		if (translated != 0) key = translated;
	}
	else if (key == VK_CONTROL)
	{
		key = (lParam & 0x01000000) ? VK_RCONTROL : VK_LCONTROL;
	}
	return key;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		int key = TranslateWinKey(wParam, lParam);
		bool repeat = (lParam & 0x40000000) != 0;
		Keyboard::onKeyDown(key, repeat, 0);
		return 0;
	}
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		int key = TranslateWinKey(wParam, lParam);
		Keyboard::onKeyUp(key);
		return 0;
	}
	case WM_CHAR:
		Keyboard::onChar((wchar_t)wParam);
		return 0;
	case WM_KILLFOCUS:
		SetMouseClip(false);
		if (GetCapture() == hWnd) ReleaseCapture();
		for (int key = 0; key < Keyboard::MAX_KEYS; ++key)
			if (Keyboard::isKeyDown(key)) Keyboard::onKeyUp(key);
		for (int button = 0; button < Mouse::MAX_BUTTONS; ++button)
			if (Mouse::isButtonDown(button)) Mouse::onMouseButton(button, false, Mouse::getX(), Mouse::getY());
		return 0;
	case WM_MOUSEMOVE:
	{
		int x = (short)LOWORD(lParam);
		int y = (short)HIWORD(lParam);
		if (g_bMouseLocked)
		{
			int dx = x - g_iMouseLockCenterX;
			int dy = y - g_iMouseLockCenterY;
			if (dx != 0 || dy != 0)
			{
				RECT clientRect;
				GetClientRect(hWnd, &clientRect);
				const int clientW = clientRect.right - clientRect.left;
				const int clientH = clientRect.bottom - clientRect.top;
				float scaledDX = (float)dx;
				float scaledDY = (float)dy;
				if (clientW > 0 && clientH > 0)
				{
					scaledDX = ((float)dx * (float)g_iScreenWidth) / (float)clientW;
					scaledDY = ((float)dy * (float)g_iScreenHeight) / (float)clientH;
				}
				scaledDX += g_fMouseDeltaRemainderX;
				scaledDY += g_fMouseDeltaRemainderY;
				int outDX = (scaledDX >= 0.0f) ? (int)(scaledDX + 0.5f) : (int)(scaledDX - 0.5f);
				int outDY = (scaledDY >= 0.0f) ? (int)(scaledDY + 0.5f) : (int)(scaledDY - 0.5f);
				g_fMouseDeltaRemainderX = scaledDX - (float)outDX;
				g_fMouseDeltaRemainderY = scaledDY - (float)outDY;
				if (outDX != 0 || outDY != 0) Mouse::onMouseDelta(outDX, outDY);
				POINT center = { g_iMouseLockCenterX, g_iMouseLockCenterY };
				ClientToScreen(hWnd, &center);
				SetCursorPos(center.x, center.y);
			}
			return 0;
		}
		ConvertClientToRenderCoords(hWnd, x, y);
		Mouse::onMouseMove(x, y);
		g_fMouseDeltaRemainderX = 0.0f;
		g_fMouseDeltaRemainderY = 0.0f;
		return 0;
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	{
		int x = (short)LOWORD(lParam);
		int y = (short)HIWORD(lParam);
		bool isDown = (message == WM_LBUTTONDOWN);
		// FIX: No tocar SetCapture/ReleaseCapture cuando el mouse está bloqueado.
		// SetMouseLocked ya gestiona su propio capture. Interferir con él provoca
		// un WM_MOUSEMOVE sintético al soltar que genera un spike de delta enorme.
		if (!g_bMouseLocked)
		{
			if (isDown) SetCapture(hWnd);
			else if (GetCapture() == hWnd) ReleaseCapture();
		}
		ConvertClientToRenderCoords(hWnd, x, y);
		Mouse::onMouseButton(0, isDown, x, y);
		return 0;
	}
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	{
		int x = (short)LOWORD(lParam);
		int y = (short)HIWORD(lParam);
		bool isDown = (message == WM_RBUTTONDOWN);
		// FIX: igual que LMB — no tocar capture cuando el mouse está bloqueado.
		if (!g_bMouseLocked)
		{
			if (isDown) SetCapture(hWnd);
			else if (GetCapture() == hWnd) ReleaseCapture();
		}
		ConvertClientToRenderCoords(hWnd, x, y);
		Mouse::onMouseButton(1, isDown, x, y);
		return 0;
	}
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	{
		int x = (short)LOWORD(lParam);
		int y = (short)HIWORD(lParam);
		bool isDown = (message == WM_MBUTTONDOWN);
		// FIX: igual.
		if (!g_bMouseLocked)
		{
			if (isDown) SetCapture(hWnd);
			else if (GetCapture() == hWnd) ReleaseCapture();
		}
		ConvertClientToRenderCoords(hWnd, x, y);
		Mouse::onMouseButton(2, isDown, x, y);
		return 0;
	}
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	{
		int x = (short)LOWORD(lParam);
		int y = (short)HIWORD(lParam);
		bool isDown = (message == WM_XBUTTONDOWN);
		int button = (HIWORD(wParam) == XBUTTON1) ? 3 : 4;
		// FIX: igual.
		if (!g_bMouseLocked)
		{
			if (isDown) SetCapture(hWnd);
			else if (GetCapture() == hWnd) ReleaseCapture();
		}
		ConvertClientToRenderCoords(hWnd, x, y);
		Mouse::onMouseButton(button, isDown, x, y);
		return 0;
	}
	case WM_MOUSEWHEEL:
	{
		POINT p;
		p.x = (short)LOWORD(lParam);
		p.y = (short)HIWORD(lParam);
		ScreenToClient(hWnd, &p);
		int delta = (short)HIWORD(wParam);
		int x = p.x; int y = p.y;
		ConvertClientToRenderCoords(hWnd, x, y);
		Mouse::onMouseWheel(delta, x, y);
		return 0;
	}
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize         = sizeof(WNDCLASSEX);
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.cbClsExtra     = 0;
	wcex.cbWndExtra     = 0;
	wcex.hInstance      = hInstance;
	wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MINECRAFTWINDOWS));
	wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName   = "Minecraft";
	wcex.lpszClassName  = "MinecraftClass";
	wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	g_hInst = hInstance;
	RECT wr = {0, 0, g_iScreenWidth, g_iScreenHeight};
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow("MinecraftClass", "Minecraft: Legacy Edition", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0,
		wr.right - wr.left, wr.bottom - wr.top,
		NULL, NULL, hInstance, NULL);
	if (!g_hWnd) return FALSE;
	ShowWindow(g_hWnd, nCmdShow);
	UpdateWindow(g_hWnd);
	return TRUE;
}

void ClearGlobalText()
{
	memset(chGlobalText, 0, 256);
	memset(ui16GlobalText, 0, 512);
}

uint16_t *GetGlobalText()
{
	char *pchBuffer = (char *)ui16GlobalText;
	for (int i = 0; i < 256; i++) pchBuffer[i*2] = chGlobalText[i];
	return ui16GlobalText;
}

void SeedEditBox()
{
	// Windows64 build no longer carries the old platform-specific dialog resource.
	// Leave the current seed text intact instead of trying to open a missing dialog.
}

LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch(Msg)
	{
	case WM_INITDIALOG: return TRUE;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			EndDialog(hWndDlg, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

HRESULT InitDevice()
{
	HRESULT hr = S_OK;
	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width  = g_iScreenWidth;
	UINT height = g_iScreenHeight;
	app.DebugPrintf("width: %d, height: %d\n", width, height);

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] = { D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_REFERENCE };
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount                        = 1;
	sd.BufferDesc.Width                   = width;
	sd.BufferDesc.Height                  = height;
	sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator   = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow                       = g_hWnd;
	sd.SampleDesc.Count                   = 1;
	sd.SampleDesc.Quality                 = 0;
	sd.Windowed                           = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL, createDeviceFlags,
			featureLevels, numFeatureLevels, D3D11_SDK_VERSION,
			&sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (HRESULT_SUCCEEDED(hr)) break;
	}
	if (FAILED(hr)) return hr;

	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr)) return hr;

	D3D11_TEXTURE2D_DESC descDepth;
	descDepth.Width              = width;
	descDepth.Height             = height;
	descDepth.MipLevels          = 1;
	descDepth.ArraySize          = 1;
	descDepth.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count   = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage              = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags          = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags     = 0;
	descDepth.MiscFlags          = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, NULL, &g_pDepthStencilBuffer);

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSView;
	descDSView.Format             = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSView.ViewDimension      = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSView.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &descDSView, &g_pDepthStencilView);

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr)) return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	D3D11_VIEWPORT vp;
	vp.Width    = (FLOAT)width;
	vp.Height   = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	RenderManager.Initialise(g_pd3dDevice, g_pSwapChain);
	return S_OK;
}

void Render()
{
	float ClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
	g_pSwapChain->Present(0, 0);
}

void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain)        g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice)        g_pd3dDevice->Release();
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
					   _In_opt_ HINSTANCE hPrevInstance,
					   _In_ LPTSTR    lpCmdLine,
					   _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (lpCmdLine)
	{
		if      (lpCmdLine[0] == '1') { g_iScreenWidth = 1280; g_iScreenHeight = 720; }
		else if (lpCmdLine[0] == '2') { g_iScreenWidth = 640;  g_iScreenHeight = 480; }
		else if (lpCmdLine[0] == '3') { g_iScreenWidth = 720;  g_iScreenHeight = 408; }
	}

	MyRegisterClass(hInstance);
	if (!InitInstance(hInstance, nCmdShow)) return FALSE;
	hMyInst = hInstance;
	if (FAILED(InitDevice())) { CleanupDevice(); return 0; }

	static bool bTrialTimerDisplayed = true;

#ifdef MEMORY_TRACKING
	ResetMem();
	MEMORYSTATUS memStat;
	GlobalMemoryStatus(&memStat);
	printf("RESETMEM start: Avail. phys %d\n", memStat.dwAvailPhys/(1024*1024));
#endif

	app.loadMediaArchive();
	RenderManager.Initialise(g_pd3dDevice, g_pSwapChain);
	app.loadStringTable();
	ui.init(g_pd3dDevice, g_pImmediateContext, g_pRenderTargetView, g_pDepthStencilView, g_iScreenWidth, g_iScreenHeight);

	InputManager.Initialise(1, 3, MINECRAFT_ACTION_MAX, ACTION_MAX_MENU);
	DefineActions();
	InputManager.SetJoypadMapVal(0, 0);
	InputManager.SetKeyRepeatRate(0.3f, 0.2f);

	ProfileManager.Initialise(TITLEID_MINECRAFT,
		app.m_dwOfferID, PROFILE_VERSION_10,
		NUM_PROFILE_VALUES, NUM_PROFILE_SETTINGS, dwProfileSettingsA,
		app.GAME_DEFINED_PROFILE_DATA_BYTES * XUSER_MAX_COUNT,
		&app.uiGameDefinedDataChangedBitmask);

	ProfileManager.SetDefaultOptionsCallback(&CConsoleMinecraftApp::DefaultOptionsCallback, (LPVOID)&app);
	g_NetworkManager.Initialise();
	ProfileManager.SetDebugFullOverride(true);

	Tesselator::CreateNewThreadStorage(1024*1024);
	AABB::CreateNewThreadStorage();
	Vec3::CreateNewThreadStorage();
	IntCache::CreateNewThreadStorage();
	Compression::CreateNewThreadStorage();
	OldChunkStorage::CreateNewThreadStorage();
	Level::enableLightingCache();
	Tile::CreateNewThreadStorage();

	Minecraft::main();
	Minecraft *pMinecraft = Minecraft::GetInstance();

	app.InitGameSettings();
	UserData_Info::EnsureLoaded();
	app.SetPlayerSkin(ProfileManager.GetPrimaryPad(), (DWORD)UserData_Info::GetSkinId());
	app.InitialiseTips();

	pMinecraft->options->set(Options::Option::MUSIC, 1.0f);
	pMinecraft->options->set(Options::Option::SOUND, 1.0f);

	MSG msg = {0};
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}
		RenderManager.StartFrame();

		app.UpdateTime();
		PIXBeginNamedEvent(0,"Input manager tick");   InputManager.Tick();   PIXEndNamedEvent();
		PIXBeginNamedEvent(0,"Profile manager tick"); /*ProfileManager.Tick();*/ PIXEndNamedEvent();
		PIXBeginNamedEvent(0,"Storage manager tick"); StorageManager.Tick(); PIXEndNamedEvent();
		PIXBeginNamedEvent(0,"Render manager tick");  RenderManager.Tick();  PIXEndNamedEvent();
		PIXBeginNamedEvent(0,"Social network manager tick"); /*CSocialManager::Instance()->Tick();*/ PIXEndNamedEvent();
		PIXBeginNamedEvent(0,"Sentient tick"); MemSect(37); /*SentientManager.Tick();*/ MemSect(0); PIXEndNamedEvent();
		PIXBeginNamedEvent(0,"Network manager do work #1"); /*g_NetworkManager.DoWork();*/ PIXEndNamedEvent();

		if (app.GetGameStarted())
		{
			pMinecraft->run_middle();
			app.SetAppPaused(g_NetworkManager.IsLocalGame() && g_NetworkManager.GetPlayerCount() == 1 && ui.IsPauseMenuDisplayed(ProfileManager.GetPrimaryPad()));
		}
		else
		{
			MemSect(28);
			pMinecraft->soundEngine->tick(NULL, 0.0f);
			MemSect(0);
			pMinecraft->textures->tick(true, false);
			IntCache::Reset();
			if (app.GetReallyChangingSessionType()) pMinecraft->tickAllConnections();
		}

		pMinecraft->soundEngine->playMusicTick();

#ifdef MEMORY_TRACKING
		static bool bResetMemTrack = false;
		static bool bDumpMemTrack  = false;
		MemPixStuff();
		if (bResetMemTrack) { ResetMem(); MEMORYSTATUS ms; GlobalMemoryStatus(&ms); printf("RESETMEM: Avail. phys %d\n",ms.dwAvailPhys/(1024*1024)); bResetMemTrack=false; }
		if (bDumpMemTrack)  { DumpMem();  bDumpMemTrack=false;  MEMORYSTATUS ms; GlobalMemoryStatus(&ms); printf("DUMPMEM: Avail. phys %d\n",ms.dwAvailPhys/(1024*1024)); printf("Renderer used: %d\n",RenderManager.CBuffSize(-1)); }
#endif

		ui.tick();
		ui.render();
		RenderManager.Present();
		ui.CheckMenuDisplayed();

		bool shouldLockMouse = false;
		if ((GetForegroundWindow() == g_hWnd) && app.GetGameStarted() && (pMinecraft != NULL))
			shouldLockMouse = (pMinecraft->getScreen() == NULL) && !ui.GetMenuDisplayed(ProfileManager.GetPrimaryPad());
		SetMouseLocked(shouldLockMouse);

		app.HandleXuiActions();
		Keyboard::tick();
		Mouse::tick();

		if (!ProfileManager.IsFullVersion())
		{
			if (app.GetGameStarted())
			{
				if (app.IsAppPaused()) app.UpdateTrialPausedTimer();
				ui.UpdateTrialTimer(ProfileManager.GetPrimaryPad());
			}
		}
		else
		{
			if (bTrialTimerDisplayed) { ui.ShowTrialTimer(false); bTrialTimerDisplayed = false; }
		}

		Vec3::resetPool();
	}

	SetMouseLocked(false);
	g_pd3dDevice->Release();
}

#ifdef MEMORY_TRACKING

int totalAllocGen = 0;
unordered_map<int,int> allocCounts;
bool trackEnable = false;
bool trackStarted = false;
volatile size_t sizeCheckMin = 1160;
volatile size_t sizeCheckMax = 1160;
volatile int sectCheck = 48;
CRITICAL_SECTION memCS;
DWORD tlsIdx;

LPVOID XMemAlloc(SIZE_T dwSize, DWORD dwAllocAttributes)
{
	if (!trackStarted)
	{
		void *p = XMemAllocDefault(dwSize, dwAllocAttributes);
		totalAllocGen += XMemSizeDefault(p, dwAllocAttributes);
		return p;
	}
	EnterCriticalSection(&memCS);
	void *p = XMemAllocDefault(dwSize + 16, dwAllocAttributes);
	size_t realSize = XMemSizeDefault(p, dwAllocAttributes) - 16;
	if (trackEnable)
	{
		int sect = ((int)TlsGetValue(tlsIdx)) & 0x3f;
		*(((unsigned char *)p)+realSize) = sect;
		if ((realSize >= sizeCheckMin) && (realSize <= sizeCheckMax) && ((sect == sectCheck) || (sectCheck == -1)))
			app.DebugPrintf("Found one\n");
		if (p) { totalAllocGen += realSize; trackEnable = false; int key = (sect<<26)|realSize; allocCounts[key]++; trackEnable = true; }
	}
	LeaveCriticalSection(&memCS);
	return p;
}

void* operator new (size_t size) { return (unsigned char *)XMemAlloc(size,MAKE_XALLOC_ATTRIBUTES(0,FALSE,TRUE,FALSE,0,XALLOC_PHYSICAL_ALIGNMENT_DEFAULT,XALLOC_MEMPROTECT_READWRITE,FALSE,XALLOC_MEMTYPE_HEAP)); }
void  operator delete (void *p)  { XMemFree(p,MAKE_XALLOC_ATTRIBUTES(0,FALSE,TRUE,FALSE,0,XALLOC_PHYSICAL_ALIGNMENT_DEFAULT,XALLOC_MEMPROTECT_READWRITE,FALSE,XALLOC_MEMTYPE_HEAP)); }

void WINAPI XMemFree(PVOID pAddress, DWORD dwAllocAttributes)
{
	if (dwAllocAttributes == 0) dwAllocAttributes = MAKE_XALLOC_ATTRIBUTES(0,FALSE,TRUE,FALSE,0,XALLOC_PHYSICAL_ALIGNMENT_DEFAULT,XALLOC_MEMPROTECT_READWRITE,FALSE,XALLOC_MEMTYPE_HEAP);
	if (!trackStarted) { totalAllocGen -= XMemSizeDefault(pAddress,dwAllocAttributes); XMemFreeDefault(pAddress,dwAllocAttributes); return; }
	EnterCriticalSection(&memCS);
	if (pAddress)
	{
		size_t realSize = XMemSizeDefault(pAddress,dwAllocAttributes) - 16;
		if (trackEnable) { int sect=*(((unsigned char*)pAddress)+realSize); totalAllocGen-=realSize; trackEnable=false; int key=(sect<<26)|realSize; allocCounts[key]--; trackEnable=true; }
		XMemFreeDefault(pAddress, dwAllocAttributes);
	}
	LeaveCriticalSection(&memCS);
}

SIZE_T WINAPI XMemSize(PVOID pAddress, DWORD dwAllocAttributes)
{
	return trackStarted ? XMemSizeDefault(pAddress,dwAllocAttributes)-16 : XMemSizeDefault(pAddress,dwAllocAttributes);
}

void DumpMem()
{
	int totalLeak = 0;
	for (AUTO_VAR(it,allocCounts.begin()); it!=allocCounts.end(); it++)
		if (it->second > 0) { app.DebugPrintf("%d %d %d %d\n",(it->first>>26)&0x3f,it->first&0x03ffffff,it->second,(it->first&0x03ffffff)*it->second); totalLeak+=(it->first&0x03ffffff)*it->second; }
	app.DebugPrintf("Total %d\n",totalLeak);
}

void ResetMem()
{
	if (!trackStarted) { trackEnable=true; trackStarted=true; totalAllocGen=0; InitializeCriticalSection(&memCS); tlsIdx=TlsAlloc(); }
	EnterCriticalSection(&memCS); trackEnable=false; allocCounts.clear(); trackEnable=true; LeaveCriticalSection(&memCS);
}

void MemSect(int section)
{
	unsigned int value = (unsigned int)TlsGetValue(tlsIdx);
	value = (section == 0) ? (value>>6)&0x03ffffff : (value<<6)|section;
	TlsSetValue(tlsIdx, (LPVOID)value);
}

void MemPixStuff()
{
	const int MAX_SECT = 46;
	int totals[MAX_SECT] = {0};
	for (AUTO_VAR(it,allocCounts.begin()); it!=allocCounts.end(); it++)
		if (it->second>0) { int sect=(it->first>>26)&0x3f; totals[sect]+=(it->first&0x03ffffff)*it->second; }
	unsigned int allSectsTotal = 0;
	for (int i=0; i<MAX_SECT; i++) { allSectsTotal+=totals[i]; PIXAddNamedCounter(((float)totals[i])/1024.0f,"MemSect%d",i); }
	PIXAddNamedCounter(((float)allSectsTotal)/4096.0f,"MemSect total pages");
}

#endif
