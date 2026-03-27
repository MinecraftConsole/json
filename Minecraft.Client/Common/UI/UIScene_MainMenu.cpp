#include "stdafx.h"
#include "..\..\..\Minecraft.World\Mth.h"
#include "..\..\..\Minecraft.World\StringHelpers.h"
#include "..\..\..\Minecraft.World\Random.h"
#include "..\..\CustomGenericButton.h"
#include "..\..\CustomSlider.h"
#include "..\..\User.h"
#include "..\..\UserData_Info.h"
#include "..\..\Windows64Media\strings.h"
#include "..\..\MinecraftServer.h"
#include "..\..\stubs.h"
#include "UIControl_PlayerSkinPreview.h"
#include "..\..\RenamePlayerScreen.h"
#if defined(_WINDOWS64)
#include "..\..\BufferedImage.h"
#include "..\..\Tesselator.h"
#ifndef GL_SCISSOR_TEST
#define GL_SCISSOR_TEST 0x0C11
#endif
#endif
#include "UI.h"
#include "UIScene_MainMenu.h"

Random *UIScene_MainMenu::random = new Random();

#if defined(_WINDOWS64)
namespace
{
	// FIX: Primary and fallback were both the same path. Give fallback a distinct name
	// so it can actually serve as a real fallback if the primary is missing.
	const wchar_t *kMainMenuLogoFilePrimary  = L"/Graphics/MenuTitle.png";
	const wchar_t *kMainMenuLogoFileFallback = L"/Graphics/MenuTitle_fallback.png";

	int g_mainMenuLogoWidth = 0;
	int g_mainMenuLogoHeight = 0;
	int g_mainMenuLogoTextureId = -1;
	bool g_mainMenuLogoChecked = false;
	bool g_mainMenuLogoReady = false;
	UIControl_PlayerSkinPreview *g_mainMenuSkinPreview = NULL;
	CustomGenericButton g_mainMenuChangeNameButton;
	const float kMainMenuPreviewX = 60.0f;
	const float kMainMenuPreviewY = 65.0f;
	const float kMainMenuChangeNameButtonX = 85.0f;
	const float kMainMenuChangeNameButtonY = 520.0f;
	bool g_changeNameButtonInitialized = false;

	// ── 6 Custom Menu Buttons (same order as SFW) ──────────────
	const int    kCustomMenuButtonCount  = 6;
	const float  kCustomMenuButtonWidth  = 480.0f;
	const float  kCustomMenuButtonHeight = 40.0f;
	const float  kCustomMenuButtonGap    = 15.0f;
	const float  kCustomMenuButtonX      = 400.0f;
	const float  kCustomMenuButtonStartY = 260.0f;

	CustomGenericButton g_customMenuButtons[kCustomMenuButtonCount];
	bool g_customMenuButtonsInitialized = false;

	// ── TEST SLIDER ─────────────────────────────────────────────
	CustomSlider g_testSlider;
	bool g_testSliderInited = false;

	enum ECustomMenuButton
	{
		eCMB_PlayGame = 0,
		eCMB_Leaderboards,
		eCMB_Achievements,
		eCMB_HelpAndOptions,
		eCMB_ChangeSkin,    // "Cambiar Aspecto"
		eCMB_Exit,
	};

	void SetupCustomMenuButtons()
	{
		if(g_customMenuButtonsInitialized) return;
		g_customMenuButtonsInitialized = true;

		for(int i = 0; i < kCustomMenuButtonCount; ++i)
		{
			float y = kCustomMenuButtonStartY + (float)i * (kCustomMenuButtonHeight + kCustomMenuButtonGap);
			g_customMenuButtons[i].SetupMenuButton(kCustomMenuButtonX, y);
		}
	}

	void DrawCustomMenuButtons(Minecraft *minecraft, C4JRender::eViewportType viewport)
	{
		if(minecraft == NULL || minecraft->textures == NULL || minecraft->font == NULL) return;
		SetupCustomMenuButtons();

		const wchar_t *labels[kCustomMenuButtonCount] = {
			app.GetString(IDS_PLAY_GAME),
			app.GetString(IDS_LEADERBOARDS),
			app.GetString(IDS_ACHIEVEMENTS),
			app.GetString(IDS_HELP_AND_OPTIONS),
			L"Cambiar Aspecto",
			app.GetString(IDS_EXIT_GAME),
		};

		for(int i = 0; i < kCustomMenuButtonCount; ++i)
		{
			g_customMenuButtons[i].Render(minecraft, minecraft->font, labels[i], (int)viewport);
		}
	}

	// Flag to enable the standard SWF/Iggy UI.
	// When true  → the Iggy flash UI (splash text, buttons, overlays) is drawn.
	// When false → only the custom PNG elements are drawn (logo, skin preview, change-name button).
	bool g_bEnableSFW = false;

	float GetMainMenuPreviewHeight(float screenHeight)
	{
		return min(screenHeight * 0.58f, 420.0f);
	}

	wstring GetMainMenuChangeNameButtonLabel()
	{
		LPCWSTR buttonLabel = app.GetString(IDS_CHANGE_NAME_BUTTON);
		if(buttonLabel != NULL && buttonLabel[0] != 0)
		{
			return buttonLabel;
		}
		return L"Cambiar Nombre";
	}

	void EnsureMainMenuLogoTexture(Minecraft *minecraft)
	{
		if(g_mainMenuLogoChecked)
		{
			return;
		}
		g_mainMenuLogoChecked = true;

		g_mainMenuLogoWidth = 512;
		g_mainMenuLogoHeight = 128;
		g_mainMenuLogoReady = false;

		const wchar_t *paths[] =
		{
			kMainMenuLogoFilePrimary,
			kMainMenuLogoFileFallback,
			NULL
		};

		for(int i = 0; paths[i] != NULL && !g_mainMenuLogoReady; ++i)
		{
			BufferedImage *image = new BufferedImage(paths[i]);
			if(image != NULL && image->getData() != NULL && image->getWidth() > 0 && image->getHeight() > 0)
			{
				g_mainMenuLogoWidth = image->getWidth();
				g_mainMenuLogoHeight = image->getHeight();
				g_mainMenuLogoTextureId = minecraft->textures->getTexture(image, C4JRender::TEXTURE_FORMAT_RxGyBzAw, false);
				g_mainMenuLogoReady = (g_mainMenuLogoTextureId >= 0);
			}
			delete image;
		}
	}

