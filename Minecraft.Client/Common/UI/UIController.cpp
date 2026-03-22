#include "stdafx.h"
#include "UIController.h"
#include "UI.h"
#include "UIScene.h"
#include "..\..\..\Minecraft.World\StringHelpers.h"
#include "..\..\LocalPlayer.h"
#include "..\..\DLCTexturePack.h"
#include "..\..\TexturePackRepository.h"
#include "..\..\Minecraft.h"
#include "..\..\..\Minecraft.World\net.minecraft.world.entity.boss.enderdragon.h"
#include "..\..\EnderDragonRenderer.h"
#include "..\..\MultiPlayerLocalPlayer.h"
#include "UIFontData.h"
#ifdef __PSVITA__
#include <message_dialog.h>
#endif

// 4J Stu - Enable this to override the Iggy Allocator
//#define ENABLE_IGGY_ALLOCATOR
//#define EXCLUDE_IGGY_ALLOCATIONS_FROM_HEAP_INSPECTOR

//#define ENABLE_IGGY_EXPLORER
#ifdef ENABLE_IGGY_EXPLORER
#include "Windows64\Iggy\include\iggyexpruntime.h"
#endif

//#define ENABLE_IGGY_PERFMON
#ifdef ENABLE_IGGY_PERFMON

#define PM_ORIGIN_X 24
#define PM_ORIGIN_Y 34

#ifdef __ORBIS__
#include "Orbis\Iggy\include\iggyperfmon.h"
#include "Orbis\Iggy\include\iggyperfmon_orbis.h"
#elif defined _DURANGO
#include "Durango\Iggy\include\iggyperfmon.h"
#elif defined __PS3__
#include "PS3\Iggy\include\iggyperfmon.h"
#include "PS3\Iggy\include\iggyperfmon_ps3.h"
#elif defined __PSVITA__
#include "PSVita\Iggy\include\iggyperfmon.h"
#include "PSVita\Iggy\include\iggyperfmon_psp2.h"
#elif defined __WINDOWS64
#include "Windows64\Iggy\include\iggyperfmon.h"
#endif

#endif

CRITICAL_SECTION UIController::ms_reloadSkinCS;
bool UIController::ms_bReloadSkinCSInitialised = false;

DWORD UIController::m_dwTrialTimerLimitSecs=DYNAMIC_CONFIG_DEFAULT_TRIAL_TIME;

static void RADLINK WarningCallback(void *user_callback_data, Iggy *player, IggyResult code, const char *message)
{
	switch(code)
	{
	case IGGY_RESULT_Warning_CannotSustainFrameRate:
		break;
	default:
		app.DebugPrintf(app.USER_SR, message);
		app.DebugPrintf(app.USER_SR, "\n");
		break;
	};
}

static void RADLINK TraceCallback(void *user_callback_data, Iggy *player, char const *utf8_string, S32 length_in_bytes)
{
	app.DebugPrintf(app.USER_UI, (char *)utf8_string);
}

#ifdef ENABLE_IGGY_PERFMON
static void *RADLINK perf_malloc(void *handle, U32 size)
{
   return malloc(size);
}

static void RADLINK perf_free(void *handle, void *ptr)
{
   return free(ptr);
}
#endif

#ifdef EXCLUDE_IGGY_ALLOCATIONS_FROM_HEAP_INSPECTOR
extern "C" void *__real_malloc(size_t t);
extern "C" void __real_free(void *t);
#endif

__int64 UIController::iggyAllocCount = 0;
static unordered_map<void *,size_t> allocations;
static void * RADLINK AllocateFunction ( void * alloc_callback_user_data , size_t size_requested , size_t * size_returned )
{
	UIController *controller = (UIController *)alloc_callback_user_data;
	EnterCriticalSection(&controller->m_Allocatorlock);
#ifdef EXCLUDE_IGGY_ALLOCATIONS_FROM_HEAP_INSPECTOR
	void *alloc = __real_malloc(size_requested);
#else
	void *alloc = malloc(size_requested);
#endif
	*size_returned = size_requested;
	UIController::iggyAllocCount += size_requested;
	allocations[alloc] = size_requested;
	app.DebugPrintf(app.USER_SR, "Allocating %d, new total: %d\n", size_requested, UIController::iggyAllocCount);
	LeaveCriticalSection(&controller->m_Allocatorlock);
	return alloc;
}

static void RADLINK DeallocateFunction ( void * alloc_callback_user_data , void * ptr )
{
	UIController *controller = (UIController *)alloc_callback_user_data;
	EnterCriticalSection(&controller->m_Allocatorlock);
	size_t size = allocations[ptr];
	UIController::iggyAllocCount -= size;
	allocations.erase(ptr);
	app.DebugPrintf(app.USER_SR, "Freeing %d, new total %d\n", size, UIController::iggyAllocCount);
#ifdef EXCLUDE_IGGY_ALLOCATIONS_FROM_HEAP_INSPECTOR
	__real_free(ptr);
#else
	free(ptr);
#endif
	LeaveCriticalSection(&controller->m_Allocatorlock);
}

UIController::UIController()
{
	m_uiDebugConsole = NULL;
	m_reloadSkinThread = NULL;
	m_navigateToHomeOnReload = false;
	m_mcTTFFont= NULL;
	m_moj7 = NULL;
	m_moj11 = NULL;

#ifdef ENABLE_IGGY_ALLOCATOR
	InitializeCriticalSection(&m_Allocatorlock);
#endif

#if defined _WINDOWS64 || defined _DURANGO || defined __ORBIS__
	m_fScreenWidth = 1920.0f;
	m_fScreenHeight = 1080.0f;
	m_bScreenWidthSetup = true;
#else
	m_fScreenWidth = 1280.0f;
	m_fScreenHeight = 720.0f;
	m_bScreenWidthSetup = false;
#endif

	for(unsigned int i = 0; i < eLibrary_Count; ++i)
	{
		m_iggyLibraries[i] = IGGY_INVALID_LIBRARY;
	}

	for(unsigned int i = 0; i < XUSER_MAX_COUNT; ++i)
	{
		m_bMenuDisplayed[i] = false;
		m_iCountDown[i]=0;
		m_bMenuToBeClosed[i]=false;

		for(unsigned int key = 0; key <= ACTION_MAX_MENU; ++key)
		{
			m_actionRepeatTimer[i][key] = 0;
		}
	}

	for(unsigned int i = 0; i < eUIGroup_COUNT; ++i)
	{
		m_bCloseAllScenes[i] = false;
	}

	m_iPressStartQuadrantsMask = 0;

	m_currentRenderViewport = C4JRender::VIEWPORT_TYPE_FULLSCREEN;
	m_bCustomRenderPosition = false;
	m_winUserIndex = 0;
	m_accumulatedTicks = 0;

	InitializeCriticalSection(&m_navigationLock);
	InitializeCriticalSection(&m_registeredCallbackScenesCS);
	m_bSystemUIShowing=false;
#ifdef __PSVITA__
	m_bTouchscreenPressed=false;
#endif

	if(!ms_bReloadSkinCSInitialised)
	{
		InitializeCriticalSection(&ms_reloadSkinCS);
		ms_bReloadSkinCSInitialised = true;
	}
}

void UIController::SetSysUIShowing(bool bVal)
{
	if(bVal) app.DebugPrintf("System UI showing\n");
	else app.DebugPrintf("System UI stopped showing\n");
	m_bSystemUIShowing=bVal;
}

void UIController::SetSystemUIShowing(LPVOID lpParam,bool bVal)
{
	UIController *pClass=(UIController *)lpParam;
	pClass->SetSysUIShowing(bVal);
}

// SETUP
void UIController::preInit(S32 width, S32 height)
{
	m_fScreenWidth = width;
	m_fScreenHeight = height;
	m_bScreenWidthSetup = true;

#ifdef ENABLE_IGGY_ALLOCATOR
	IggyAllocator allocator;
	allocator.user_callback_data = this;
	allocator.mem_alloc = &AllocateFunction;
	allocator.mem_free = &DeallocateFunction;
	IggyInit(&allocator);
#else
	IggyInit(0);
#endif

	IggySetWarningCallback(WarningCallback, 0);
	IggySetTraceCallbackUTF8(TraceCallback, 0);

	setFontCachingCalculationBuffer(-1);
}

void UIController::postInit()
{
	IggySetCustomDrawCallback(&UIController::CustomDrawCallback, this);
	IggySetAS3ExternalFunctionCallbackUTF16 ( &UIController::ExternalFunctionCallback, this );
	IggySetTextureSubstitutionCallbacks ( &UIController::TextureSubstitutionCreateCallback , &UIController::TextureSubstitutionDestroyCallback, this );

	SetupFont();
	loadSkins();

	for(unsigned int i = 0; i < eUIGroup_COUNT; ++i)
	{
		m_groups[i] = new UIGroup((EUIGroup)i,i-1);
	}


#ifdef ENABLE_IGGY_EXPLORER
	iggy_explorer = IggyExpCreate("127.0.0.1", 9190, malloc(IGGYEXP_MIN_STORAGE), IGGYEXP_MIN_STORAGE);
	if ( iggy_explorer == NULL )
	{
		app.DebugPrintf( "Couldn't connect to Iggy Explorer, did you run it first?" );
	}
	else
	{
		IggyUseExplorer( m_groups[1]->getHUD()->getMovie(), iggy_explorer);
	}
#endif

#ifdef ENABLE_IGGY_PERFMON
	m_iggyPerfmonEnabled = false;
	iggy_perfmon = IggyPerfmonCreate(perf_malloc, perf_free, NULL);
	IggyInstallPerfmon(iggy_perfmon);
#endif

	NavigateToScene(0, eUIScene_Intro);
}

void UIController::SetupFont()
{
	bool bBitmapFont=false;

	if(m_mcTTFFont!=NULL)
	{
		delete m_mcTTFFont;
	}

	switch(XGetLanguage())
	{
#if defined(__PS3__) || defined(__ORBIS__) || defined(__PSVITA__)
	case XC_LANGUAGE_JAPANESE:
		m_mcTTFFont = new UITTFFont("Common/Media/font/JPN/DF-DotDotGothic16.ttf", 0x203B);
		break;
	case XC_LANGUAGE_SCHINESE:
	case XC_LANGUAGE_TCHINESE:
		m_mcTTFFont = new UITTFFont("Common/Media/font/CHT/DFTT_R5.TTC", 0x203B);
		break;
	case XC_LANGUAGE_KOREAN:
		m_mcTTFFont = new UITTFFont("Common/Media/font/KOR/candadite2.ttf", 0x203B);
		break;
#else
	case XC_LANGUAGE_JAPANESE:
		m_mcTTFFont = new UITTFFont("Common/Media/font/JPN/DFGMaruGothic-Md.ttf", 0x2022);
		break;
	case XC_LANGUAGE_SCHINESE:
	case XC_LANGUAGE_TCHINESE:
		m_mcTTFFont = new UITTFFont("Common/Media/font/CHT/DFHeiMedium-B5.ttf", 0x2022);
		break;
	case XC_LANGUAGE_KOREAN:
		m_mcTTFFont = new UITTFFont("Common/Media/font/KOR/BOKMSD.ttf", 0x2022);
		break;
#endif
	default:
		bBitmapFont=true;
		break;
	}

	if(bBitmapFont)
	{
		if(m_moj7==NULL)
		{
			m_moj7 = new UIBitmapFont(SFontData::Mojangles_7);
			m_moj7->registerFont();
		}
		if(m_moj11==NULL)
		{
			m_moj11 = new UIBitmapFont(SFontData::Mojangles_11);
			m_moj11->registerFont();
		}
	}
	else
	{
		app.DebugPrintf("IggyFontSetIndirectUTF8\n");
		IggyFontSetIndirectUTF8( "Mojangles7", -1, IGGY_FONTFLAG_all, "Mojangles_TTF",-1 ,IGGY_FONTFLAG_none );
		IggyFontSetIndirectUTF8( "Mojangles11", -1, IGGY_FONTFLAG_all, "Mojangles_TTF",-1 ,IGGY_FONTFLAG_none );
	}
}