	void DrawMainMenuLogo(Minecraft *minecraft, S32 width, S32 height, C4JRender::eViewportType viewport)
	{
		EnsureMainMenuLogoTexture(minecraft);
		if(!g_mainMenuLogoReady || g_mainMenuLogoWidth <= 0 || g_mainMenuLogoHeight <= 0)
		{
			return;
		}

		const float aspect = (float)g_mainMenuLogoWidth / (float)g_mainMenuLogoHeight;
		float drawWidth = (float)width * 0.676f;
		float drawHeight = drawWidth / aspect;
		const float maxHeight = (float)height * 0.234f;
		if(drawHeight > maxHeight)
		{
			drawHeight = maxHeight;
			drawWidth = drawHeight * aspect;
		}

		const int x = (int)(((float)width - drawWidth) * 0.5f);
		const int y = (int)((float)height * 0.045f);

		ui.setupRenderPosition(viewport);

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glDisable(GL_FOG);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.0f);
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0, (double)width, (double)height, 0.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		minecraft->textures->bind(g_mainMenuLogoTextureId);
		Tesselator *t = Tesselator::getInstance();
		t->begin();
		t->color(0xffffff);
		t->vertexUV((float)x,                 (float)(y + drawHeight), 0.0f, 0.0f, 1.0f);
		t->vertexUV((float)(x + drawWidth),   (float)(y + drawHeight), 0.0f, 1.0f, 1.0f);
		t->vertexUV((float)(x + drawWidth),   (float)y,                0.0f, 1.0f, 0.0f);
		t->vertexUV((float)x,                 (float)y,                0.0f, 0.0f, 0.0f);
		t->end();

		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}

	void DrawMainMenuChangeNameButton(Minecraft *minecraft, S32 width, S32 height, C4JRender::eViewportType viewport)
	{
		if(minecraft == NULL || minecraft->textures == NULL || minecraft->font == NULL) return;
		if(!g_changeNameButtonInitialized)
		{
			g_mainMenuChangeNameButton.Setup(NULL, NULL,
				kMainMenuChangeNameButtonX, kMainMenuChangeNameButtonY,
				200.0f, 40.0f);
			g_changeNameButtonInitialized = true;
		}
		g_mainMenuChangeNameButton.Render(minecraft, minecraft->font, GetMainMenuChangeNameButtonLabel(), (int)viewport);
	}

	UIControl_PlayerSkinPreview *EnsureMainMenuSkinPreview()
	{
		if(g_mainMenuSkinPreview == NULL)
		{
			g_mainMenuSkinPreview = new UIControl_PlayerSkinPreview();
			g_mainMenuSkinPreview->SetStandingPose(true);
			g_mainMenuSkinPreview->SetAutoRotate(false);
		}
		return g_mainMenuSkinPreview;
	}

	void DrawMainMenuSkinPreview(Minecraft *minecraft, S32 width, S32 height, C4JRender::eViewportType viewport)
	{
		if(minecraft == NULL || minecraft->textures == NULL || minecraft->options == NULL)
		{
			return;
		}

		UIControl_PlayerSkinPreview *preview = EnsureMainMenuSkinPreview();
		if(preview == NULL)
		{
			return;
		}

		const int previewPad = ProfileManager.GetPrimaryPad();
		const DWORD skinId = app.GetPlayerSkinId(previewPad);
		const wstring skinPath = app.GetPlayerSkinName(previewPad);
		const wstring capePath = app.GetPlayerCapeName(previewPad);
		const wstring safeSkinPath = (skinPath.empty() ? app.getSkinPathFromId(skinId) : skinPath);
		TEXTURE_NAME backupTexture = TN_MOB_CHAR;

		switch(skinId)
		{
		case eDefaultSkins_ServerSelected:
		case eDefaultSkins_Skin0:
			backupTexture = TN_MOB_CHAR;
			break;
		case eDefaultSkins_Skin1:
			backupTexture = TN_MOB_CHAR1;
			break;
		case eDefaultSkins_Skin2:
			backupTexture = TN_MOB_CHAR2;
			break;
		case eDefaultSkins_Skin3:
			backupTexture = TN_MOB_CHAR3;
			break;
		case eDefaultSkins_Skin4:
			backupTexture = TN_MOB_CHAR4;
			break;
		case eDefaultSkins_Skin5:
			backupTexture = TN_MOB_CHAR5;
			break;
		case eDefaultSkins_Skin6:
			backupTexture = TN_MOB_CHAR6;
			break;
		case eDefaultSkins_Skin7:
			backupTexture = TN_MOB_CHAR7;
			break;
		}

		preview->SetTexture(safeSkinPath.empty() ? L"default" : safeSkinPath, backupTexture);
		preview->SetCapeTexture(capePath);
		preview->SetStandingPose(true);
		const float previewX = kMainMenuPreviewX;
		const float previewY = kMainMenuPreviewY;
		const float previewWidth = min((float)width * 0.18f, 240.0f);
		const float previewHeight = min((float)height * 0.58f, 420.0f);

		const float centerX = previewX + (previewWidth * 0.5f);
		const float centerY = previewY + (previewHeight * 0.36f);

		float normX = (centerX - (float)Mouse::getX()) / (previewWidth * 0.65f);
		float normY = (centerY - (float)Mouse::getY()) / (previewHeight * 0.55f);

		if(normX > 1.0f) normX = 1.0f;
		else if(normX < -1.0f) normX = -1.0f;

		if(normY > 1.0f) normY = 1.0f;
		else if(normY < -1.0f) normY = -1.0f;

		preview->SetRotation((int)(normY * 20.0f), (int)(normX * 45.0f));

		ui.setupRenderPosition(viewport);

		// Use a stable render path for the preview in the custom-draw splash.
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glClear(GL_DEPTH_BUFFER_BIT);
		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, 0.1f);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0, (double)width, (double)height, 0.0, -200.0, 200.0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		preview->renderScreenRect(previewX, previewY, previewWidth, previewHeight);

		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glDisable(GL_LIGHTING);
		glDisable(GL_RESCALE_NORMAL);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	}
}
#endif

UIScene_MainMenu::UIScene_MainMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{

	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	parentLayer->addComponent(iPad,eUIComponent_Panorama);
#if !defined(_WINDOWS64)
	parentLayer->addComponent(iPad,eUIComponent_Logo);
#endif

	m_eAction=eAction_None;
	m_bIgnorePress=false;


	m_buttons[(int)eControl_PlayGame].init(app.GetString(IDS_PLAY_GAME),eControl_PlayGame);

#ifdef _XBOX_ONE
	if(!ProfileManager.IsFullVersion()) m_buttons[(int)eControl_PlayGame].setLabel(app.GetString(IDS_PLAY_TRIAL_GAME));
	app.SetReachedMainMenu();
#endif

	m_buttons[(int)eControl_Leaderboards].init(app.GetString(IDS_LEADERBOARDS),eControl_Leaderboards);
	m_buttons[(int)eControl_Achievements].init(app.GetString(IDS_ACHIEVEMENTS),eControl_Achievements);
	m_buttons[(int)eControl_HelpAndOptions].init(app.GetString(IDS_HELP_AND_OPTIONS),eControl_HelpAndOptions);
	if(ProfileManager.IsFullVersion())
	{
		m_bTrialVersion=false;
		m_buttons[(int)eControl_UnlockOrDLC].init(L"Cambiar Aspecto",eControl_UnlockOrDLC);
	}
	else
	{
		m_bTrialVersion=true;
		m_buttons[(int)eControl_UnlockOrDLC].init(L"Cambiar Aspecto",eControl_UnlockOrDLC);
	}

#ifndef _DURANGO
	m_buttons[(int)eControl_Exit].init(app.GetString(IDS_EXIT_GAME),eControl_Exit);
#else
	m_buttons[(int)eControl_XboxHelp].init(app.GetString(IDS_XBOX_HELP_APP), eControl_XboxHelp);
#endif

#ifdef _DURANGO
	// Allowed to not have achievements in the menu
	removeControl( &m_buttons[(int)eControl_Achievements], false );
	// Not allowed to exit from a Xbox One game from the game - have to use the Home button
	//removeControl( &m_buttons[(int)eControl_Exit], false );
	m_bWaitingForDLCInfo=false;
#endif

#if defined(_WINDOWS64)
	// When the SWF menu is disabled, remove ALL Flash buttons from the
	// Iggy scene.  Without visible buttons Iggy has nothing to detect
	// hover on, so no rollover sounds fire.  The customDrawSplash
	// callback still runs during IggyPlayerDraw and draws the custom
	// PNG elements (logo, skin preview, change-name button).
	if(!g_bEnableSFW)
	{
		removeControl( &m_buttons[(int)eControl_PlayGame], false );
		removeControl( &m_buttons[(int)eControl_Leaderboards], false );
		removeControl( &m_buttons[(int)eControl_Achievements], false );
		removeControl( &m_buttons[(int)eControl_HelpAndOptions], false );
		removeControl( &m_buttons[(int)eControl_UnlockOrDLC], false );
#ifndef _DURANGO
		removeControl( &m_buttons[(int)eControl_Exit], false );
#endif
	}
#endif

	doHorizontalResizeCheck();

	m_splash = L"";

	wstring filename = L"splashes.txt";
	if( app.hasArchiveFile(filename) )
	{
		byteArray splashesArray = app.getArchiveFile(filename);
		ByteArrayInputStream bais(splashesArray);
		InputStreamReader isr( &bais );
		BufferedReader br( &isr );

		wstring line = L"";
		while ( !(line = br.readLine()).empty() )
		{
			line = trimString( line );
			if (line.length() > 0)
			{
				m_splashes.push_back(line);
			}
		}

		br.close();
	}

	m_bIgnorePress=false;
	m_bLoadTrialOnNetworkManagerReady = false;

	// 4J Stu - Clear out any loaded game rules
	app.setLevelGenerationOptions(NULL);

	// 4J Stu - Reset the leaving game flag so that we correctly handle signouts while in the menus
	g_NetworkManager.ResetLeavingGame();

#if TO_BE_IMPLEMENTED
	// Fix for #45154 - Frontend: DLC: Content can only be downloaded from the frontend if you have not joined/exited multiplayer
	XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_ALWAYS_ALLOW);
#endif
}

UIScene_MainMenu::~UIScene_MainMenu()
{
	m_parentLayer->removeComponent(eUIComponent_Panorama);

	// FIX: eUIComponent_Logo is only added to the layer on non-Windows64 builds.
	// Attempting to remove it on Windows64 (where it was never added) causes a
	// crash or silent corruption depending on removeComponent's implementation.
#if !defined(_WINDOWS64)
	m_parentLayer->removeComponent(eUIComponent_Logo);
#endif

	// FIX: Reset the logo texture state so it is reloaded correctly if the main
	// menu scene is destroyed and recreated (e.g. returning from a game session).
	// Without this, g_mainMenuLogoChecked stays true and EnsureMainMenuLogoTexture
	// returns immediately on the next visit, leaving a stale/invalid texture ID.
#if defined(_WINDOWS64)
	g_mainMenuLogoChecked   = false;
	g_mainMenuLogoReady     = false;
	g_mainMenuLogoTextureId = -1;
#endif
}

void UIScene_MainMenu::updateTooltips()
{
	int iX = -1;
	int iA = -1;
	if(!m_bIgnorePress)
	{
		iA = IDS_TOOLTIPS_SELECT;

#ifdef _XBOX_ONE
		iX = IDS_TOOLTIPS_CHOOSE_USER;
#endif
	}
	ui.SetTooltips( DEFAULT_XUI_MENU_USER, iA, -1, iX);
}

void UIScene_MainMenu::updateComponents()
{
	m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,true);
#if !defined(_WINDOWS64)
	m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
#endif
}

void UIScene_MainMenu::handleGainFocus(bool navBack)
{
	UIScene::handleGainFocus(navBack);
	ui.ShowPlayerDisplayname(false);
	m_bIgnorePress=false;
	
	// 4J-JEV: This needs to come before SetLockedProfile(-1) as it wipes the XbLive contexts.
	if (!navBack)
	{
		for (int iPad = 0; iPad < MAX_LOCAL_PLAYERS; iPad++)
		{
			// For returning to menus after exiting a game.
			if (ProfileManager.IsSignedIn(iPad) )
			{
				ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);
			}
		}
	}
	ProfileManager.SetLockedProfile(-1);

	m_bIgnorePress = false;
	updateTooltips();

#ifdef _DURANGO
	ProfileManager.ClearGameUsers();
#endif
		
	if(navBack && ProfileManager.IsFullVersion())
	{
		m_buttons[(int)eControl_UnlockOrDLC].setLabel(L"Cambiar Aspecto");
	}

#if TO_BE_IMPLEMENTED
	// Fix for #45154 - Frontend: DLC: Content can only be downloaded from the frontend if you have not joined/exited multiplayer
	XBackgroundDownloadSetMode(XBACKGROUND_DOWNLOAD_MODE_ALWAYS_ALLOW);
	m_Timer.SetShow(FALSE);
#endif
	m_controlTimer.setVisible( false );

	// 4J-PB - remove the "hobo humping" message legal say we can't have, and the 1080p one for Vita
	int splashIndex = eSplashRandomStart + 1 + random->nextInt( (int)m_splashes.size() - (eSplashRandomStart + 1) );

	// Override splash text on certain dates
	SYSTEMTIME LocalSysTime;
	GetLocalTime( &LocalSysTime );
	if (LocalSysTime.wMonth == 11 && LocalSysTime.wDay == 9)
	{
		splashIndex = eSplashHappyBirthdayEx;
	}
	else if (LocalSysTime.wMonth == 6 && LocalSysTime.wDay == 1)
	{
		splashIndex = eSplashHappyBirthdayNotch;
	}
	else if (LocalSysTime.wMonth == 12 && LocalSysTime.wDay == 24) // the Java game shows this on Christmas Eve, so we will too
	{
		splashIndex = eSplashMerryXmas;
	}
	else if (LocalSysTime.wMonth == 1 && LocalSysTime.wDay == 1)
	{
		splashIndex = eSplashHappyNewYear;
	}
	//splashIndex = 47; // Very short string
	//splashIndex = 194; // Very long string
	//splashIndex = 295; // Coloured
	//splashIndex = 296; // Noise
	m_splash = m_splashes.at( splashIndex );
}

wstring UIScene_MainMenu::getMoviePath()
{
	return L"MainMenu";
}