// TICKING
void UIController::tick()
{
	if(m_navigateToHomeOnReload && !ui.IsReloadingSkin())
	{
		ui.CleanUpSkinReload();
		m_navigateToHomeOnReload = false;
		ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_MainMenu);
	}

	for(unsigned int i = 0; i < eUIGroup_COUNT; ++i)
	{
		if(m_bCloseAllScenes[i])
		{
			m_groups[i]->closeAllScenes();
			m_groups[i]->getTooltips()->SetTooltips(-1);
			m_bCloseAllScenes[i] = false;
		}
	}

	if(m_accumulatedTicks == 0) tickInput();
	m_accumulatedTicks = 0;

	for(unsigned int i = 0; i < eUIGroup_COUNT; ++i)
	{
		m_groups[i]->tick();
	}

	EnderDragonRenderer::bossInstance = nullptr;

	__int64 currentTime = System::currentTimeMillis();
	for(AUTO_VAR(it, m_cachedMovieData.begin()); it != m_cachedMovieData.end();)
	{
		if(it->second.m_expiry < currentTime)
		{
			delete [] it->second.m_ba.data;
			it = m_cachedMovieData.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void UIController::loadSkins()
{
	wstring platformSkinPath = L"";

#ifdef __PS3__
	platformSkinPath = L"skinPS3.swf";
#elif defined __PSVITA__
	platformSkinPath = L"skinVita.swf";
#elif defined _WINDOWS64
	if(m_fScreenHeight==1080.0f)
	{
		platformSkinPath = L"skinHDWin.swf";
	}
	else
	{
		platformSkinPath = L"skinWin.swf";	
	}
#elif defined _DURANGO
	if(m_fScreenHeight==1080.0f)
	{
		platformSkinPath = L"skinHDDurango.swf";	
	}
	else
	{
		platformSkinPath = L"skinDurango.swf";	
	}
#elif defined __ORBIS__
	if(m_fScreenHeight==1080.0f)
	{
		platformSkinPath = L"skinHDOrbis.swf";	
	}
	else
	{
		platformSkinPath = L"skinOrbis.swf";	
	}

#endif
	if(m_fScreenHeight==1080.0f)
	{
		m_iggyLibraries[eLibrary_Platform] = loadSkin(platformSkinPath, L"platformskinHD.swf");
	}
	else
	{
		m_iggyLibraries[eLibrary_Platform] = loadSkin(platformSkinPath, L"platformskin.swf");
	}

#if defined(__PS3__) || defined(__PSVITA__)
	m_iggyLibraries[eLibrary_GraphicsDefault] = loadSkin(L"skinGraphics.swf", L"skinGraphics.swf");
	m_iggyLibraries[eLibrary_GraphicsHUD] = loadSkin(L"skinGraphicsHud.swf", L"skinGraphicsHud.swf");
	m_iggyLibraries[eLibrary_GraphicsInGame] = loadSkin(L"skinGraphicsInGame.swf", L"skinGraphicsInGame.swf");
	m_iggyLibraries[eLibrary_GraphicsTooltips] = loadSkin(L"skinGraphicsTooltips.swf", L"skinGraphicsTooltips.swf");
	m_iggyLibraries[eLibrary_GraphicsLabels] = loadSkin(L"skinGraphicsLabels.swf", L"skinGraphicsLabels.swf");
	m_iggyLibraries[eLibrary_Labels] = loadSkin(L"skinLabels.swf", L"skinLabels.swf");
	m_iggyLibraries[eLibrary_InGame] = loadSkin(L"skinInGame.swf", L"skinInGame.swf");
	m_iggyLibraries[eLibrary_HUD] = loadSkin(L"skinHud.swf", L"skinHud.swf");
	m_iggyLibraries[eLibrary_Tooltips] = loadSkin(L"skinTooltips.swf", L"skinTooltips.swf");
	m_iggyLibraries[eLibrary_Default] = loadSkin(L"skin.swf", L"skin.swf");
#endif

#if ( defined(_WINDOWS64) || defined(_DURANGO) || defined(__ORBIS__) )

#if defined(_WINDOWS64)
#ifndef _FINAL_BUILD
	m_iggyLibraries[eLibraryFallback_GraphicsDefault] = loadSkin(L"skinGraphics.swf", L"skinGraphics.swf");
	m_iggyLibraries[eLibraryFallback_GraphicsHUD] = loadSkin(L"skinGraphicsHud.swf", L"skinGraphicsHud.swf");
	m_iggyLibraries[eLibraryFallback_GraphicsInGame] = loadSkin(L"skinGraphicsInGame.swf", L"skinGraphicsInGame.swf");
	m_iggyLibraries[eLibraryFallback_GraphicsTooltips] = loadSkin(L"skinGraphicsTooltips.swf", L"skinGraphicsTooltips.swf");
	m_iggyLibraries[eLibraryFallback_GraphicsLabels] = loadSkin(L"skinGraphicsLabels.swf", L"skinGraphicsLabels.swf");
	m_iggyLibraries[eLibraryFallback_Labels] = loadSkin(L"skinLabels.swf", L"skinLabels.swf");
	m_iggyLibraries[eLibraryFallback_InGame] = loadSkin(L"skinInGame.swf", L"skinInGame.swf");
	m_iggyLibraries[eLibraryFallback_HUD] = loadSkin(L"skinHud.swf", L"skinHud.swf");
	m_iggyLibraries[eLibraryFallback_Tooltips] = loadSkin(L"skinTooltips.swf", L"skinTooltips.swf");
	m_iggyLibraries[eLibraryFallback_Default] = loadSkin(L"skin.swf", L"skin.swf");
#endif
#endif

	m_iggyLibraries[eLibrary_GraphicsDefault] = loadSkin(L"skinHDGraphics.swf", L"skinHDGraphics.swf");
	m_iggyLibraries[eLibrary_GraphicsHUD] = loadSkin(L"skinHDGraphicsHud.swf", L"skinHDGraphicsHud.swf");
	m_iggyLibraries[eLibrary_GraphicsInGame] = loadSkin(L"skinHDGraphicsInGame.swf", L"skinHDGraphicsInGame.swf");
	m_iggyLibraries[eLibrary_GraphicsTooltips] = loadSkin(L"skinHDGraphicsTooltips.swf", L"skinHDGraphicsTooltips.swf");
	m_iggyLibraries[eLibrary_GraphicsLabels] = loadSkin(L"skinHDGraphicsLabels.swf", L"skinHDGraphicsLabels.swf");
	m_iggyLibraries[eLibrary_Labels] = loadSkin(L"skinHDLabels.swf", L"skinHDLabels.swf");
	m_iggyLibraries[eLibrary_InGame] = loadSkin(L"skinHDInGame.swf", L"skinHDInGame.swf");
	m_iggyLibraries[eLibrary_HUD] = loadSkin(L"skinHDHud.swf", L"skinHDHud.swf");
	m_iggyLibraries[eLibrary_Tooltips] = loadSkin(L"skinHDTooltips.swf", L"skinHDTooltips.swf");
	m_iggyLibraries[eLibrary_Default] = loadSkin(L"skinHD.swf", L"skinHD.swf");
#endif
}

IggyLibrary UIController::loadSkin(const wstring &skinPath, const wstring &skinName)
{
	IggyLibrary lib = IGGY_INVALID_LIBRARY;
	if(!skinPath.empty() && app.hasArchiveFile(skinPath))
	{
		byteArray baFile = app.getArchiveFile(skinPath);
		lib = IggyLibraryCreateFromMemoryUTF16( (IggyUTF16 *)skinName.c_str() , (void *)baFile.data, baFile.length, NULL );

		delete[] baFile.data;
#ifdef _DEBUG
		IggyMemoryUseInfo memoryInfo;
		rrbool res;
		int iteration = 0;
		__int64 totalStatic = 0;
		while(res = IggyDebugGetMemoryUseInfo ( NULL ,
			lib ,
			"" ,
			0 ,
			iteration ,
			&memoryInfo ))
		{
			totalStatic += memoryInfo.static_allocation_bytes;
			app.DebugPrintf(app.USER_SR, "%ls - %.*s, static: %dB, dynamic: %dB\n", skinPath.c_str(), memoryInfo.subcategory_stringlen, memoryInfo.subcategory, memoryInfo.static_allocation_bytes, memoryInfo.dynamic_allocation_bytes);
			++iteration;
		}

		app.DebugPrintf(app.USER_SR, "%ls - Total static: %dB (%dKB)\n", skinPath.c_str(), totalStatic, totalStatic/1024);
#endif
	}
	return lib;
}

void UIController::ReloadSkin()
{
	for(unsigned int i = 0; i < eUIGroup_COUNT; ++i)
	{
		m_groups[i]->DestroyAll();
	}

	for(int i = eLibrary_Count - 1; i >= 0; --i)
	{
		if(m_iggyLibraries[i] != IGGY_INVALID_LIBRARY) IggyLibraryDestroy(m_iggyLibraries[i]);
		m_iggyLibraries[i] = IGGY_INVALID_LIBRARY;
	}

#ifdef _WINDOWS64
	reloadSkinThreadProc(this);
#else
	ui.NavigateToScene(0,eUIScene_Timer,(void *)1,eUILayer_Tooltips,eUIGroup_Fullscreen);

	m_reloadSkinThread = new C4JThread(reloadSkinThreadProc, (void*)this, "Reload skin thread");
	m_reloadSkinThread->SetProcessor(CPU_CORE_UI_SCENE);
#endif
}

void UIController::StartReloadSkinThread()
{
	if(m_reloadSkinThread) m_reloadSkinThread->Run();
}

int UIController::reloadSkinThreadProc(void* lpParam)
{
	EnterCriticalSection(&ms_reloadSkinCS);
	UIController *controller = (UIController *)lpParam;
	controller->loadSkins();

	for(int i = eUIGroup_Player1; i < eUIGroup_COUNT; ++i)
	{
		controller->m_groups[i]->ReloadAll();
	}

	controller->m_groups[eUIGroup_Fullscreen]->ReloadAll();

#ifndef _WINDOW64
	controller->NavigateBack(0, false, eUIScene_COUNT, eUILayer_Tooltips);
#endif
	LeaveCriticalSection(&ms_reloadSkinCS);

	return 0;
}

bool UIController::IsReloadingSkin()
{
	return m_reloadSkinThread && (!m_reloadSkinThread->hasStarted() || m_reloadSkinThread->isRunning());
}

bool UIController::IsExpectingOrReloadingSkin()
{
	return Minecraft::GetInstance()->skins->getSelected()->isLoadingData() || Minecraft::GetInstance()->skins->needsUIUpdate() || IsReloadingSkin();
}

void UIController::CleanUpSkinReload()
{
	delete m_reloadSkinThread;
	m_reloadSkinThread = NULL;

	if(!Minecraft::GetInstance()->skins->isUsingDefaultSkin())
	{
		if(!Minecraft::GetInstance()->skins->getSelected()->hasAudio())
		{
#ifdef _DURANGO			
			DWORD result = StorageManager.UnmountInstalledDLC(L"TPACK");
#else
			DWORD result = StorageManager.UnmountInstalledDLC("TPACK");
#endif
		}
	}

	for(AUTO_VAR(it,m_queuedMessageBoxData.begin()); it != m_queuedMessageBoxData.end(); ++it)
	{
		QueuedMessageBoxData *queuedData = *it;
		ui.NavigateToScene(queuedData->iPad, eUIScene_MessageBox, &queuedData->info, queuedData->layer, eUIGroup_Fullscreen);
		delete queuedData->info.uiOptionA;
		delete queuedData;
	}
	m_queuedMessageBoxData.clear();
}

byteArray UIController::getMovieData(const wstring &filename)
{
	__int64 targetTime = System::currentTimeMillis() + (1000LL * 60);
	AUTO_VAR(it,m_cachedMovieData.find(filename));
	if(it == m_cachedMovieData.end() )
	{
		byteArray baFile = app.getArchiveFile(filename);
		CachedMovieData cmd;
		cmd.m_ba = baFile;
		cmd.m_expiry = targetTime;
		m_cachedMovieData[filename] = cmd;
		return baFile;
	}
	else
	{
		it->second.m_expiry = targetTime;
		return it->second.m_ba;
	}
}

#ifdef _WINDOWS64
static bool FocusMouseControl(UIGroup *pGroup, S32 mouseX, S32 mouseY, S32 screenW, S32 screenH);
#endif

// INPUT
void UIController::tickInput()
{
	if(!m_bSystemUIShowing)
	{
#ifdef ENABLE_IGGY_PERFMON
		if (m_iggyPerfmonEnabled)
		{
			if(InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(), ACTION_MENU_STICK_PRESS)) m_iggyPerfmonEnabled = !m_iggyPerfmonEnabled;
		}
		else
#endif
		{
			handleInput();
			++m_accumulatedTicks;
		}
	}
}

void UIController::handleInput()
{
	for(unsigned int iPad = 0; iPad < XUSER_MAX_COUNT; ++iPad)
	{
#ifdef _DURANGO
		if(iPad != ProfileManager.GetPrimaryPad() 
			&& (!InputManager.IsPadConnected(iPad) || !InputManager.IsPadLocked(iPad)) ) continue;
#endif
#ifdef _WINDOWS64
		// FIX: Update mouse focus for ALL menus every tick, not just when a menu is displayed.
		// This ensures submenus and alert layers also respond to hover/focus correctly.
		if((int)iPad == ProfileManager.GetPrimaryPad())
		{
			const S32 mouseX = (S32)Mouse::getX();
			const S32 mouseY = (S32)Mouse::getY();
			const S32 screenW = (S32)getScreenWidth();
			const S32 screenH = (S32)getScreenHeight();

			// FIX: Try ALL groups when updating mouse focus, not just Fullscreen + player group.
			// Submenus can live in any group/layer so we need to check them all.
			bool mouseHitControl = false;
			for(int g = eUIGroup_COUNT - 1; g >= 0 && !mouseHitControl; --g)
			{
				mouseHitControl = FocusMouseControl(m_groups[g], mouseX, mouseY, screenW, screenH);
			}
		}
#endif
		for(unsigned int key = 0; key <= ACTION_MAX_MENU; ++key)
		{
			handleKeyPress(iPad, key);
		}

#ifdef __PSVITA__
		handleKeyPress(iPad, MINECRAFT_ACTION_GAME_INFO);
#endif
	}

#ifdef _DURANGO
	if(!app.GetGameStarted())
	{
		bool repeat = false;
		int firstUnfocussedUnhandledPad = -1;

		for(unsigned int iPad = XUSER_MAX_COUNT; iPad < (XUSER_MAX_COUNT + InputManager.MAX_GAMEPADS); ++iPad)
		{
			if(InputManager.IsPadLocked(iPad) || !InputManager.IsPadConnected(iPad) ) continue;

			for(unsigned int key = 0; key <= ACTION_MAX_MENU; ++key)
			{
				bool pressed = InputManager.ButtonPressed(iPad,key);
				bool released = InputManager.ButtonReleased(iPad,key);

				if(pressed || released)
				{
					bool handled = false;
					m_groups[(int)eUIGroup_Fullscreen]->handleInput(iPad, key, repeat, pressed, released, handled);

					if(firstUnfocussedUnhandledPad < 0 && !m_groups[(int)eUIGroup_Fullscreen]->HasFocus(iPad))
					{
						firstUnfocussedUnhandledPad = iPad;
					}
				}
			}
		}

		if(ProfileManager.GetLockedProfile() >= 0 && !InputManager.IsPadLocked( ProfileManager.GetLockedProfile() ) && firstUnfocussedUnhandledPad >= 0)
		{
			ProfileManager.RequestSignInUI(false, false, false, false, true, NULL, NULL, firstUnfocussedUnhandledPad );
		}
	}
#endif
}

#ifdef _WINDOWS64
static bool IsMouseHitControlType(UIControl::eUIControlType controlType)
{
	return controlType != UIControl::eNoControl &&
		controlType != UIControl::eCursor &&
		controlType != UIControl::eLabel &&
		controlType != UIControl::eDynamicLabel &&
		controlType != UIControl::eHTMLLabel &&
		controlType != UIControl::eBitmapIcon &&
		controlType != UIControl::eMinecraftPlayer &&
		controlType != UIControl::ePlayerSkinPreview &&
		controlType != UIControl::eProgress;
}

static int s_iPendingMouseWheelMenuSteps = 0;

static void AddKeyboardState(bool &down, bool &pressed, bool &released, int keyCode)
{
	down = down || Keyboard::isKeyDown(keyCode);
	pressed = pressed || Keyboard::isKeyPressed(keyCode);
	released = released || Keyboard::isKeyReleased(keyCode);
}

static void ApplyWindowsMenuKeyboardFallback(unsigned int action, bool &down, bool &pressed, bool &released)
{
	switch(action)
	{
	case ACTION_MENU_UP:
		AddKeyboardState(down, pressed, released, VK_UP);
		AddKeyboardState(down, pressed, released, 'W');
		break;
	case ACTION_MENU_DOWN:
		AddKeyboardState(down, pressed, released, VK_DOWN);
		AddKeyboardState(down, pressed, released, 'S');
		break;
	case ACTION_MENU_LEFT:
		AddKeyboardState(down, pressed, released, VK_LEFT);
		AddKeyboardState(down, pressed, released, 'A');
		break;
	case ACTION_MENU_RIGHT:
		AddKeyboardState(down, pressed, released, VK_RIGHT);
		AddKeyboardState(down, pressed, released, 'D');
		break;
	case ACTION_MENU_LEFT_SCROLL:
		AddKeyboardState(down, pressed, released, 'Q');
		break;
	case ACTION_MENU_RIGHT_SCROLL:
		AddKeyboardState(down, pressed, released, 'E');
		break;
	case ACTION_MENU_PAGEUP:
		AddKeyboardState(down, pressed, released, VK_PRIOR);
		break;
	case ACTION_MENU_PAGEDOWN:
		AddKeyboardState(down, pressed, released, VK_NEXT);
		break;
	case ACTION_MENU_OK:
		AddKeyboardState(down, pressed, released, VK_RETURN);
		AddKeyboardState(down, pressed, released, VK_SPACE);
		break;
	case ACTION_MENU_CANCEL:
	case ACTION_MENU_B:
		AddKeyboardState(down, pressed, released, VK_ESCAPE);
		AddKeyboardState(down, pressed, released, VK_BACK);
		break;
	}
}

static void ApplyWindowsMenuMouseWheelFallback(unsigned int action, bool menuDisplayed, bool &down, bool &pressed)
{
	if(!menuDisplayed)
	{
		s_iPendingMouseWheelMenuSteps = 0;
		return;
	}

	if(action == ACTION_MENU_UP && s_iPendingMouseWheelMenuSteps == 0)
	{
		s_iPendingMouseWheelMenuSteps = Mouse::getWheelDelta();
	}

	if(action == ACTION_MENU_UP && s_iPendingMouseWheelMenuSteps > 0)
	{
		down = true;
		pressed = true;
		--s_iPendingMouseWheelMenuSteps;
	}
	else if(action == ACTION_MENU_DOWN && s_iPendingMouseWheelMenuSteps < 0)
	{
		down = true;
		pressed = true;
		++s_iPendingMouseWheelMenuSteps;
	}
}

static void GetViewportRect(C4JRender::eViewportType viewport, S32 screenW, S32 screenH, S32 &x, S32 &y, S32 &w, S32 &h)
{
	x = 0;
	y = 0;
	w = screenW;
	h = screenH;

	switch(viewport)
	{
	case C4JRender::VIEWPORT_TYPE_FULLSCREEN:
		break;
	case C4JRender::VIEWPORT_TYPE_SPLIT_TOP:
		x = screenW / 4;
		w = screenW / 2;
		h = screenH / 2;
		break;
	case C4JRender::VIEWPORT_TYPE_SPLIT_BOTTOM:
		x = screenW / 4;
		y = screenH / 2;
		w = screenW / 2;
		h = screenH / 2;
		break;
	case C4JRender::VIEWPORT_TYPE_SPLIT_LEFT:
		y = screenH / 4;
		w = screenW / 2;
		h = screenH / 2;
		break;
	case C4JRender::VIEWPORT_TYPE_SPLIT_RIGHT:
		x = screenW / 2;
		y = screenH / 4;
		w = screenW / 2;
		h = screenH / 2;
		break;
	case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
		w = screenW / 2;
		h = screenH / 2;
		break;
	case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
		x = screenW / 2;
		w = screenW / 2;
		h = screenH / 2;
		break;
	case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
		y = screenH / 2;
		w = screenW / 2;
		h = screenH / 2;
		break;
	case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
		x = screenW / 2;
		y = screenH / 2;
		w = screenW / 2;
		h = screenH / 2;
		break;
	}
}

static bool FocusMouseControlInScene(UIScene *pScene, S32 mouseX, S32 mouseY, S32 viewportX, S32 viewportY, S32 viewportW, S32 viewportH)
{
	if(!pScene) return false;
	if(!pScene->hasMovie()) return false;
	if(viewportW <= 0 || viewportH <= 0) return false;
	if(mouseX < viewportX || mouseX > (viewportX + viewportW) || mouseY < viewportY || mouseY > (viewportY + viewportH)) return false;

	// FIX: Always refresh control state before hit-testing.
	// Stale control positions are the main reason submenu buttons don't respond —
	// UpdateSceneControls was only called once at scene open, not each tick.
	pScene->UpdateSceneControls();

	vector<UIControl *> *pControls = pScene->GetControls();
	if(!pControls) return false;

	S32 localX = mouseX - viewportX;
	S32 localY = mouseY - viewportY;
	const S32 sceneW = pScene->getRenderWidth();
	const S32 sceneH = pScene->getRenderHeight();
	if(sceneW > 0 && sceneH > 0)
	{
		localX = (localX * sceneW) / viewportW;
		localY = (localY * sceneH) / viewportH;
	}

	S32 iMainPanelOffsetX = 0;
	S32 iMainPanelOffsetY = 0;
	UIControl *pMainPanel = pScene->GetMainPanel();
	if(pMainPanel)
	{
		// FIX: Always update the main panel so its offset is current.
		// Without this, submenus that animate in have wrong offsets at first.
		pMainPanel->UpdateControl();

		if(!pMainPanel->getHidden())
		{
			iMainPanelOffsetX = pMainPanel->getXPos();
			iMainPanelOffsetY = pMainPanel->getYPos();
		}
	}

	for(S32 controlIndex = (S32)pControls->size() - 1; controlIndex >= 0; --controlIndex)
	{
		UIControl *control = (UIControl *)(*pControls)[controlIndex];
		if(!control || control->getHidden() || !control->isVisible())
		{
			continue;
		}

		if(!IsMouseHitControlType(control->getControlType()))
		{
			continue;
		}

		// FIX: Update each control's position before hit-testing it.
		// Flash animations can move controls after the scene opened, so cached
		// positions can be wrong — especially in submenus that slide in.
		control->UpdateControl();

		S32 iControlWidth = control->getWidth();
		S32 iControlHeight = control->getHeight();

		if(control->getControlType() == UIControl::eSlider)
		{
			UIControl_Slider *pSlider = (UIControl_Slider *)control;
			iControlWidth = pSlider->GetRealWidth();
		}
		else if(control->getControlType() == UIControl::eTexturePackList)
		{
			UIControl_TexturePackList *pTexturePackList = (UIControl_TexturePackList *)control;
			iControlHeight = pTexturePackList->GetRealHeight();
		}
		else if(control->getControlType() == UIControl::eDynamicLabel)
		{
			UIControl_DynamicLabel *pDynamicLabel = (UIControl_DynamicLabel *)control;
			iControlWidth = pDynamicLabel->GetRealWidth();
			iControlHeight = pDynamicLabel->GetRealHeight();
		}
		else if(control->getControlType() == UIControl::eHTMLLabel)
		{
			UIControl_HTMLLabel *pHtmlLabel = (UIControl_HTMLLabel *)control;
			iControlWidth = pHtmlLabel->GetRealWidth();
			iControlHeight = pHtmlLabel->GetRealHeight();
		}

		const S32 x1 = control->getXPos() + iMainPanelOffsetX;
		const S32 y1 = control->getYPos() + iMainPanelOffsetY;
		const S32 x2 = x1 + iControlWidth;
		const S32 y2 = y1 + iControlHeight;

		const bool mouseDragging = Mouse::isButtonDown(0) || Mouse::isButtonPressed(0);
		bool mouseHitControl = (localX >= x1 && localX <= x2 && localY >= y1 && localY <= y2);

		if(!mouseHitControl && control->getControlType() == UIControl::eSlider && mouseDragging && pScene->getControlFocus() == control->getId())
		{
			const S32 dragPaddingX = ((iControlWidth / 5) > 16) ? (iControlWidth / 5) : 16;
			const S32 dragPaddingY = (iControlHeight > 12) ? iControlHeight : 12;
			if(localX >= (x1 - dragPaddingX) && localX <= (x2 + dragPaddingX) &&
			   localY >= (y1 - dragPaddingY) && localY <= (y2 + dragPaddingY))
			{
				mouseHitControl = true;
			}
		}

		if(mouseHitControl)
		{
			if(control->getControlType() == UIControl::eSlider)
			{
				UIControl_Slider *pSlider = (UIControl_Slider *)control;
				if(mouseDragging)
				{
					S32 sliderLocalX = localX - x1;
					if(sliderLocalX < 0) sliderLocalX = 0;
					if(sliderLocalX > iControlWidth) sliderLocalX = iControlWidth;

					float fNewSliderPos = 0.0f;
					if(iControlWidth > 0)
					{
						fNewSliderPos = (float)sliderLocalX / (float)iControlWidth;
					}
					pSlider->SetSliderTouchPos(fNewSliderPos);
				}
			}

			if(control->getControlType() == UIControl::eButtonList)
			{
				UIControl_ButtonList *pButtonList = (UIControl_ButtonList *)control;
				if(!pButtonList->CanTouchTrigger(localX, localY))
				{
					continue;
				}
				pButtonList->SetTouchFocus(localX, localY, false);
			}

			if(pScene->getControlFocus() != control->getId())
			{
				pScene->SetFocusToElement(control->getId());
			}
			return true;
		}
	}

	return false;
}

static bool FocusMouseControl(UIGroup *pGroup, S32 mouseX, S32 mouseY, S32 screenW, S32 screenH)
{
	if(!pGroup) return false;

	S32 viewportX = 0;
	S32 viewportY = 0;
	S32 viewportW = 0;
	S32 viewportH = 0;
	GetViewportRect(pGroup->GetViewportType(), screenW, screenH, viewportX, viewportY, viewportW, viewportH);

	// FIX: Check layers from top to bottom (highest index first) so that
	// alert/error dialogs on top layers capture the click before layers beneath them.
	for(int i = (int)eUILayer_COUNT - 1; i >= 0; --i)
	{
		if(FocusMouseControlInScene(pGroup->GetTopScene((EUILayer)i), mouseX, mouseY, viewportX, viewportY, viewportW, viewportH))
		{
			return true;
		}
	}

	return FocusMouseControlInScene(pGroup->getTooltips(), mouseX, mouseY, viewportX, viewportY, viewportW, viewportH);
}
#endif

void UIController::handleKeyPress(unsigned int iPad, unsigned int key)
{
	bool down = false;
	bool pressed = false;
	bool released = false;
	bool repeat = false;

#ifdef __PSVITA__
	if(key==ACTION_MENU_OK)
	{
		bool bTouchScreenInput=false;

		SceTouchData* pTouchData = InputManager.GetTouchPadData(iPad,false);

		if((m_bTouchscreenPressed==false) && pTouchData->reportNum==1)
		{
			m_ActiveUIElement = NULL;
			m_HighlightedUIElement = NULL; 

			UIScene *pScene=m_groups[(int)eUIGroup_Fullscreen]->getCurrentScene();
			UIScene *pToolTips=m_groups[(int)eUIGroup_Fullscreen]->getTooltips();
			if(pScene)
			{
				if(TouchBoxHit(pScene,pTouchData->report[0].x,pTouchData->report[0].y))
				{
					down=pressed=m_bTouchscreenPressed=true;
					bTouchScreenInput=true;
				}
				else if(TouchBoxHit(pToolTips,pTouchData->report[0].x,pTouchData->report[0].y))
				{
					down=pressed=m_bTouchscreenPressed=true;
					bTouchScreenInput=true;
				}
			}
			else
			{
				pScene=m_groups[(EUIGroup)(iPad+1)]->getCurrentScene();
				pToolTips=m_groups[(int)iPad+1]->getTooltips();
				if(pScene)
				{
					if(TouchBoxHit(pScene,pTouchData->report[0].x,pTouchData->report[0].y))
					{
						down=pressed=m_bTouchscreenPressed=true;
						bTouchScreenInput=true;
					}
					else if(TouchBoxHit(pToolTips,pTouchData->report[0].x,pTouchData->report[0].y))
					{
						down=pressed=m_bTouchscreenPressed=true;
						bTouchScreenInput=true;
					}
				}
				else if(pToolTips)
				{
					if(TouchBoxHit(pToolTips,pTouchData->report[0].x,pTouchData->report[0].y))
					{
						down=pressed=m_bTouchscreenPressed=true;
						bTouchScreenInput=true;
					}
				}
			}
		}	
		else if(m_bTouchscreenPressed && pTouchData->reportNum==1)
		{
			UIScene *pScene=m_groups[(int)eUIGroup_Fullscreen]->getCurrentScene();
			UIScene *pToolTips=m_groups[(int)eUIGroup_Fullscreen]->getTooltips();
			if(pScene)
			{
				if(TouchBoxHit(pScene,pTouchData->report[0].x,pTouchData->report[0].y))
				{
					down=true;
					bTouchScreenInput=true;
				}
				else if(TouchBoxHit(pToolTips,pTouchData->report[0].x,pTouchData->report[0].y))
				{
					down=true;
					bTouchScreenInput=true;
				}
			}
			else
			{
				pScene=m_groups[(EUIGroup)(iPad+1)]->getCurrentScene();
				pToolTips=m_groups[(int)iPad+1]->getTooltips();
				if(pScene)
				{
					if(TouchBoxHit(pScene,pTouchData->report[0].x,pTouchData->report[0].y))
					{
						down=true;
						bTouchScreenInput=true;
					}
					else if(TouchBoxHit(pToolTips,pTouchData->report[0].x,pTouchData->report[0].y))
					{
						down=true;
						bTouchScreenInput=true;
					}
				}
				else if(pToolTips)
				{
					if(TouchBoxHit(pToolTips,pTouchData->report[0].x,pTouchData->report[0].y))
					{
						down=true;
						bTouchScreenInput=true;
					}
				}
			}
		}
		else if(m_bTouchscreenPressed && pTouchData->reportNum==0)
		{
			bTouchScreenInput=true;
			m_bTouchscreenPressed=false;
			released=true;
		}

		if(pressed)
		{
			m_actionRepeatTimer[iPad][key] = GetTickCount() + UI_REPEAT_KEY_DELAY_MS;
		}
		else if (released)
		{
			m_actionRepeatTimer[iPad][key] = 0;
		}
		else if (down)
		{
			DWORD currentTime = GetTickCount();
			if(m_actionRepeatTimer[iPad][key] > 0 && currentTime > m_actionRepeatTimer[iPad][key])
			{
				repeat = true;
				pressed = true;
				m_actionRepeatTimer[iPad][key] = currentTime + UI_REPEAT_KEY_REPEAT_RATE_MS;
			}
		}

		HandleTouchInput(iPad, key, pressed, repeat, released);

		if(bTouchScreenInput) return;
	}
#endif

	down = InputManager.ButtonDown(iPad,key);
	pressed = InputManager.ButtonPressed(iPad,key);
	released = InputManager.ButtonReleased(iPad,key);

#ifdef _WINDOWS64
	if((int)iPad == ProfileManager.GetPrimaryPad())
	{
		const bool menuDisplayed = GetMenuDisplayed(iPad) || m_groups[(int)eUIGroup_Fullscreen]->GetMenuDisplayed();

		if(key == ACTION_MENU_OK)
		{
			if(Mouse::isButtonPressed(0))
			{
				const S32 mouseX = (S32)Mouse::getX();
				const S32 mouseY = (S32)Mouse::getY();
				const S32 screenW = (S32)getScreenWidth();
				const S32 screenH = (S32)getScreenHeight();

				// FIX: Try ALL groups top-down when a click lands, not just
				// Fullscreen + player group. This is what makes submenu buttons
				// actually register the click after focus has been set correctly.
				bool mouseHitControl = false;
				for(int g = (int)eUIGroup_COUNT - 1; g >= 0 && !mouseHitControl; --g)
				{
					mouseHitControl = FocusMouseControl(m_groups[g], mouseX, mouseY, screenW, screenH);
				}

				// FIX: Always activate the click as long as any menu is visible,
				// even if no hittable control was found under the cursor. This lets
				// Escape/Cancel areas and modal backdrops work too.
				if(mouseHitControl || menuDisplayed)
				{
					down = true;
					pressed = true;
				}
			}
		}
		ApplyWindowsMenuKeyboardFallback(key, down, pressed, released);
		ApplyWindowsMenuMouseWheelFallback(key, menuDisplayed, down, pressed);
	}
#endif

	if(pressed) app.DebugPrintf("Pressed %d\n",key);
	if(released) app.DebugPrintf("Released %d\n",key);

	if(pressed)
	{
		m_actionRepeatTimer[iPad][key] = GetTickCount() + UI_REPEAT_KEY_DELAY_MS;
	}
	else if (released)
	{
		m_actionRepeatTimer[iPad][key] = 0;
	}
	else if (down)
	{
		DWORD currentTime = GetTickCount();
		if(m_actionRepeatTimer[iPad][key] > 0 && currentTime > m_actionRepeatTimer[iPad][key])
		{
			repeat = true;
			pressed = true;
			m_actionRepeatTimer[iPad][key] = currentTime + UI_REPEAT_KEY_REPEAT_RATE_MS;
		}
	}

#ifndef _CONTENT_PACKAGE

#ifdef ENABLE_IGGY_PERFMON
	if ( pressed && !repeat && key == ACTION_MENU_STICK_PRESS)
	{
		m_iggyPerfmonEnabled = !m_iggyPerfmonEnabled;
	}
#endif

#endif
	if(repeat || pressed || released)
	{
		bool handled = false;

		m_groups[(int)eUIGroup_Fullscreen]->handleInput(iPad, key, repeat, pressed, released, handled);
		if(!handled)
		{
			m_groups[(iPad+1)]->handleInput(iPad, key, repeat, pressed, released, handled);
		}
	}
}

rrbool RADLINK UIController::ExternalFunctionCallback( void * user_callback_data , Iggy * player , IggyExternalFunctionCallUTF16 * call)
{
	UIScene *scene = (UIScene *)IggyPlayerGetUserdata(player);

	if(scene != NULL)
	{
		scene->externalCallback(call);
	}

	return true;
}

// RENDERING
void UIController::renderScenes()
{
	PIXBeginNamedEvent(0, "Rendering Iggy scenes");
	if(app.GetGameStarted() && !m_groups[eUIGroup_Fullscreen]->hidesLowerScenes())
	{
		for(int i = eUIGroup_Player1; i < eUIGroup_COUNT; ++i)
		{
			PIXBeginNamedEvent(0, "Rendering layer %d scenes", i);
			m_groups[i]->render();
			PIXEndNamedEvent();
		}
	}

	PIXBeginNamedEvent(0, "Rendering fullscreen scenes");
	m_groups[eUIGroup_Fullscreen]->render();
	PIXEndNamedEvent();

	PIXEndNamedEvent();

#ifdef ENABLE_IGGY_PERFMON
	if (m_iggyPerfmonEnabled)
	{
		IggyPerfmonPad pm_pad;

		pm_pad.bits = 0;
		pm_pad.field.dpad_up           = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_UP);
		pm_pad.field.dpad_down         = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_DOWN);
		pm_pad.field.dpad_left         = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_LEFT);
		pm_pad.field.dpad_right        = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_RIGHT);
		pm_pad.field.button_up         = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_Y);
		pm_pad.field.button_down       = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_A);
		pm_pad.field.button_left       = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_X);
		pm_pad.field.button_right      = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_B);
		pm_pad.field.shoulder_left_hi  = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_LEFT_SCROLL);
		pm_pad.field.shoulder_right_hi = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_RIGHT_SCROLL);
		pm_pad.field.trigger_left_low  = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_PAGEUP);
		pm_pad.field.trigger_right_low = InputManager.ButtonPressed(ProfileManager.GetPrimaryPad(),ACTION_MENU_PAGEDOWN);

		IggyPerfmonTickAndDraw(iggy_perfmon, gdraw_funcs, &pm_pad,
			PM_ORIGIN_X, PM_ORIGIN_Y, getScreenWidth(), getScreenHeight());
	}