void UIScene_MainMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	//app.DebugPrintf("UIScene_DebugOverlay handling input for pad %d, key %d, down- %s, pressed- %s, released- %s\n", iPad, key, down?"TRUE":"FALSE", pressed?"TRUE":"FALSE", released?"TRUE":"FALSE");
	
	if(m_bIgnorePress) return;


	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch(key)
	{
	case ACTION_MENU_OK:
		if(pressed)
		{
			ProfileManager.SetPrimaryPad(iPad);
			ProfileManager.SetLockedProfile(-1);
#if defined(_WINDOWS64)
			if(g_bEnableSFW)
#endif
				sendInputToMovie(key, repeat, pressed, released);
		}
		break;
#ifdef _XBOX_ONE
	case ACTION_MENU_X:
		if(pressed)
		{
			m_bIgnorePress = true;
			ProfileManager.RequestSignInUI(false, false, false, false, false, ChooseUser_SignInReturned, this, iPad);
		}
		break;
#endif

	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
#if defined(_WINDOWS64)
		if(g_bEnableSFW)
#endif
			sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_MainMenu::handlePress(F64 controlId, F64 childId)
{
#if defined(_WINDOWS64)
	// When the SWF is disabled, block all SWF button presses
	// so their hitboxes have no effect.
	if(!g_bEnableSFW) return;
#endif
	int primaryPad = ProfileManager.GetPrimaryPad();
	
	int (*signInReturnedFunc) (LPVOID,const bool, const int iPad) = NULL;

	switch((int)controlId)
	{
	case eControl_PlayGame:
		m_eAction=eAction_RunGame;
		//CD - Added for audio
		ui.PlayUISFX(eSFX_Press);

		signInReturnedFunc = &UIScene_MainMenu::CreateLoad_SignInReturned;
		break;
	case eControl_Leaderboards:
		//CD - Added for audio
		ui.PlayUISFX(eSFX_Press);
		m_eAction=eAction_RunLeaderboards;
		signInReturnedFunc = &UIScene_MainMenu::Leaderboards_SignInReturned;
		break;
	case eControl_Achievements:
		//CD - Added for audio
		ui.PlayUISFX(eSFX_Press);

		m_eAction=eAction_RunAchievements;
		signInReturnedFunc = &UIScene_MainMenu::Achievements_SignInReturned;
		break;
	case eControl_HelpAndOptions:
		//CD - Added for audio
		ui.PlayUISFX(eSFX_Press);

		m_eAction=eAction_RunHelpAndOptions;
		signInReturnedFunc = &UIScene_MainMenu::HelpAndOptions_SignInReturned;
		break;
	case eControl_UnlockOrDLC:
		//CD - Added for audio
		ui.PlayUISFX(eSFX_Press);

		m_eAction=eAction_RunUnlockOrDLC;
		signInReturnedFunc = &UIScene_MainMenu::UnlockFullGame_SignInReturned;
		break;
#if defined _XBOX
	case eControl_Exit:
		if( ProfileManager.IsFullVersion() )
		{				
			UINT uiIDA[2];
			uiIDA[0]=IDS_CANCEL;
			uiIDA[1]=IDS_OK;
			ui.RequestMessageBox(IDS_WARNING_ARCADE_TITLE, IDS_WARNING_ARCADE_TEXT, uiIDA, 2, XUSER_INDEX_ANY,&UIScene_MainMenu::ExitGameReturned,this);
		}
		else
		{
#ifdef _XBOX_ONE
				ui.ShowPlayerDisplayname(true);
#endif
			ui.NavigateToScene(primaryPad,eUIScene_TrialExitUpsell);
		}
		break;
#endif

#ifdef _DURANGO
	case eControl_XboxHelp:
		ui.PlayUISFX(eSFX_Press);

		m_eAction=eAction_RunXboxHelp;
		signInReturnedFunc = &UIScene_MainMenu::XboxHelp_SignInReturned;
		break;
#endif

	default:	__debugbreak();
	}
	
	bool confirmUser = false;

	// Note: if no sign in returned func, assume this isn't required
	if (signInReturnedFunc != NULL)
	{
		if(ProfileManager.IsSignedIn(primaryPad))
		{
			if (confirmUser)
			{
				ProfileManager.RequestSignInUI(false, false, true, false, true, signInReturnedFunc, this, primaryPad);
			}
			else
			{
				RunAction(primaryPad);
			}
		}
		else
		{
			// Ask user to sign in
			UINT uiIDA[2];
			uiIDA[0]=IDS_CONFIRM_OK;
			uiIDA[1]=IDS_CONFIRM_CANCEL;
			ui.RequestMessageBox(IDS_MUST_SIGN_IN_TITLE, IDS_MUST_SIGN_IN_TEXT, uiIDA, 2, primaryPad, &UIScene_MainMenu::MustSignInReturned, this, app.GetStringTable());
		}
	}
}

// Run current action
void UIScene_MainMenu::RunAction(int iPad)
{
	switch(m_eAction)
	{
	case eAction_RunGame:
		RunPlayGame(iPad);
		break;
	case eAction_RunLeaderboards:
		RunLeaderboards(iPad);
		break;
	case eAction_RunAchievements:
		RunAchievements(iPad);
		break;
	case eAction_RunHelpAndOptions:
		RunHelpAndOptions(iPad);
		break;
	case eAction_RunUnlockOrDLC:
		RunUnlockOrDLC(iPad);
		break;
#ifdef _DURANGO
	case eAction_RunXboxHelp:
		// 4J: Launch the dummy xbox help application.
		WXS::User^ user = ProfileManager.GetUser(ProfileManager.GetPrimaryPad());
		Windows::Xbox::ApplicationModel::Help::Show(user);
		break;
#endif
	}
}

void UIScene_MainMenu::customDraw(IggyCustomDrawCallbackRegion *region)
{
		if(wcscmp((wchar_t *)region->name,L"Splash")==0)
		{
			PIXBeginNamedEvent(0,"Custom draw splash");
			customDrawSplash(region);
			PIXEndNamedEvent();
		}
	}

// render – main entry point for rendering the scene.
void UIScene_MainMenu::render(S32 width, S32 height, C4JRender::eViewportType viewport)
{
	UIScene::render(width, height, viewport);
}
void UIScene_MainMenu::customDrawSplash(IggyCustomDrawCallbackRegion *region)
{
		Minecraft *pMinecraft = Minecraft::GetInstance();
		if(region == NULL || pMinecraft == NULL)
		{
			return;
		}

		if(pMinecraft->options == NULL)
		{
			return;
		}

		// 4J Stu - Move this to the ctor when the main menu is not the first scene we navigate to
		ScreenSizeCalculator ssc(pMinecraft->options, pMinecraft->width_phys, pMinecraft->height_phys);
	m_fScreenWidth=(float)pMinecraft->width_phys;
	m_fRawWidth=(float)ssc.rawWidth;
	m_fScreenHeight=(float)pMinecraft->height_phys;
	m_fRawHeight=(float)ssc.rawHeight;


		CustomDrawData *customDrawRegion = ui.setupCustomDraw(this,region);
		if(customDrawRegion == NULL)
		{
			return;
		}

		Font *font = pMinecraft->font;
		if(font == NULL)
		{
			delete customDrawRegion;
			ui.endCustomDraw(region);
			return;
		}

		// build and render with the game call
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);

#if defined(_WINDOWS64)
	if(pMinecraft != NULL && pMinecraft->textures != NULL)
	{
		DrawMainMenuLogo(pMinecraft, pMinecraft->width_phys, pMinecraft->height_phys, m_parentLayer->getViewport());
		DrawMainMenuSkinPreview(pMinecraft, pMinecraft->width_phys, pMinecraft->height_phys, m_parentLayer->getViewport());
		DrawMainMenuChangeNameButton(pMinecraft, pMinecraft->width_phys, pMinecraft->height_phys, m_parentLayer->getViewport());
		DrawCustomMenuButtons(pMinecraft, m_parentLayer->getViewport());

		// ── TEST SLIDER at (0, 0) ────────────────────────────────
		if(!g_testSliderInited)
		{
			g_testSlider.Setup(0.0f, 0.0f); // x=0, y=0, range 0-100
			g_testSliderInited = true;
		}
		g_testSlider.Render(pMinecraft, pMinecraft->font, IDS_PLAY_GAME, (int)m_parentLayer->getViewport());
	}
#endif

	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.1f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// Yellow splash text — always draws regardless of g_bEnableSFW
	{
		glPushMatrix();

		float width = region->x1 - region->x0;
		float height = region->y1 - region->y0;
		float xo = width/2;
		float yo = height;

		glTranslatef(xo, yo, 0);

		glRotatef(-17, 0, 0, 1);
		float sss = 1.8f - Mth::abs(Mth::sin(System::currentTimeMillis() % 1000 / 1000.0f * PI * 2) * 0.1f);
		sss*=(m_fScreenWidth/m_fRawWidth);

		sss = sss * 100 / (font->width(m_splash) + 8 * 4);
		glScalef(sss, sss, sss);
		font->draw(m_splash, 0 - (font->width(m_splash)) / 2, -8, 0xffff00);
		glPopMatrix();
	}

#if defined(_WINDOWS64)
	{
		// Draw player name above the preview, always fixed for this menu.
		wstring overlayPlayerName = UserData_Info::GetPlayerName();
		if(overlayPlayerName.empty())
		{
			overlayPlayerName = L"Steve";
		}

		const float overlayScale = 2.0f;
		const float overlayX = 115.0f;
		const float overlayY = 170.0f;
		const float overlayNameWidth = (float)font->width(overlayPlayerName) * overlayScale;
		const float overlayPadX = 14.0f;
		const float overlayPadY = 8.0f;
		const float overlayExtraWidth = 28.0f;
		const float overlayTextHeight = 8.0f * overlayScale;
		const float overlayBaseHeight = 18.0f * overlayScale;
		const float overlayBgWidth = overlayNameWidth + overlayPadX * 2.0f + overlayExtraWidth;
		const float overlayBgHeight = (overlayBaseHeight + overlayPadY * 2.0f) * 0.75f;
		const float overlayBgX0 = overlayX;
		const float overlayBgX1 = overlayX + overlayBgWidth;
		const float overlayBgY0 = overlayY;
		const float overlayBgY1 = overlayY + overlayBgHeight;
		const float overlayTextX = overlayBgX0 + ((overlayBgWidth - overlayNameWidth) * 0.5f);
		const float overlayTextY = overlayBgY0 + ((overlayBgHeight - overlayTextHeight) * 0.5f);

		ui.setupRenderPosition(m_parentLayer->getViewport());

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0.0, (double)pMinecraft->width_phys, (double)pMinecraft->height_phys, 0.0, -1.0, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		Tesselator *overlayBg = Tesselator::getInstance();
		overlayBg->begin();
		overlayBg->color(0, 0, 0, 160);
		overlayBg->vertex(overlayBgX0, overlayBgY0, 0.0f);
		overlayBg->vertex(overlayBgX1, overlayBgY0, 0.0f);
		overlayBg->vertex(overlayBgX1, overlayBgY1, 0.0f);
		overlayBg->vertex(overlayBgX0, overlayBgY1, 0.0f);
		overlayBg->end();

		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glPushMatrix();
		glTranslatef(overlayTextX, overlayTextY, 0.0f);
		glScalef(overlayScale, overlayScale, 1.0f);
		font->draw(overlayPlayerName, 0, 0, 0xffffffff);
		glPopMatrix();

		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
	}
#endif

		glDisable(GL_RESCALE_NORMAL);
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		delete customDrawRegion;

		// Finish GDraw and anything else that needs to be finalised
		ui.endCustomDraw(region);	
	}