#endif
}

void UIController::getRenderDimensions(C4JRender::eViewportType viewport, S32 &width, S32 &height)
{
	switch( viewport )
	{
	case C4JRender::VIEWPORT_TYPE_FULLSCREEN:
		width = (S32)(getScreenWidth());
		height = (S32)(getScreenHeight());
		break;
	case C4JRender::VIEWPORT_TYPE_SPLIT_TOP:
	case C4JRender::VIEWPORT_TYPE_SPLIT_BOTTOM:
		width = (S32)(getScreenWidth() / 2);
		height = (S32)(getScreenHeight() / 2);
		break;
	case C4JRender::VIEWPORT_TYPE_SPLIT_LEFT:
	case C4JRender::VIEWPORT_TYPE_SPLIT_RIGHT:
		width = (S32)(getScreenWidth() / 2);
		height = (S32)(getScreenHeight() / 2);
		break;
	case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
	case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
	case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
	case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
		width = (S32)(getScreenWidth() / 2);
		height = (S32)(getScreenHeight() / 2);
		break;
	}
}

void UIController::setupRenderPosition(C4JRender::eViewportType viewport)
{
	if(m_bCustomRenderPosition || m_currentRenderViewport != viewport)
	{
		m_currentRenderViewport = viewport;
		m_bCustomRenderPosition = false;
		S32 xPos = 0;
		S32 yPos = 0;
		switch( viewport )
		{
		case C4JRender::VIEWPORT_TYPE_SPLIT_TOP:
			xPos = (S32)(getScreenWidth() / 4);
			break;
		case C4JRender::VIEWPORT_TYPE_SPLIT_BOTTOM:
			xPos = (S32)(getScreenWidth() / 4);
			yPos = (S32)(getScreenHeight() / 2);
			break;
		case C4JRender::VIEWPORT_TYPE_SPLIT_LEFT:
			yPos = (S32)(getScreenHeight() / 4);
			break;
		case C4JRender::VIEWPORT_TYPE_SPLIT_RIGHT:
			xPos = (S32)(getScreenWidth() / 2);
			yPos = (S32)(getScreenHeight() / 4);
			break;
		case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
			break;
		case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
			xPos = (S32)(getScreenWidth() / 2);
			break;
		case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
			yPos = (S32)(getScreenHeight() / 2);
			break;
		case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
			xPos = (S32)(getScreenWidth() / 2);
			yPos = (S32)(getScreenHeight() / 2);
			break;
		}
		m_tileOriginX = xPos;
		m_tileOriginY = yPos;
		setTileOrigin(xPos, yPos);
	}
}