int UIScene_MainMenu::MustSignInReturned(void *pParam, int iPad, C4JStorage::EMessageResult result)
{
	UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

	if(result==C4JStorage::EMessage_ResultAccept) 
	{
		// we need to specify local game here to display local and LIVE profiles in the list
		switch(pClass->m_eAction)
		{
		case eAction_RunGame:			ProfileManager.RequestSignInUI(false,  true, false, false, true, &UIScene_MainMenu::CreateLoad_SignInReturned,		pClass,	iPad );	break;
		case eAction_RunHelpAndOptions:	ProfileManager.RequestSignInUI(false, false,  true, false, true, &UIScene_MainMenu::HelpAndOptions_SignInReturned,	pClass,	iPad );	break;										 	   
		case eAction_RunLeaderboards:	ProfileManager.RequestSignInUI(false, false,  true, false, true, &UIScene_MainMenu::Leaderboards_SignInReturned,	pClass,	iPad );	break;										 	   
		case eAction_RunAchievements:	ProfileManager.RequestSignInUI(false, false,  true, false, true, &UIScene_MainMenu::Achievements_SignInReturned,	pClass,	iPad );	break;										 	   
		case eAction_RunUnlockOrDLC:	ProfileManager.RequestSignInUI(false, false,  true, false, true, &UIScene_MainMenu::UnlockFullGame_SignInReturned,	pClass,	iPad );	break;										  
#ifdef _DURANGO						 					  
		case eAction_RunXboxHelp:		ProfileManager.RequestSignInUI(false, false,  true, false, true, &UIScene_MainMenu::XboxHelp_SignInReturned,		pClass,	iPad ); break;
#endif
		}
	}
	else
	{
		pClass->m_bIgnorePress=false;
		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i, CONTEXT_PRESENCE_MENUS, false);
			}
		}
	}	

	return 0;
}


int UIScene_MainMenu::HelpAndOptions_SignInReturned(void *pParam,bool bContinue,int iPad)
{
	UIScene_MainMenu *pClass = (UIScene_MainMenu *)pParam;

	if(bContinue)
	{
		// 4J-JEV: Don't we only need to update rich-presence if the sign-in status changes.
		ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);

#if TO_BE_IMPLEMENTED
 		if(app.GetTMSDLCInfoRead())
#endif
 		{
			ProfileManager.SetLockedProfile(ProfileManager.GetPrimaryPad());
#ifdef _XBOX_ONE
			ui.ShowPlayerDisplayname(true);
#endif
 			ui.NavigateToScene(iPad,eUIScene_HelpAndOptionsMenu);
 		}
#if TO_BE_IMPLEMENTED
 		else
 		{
 			// Changing to async TMS calls
 			app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_HelpAndOptions);
 
 			// block all input
 			pClass->m_bIgnorePress=true;
 			// We want to hide everything in this scene and display a timer until we get a completion for the TMS files
 			for(int i=0;i<BUTTONS_MAX;i++)
 			{
 				pClass->m_Buttons[i].SetShow(FALSE);
 			}
 
 			pClass->updateTooltips();
 
 			pClass->m_Timer.SetShow(TRUE);
 		}
#endif
	}
	else
	{
		pClass->m_bIgnorePress=false;
		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i,CONTEXT_PRESENCE_MENUS,false);
			}
		}
	}	

	return 0;
}

#ifdef _XBOX_ONE
int UIScene_MainMenu::ChooseUser_SignInReturned(void *pParam, bool bContinue, int iPad)
{
	UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;
	pClass->m_bIgnorePress = false;

	return 0;
}
#endif

int UIScene_MainMenu::CreateLoad_SignInReturned(void *pParam, bool bContinue, int iPad)
{
	UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;
	
	if(bContinue)
	{
		// 4J-JEV: We only need to update rich-presence if the sign-in status changes.
		ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);

		UINT uiIDA[1] = { IDS_OK };

		if(ProfileManager.IsGuest(ProfileManager.GetPrimaryPad()))
		{
			pClass->m_bIgnorePress=false;
			ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
		}
		else
		{
			ProfileManager.SetLockedProfile(ProfileManager.GetPrimaryPad());


			// change the minecraft player name
			Minecraft::GetInstance()->user->name = convStringToWstring( ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()));

			if(ProfileManager.IsFullVersion())
			{
				bool bSignedInLive = ProfileManager.IsSignedInLive(iPad);

				// Check if we're signed in to LIVE
				if(bSignedInLive)
				{
					// 4J-PB - Need to check for installed DLC
					if(!app.DLCInstallProcessCompleted()) app.StartInstallDLCProcess(iPad);

					if(ProfileManager.IsGuest(iPad))
					{
						pClass->m_bIgnorePress=false;
						ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
					}
					else
					{
						// 4J Stu - Not relevant to PS3
#ifdef _XBOX_ONE
//						if(app.GetTMSDLCInfoRead() && app.GetBanListRead(iPad))
						if(app.GetBanListRead(iPad))
						{
							Minecraft *pMinecraft=Minecraft::GetInstance();
							pMinecraft->user->name = convStringToWstring( ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()));

							// ensure we've applied this player's settings
							app.ApplyGameSettingsChanged(iPad);

#ifdef _XBOX_ONE
							ui.ShowPlayerDisplayname(true);
#endif
							ui.NavigateToScene(ProfileManager.GetPrimaryPad(), eUIScene_LoadOrJoinMenu);
						}
						else
						{
							app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_RunPlayGame);

							// block all input
							pClass->m_bIgnorePress=true;
							// We want to hide everything in this scene and display a timer until we get a completion for the TMS files
							// 					for(int i=0;i<eControl_Count;i++)
							// 					{
							// 						m_buttons[i].set(false);
							// 					}

							pClass->updateTooltips();

							pClass->m_controlTimer.setVisible( true );
						}
#endif
#if TO_BE_IMPLEMENTED
						// check if all the TMS files are loaded
						if(app.GetTMSDLCInfoRead() && app.GetTMSXUIDsFileRead() && app.GetBanListRead(iPad))
						{
							if(StorageManager.SetSaveDevice(&UIScene_MainMenu::DeviceSelectReturned,pClass)==true)
							{
								// save device already selected

								// ensure we've applied this player's settings
								app.ApplyGameSettingsChanged(ProfileManager.GetPrimaryPad());
								// check for DLC
								// start timer to track DLC check finished
								pClass->m_Timer.SetShow(TRUE);
								XuiSetTimer(pClass->m_hObj,DLC_INSTALLED_TIMER_ID,DLC_INSTALLED_TIMER_TIME);
								//app.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_MultiGameJoinLoad);
							}
						}
						else
						{
							// Changing to async TMS calls
							app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_RunPlayGame);

							// block all input
							pClass->m_bIgnorePress=true;
							// We want to hide everything in this scene and display a timer until we get a completion for the TMS files
							for(int i=0;i<BUTTONS_MAX;i++)
							{
								pClass->m_Buttons[i].SetShow(FALSE);
							}

							updateTooltips();

							pClass->m_Timer.SetShow(TRUE);
						}
#else
						Minecraft *pMinecraft=Minecraft::GetInstance();
						pMinecraft->user->name = convStringToWstring( ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()));

						// ensure we've applied this player's settings
						app.ApplyGameSettingsChanged(iPad);

#ifdef _XBOX_ONE
						ui.ShowPlayerDisplayname(true);
#endif
						ui.NavigateToScene(ProfileManager.GetPrimaryPad(), eUIScene_LoadOrJoinMenu);
#endif
					}
				}
				else
				{
#if TO_BE_IMPLEMENTED
					// offline
					ProfileManager.DisplayOfflineProfile(&CScene_Main::CreateLoad_OfflineProfileReturned,pClass, ProfileManager.GetPrimaryPad() );
#else
					app.DebugPrintf("Offline Profile returned not implemented\n");		
#ifdef _XBOX_ONE
					ui.ShowPlayerDisplayname(true);
#endif
					ui.NavigateToScene(ProfileManager.GetPrimaryPad(), eUIScene_LoadOrJoinMenu);
#endif
				}
			}
			else
			{
				// 4J-PB - if this is the trial game, we can't have any networking
				// Can't apply the player's settings here - they haven't come back from the QuerySignInStatud call above yet.
				// Need to let them action in the main loop when they come in
				// ensure we've applied this player's settings
				//app.ApplyGameSettingsChanged(iPad);


				{
					// go straight in to the trial level
					LoadTrial();
				}
			}
		}
	}
	else
	{
		pClass->m_bIgnorePress=false;

		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i,CONTEXT_PRESENCE_MENUS,false);
			}
		}

	}	
	return 0;
}

int UIScene_MainMenu::Leaderboards_SignInReturned(void *pParam,bool bContinue,int iPad)
{
	UIScene_MainMenu *pClass = (UIScene_MainMenu *)pParam;

	if(bContinue)
	{
		// 4J-JEV: We only need to update rich-presence if the sign-in status changes.
		ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);

		UINT uiIDA[1] = { IDS_OK };

		// guests can't look at leaderboards
		if(ProfileManager.IsGuest(ProfileManager.GetPrimaryPad()))
		{
			pClass->m_bIgnorePress=false;
			ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
		}
		else if(!ProfileManager.IsSignedInLive(ProfileManager.GetPrimaryPad()))
		{
			pClass->m_bIgnorePress=false;
			ui.RequestMessageBox(IDS_PRO_NOTONLINE_TITLE, IDS_PRO_XBOXLIVE_NOTIFICATION, uiIDA, 1);
		}
		else
		{
			bool bContentRestricted=false;
			if(bContentRestricted)
			{				
				pClass->m_bIgnorePress=false;
#if !(defined(_XBOX) || defined(_WIN64)) // 4J Stu - Temp to get the win build running, but so we check this for other platforms
				// you can't see leaderboards
				UINT uiIDA[1];
				uiIDA[0]=IDS_CONFIRM_OK;
				ui.RequestMessageBox(IDS_ONLINE_SERVICE_TITLE, IDS_CONTENT_RESTRICTION, uiIDA, 1, ProfileManager.GetPrimaryPad(),NULL,NULL, app.GetStringTable());
#endif
			}
			else
			{
				ProfileManager.SetLockedProfile(ProfileManager.GetPrimaryPad());
#ifdef _XBOX_ONE
				ui.ShowPlayerDisplayname(true);
#endif
				ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_LeaderboardsMenu);
			}
		}
	}
	else
	{
		pClass->m_bIgnorePress=false;
		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i,CONTEXT_PRESENCE_MENUS,false);
			}
		}

	}	
	return 0;
}