void UIController::setupRenderPosition(S32 xOrigin, S32 yOrigin)
{
	m_bCustomRenderPosition = true;
	m_tileOriginX = xOrigin;
	m_tileOriginY = yOrigin;
	setTileOrigin(xOrigin, yOrigin);
}

void UIController::setupCustomDrawGameState()
{
	m_customRenderingClearRect.left = LONG_MAX;
	m_customRenderingClearRect.right = LONG_MIN;
	m_customRenderingClearRect.top = LONG_MAX;
	m_customRenderingClearRect.bottom = LONG_MIN;

#if defined _WINDOWS64 || _DURANGO
	PIXBeginNamedEvent(0,"StartFrame");
	RenderManager.StartFrame();
	PIXEndNamedEvent();
	gdraw_D3D11_setViewport_4J();
#elif defined __PS3__
	RenderManager.StartFrame();
#elif defined __PSVITA__
	RenderManager.StartFrame();
#elif defined __ORBIS__
	RenderManager.StartFrame(false);
	gdraw_orbis_setViewport_4J();
#endif
	RenderManager.Set_matrixDirty();

	PIXBeginNamedEvent(0,"Final setup");
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, m_fScreenWidth, m_fScreenHeight, 0, 1000, 3000);
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_ALPHA_TEST);			
	glAlphaFunc(GL_GREATER, 0.1f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(true);
	PIXEndNamedEvent();
}