int UIScene_MainMenu::Achievements_SignInReturned(void *pParam,bool bContinue,int iPad)
{
	UIScene_MainMenu *pClass = (UIScene_MainMenu *)pParam;

	if (bContinue)
	{
		pClass->m_bIgnorePress=false;
		// 4J-JEV: We only need to update rich-presence if the sign-in status changes.
		ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);

		XShowAchievementsUI( ProfileManager.GetPrimaryPad() );
	}
	else
	{
		pClass->m_bIgnorePress=false;
		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i,CONTEXT_PRESENCE_MENUS,false);
			}
		}

	}	
	return 0;
}

int UIScene_MainMenu::UnlockFullGame_SignInReturned(void *pParam,bool bContinue,int iPad)
{
	UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

	if (bContinue)
	{
		// 4J-JEV: We only need to update rich-presence if the sign-in status changes.
		ProfileManager.SetCurrentGameActivity(iPad, CONTEXT_PRESENCE_MENUS, false);

		pClass->RunUnlockOrDLC(iPad);
	}
	else
	{
		pClass->m_bIgnorePress=false;
		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i,CONTEXT_PRESENCE_MENUS,false);
			}
		}

	}	

	return 0;
}

#ifdef _DURANGO
int UIScene_MainMenu::XboxHelp_SignInReturned(void *pParam, bool bContinue, int iPad)
{
	UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

	if (bContinue)
	{
		// 4J: Launch the dummy xbox help application.
		WXS::User^ user = ProfileManager.GetUser(ProfileManager.GetPrimaryPad());
		Windows::Xbox::ApplicationModel::Help::Show(user);
	}
	else
	{
		// unlock the profile
		ProfileManager.SetLockedProfile(-1);
		for(int i=0;i<XUSER_MAX_COUNT;i++)
		{
			// if the user is valid, we should set the presence
			if(ProfileManager.IsSignedIn(i))
			{
				ProfileManager.SetCurrentGameActivity(i,CONTEXT_PRESENCE_MENUS,false);
			}
		}
	}	

	return 0;
}
#endif

int UIScene_MainMenu::ExitGameReturned(void *pParam,int iPad,C4JStorage::EMessageResult result)
{
	//UIScene_MainMenu* pClass = (UIScene_MainMenu*)pParam;

	// buttons reversed on this
	if(result==C4JStorage::EMessage_ResultDecline) 
	{
		//XLaunchNewImage(XLAUNCH_KEYWORD_DASH_ARCADE, 0);
		app.ExitGame();
	}

	return 0;
}


void UIScene_MainMenu::RunPlayGame(int iPad)
{
	Minecraft *pMinecraft=Minecraft::GetInstance();

	// clear the remembered signed in users so their profiles get read again
	app.ClearSignInChangeUsersMask();

	app.ReleaseSaveThumbnail();

	if(ProfileManager.IsGuest(iPad))
	{
		UINT uiIDA[1];
		uiIDA[0]=IDS_OK;
		
		m_bIgnorePress=false;
		ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
	}
	else
	{
		ProfileManager.SetLockedProfile(iPad);

		// If the player was signed in before selecting play, we'll not have read the profile yet, so query the sign-in status to get this to happen
		ProfileManager.QuerySigninStatus();

		// 4J-PB - Need to check for installed DLC
		if(!app.DLCInstallProcessCompleted()) app.StartInstallDLCProcess(iPad);

		if(ProfileManager.IsFullVersion())
		{
			// are we offline?
			bool bSignedInLive = ProfileManager.IsSignedInLive(iPad);

			if(!bSignedInLive)
			{
				ProfileManager.SetLockedProfile(iPad);
#ifdef _XBOX_ONE
				ui.ShowPlayerDisplayname(true);
#endif
				ui.NavigateToScene(ProfileManager.GetPrimaryPad(), eUIScene_LoadOrJoinMenu);
			}
			else
			{
#ifdef _XBOX_ONE
				if(!app.GetBanListRead(iPad))
				{
					app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_RunPlayGame);

					// block all input
					m_bIgnorePress=true;

					updateTooltips();

					m_controlTimer.setVisible( true );
				}
#endif
#if TO_BE_IMPLEMENTED
				// Check if there is any new DLC
				app.ClearNewDLCAvailable();
				StorageManager.GetAvailableDLCCount(iPad);

				// check if all the TMS files are loaded
				if(app.GetTMSDLCInfoRead() && app.GetTMSXUIDsFileRead() && app.GetBanListRead(iPad))
				{
					if(StorageManager.SetSaveDevice(&CScene_Main::DeviceSelectReturned,this)==true)
					{
						// change the minecraft player name
						pMinecraft->user->name = convStringToWstring( ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()));
						// save device already selected

						// ensure we've applied this player's settings
						app.ApplyGameSettingsChanged(iPad);
						// check for DLC
						// start timer to track DLC check finished
						m_Timer.SetShow(TRUE);
						XuiSetTimer(m_hObj,DLC_INSTALLED_TIMER_ID,DLC_INSTALLED_TIMER_TIME);
						//app.NavigateToScene(iPad,eUIScene_MultiGameJoinLoad);
					}
				}
				else
				{
					// Changing to async TMS calls
					app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_RunPlayGame);

					// block all input
					m_bIgnorePress=true;
					// We want to hide everything in this scene and display a timer until we get a completion for the TMS files
					for(int i=0;i<BUTTONS_MAX;i++)
					{
						m_Buttons[i].SetShow(FALSE);
					}

					updateTooltips();

					m_Timer.SetShow(TRUE);
				}
#else
				pMinecraft->user->name = convStringToWstring( ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()));

				// ensure we've applied this player's settings
				app.ApplyGameSettingsChanged(iPad);

#ifdef _XBOX_ONE
				ui.ShowPlayerDisplayname(true);
#endif
				ui.NavigateToScene(ProfileManager.GetPrimaryPad(), eUIScene_LoadOrJoinMenu);
#endif
			}
		}
		else
		{
			// 4J-PB - if this is the trial game, we can't have any networking
			// go straight in to the trial level
			// change the minecraft player name
			Minecraft::GetInstance()->user->name = convStringToWstring( ProfileManager.GetGamertag(ProfileManager.GetPrimaryPad()));


			{
				LoadTrial();
			}
		}
	}
}

void UIScene_MainMenu::RunLeaderboards(int iPad)
{
	UINT uiIDA[1];
	uiIDA[0]=IDS_OK;
	
	// guests can't look at leaderboards
	if(ProfileManager.IsGuest(iPad))
	{
		ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
	}
	else if(!ProfileManager.IsSignedInLive(iPad))
	{
		ui.RequestMessageBox(IDS_PRO_NOTONLINE_TITLE, IDS_PRO_XBOXLIVE_NOTIFICATION, uiIDA, 1);
	}
	else
	{	
		bool bContentRestricted=false;
		if(bContentRestricted)
		{
#if !(defined(_XBOX) || defined(_WIN64))
			UINT uiIDA[1];
			uiIDA[0]=IDS_CONFIRM_OK;
			ui.RequestMessageBox(IDS_ONLINE_SERVICE_TITLE, IDS_CONTENT_RESTRICTION, uiIDA, 1, ProfileManager.GetPrimaryPad(),NULL,this, app.GetStringTable());
#endif
		}
		else
		{
			ProfileManager.SetLockedProfile(iPad);
			ProfileManager.QuerySigninStatus();

#ifdef _XBOX_ONE
			ui.ShowPlayerDisplayname(true);
#endif
			ui.NavigateToScene(iPad, eUIScene_LeaderboardsMenu);
		}
	}
}

void UIScene_MainMenu::RunUnlockOrDLC(int iPad)
{
	ui.NavigateToScene(iPad, eUIScene_SkinSelectMenu);
}

void UIScene_MainMenu::tick()
{
	// Always tick Iggy so the scene stays properly initialised
	// (m_hasTickedOnce must be true or the layer won't call render() at all).
	// Hover sounds come from IggyPlayerDraw (in renderSFW), NOT from the tick,
	// so ticking Iggy when the flag is false doesn't produce unwanted audio.
	UIScene::tick();

#if defined(_WINDOWS64)
	// The custom buttons should only be interactive if this specific scene has focus.
	// This prevents clicks from "passing through" from submenus (like Play Game or MessageBox).
	Minecraft *minecraft = Minecraft::GetInstance();
	if(minecraft != NULL && hasFocus(ProfileManager.GetPrimaryPad()))
	{
		if(g_mainMenuChangeNameButton.Update(minecraft))
		{
			minecraft->setScreen(new RenamePlayerScreen(minecraft->screen));
		}

		// ── 6 Custom Menu Buttons ──
		SetupCustomMenuButtons();
		int primaryPad = ProfileManager.GetPrimaryPad();

		if(g_customMenuButtons[eCMB_PlayGame].Update(minecraft))
		{
			m_bIgnorePress = true;
			m_eAction = eAction_RunGame;
			RunPlayGame(primaryPad);
		}
		if(g_customMenuButtons[eCMB_Leaderboards].Update(minecraft))
		{
			m_bIgnorePress = true;
			m_eAction = eAction_RunLeaderboards;
			RunLeaderboards(primaryPad);
		}
		if(g_customMenuButtons[eCMB_Achievements].Update(minecraft))
		{
			m_bIgnorePress = true;
			m_eAction = eAction_RunAchievements;
			RunAchievements(primaryPad);
		}
		if(g_customMenuButtons[eCMB_HelpAndOptions].Update(minecraft))
		{
			m_bIgnorePress = true;
			m_eAction = eAction_RunHelpAndOptions;
			RunHelpAndOptions(primaryPad);
		}
		if(g_customMenuButtons[eCMB_ChangeSkin].Update(minecraft))
		{
			m_bIgnorePress = true;
			m_eAction = eAction_RunUnlockOrDLC;
			RunUnlockOrDLC(primaryPad);
		}
		if(g_customMenuButtons[eCMB_Exit].Update(minecraft))
		{
			PostQuitMessage(0);
		}

		// ── TEST SLIDER update ──
		g_testSlider.Update(minecraft);
	}
#endif


#if defined _XBOX_ONE
	if(m_bWaitingForDLCInfo)
	{	
		if(app.GetTMSDLCInfoRead())
		{		
			m_bWaitingForDLCInfo=false;
			ProfileManager.SetLockedProfile(m_iPad);
			ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_DLCMainMenu);		
		}
	}

	if(g_NetworkManager.ShouldMessageForFullSession())
	{
		UINT uiIDA[1];
		uiIDA[0]=IDS_CONFIRM_OK;
		ui.RequestMessageBox( IDS_CONNECTION_FAILED, IDS_IN_PARTY_SESSION_FULL, uiIDA,1,ProfileManager.GetPrimaryPad(),NULL,NULL, app.GetStringTable());
	}
#endif

}

void UIScene_MainMenu::RunAchievements(int iPad)
{
#if TO_BE_IMPLEMENTED
	UINT uiIDA[1];
	uiIDA[0]=IDS_OK;

	// guests can't look at achievements
	if(ProfileManager.IsGuest(iPad))
	{
		ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
	}
	else
	{
		XShowAchievementsUI( iPad );
	}
#endif
}

void UIScene_MainMenu::RunHelpAndOptions(int iPad)
{
	if(ProfileManager.IsGuest(iPad))
	{
		UINT uiIDA[1];
		uiIDA[0]=IDS_OK;
		ui.RequestMessageBox(IDS_PRO_GUESTPROFILE_TITLE, IDS_PRO_GUESTPROFILE_TEXT, uiIDA, 1);
	}
	else
	{
		ProfileManager.QuerySigninStatus();

#if TO_BE_IMPLEMENTED
		if(app.GetTMSDLCInfoRead() || !ProfileManager.IsSignedInLive(iPad))
#endif
		{
			ProfileManager.SetLockedProfile(iPad);
#ifdef _XBOX_ONE
			ui.ShowPlayerDisplayname(true);
#endif
			ui.NavigateToScene(iPad,eUIScene_HelpAndOptionsMenu);
		}
#if TO_BE_IMPLEMENTED
		else
		{
			app.SetTMSAction(iPad,eTMSAction_TMSPP_RetrieveFiles_HelpAndOptions);
			m_bIgnorePress=true;
			for(int i=0;i<BUTTONS_MAX;i++)
			{
				m_Buttons[i].SetShow(FALSE);
			}
			updateTooltips();
			m_Timer.SetShow(TRUE);
		}
#endif
	}
}

void UIScene_MainMenu::LoadTrial(void)
{		
	app.SetTutorialMode( true );

	app.ClearTerrainFeaturePosition();

	StorageManager.ResetSaveData();

	ProfileManager.StartTrialGame();

	StorageManager.SetSaveDisabled(true);

	app.SetGameHostOption(eGameHostOption_DisableSaving, 1);

	StorageManager.SetSaveTitle(L"Tutorial");

	app.SetAutosaveTimerTime();

	g_NetworkManager.HostGame(0,false,true,MINECRAFT_NET_MAX_PLAYERS,0);

#ifndef _XBOX
	g_NetworkManager.FakeLocalPlayerJoined();
#endif

	NetworkGameInitData *param = new NetworkGameInitData();
	param->seed = 0;
	param->saveData = NULL;
	param->settings = app.GetGameHostOption( eGameHostOption_Tutorial ) | app.GetGameHostOption(eGameHostOption_DisableSaving);

	vector<LevelGenerationOptions *> *generators = app.getLevelGenerators();
	param->levelGen = generators->at(0);

	LoadingInputParams *loadingParams = new LoadingInputParams();
	loadingParams->func = &CGameNetworkManager::RunNetworkGameThreadProc;
	loadingParams->lpParam = (LPVOID)param;

	UIFullscreenProgressCompletionData *completionData = new UIFullscreenProgressCompletionData();
	completionData->bShowBackground=TRUE;
	completionData->bShowLogo=TRUE;
	completionData->type = e_ProgressCompletion_CloseAllPlayersUIScenes;
	completionData->iPad = ProfileManager.GetPrimaryPad();
	loadingParams->completionData = completionData;

	ui.ShowTrialTimer(true);
	
#ifdef _XBOX_ONE
	ui.ShowPlayerDisplayname(true);
#endif
	ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_FullscreenProgress, loadingParams);
}

void UIScene_MainMenu::handleUnlockFullVersion()
{
	m_buttons[(int)eControl_UnlockOrDLC].setLabel(L"Cambiar Aspecto",true);
}