void UIController::setupCustomDrawMatrices(UIScene *scene, CustomDrawData *customDrawRegion)
{
	Minecraft *pMinecraft=Minecraft::GetInstance();

	float sceneWidth = (float)scene->getRenderWidth();
	float sceneHeight = (float)scene->getRenderHeight();

	LONG left, right, top, bottom;
#ifdef __PS3__
	if(!RenderManager.IsHiDef() && !RenderManager.IsWidescreen())
	{
		left = m_tileOriginX + (sceneWidth + customDrawRegion->mat[(0*4)+3]*sceneWidth);
		right = left + ( (sceneWidth * customDrawRegion->mat[0]) ) * customDrawRegion->x1;
	}
	else
#endif
	{
		left = m_tileOriginX + (sceneWidth + customDrawRegion->mat[(0*4)+3]*sceneWidth)/2;
		right = left + ( (sceneWidth * customDrawRegion->mat[0])/2 ) * customDrawRegion->x1;
	}

	top = m_tileOriginY + (sceneHeight - customDrawRegion->mat[(1*4)+3]*sceneHeight)/2;
	bottom = top + (sceneHeight * -customDrawRegion->mat[(1*4) + 1])/2 * customDrawRegion->y1;

	m_customRenderingClearRect.left = min(m_customRenderingClearRect.left, left);
	m_customRenderingClearRect.right = max(m_customRenderingClearRect.right, right);;
	m_customRenderingClearRect.top = min(m_customRenderingClearRect.top, top);
	m_customRenderingClearRect.bottom = max(m_customRenderingClearRect.bottom, bottom);

	if(!m_bScreenWidthSetup)
	{
		Minecraft *pMinecraft=Minecraft::GetInstance();
		if(pMinecraft != NULL)
		{
			m_fScreenWidth=(float)pMinecraft->width_phys;
			m_fScreenHeight=(float)pMinecraft->height_phys;
			m_bScreenWidthSetup = true;
		}
	}

	glLoadIdentity();
	glTranslatef(0, 0, -2000);
	glTranslatef((m_fScreenWidth + customDrawRegion->mat[(0*4)+3]*m_fScreenWidth)/2,(m_fScreenHeight - customDrawRegion->mat[(1*4)+3]*m_fScreenHeight)/2,0);
	glScalef( (m_fScreenWidth * customDrawRegion->mat[0])/2,(m_fScreenHeight * -customDrawRegion->mat[(1*4) + 1])/2,1.0f);
}

void UIController::setupCustomDrawGameStateAndMatrices(UIScene *scene, CustomDrawData *customDrawRegion)
{
	setupCustomDrawGameState();
	setupCustomDrawMatrices(scene, customDrawRegion);
}

void UIController::endCustomDrawGameState()
{
#ifdef __ORBIS__
	RenderManager.Clear(GL_DEPTH_BUFFER_BIT);
#else
	RenderManager.Clear(GL_DEPTH_BUFFER_BIT, &m_customRenderingClearRect);
#endif
	glDepthMask(false);
	glDisable(GL_ALPHA_TEST);
}

void UIController::endCustomDrawMatrices()
{
}

void UIController::endCustomDrawGameStateAndMatrices()
{
	endCustomDrawMatrices();
	endCustomDrawGameState();
}

void RADLINK UIController::CustomDrawCallback(void *user_callback_data, Iggy *player, IggyCustomDrawCallbackRegion *region)
{
	UIScene *scene = (UIScene *)IggyPlayerGetUserdata(player);

	if(scene != NULL)
	{
		scene->customDraw(region);
	}
}

GDrawTexture * RADLINK UIController::TextureSubstitutionCreateCallback ( void * user_callback_data , IggyUTF16 * texture_name , S32 * width , S32 * height , void * * destroy_callback_data )
{
	UIController *uiController = (UIController *)user_callback_data;
	AUTO_VAR(it,uiController->m_substitutionTextures.find((wchar_t *)texture_name));

	if(it != uiController->m_substitutionTextures.end())
	{
		app.DebugPrintf("Found substitution texture %ls, with %d bytes\n", (wchar_t *)texture_name,it->second.length);

		BufferedImage image(it->second.data, it->second.length);
		if( image.getData() != NULL )
		{
			image.preMultiplyAlpha();
			Textures *t = Minecraft::GetInstance()->textures;
			int id = t->getTexture(&image,C4JRender::TEXTURE_FORMAT_RxGyBzAw,false);

	#if (defined __ORBIS__ || defined _DURANGO )
			*width = 96;
			*height = 96;
	#else
			*width = 64;
			*height = 64;
	#endif
			*destroy_callback_data = (void *)id;

			app.DebugPrintf("Found substitution texture %ls (%d) - %dx%d\n", (wchar_t *)texture_name, id, image.getWidth(), image.getHeight());
			return ui.getSubstitutionTexture(id);
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		app.DebugPrintf("Could not find substitution texture %ls\n", (wchar_t *)texture_name);
		return NULL;
	}
}

void RADLINK UIController::TextureSubstitutionDestroyCallback ( void * user_callback_data , void * destroy_callback_data , GDrawTexture * handle )
{
	LONGLONG llVal=(LONGLONG)destroy_callback_data;
	int id=(int)llVal;
	app.DebugPrintf("Destroying iggy texture %d\n", id);

	ui.destroySubstitutionTexture(user_callback_data, handle);

	Textures *t = Minecraft::GetInstance()->textures;
	t->releaseTexture( id );
}

void UIController::registerSubstitutionTexture(const wstring &textureName, PBYTE pbData, DWORD dwLength)
{
	unregisterSubstitutionTexture(textureName,false);
	m_substitutionTextures[textureName] = byteArray(pbData, dwLength);
}

void UIController::unregisterSubstitutionTexture(const wstring &textureName, bool deleteData)
{
	AUTO_VAR(it,m_substitutionTextures.find(textureName));

	if(it != m_substitutionTextures.end())
	{
		if(deleteData) delete [] it->second.data;
		m_substitutionTextures.erase(it);
	}
}

// NAVIGATION
bool UIController::NavigateToScene(int iPad, EUIScene scene, void *initData, EUILayer layer, EUIGroup group)
{
	if(GetMenuDisplayed(iPad))
	{
		switch(scene)
		{
		case eUIScene_PauseMenu:
		case eUIScene_Crafting2x2Menu:
		case eUIScene_Crafting3x3Menu:
		case eUIScene_FurnaceMenu:
		case eUIScene_ContainerMenu:
		case eUIScene_LargeContainerMenu:
		case eUIScene_InventoryMenu:
		case eUIScene_CreativeMenu:
		case eUIScene_DispenserMenu:
		case eUIScene_SignEntryMenu:
		case eUIScene_InGameInfoMenu:
		case eUIScene_EnchantingMenu:
		case eUIScene_BrewingStandMenu:
		case eUIScene_AnvilMenu:
		case eUIScene_TradingMenu:
			app.DebugPrintf("IGNORING NAVIGATE - we're trying to navigate to a user selected scene when there's already a scene up: pad:%d, scene:%d\n", iPad, scene);
			return false;
			break;
		}
	}

	switch(scene)
	{
	case eUIScene_FullscreenProgress:
		{
			layer = eUILayer_Fullscreen;
			group = eUIGroup_Fullscreen;
		}
		break;
	case eUIScene_ConnectingProgress:
		{
			layer = eUILayer_Fullscreen;
		}
		break;
	case eUIScene_EndPoem:
		{
			group = eUIGroup_Fullscreen;
			layer = eUILayer_Scene;
		}
		break;
	};
	int menuDisplayedPad = XUSER_INDEX_ANY;
	if(group == eUIGroup_PAD)
	{
		if( app.GetGameStarted() )
		{
			if( ( iPad != 255 ) && ( iPad >= 0 ) )
			{
				menuDisplayedPad = iPad;
				group = (EUIGroup)(iPad+1);
			}
			else group = eUIGroup_Fullscreen;
		}
		else
		{
			layer = eUILayer_Fullscreen;
			group = eUIGroup_Fullscreen;
		}
	}

	PerformanceTimer timer;

	EnterCriticalSection(&m_navigationLock);
	SetMenuDisplayed(menuDisplayedPad,true);
	bool success = m_groups[(int)group]->NavigateToScene(iPad, scene, initData, layer);
	if(success && group == eUIGroup_Fullscreen) setFullscreenMenuDisplayed(true);
	LeaveCriticalSection(&m_navigationLock);

	timer.PrintElapsedTime(L"Navigate to scene");

	return success;
}

bool UIController::NavigateBack(int iPad, bool forceUsePad, EUIScene eScene, EUILayer eLayer)
{
	bool navComplete = false;
	if( app.GetGameStarted() )
	{
		bool navComplete = m_groups[(int)eUIGroup_Fullscreen]->NavigateBack(iPad, eScene, eLayer);

		if(!navComplete && ( iPad != 255 ) && ( iPad >= 0 ) )
		{
			EUIGroup group = (EUIGroup)(iPad+1);
			navComplete = m_groups[(int)group]->NavigateBack(iPad, eScene, eLayer);
			if(!m_groups[(int)group]->GetMenuDisplayed())SetMenuDisplayed(iPad,false);
		}
		else
		{
			if(!m_groups[(int)eUIGroup_Fullscreen]->GetMenuDisplayed())
			{
				setFullscreenMenuDisplayed(false);
				for(unsigned int i = 0; i < XUSER_MAX_COUNT; ++i)
				{
					SetMenuDisplayed(i,m_groups[i+1]->GetMenuDisplayed());
				}
			}
		}
	}
	else
	{
		navComplete = m_groups[(int)eUIGroup_Fullscreen]->NavigateBack(iPad, eScene, eLayer);
		if(!m_groups[(int)eUIGroup_Fullscreen]->GetMenuDisplayed()) SetMenuDisplayed(XUSER_INDEX_ANY,false);
	}
	return navComplete;
}

void UIController::NavigateToHomeMenu()
{
	ui.CloseAllPlayersScenes();
	
	app.SetLiveLinkRequired( false );

	Minecraft *pMinecraft = Minecraft::GetInstance();

	TexturePack *pTexPack=Minecraft::GetInstance()->skins->getSelected();

	DLCTexturePack *pDLCTexPack=NULL;
	if(pTexPack->hasAudio())
	{
		pDLCTexPack=(DLCTexturePack *)pTexPack;
	}

	pMinecraft->skins->selectTexturePackById(TexturePackRepository::DEFAULT_TEXTURE_PACK_ID);

	if(pTexPack->hasAudio())
	{
		pMinecraft->soundEngine->SetStreamingSounds(eStream_Overworld_Calm1,eStream_Overworld_piano3,
			eStream_Nether1,eStream_Nether4,
			eStream_end_dragon,eStream_end_end,
			eStream_CD_1);
		pMinecraft->soundEngine->playStreaming(L"", 0, 0, 0, 1, 1);

#ifdef _XBOX_ONE
		DWORD result = StorageManager.UnmountInstalledDLC(L"TPACK");
#else
		DWORD result = StorageManager.UnmountInstalledDLC("TPACK");
#endif

		app.DebugPrintf("Unmount result is %d\n",result);
	}

	g_NetworkManager.ForceFriendsSessionRefresh();

	if(pMinecraft->skins->needsUIUpdate())
	{
		m_navigateToHomeOnReload = true;
	}
	else
	{
		ui.NavigateToScene(ProfileManager.GetPrimaryPad(),eUIScene_MainMenu);
	}
}

UIScene *UIController::GetTopScene(int iPad, EUILayer layer, EUIGroup group)
{
	if(group == eUIGroup_PAD)
	{
		if( app.GetGameStarted() )
		{
			if( ( iPad != 255 ) && ( iPad >= 0 ) )
			{
				group = (EUIGroup)(iPad+1);
			}
			else group = eUIGroup_Fullscreen;
		}
		else
		{
			layer = eUILayer_Fullscreen;
			group = eUIGroup_Fullscreen;
		}
	}
	return m_groups[(int)group]->GetTopScene(layer);
}

size_t UIController::RegisterForCallbackId(UIScene *scene)
{
	EnterCriticalSection(&m_registeredCallbackScenesCS);
	size_t newId = GetTickCount();
	newId &= 0xFFFFFF;
	newId |= (scene->getSceneType() << 24);
	m_registeredCallbackScenes[newId] = scene;
	LeaveCriticalSection(&m_registeredCallbackScenesCS);
	return newId;
}

void UIController::UnregisterCallbackId(size_t id)
{
	EnterCriticalSection(&m_registeredCallbackScenesCS);
	AUTO_VAR(it, m_registeredCallbackScenes.find(id) );
	if(it != m_registeredCallbackScenes.end() )
	{
		m_registeredCallbackScenes.erase(it);
	}
	LeaveCriticalSection(&m_registeredCallbackScenesCS);
}

UIScene *UIController::GetSceneFromCallbackId(size_t id)
{
	UIScene *scene = NULL;
	AUTO_VAR(it, m_registeredCallbackScenes.find(id) );
	if(it != m_registeredCallbackScenes.end() )
	{
		scene = it->second;
	}
	return scene;
}

void UIController::EnterCallbackIdCriticalSection()
{
	EnterCriticalSection(&m_registeredCallbackScenesCS);
}

void UIController::LeaveCallbackIdCriticalSection()
{
	LeaveCriticalSection(&m_registeredCallbackScenesCS);
}

void UIController::CloseAllPlayersScenes()
{
	m_groups[(int)eUIGroup_Fullscreen]->getTooltips()->SetTooltips(-1);
	for(unsigned int i = 0; i < eUIGroup_COUNT; ++i)
	{
		m_groups[i]->closeAllScenes();
		m_groups[i]->getTooltips()->SetTooltips(-1);
	}

	if (!m_groups[eUIGroup_Fullscreen]->GetMenuDisplayed()) {
		for(unsigned int i = 0; i < XUSER_MAX_COUNT; ++i)
		{
			SetMenuDisplayed(i,false);
		}
	}
	setFullscreenMenuDisplayed(false);
}

void UIController::CloseUIScenes(int iPad, bool forceIPad)
{
	EUIGroup group;
	if( app.GetGameStarted() || forceIPad )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}

	m_groups[(int)group]->closeAllScenes();
	m_groups[(int)group]->getTooltips()->SetTooltips(-1);
	
	TutorialPopupInfo popupInfo;
	if(m_groups[(int)group]->getTutorialPopup()) m_groups[(int)group]->getTutorialPopup()->SetTutorialDescription(&popupInfo);

	if(group==eUIGroup_Fullscreen) setFullscreenMenuDisplayed(false);

	SetMenuDisplayed((group == eUIGroup_Fullscreen ? XUSER_INDEX_ANY : iPad), m_groups[(int)group]->GetMenuDisplayed());
}

void UIController::setFullscreenMenuDisplayed(bool displayed)
{
	m_groups[(int)eUIGroup_Fullscreen]->showComponent(ProfileManager.GetPrimaryPad(),eUIComponent_Tooltips,eUILayer_Tooltips,displayed);

	for(unsigned int i = (eUIGroup_Fullscreen+1); i < eUIGroup_COUNT; ++i)
	{
		m_groups[i]->showComponent(i,eUIComponent_Tooltips,eUILayer_Tooltips,!displayed);
	}
}

bool UIController::IsPauseMenuDisplayed(int iPad)
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	return m_groups[(int)group]->IsPauseMenuDisplayed();
}

bool UIController::IsContainerMenuDisplayed(int iPad)
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	return m_groups[(int)group]->IsContainerMenuDisplayed();
}

bool UIController::IsIgnorePlayerJoinMenuDisplayed(int iPad)
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	return m_groups[(int)group]->IsIgnorePlayerJoinMenuDisplayed();
}

bool UIController::IsIgnoreAutosaveMenuDisplayed(int iPad)
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	return m_groups[(int)eUIGroup_Fullscreen]->IsIgnoreAutosaveMenuDisplayed() || (group != eUIGroup_Fullscreen && m_groups[(int)group]->IsIgnoreAutosaveMenuDisplayed());
}

void UIController::SetIgnoreAutosaveMenuDisplayed(int iPad, bool displayed)
{
	app.DebugPrintf(app.USER_SR, "UIController::SetIgnoreAutosaveMenuDisplayed is not implemented\n");
}

bool UIController::IsSceneInStack(int iPad, EUIScene eScene)
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	return m_groups[(int)group]->IsSceneInStack(eScene);
}

bool UIController::GetMenuDisplayed(int iPad)
{
	return m_bMenuDisplayed[iPad];
}

void UIController::SetMenuDisplayed(int iPad,bool bVal)
{
	if(bVal)
	{
		if(iPad==XUSER_INDEX_ANY)
		{
			for(int i=0;i<XUSER_MAX_COUNT;i++)
			{
				InputManager.SetMenuDisplayed(i,true);
				m_bMenuDisplayed[i]=true;
				m_bMenuToBeClosed[i]=false;
			}
		}
		else
		{
			InputManager.SetMenuDisplayed(iPad,true);
			m_bMenuDisplayed[iPad]=true;
			m_bMenuToBeClosed[iPad]=false;
		}
	}
	else
	{
		if(iPad==XUSER_INDEX_ANY)
		{
			for(int i=0;i<XUSER_MAX_COUNT;i++)
			{
				m_bMenuToBeClosed[i]=true;
				m_iCountDown[i]=10;
			}
		}
		else
		{
			m_bMenuToBeClosed[iPad]=true;
			m_iCountDown[iPad]=10;

#ifdef _DURANGO
			if (Minecraft::GetInstance()->running) 
				InputManager.SetEnabledGtcButtons(_360_GTC_MENU | _360_GTC_PAUSE | _360_GTC_VIEW);
#endif
		}
	}
}

void UIController::CheckMenuDisplayed()
{
	for(int iPad=0;iPad<XUSER_MAX_COUNT;iPad++)
	{
		if(m_bMenuToBeClosed[iPad])
		{
			if(m_iCountDown[iPad]!=0)
			{
				m_iCountDown[iPad]--;
			}
			else
			{
				m_bMenuToBeClosed[iPad]=false;
				m_bMenuDisplayed[iPad]=false;
				InputManager.SetMenuDisplayed(iPad,false);
			}
		}
	}
}

void UIController::SetTooltipText( unsigned int iPad, unsigned int tooltip, int iTextID )
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	if(m_groups[(int)group]->getTooltips()) m_groups[(int)group]->getTooltips()->SetTooltipText(tooltip, iTextID);
}

void UIController::SetEnableTooltips( unsigned int iPad, BOOL bVal )
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	if(m_groups[(int)group]->getTooltips()) m_groups[(int)group]->getTooltips()->SetEnableTooltips(bVal);
}

void UIController::ShowTooltip( unsigned int iPad, unsigned int tooltip, bool show )
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	if(m_groups[(int)group]->getTooltips()) m_groups[(int)group]->getTooltips()->ShowTooltip(tooltip,show);
}

void UIController::SetTooltips( unsigned int iPad, int iA, int iB, int iX, int iY, int iLT, int iRT, int iLB, int iRB, int iLS, bool forceUpdate)
{
	EUIGroup group;

#ifndef _XBOX
	if(iX==IDS_TOOLTIPS_SELECTDEVICE) iX=-1;
	if(iX==IDS_TOOLTIPS_CHANGEDEVICE) iX=-1;

#if defined(__PS3__) || defined(__ORBIS__) || defined(__PSVITA__)
	if(iY==IDS_TOOLTIPS_VIEW_GAMERCARD) iY=-1;
	if(iY==IDS_TOOLTIPS_VIEW_GAMERPROFILE) iY=-1;
#endif
#endif

	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	if(m_groups[(int)group]->getTooltips()) m_groups[(int)group]->getTooltips()->SetTooltips(iA, iB, iX, iY, iLT, iRT, iLB, iRB, iLS, forceUpdate);
}

void UIController::EnableTooltip( unsigned int iPad, unsigned int tooltip, bool enable )
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	if(m_groups[(int)group]->getTooltips()) m_groups[(int)group]->getTooltips()->EnableTooltip(tooltip,enable);
}

void UIController::RefreshTooltips(unsigned int iPad)
{
	app.DebugPrintf(app.USER_SR, "UIController::RefreshTooltips is not implemented\n");
}

void UIController::AnimateKeyPress(int iPad, int iAction, bool bRepeat, bool bPressed, bool bReleased)
{
	EUIGroup group;
	if(bPressed==false) return;

	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	bool handled = false;
	if(m_groups[(int)group]->getTooltips()) m_groups[(int)group]->getTooltips()->handleInput(iPad, iAction, bRepeat, bPressed, bReleased, handled);
}

void UIController::OverrideSFX(int iPad, int iAction,bool bVal)
{
	EUIGroup group;

	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	bool handled = false;
	if(m_groups[(int)group]->getTooltips()) m_groups[(int)group]->getTooltips()->overrideSFX(iPad, iAction,bVal);
}

void UIController::PlayUISFX(ESoundEffect eSound)
{
	Minecraft::GetInstance()->soundEngine->playUI(eSound,1.0f,1.0f);
}

void UIController::DisplayGamertag(unsigned int iPad, bool show)
{
	if( app.GetGameSettings(ProfileManager.GetPrimaryPad(),eGameSetting_DisplaySplitscreenGamertags) == 0)
	{
		show = false;
	}
	EUIGroup group = (EUIGroup)(iPad+1);
	if(m_groups[(int)group]->getHUD()) m_groups[(int)group]->getHUD()->ShowDisplayName(show);

	if(app.GetLocalPlayerCount() > 1 && m_groups[(int)group]->getTutorialPopup() && !m_groups[(int)group]->IsContainerMenuDisplayed())
	{
		m_groups[(int)group]->getTutorialPopup()->UpdateTutorialPopup();
	}
}

void UIController::SetSelectedItem(unsigned int iPad, const wstring &name)
{
	EUIGroup group;

	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	bool handled = false;
	if(m_groups[(int)group]->getHUD()) m_groups[(int)group]->getHUD()->SetSelectedLabel(name);
}

void UIController::UpdateSelectedItemPos(unsigned int iPad)
{
	app.DebugPrintf(app.USER_SR, "UIController::UpdateSelectedItemPos not implemented\n");
}

void UIController::HandleDLCMountingComplete()
{
	for(unsigned int i = 0; i < eUIGroup_COUNT; ++i)
	{
		app.DebugPrintf("UIController::HandleDLCMountingComplete - m_groups[%d]\n",i);
		m_groups[i]->HandleDLCMountingComplete();
	}
}

void UIController::HandleDLCInstalled(int iPad)
{
	for(unsigned int i = 0; i < eUIGroup_COUNT; ++i)
	{
		m_groups[i]->HandleDLCInstalled();
	}
}


#ifdef _XBOX_ONE
void UIController::HandleDLCLicenseChange()
{
	for(unsigned int i = 0; i < eUIGroup_COUNT; ++i)
	{
		app.DebugPrintf("UIController::HandleDLCLicenseChange - m_groups[%d]\n",i);
		m_groups[i]->HandleDLCLicenseChange();
	}
}
#endif

void UIController::HandleTMSDLCFileRetrieved(int iPad)
{
	app.DebugPrintf(app.USER_SR, "UIController::HandleTMSDLCFileRetrieved not implemented\n");
}

void UIController::HandleTMSBanFileRetrieved(int iPad)
{
	app.DebugPrintf(app.USER_SR, "UIController::HandleTMSBanFileRetrieved not implemented\n");
}

void UIController::HandleInventoryUpdated(int iPad)
{
	app.DebugPrintf(app.USER_SR, "UIController::HandleInventoryUpdated not implemented\n");
}

void UIController::HandleGameTick()
{
	tickInput();

	for(unsigned int i = 0; i < eUIGroup_COUNT; ++i)
	{
		if(m_groups[i]->getHUD()) m_groups[i]->getHUD()->handleGameTick();
	}
}

void UIController::SetTutorial(int iPad, Tutorial *tutorial)
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	if(m_groups[(int)group]->getTutorialPopup()) m_groups[(int)group]->getTutorialPopup()->SetTutorial(tutorial);
}

void UIController::SetTutorialDescription(int iPad, TutorialPopupInfo *info)
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}

	if(m_groups[(int)group]->getTutorialPopup())
	{
		m_groups[(int)group]->getTutorialPopup()->SetContainerMenuVisible(m_groups[(int)group]->IsContainerMenuDisplayed());
		m_groups[(int)group]->getTutorialPopup()->SetTutorialDescription(info);
	}
}

#ifndef _XBOX
void UIController::RemoveInteractSceneReference(int iPad, UIScene *scene)
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	if(m_groups[(int)group]->getTutorialPopup()) m_groups[(int)group]->getTutorialPopup()->RemoveInteractSceneReference(scene);
}
#endif

void UIController::SetTutorialVisible(int iPad, bool visible)
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	if(m_groups[(int)group]->getTutorialPopup()) m_groups[(int)group]->getTutorialPopup()->SetVisible(visible);
}

bool UIController::IsTutorialVisible(int iPad)
{
	EUIGroup group;
	if( app.GetGameStarted() )
	{
		if( ( iPad != 255 ) && ( iPad >= 0 ) ) group = (EUIGroup)(iPad+1);
		else group = eUIGroup_Fullscreen;
	}
	else
	{
		group = eUIGroup_Fullscreen;
	}
	bool visible = false;
	if(m_groups[(int)group]->getTutorialPopup()) visible = m_groups[(int)group]->getTutorialPopup()->IsVisible();
	return visible;
}

void UIController::UpdatePlayerBasePositions()
{
	Minecraft *pMinecraft = Minecraft::GetInstance();

	for( BYTE idx = 0; idx < XUSER_MAX_COUNT; ++idx)
	{
		if(pMinecraft->localplayers[idx] != NULL)
		{
			if(pMinecraft->localplayers[idx]->m_iScreenSection==C4JRender::VIEWPORT_TYPE_FULLSCREEN)
			{
				DisplayGamertag(idx,false);
			}
			else
			{
				DisplayGamertag(idx,true);
			}
			m_groups[idx+1]->SetViewportType((C4JRender::eViewportType)pMinecraft->localplayers[idx]->m_iScreenSection);
		}
		else
		{
#ifndef __ORBIS__
			m_groups[idx+1]->SetViewportType(C4JRender::VIEWPORT_TYPE_FULLSCREEN);
#endif
			DisplayGamertag(idx,false);
		}
	}
}

void UIController::SetEmptyQuadrantLogo(int iSection)
{
}

void UIController::HideAllGameUIElements()
{
	app.DebugPrintf(app.USER_SR, "UIController::HideAllGameUIElements not implemented\n");
}

void UIController::ShowOtherPlayersBaseScene(unsigned int iPad, bool show)
{
}

void UIController::ShowTrialTimer(bool show)
{
	if(m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()) m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->showTrialTimer(show);
}

void UIController::SetTrialTimerLimitSecs(unsigned int uiSeconds)
{
	UIController::m_dwTrialTimerLimitSecs = uiSeconds;
}

void UIController::UpdateTrialTimer(unsigned int iPad)
{
	WCHAR wcTime[20]; 

	DWORD dwTimeTicks=(DWORD)app.getTrialTimer();

	if(dwTimeTicks>m_dwTrialTimerLimitSecs)
	{
		dwTimeTicks=m_dwTrialTimerLimitSecs;
	}

	dwTimeTicks=m_dwTrialTimerLimitSecs-dwTimeTicks;

#ifndef _CONTENT_PACKAGE
	if(true)
#else
	if(dwTimeTicks<180)
#endif
	{
		int iMins=dwTimeTicks/60;
		int iSeconds=dwTimeTicks%60;
		swprintf( wcTime, 20, L"%d:%02d",iMins,iSeconds);
		if(m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()) m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->setTrialTimer(wcTime);
	}
	else
	{
		if(m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()) m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->setTrialTimer(L"");
	}

	if((dwTimeTicks==0))
	{
		if(!ui.GetMenuDisplayed( iPad ) )
		{
			ui.NavigateToScene(iPad, eUIScene_PauseMenu, NULL, eUILayer_Scene);
			app.SetAction(iPad,eAppAction_TrialOver);
		}
	}
}

void UIController::ReduceTrialTimerValue()
{
	DWORD dwTimeTicks=(int)app.getTrialTimer();

	if(dwTimeTicks>m_dwTrialTimerLimitSecs)
	{
		dwTimeTicks=m_dwTrialTimerLimitSecs;
	}

	m_dwTrialTimerLimitSecs-=dwTimeTicks;
}

void UIController::ShowAutosaveCountdownTimer(bool show)
{
	if(m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()) m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->showTrialTimer(show);
}

void UIController::UpdateAutosaveCountdownTimer(unsigned int uiSeconds)
{
#if !(defined(_XBOX_ONE) || defined(__ORBIS__))
	WCHAR wcAutosaveCountdown[100]; 
	swprintf( wcAutosaveCountdown, 100, app.GetString(IDS_AUTOSAVE_COUNTDOWN),uiSeconds);
	if(m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()) m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->setTrialTimer(wcAutosaveCountdown);
#endif
}

void UIController::ShowSavingMessage(unsigned int iPad, C4JStorage::ESavingMessage eVal)
{
	bool show = false;
	switch(eVal)
	{
	case C4JStorage::ESavingMessage_None:
		show = false;
		break;
	case C4JStorage::ESavingMessage_Short:
	case C4JStorage::ESavingMessage_Long:
		show = true;
		break;
	}
	if(m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()) m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->showSaveIcon(show);
}

void UIController::ShowPlayerDisplayname(bool show)
{
	if(m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()) m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->showPlayerDisplayName(show);
}

void UIController::SetWinUserIndex(unsigned int iPad)
{
	m_winUserIndex = iPad;
}

unsigned int UIController::GetWinUserIndex()
{
	return m_winUserIndex;
}

void UIController::ShowUIDebugConsole(bool show)
{
#ifndef _CONTENT_PACKAGE
	if(show)
	{
		m_uiDebugConsole = (UIComponent_DebugUIConsole *)m_groups[eUIGroup_Fullscreen]->addComponent(0, eUIComponent_DebugUIConsole, eUILayer_Debug);
	}
	else
	{
		m_groups[eUIGroup_Fullscreen]->removeComponent(eUIComponent_DebugUIConsole, eUILayer_Debug);
		m_uiDebugConsole = NULL;
	}
#endif
}

void UIController::ShowUIDebugMarketingGuide(bool show)
{
#ifndef _CONTENT_PACKAGE
	if(show)
	{
		m_uiDebugMarketingGuide = (UIComponent_DebugUIMarketingGuide *)m_groups[eUIGroup_Fullscreen]->addComponent(0, eUIComponent_DebugUIMarketingGuide, eUILayer_Debug);
	}
	else
	{
		m_groups[eUIGroup_Fullscreen]->removeComponent(eUIComponent_DebugUIMarketingGuide, eUILayer_Debug);
		m_uiDebugMarketingGuide = NULL;
	}
#endif
}

void UIController::logDebugString(const string &text)
{
	if(m_uiDebugConsole) m_uiDebugConsole->addText(text);
}

bool UIController::PressStartPlaying(unsigned int iPad)
{
	return m_iPressStartQuadrantsMask&(1<<iPad)?true:false;
}

void UIController::ShowPressStart(unsigned int iPad)
{
	m_iPressStartQuadrantsMask|=1<<iPad;
	if(m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()) m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->showPressStart(iPad, true);
}

void UIController::HidePressStart()
{
	ClearPressStart();
	if(m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()) m_groups[(int)eUIGroup_Fullscreen]->getPressStartToPlay()->showPressStart(0, false);
}

void UIController::ClearPressStart()
{
	m_iPressStartQuadrantsMask = 0;
}

#ifndef __PS3__
C4JStorage::EMessageResult UIController::RequestMessageBox(UINT uiTitle, UINT uiText, UINT *uiOptionA,UINT uiOptionC, DWORD dwPad,
														   int( *Func)(LPVOID,int,const C4JStorage::EMessageResult),LPVOID lpParam, C4JStringTable *pStringTable, WCHAR *pwchFormatString,DWORD dwFocusButton, bool bIsError)
#else
C4JStorage::EMessageResult UIController::RequestMessageBox(UINT uiTitle, UINT uiText, UINT *uiOptionA,UINT uiOptionC, DWORD dwPad,
														   int( *Func)(LPVOID,int,const C4JStorage::EMessageResult),LPVOID lpParam, StringTable *pStringTable, WCHAR *pwchFormatString,DWORD dwFocusButton, bool bIsError)
#endif
{
	MessageBoxInfo param;
	param.uiTitle = uiTitle;
	param.uiText = uiText;
	param.uiOptionA = uiOptionA;
	param.uiOptionC = uiOptionC;
	param.dwPad = dwPad;
	param.Func = Func;\
	param.lpParam = lpParam;
	param.pwchFormatString = pwchFormatString;
	param.dwFocusButton = dwFocusButton;

	EUILayer layer = bIsError?eUILayer_Error:eUILayer_Alert;

	bool completed = false;
	if(ui.IsReloadingSkin())
	{
		QueuedMessageBoxData *queuedData = new QueuedMessageBoxData();
		queuedData->info = param;
		queuedData->info.uiOptionA = new UINT[param.uiOptionC];
		memcpy(queuedData->info.uiOptionA, param.uiOptionA, param.uiOptionC * sizeof(UINT));
		queuedData->iPad = dwPad;
		queuedData->layer = eUILayer_Error;
		m_queuedMessageBoxData.push_back(queuedData);
	}
	else
	{
		completed = ui.NavigateToScene(dwPad, eUIScene_MessageBox, &param, layer, eUIGroup_Fullscreen);
	}

	if( completed )
	{
		return C4JStorage::EMessage_Pending;
	}
	else
	{
		return C4JStorage::EMessage_Busy;
	}
}

C4JStorage::EMessageResult UIController::RequestUGCMessageBox(UINT title, UINT message, int iPad, int( *Func)(LPVOID,int,const C4JStorage::EMessageResult), LPVOID lpParam)
{
	if (title == -1) title = IDS_FAILED_TO_CREATE_GAME_TITLE;
	if (message == -1) message = IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_CREATE;
	if (iPad == -1) iPad = ProfileManager.GetPrimaryPad();

#ifdef __ORBIS__
	ProfileManager.DisplaySystemMessage( SCE_MSG_DIALOG_SYSMSG_TYPE_TRC_PSN_UGC_RESTRICTION, iPad );
	return C4JStorage::EMessage_ResultAccept; 
#elif defined(__PSVITA__)
	ProfileManager.ShowSystemMessage( SCE_MSG_DIALOG_SYSMSG_TYPE_TRC_PSN_CHAT_RESTRICTION, iPad );
	UINT uiIDA[1];
	uiIDA[0]=IDS_CONFIRM_OK;
	return ui.RequestMessageBox( title, IDS_CHAT_RESTRICTION_UGC, uiIDA, 1, iPad, Func, lpParam, app.GetStringTable(), NULL, 0, false);
#else
	UINT uiIDA[1];
	uiIDA[0]=IDS_CONFIRM_OK;
	return ui.RequestMessageBox( title, message, uiIDA, 1, iPad, Func, lpParam, app.GetStringTable(), NULL, 0, false);
#endif
}

C4JStorage::EMessageResult UIController::RequestContentRestrictedMessageBox(UINT title, UINT message, int iPad, int( *Func)(LPVOID,int,const C4JStorage::EMessageResult), LPVOID lpParam)
{
	if (title == -1) title = IDS_FAILED_TO_CREATE_GAME_TITLE;
	if (message == -1)
	{
#if defined(_XBOX_ONE) || defined(_WINDOWS64)
		message = IDS_NO_USER_CREATED_CONTENT_PRIVILEGE_CREATE;
#else
		message = IDS_CONTENT_RESTRICTION;
#endif
	}
	if (iPad == -1) iPad = ProfileManager.GetPrimaryPad();

#ifdef __ORBIS__
	ProfileManager.DisplaySystemMessage( SCE_MSG_DIALOG_SYSMSG_TYPE_TRC_PSN_UGC_RESTRICTION, iPad );
	return C4JStorage::EMessage_ResultAccept; 
#elif defined(__PSVITA__)
	ProfileManager.ShowSystemMessage( SCE_MSG_DIALOG_SYSMSG_TYPE_TRC_PSN_AGE_RESTRICTION, iPad );
	return C4JStorage::EMessage_ResultAccept;
#else
	UINT uiIDA[1];
	uiIDA[0]=IDS_CONFIRM_OK;
	return ui.RequestMessageBox( title, message, uiIDA, 1, iPad, Func, lpParam, app.GetStringTable(), NULL, 0, false);
#endif
}

void UIController::setFontCachingCalculationBuffer(int length)
{
#if defined __ORBIS__ || defined _DURANGO || defined _WIN64
	static const int CHAR_SIZE = 24;
#else
	static const int CHAR_SIZE = 16;
#endif

	if (m_tempBuffer != NULL) delete [] m_tempBuffer;
	if (length<0)
	{
		if (m_defaultBuffer == NULL) m_defaultBuffer = new char[CHAR_SIZE*5000];
		IggySetFontCachingCalculationBuffer(5000, m_defaultBuffer, CHAR_SIZE*5000);
	}
	else
	{
		m_tempBuffer = new char[CHAR_SIZE*length];
		IggySetFontCachingCalculationBuffer(length, m_tempBuffer, CHAR_SIZE*length);
	}
}

UIScene *UIController::FindScene(EUIScene sceneType)
{
	UIScene *pScene = NULL;

	for (int i = 0; i < eUIGroup_COUNT; i++)
	{
		pScene = m_groups[i]->FindScene(sceneType);
#ifdef __PS3__
		if (pScene != NULL) return pScene;
#else
		if (pScene != nullptr) return pScene;
#endif
	}

	return pScene;
}

#ifdef __PSVITA__

void UIController::TouchBoxAdd(UIControl *pControl,UIScene *pUIScene)
{
	EUIGroup eUIGroup=pUIScene->GetParentLayerGroup();
	EUILayer eUILayer=pUIScene->GetParentLayer()->m_iLayer;
	EUIScene eUIscene=pUIScene->getSceneType();

	TouchBoxAdd(pControl,eUIGroup,eUILayer,eUIscene, pUIScene->GetMainPanel());
}

void UIController::TouchBoxAdd(UIControl *pControl,EUIGroup eUIGroup,EUILayer eUILayer,EUIScene eUIscene, UIControl *pMainPanelControl)
{
	UIELEMENT *puiElement = new UIELEMENT;
	puiElement->pControl = pControl;

	S32 iControlWidth = pControl->getWidth();
	S32 iControlHeight = pControl->getHeight();
	S32 iMainPanelOffsetX = 0;
	S32 iMainPanelOffsetY= 0;

	if(pMainPanelControl)
	{
		iMainPanelOffsetX = pMainPanelControl->getXPos();
		iMainPanelOffsetY = pMainPanelControl->getYPos();
	}

	if(puiElement->pControl->getControlType() == UIControl::eSlider)
	{
		UIControl_Slider *pSlider = (UIControl_Slider *)puiElement->pControl;
		iControlWidth = pSlider->GetRealWidth();
	}
	else if(puiElement->pControl->getControlType() == UIControl::eTexturePackList)
	{
		UIControl_TexturePackList *pTexturePackList = (UIControl_TexturePackList *)puiElement->pControl;
		iControlHeight = pTexturePackList->GetRealHeight();
	}
	else if(puiElement->pControl->getControlType() == UIControl::eDynamicLabel)
	{
		UIControl_DynamicLabel *pDynamicLabel = (UIControl_DynamicLabel *)puiElement->pControl;
		iControlWidth = pDynamicLabel->GetRealWidth();
		iControlHeight = pDynamicLabel->GetRealHeight();
	}
	else if(puiElement->pControl->getControlType() == UIControl::eHTMLLabel)
	{
		UIControl_HTMLLabel *pHtmlLabel = (UIControl_HTMLLabel *)puiElement->pControl;
		iControlWidth = pHtmlLabel->GetRealWidth();
		iControlHeight = pHtmlLabel->GetRealHeight();
	}

	puiElement->x1=(S32)((float)pControl->getXPos() + (float)iMainPanelOffsetX);
	puiElement->y1=(S32)((float)pControl->getYPos() + (float)iMainPanelOffsetY);
	puiElement->x2=(S32)(((float)pControl->getXPos() + (float)iControlWidth + (float)iMainPanelOffsetX));
	puiElement->y2=(S32)(((float)pControl->getYPos() + (float)iControlHeight + (float)iMainPanelOffsetY));

	if(puiElement->pControl->getControlType() == UIControl::eNoControl)
	{
		app.DebugPrintf("NO CONTROL!");
	}

	if(puiElement->x1 == puiElement->x2 || puiElement->y1 == puiElement->y2)
	{
		app.DebugPrintf("NOT adding touchbox %d,%d,%d,%d\n",puiElement->x1,puiElement->y1,puiElement->x2,puiElement->y2);
	}
	else
	{
		app.DebugPrintf("Adding touchbox %d,%d,%d,%d\n",puiElement->x1,puiElement->y1,puiElement->x2,puiElement->y2);
		m_TouchBoxes[eUIGroup][eUILayer][eUIscene].push_back(puiElement);
	}
}

void UIController::TouchBoxRebuild(UIScene *pUIScene)
{
	EUIGroup eUIGroup=pUIScene->GetParentLayerGroup();
	EUILayer eUILayer=pUIScene->GetParentLayer()->m_iLayer;
	EUIScene eUIscene=pUIScene->getSceneType();

	ui.TouchBoxesClear(pUIScene);

	AUTO_VAR(itEnd, pUIScene->GetControls()->end());
	for (AUTO_VAR(it, pUIScene->GetControls()->begin()); it != itEnd; it++)
	{
		UIControl *control=(UIControl *)*it;

		if(control->getControlType() == UIControl::eButton ||
			control->getControlType() == UIControl::eSlider ||
			control->getControlType() == UIControl::eCheckBox ||
			control->getControlType() == UIControl::eTexturePackList ||
			control->getControlType() == UIControl::eButtonList ||
			control->getControlType() == UIControl::eTextInput ||
			control->getControlType() == UIControl::eDynamicLabel ||
			control->getControlType() == UIControl::eHTMLLabel ||
			control->getControlType() == UIControl::eLeaderboardList ||
			control->getControlType() == UIControl::eTouchControl)
		{
			control->UpdateControl();
			ui.TouchBoxAdd(control,eUIGroup,eUILayer,eUIscene, pUIScene->GetMainPanel());
		}
	}
}

void UIController::TouchBoxesClear(UIScene *pUIScene)
{
	EUIGroup eUIGroup=pUIScene->GetParentLayerGroup();
	EUILayer eUILayer=pUIScene->GetParentLayer()->m_iLayer;
	EUIScene eUIscene=pUIScene->getSceneType();

	AUTO_VAR(itEnd, m_TouchBoxes[eUIGroup][eUILayer][eUIscene].end());
	for (AUTO_VAR(it, m_TouchBoxes[eUIGroup][eUILayer][eUIscene].begin()); it != itEnd; it++)
	{
		UIELEMENT *element=(UIELEMENT *)*it;
		delete element;		
	}
	m_TouchBoxes[eUIGroup][eUILayer][eUIscene].clear();
}

bool UIController::TouchBoxHit(UIScene *pUIScene,S32 x, S32 y)
{
	EUIGroup eUIGroup=pUIScene->GetParentLayerGroup();
	EUILayer eUILayer=pUIScene->GetParentLayer()->m_iLayer;
	EUIScene eUIscene=pUIScene->getSceneType();

	x *= (m_fScreenWidth/1920.0f);
	y *= (m_fScreenHeight/1080.0f);

	if(m_TouchBoxes[eUIGroup][eUILayer][eUIscene].size()>0)
	{
		AUTO_VAR(itEnd, m_TouchBoxes[eUIGroup][eUILayer][eUIscene].end());
		for (AUTO_VAR(it, m_TouchBoxes[eUIGroup][eUILayer][eUIscene].begin()); it != itEnd; it++)
		{
			UIELEMENT *element=(UIELEMENT *)*it;
			if(element->pControl->getHidden() == false && element->pControl->getVisible())
			{
				if((x>=element->x1) &&(x<=element->x2) && (y>=element->y1) && (y<=element->y2))
				{
					if(!m_bTouchscreenPressed)
					{
						app.DebugPrintf("SET m_ActiveUIElement (Layer: %i) at x = %i y = %i\n", (int)eUILayer, (int)x, (int)y);
						m_ActiveUIElement = element;
					}
					m_HighlightedUIElement = element;

					return true;
				}
			}
		}			
	}

	m_HighlightedUIElement = NULL;
	return false;
}

void UIController::HandleTouchInput(unsigned int iPad, unsigned int key, bool bPressed, bool bRepeat, bool bReleased)
{
	if(!bPressed && !bRepeat && !bReleased)
	{
		if(m_bTouchscreenPressed && m_ActiveUIElement && (
			m_ActiveUIElement->pControl->getControlType() == UIControl::eSlider ||
			m_ActiveUIElement->pControl->getControlType() == UIControl::eButtonList ||
			m_ActiveUIElement->pControl->getControlType() == UIControl::eTexturePackList ||
			m_ActiveUIElement->pControl->getControlType() == UIControl::eDynamicLabel ||
			m_ActiveUIElement->pControl->getControlType() == UIControl::eHTMLLabel ||
			m_ActiveUIElement->pControl->getControlType() == UIControl::eLeaderboardList ||
			m_ActiveUIElement->pControl->getControlType() == UIControl::eTouchControl))
			bRepeat = true;
		else
			return;
	}

	SceTouchData* pTouchData = InputManager.GetTouchPadData(iPad,false);
	S32 x = pTouchData->report[0].x * (m_fScreenWidth/1920.0f);
	S32 y = pTouchData->report[0].y * (m_fScreenHeight/1080.0f);

	if(bPressed && !bRepeat && !bReleased)
	{
		app.DebugPrintf("touch input pressed\n");
		switch(m_ActiveUIElement->pControl->getControlType())
		{
		case UIControl::eButton:
			UIControl_Button *pButton=(UIControl_Button *)m_ActiveUIElement->pControl;
			pButton->getParentScene()->SetFocusToElement(m_ActiveUIElement->pControl->getId());
			bPressed = false;
			break;
		case UIControl::eSlider:
			UIControl_Slider *pSlider=(UIControl_Slider *)m_ActiveUIElement->pControl;
			pSlider->getParentScene()->SetFocusToElement(m_ActiveUIElement->pControl->getId());
			break;
		case UIControl::eCheckBox:
			UIControl_CheckBox *pCheckbox=(UIControl_CheckBox *)m_ActiveUIElement->pControl;
			pCheckbox->getParentScene()->SetFocusToElement(m_ActiveUIElement->pControl->getId());
			bPressed = false;
			break;
		case UIControl::eButtonList:
			UIControl_ButtonList *pButtonList=(UIControl_ButtonList *)m_ActiveUIElement->pControl;
			pButtonList->SetTouchFocus((float)x, (float)y, false);
			bPressed = false;
			break;
		case UIControl::eTexturePackList:
			UIControl_TexturePackList *pTexturePackList=(UIControl_TexturePackList *)m_ActiveUIElement->pControl;
			pTexturePackList->getParentScene()->SetFocusToElement(m_ActiveUIElement->pControl->getId());
			pTexturePackList->SetTouchFocus((float)x - (float)m_ActiveUIElement->x1, (float)y - (float)m_ActiveUIElement->y1, false);
			bPressed = false;
			break;
		case UIControl::eTextInput:
			UIControl_TextInput *pTextInput=(UIControl_TextInput *)m_ActiveUIElement->pControl;
			pTextInput->getParentScene()->SetFocusToElement(m_ActiveUIElement->pControl->getId());
			bPressed = false;
			break;
		case UIControl::eDynamicLabel:
			UIControl_DynamicLabel *pDynamicLabel=(UIControl_DynamicLabel *)m_ActiveUIElement->pControl;
			pDynamicLabel->TouchScroll(y, true);
			bPressed = false;
			break;
		case UIControl::eHTMLLabel:
			UIControl_HTMLLabel *pHtmlLabel=(UIControl_HTMLLabel *)m_ActiveUIElement->pControl;
			pHtmlLabel->TouchScroll(y, true);
			bPressed = false;
			break;
		case UIControl::eLeaderboardList:
			UIControl_LeaderboardList *pLeaderboardList=(UIControl_LeaderboardList *)m_ActiveUIElement->pControl;
			pLeaderboardList->SetTouchFocus((float)x, (float)y, false);
			bPressed = false;
			break;
		case UIControl::eTouchControl:
			m_ActiveUIElement->pControl->getParentScene()->handleTouchInput(iPad, x, y, m_ActiveUIElement->pControl->getId(), bPressed, bRepeat, bReleased);
			bPressed = false;
			break;
		default:
			app.DebugPrintf("PRESSED - UNHANDLED UI ELEMENT\n");
			break;
		}
	}
	else if(bRepeat)
	{
		switch(m_ActiveUIElement->pControl->getControlType())
		{
		case UIControl::eButton:
			break;
		case UIControl::eSlider:
			UIControl_Slider *pSlider=(UIControl_Slider *)m_ActiveUIElement->pControl;
			float fNewSliderPos = ((float)x - (float)m_ActiveUIElement->x1) / (float)pSlider->GetRealWidth();
			pSlider->SetSliderTouchPos(fNewSliderPos);
			break;
		case UIControl::eCheckBox:
			bRepeat = false;
			bPressed = false;
			break;
		case UIControl::eButtonList:
			UIControl_ButtonList *pButtonList=(UIControl_ButtonList *)m_ActiveUIElement->pControl;
			pButtonList->SetTouchFocus((float)x, (float)y, true);
			break;
		case UIControl::eTexturePackList:
			UIControl_TexturePackList *pTexturePackList=(UIControl_TexturePackList *)m_ActiveUIElement->pControl;
			pTexturePackList->SetTouchFocus((float)x - (float)m_ActiveUIElement->x1, (float)y - (float)m_ActiveUIElement->y1, true);
			break;
		case UIControl::eTextInput:
			bRepeat = false;
			bPressed = false;
			break;
		case UIControl::eDynamicLabel:
			UIControl_DynamicLabel *pDynamicLabel=(UIControl_DynamicLabel *)m_ActiveUIElement->pControl;
			pDynamicLabel->TouchScroll(y, true);
			bPressed = false;
			bRepeat = false;
			break;
		case UIControl::eHTMLLabel:
			UIControl_HTMLLabel *pHtmlLabel=(UIControl_HTMLLabel *)m_ActiveUIElement->pControl;
			pHtmlLabel->TouchScroll(y, true);
			bPressed = false;
			bRepeat = false;
			break;
		case UIControl::eLeaderboardList:
			UIControl_LeaderboardList *pLeaderboardList=(UIControl_LeaderboardList *)m_ActiveUIElement->pControl;
			pLeaderboardList->SetTouchFocus((float)x, (float)y, true);
			break;
		case UIControl::eTouchControl:
			bPressed = false;
			m_ActiveUIElement->pControl->getParentScene()->handleTouchInput(iPad, x, y, m_ActiveUIElement->pControl->getId(), bPressed, bRepeat, bReleased);
			bRepeat = false;
			break;
		default:
			app.DebugPrintf("REPEAT - UNHANDLED UI ELEMENT\n");
			break;
		}
	}
	if(bReleased)
	{
		app.DebugPrintf("touch input released\n");
		switch(m_ActiveUIElement->pControl->getControlType())
		{
		case UIControl::eButton:
			if(m_HighlightedUIElement && m_ActiveUIElement->pControl == m_HighlightedUIElement->pControl)
				bPressed = true;
			break;
		case UIControl::eSlider:
			break;
		case UIControl::eCheckBox:
			if(m_HighlightedUIElement && m_ActiveUIElement->pControl == m_HighlightedUIElement->pControl)
			{
				UIControl_CheckBox *pCheckbox=(UIControl_CheckBox *)m_ActiveUIElement->pControl;
				pCheckbox->TouchSetCheckbox(!pCheckbox->IsChecked());
			}
			bReleased = false;
			break;
		case UIControl::eButtonList:
			if(m_HighlightedUIElement && m_ActiveUIElement->pControl == m_HighlightedUIElement->pControl)
			{
				UIControl_ButtonList *pButtonList=(UIControl_ButtonList *)m_ActiveUIElement->pControl;
				if(pButtonList->CanTouchTrigger(x,y))
					bPressed = true;
			}
			break;
		case UIControl::eTexturePackList:
			if(m_HighlightedUIElement && m_ActiveUIElement->pControl == m_HighlightedUIElement->pControl)
			{
				UIControl_TexturePackList *pTexturePackList=(UIControl_TexturePackList *)m_ActiveUIElement->pControl;
				if(pTexturePackList->CanTouchTrigger((float)x - (float)m_ActiveUIElement->x1, (float)y - (float)m_ActiveUIElement->y1))
					bPressed = true;
			}
			break;
		case UIControl::eTextInput:
			if(m_HighlightedUIElement && m_ActiveUIElement->pControl == m_HighlightedUIElement->pControl)
				bPressed = true;
			break;
		case UIControl::eDynamicLabel:
			UIControl_DynamicLabel *pDynamicLabel=(UIControl_DynamicLabel *)m_ActiveUIElement->pControl;
			pDynamicLabel->TouchScroll(y, false);
			break;
		case UIControl::eHTMLLabel:
			UIControl_HTMLLabel *pHtmlLabel=(UIControl_HTMLLabel *)m_ActiveUIElement->pControl;
			pHtmlLabel->TouchScroll(y, false);
			break;
		case UIControl::eLeaderboardList:
			break;
		case UIControl::eTouchControl:
			if(m_HighlightedUIElement && m_ActiveUIElement->pControl == m_HighlightedUIElement->pControl)
			{
				m_ActiveUIElement->pControl->getParentScene()->handleTouchInput(iPad, x, y, m_ActiveUIElement->pControl->getId(), bPressed, bRepeat, bReleased);
			}
			bReleased = false;
			break;
		default:
			app.DebugPrintf("RELEASED - UNHANDLED UI ELEMENT\n");
			break;
		}
	}

	if(bPressed || bRepeat || bReleased)
	{
		SendTouchInput(iPad, key, bPressed, bRepeat, bReleased);
	}
}

void UIController::SendTouchInput(unsigned int iPad, unsigned int key, bool bPressed, bool bRepeat, bool bReleased)
{
	bool handled = false;

	m_groups[(int)eUIGroup_Fullscreen]->handleInput(iPad, key, bRepeat, bPressed, bReleased, handled);
	if(!handled)
	{
		m_groups[(iPad+1)]->handleInput(iPad, key, bRepeat, bPressed, bReleased, handled);
	}
}

#endif
