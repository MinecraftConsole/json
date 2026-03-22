#include "stdafx.h"
#include "UI.h"
#include "UIScene_HowToPlayMenu.h"

// strings for buttons in the list				  
unsigned int UIScene_HowToPlayMenu::m_uiHTPButtonNameA[]=
{
	IDS_HOW_TO_PLAY_MENU_WHATSNEW,		  // eHTPButton_WhatsNew
	IDS_HOW_TO_PLAY_MENU_BASICS,		  // eHTPButton_Basics, 
	IDS_HOW_TO_PLAY_MENU_MULTIPLAYER,	  // eHTPButton_Multiplayer
	IDS_HOW_TO_PLAY_MENU_HUD,			  // eHTPButton_Hud,
	IDS_HOW_TO_PLAY_MENU_CREATIVE,		  // eHTPButton_Creative,
	IDS_HOW_TO_PLAY_MENU_INVENTORY,		  // eHTPButton_Inventory,
	IDS_HOW_TO_PLAY_MENU_CHESTS,		  // eHTPButton_Chest,
	IDS_HOW_TO_PLAY_MENU_CRAFTING,		  // eHTPButton_Crafting,
	IDS_HOW_TO_PLAY_MENU_FURNACE,		  // eHTPButton_Furnace,
	IDS_HOW_TO_PLAY_MENU_DISPENSER,		  // eHTPButton_Dispenser,

	IDS_HOW_TO_PLAY_MENU_BREWING,		  // eHTPButton_Brewing,
	IDS_HOW_TO_PLAY_MENU_ENCHANTMENT,	  // eHTPButton_Enchantment,
	IDS_HOW_TO_PLAY_MENU_ANVIL,
	IDS_HOW_TO_PLAY_MENU_FARMANIMALS,	  // eHTPButton_FarmingAnimals,
	IDS_HOW_TO_PLAY_MENU_BREEDANIMALS,	  // eHTPButton_Breeding,
	IDS_HOW_TO_PLAY_MENU_TRADING,

	IDS_HOW_TO_PLAY_MENU_NETHERPORTAL,	// eHTPButton_NetherPortal,
	IDS_HOW_TO_PLAY_MENU_THEEND,		// eHTPButton_TheEnd,
#ifdef _XBOX
	IDS_HOW_TO_PLAY_MENU_SOCIALMEDIA,	// eHTPButton_SocialMedia,
	IDS_HOW_TO_PLAY_MENU_BANLIST,		// eHTPButton_BanningLevels,
#endif
	IDS_HOW_TO_PLAY_MENU_HOSTOPTIONS,	// eHTPButton_HostOptions,
};

// mapping the buttons to a scene value
unsigned int UIScene_HowToPlayMenu::m_uiHTPSceneA[]=
{
	eHowToPlay_WhatsNew,
	eHowToPlay_Basics,
	eHowToPlay_Multiplayer,
	eHowToPlay_HUD,
	eHowToPlay_Creative,
	eHowToPlay_Inventory,
	eHowToPlay_Chest,
	eHowToPlay_InventoryCrafting,
	eHowToPlay_Furnace,
	eHowToPlay_Dispenser,

	eHowToPlay_Brewing,
	eHowToPlay_Enchantment,
	eHowToPlay_Anvil,
	eHowToPlay_FarmingAnimals,
	eHowToPlay_Breeding,
	eHowToPlay_Trading,

	eHowToPlay_NetherPortal,
	eHowToPlay_TheEnd,
#ifdef _XBOX
	eHowToPlay_SocialMedia,
	eHowToPlay_BanList,
#endif
	eHowToPlay_HostOptions,
};

UIScene_HowToPlayMenu::UIScene_HowToPlayMenu(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
	initialiseMovie();

	m_buttonListHowTo.init(eControl_Buttons);

	// FIX: Initialise the double-fire guard.
	m_bHandlePressCalledThisTick = false;

	for(unsigned int i = 0; i < eHTPButton_Max; ++i)
	{
#ifdef __PS3__
		if(StorageManager.GetBootTypeDisc() || (app.GetProductSKU()==e_sku_SCEJ))
		{
			if(!(i==eHTPButton_WhatsNew) )
			{
				m_buttonListHowTo.addItem( app.GetString(m_uiHTPButtonNameA[i]) , i);
			}
		}
		else
#elif defined(__ORBIS__) || defined(_DURANGO) || defined(__PSVITA__)
		if(true)
		{
			if(!(i==eHTPButton_WhatsNew) )
			{
				m_buttonListHowTo.addItem( app.GetString(m_uiHTPButtonNameA[i]) , i);
			}
		}
		else
#endif
		{
			m_buttonListHowTo.addItem( app.GetString(m_uiHTPButtonNameA[i]) , i);
		}
	}
}

wstring UIScene_HowToPlayMenu::getMoviePath()
{
	if(app.GetLocalPlayerCount() > 1)
	{
		return L"HowToPlayMenuSplit";
	}
	else
	{
		return L"HowToPlayMenu";
	}
}

void UIScene_HowToPlayMenu::updateTooltips()
{
	ui.SetTooltips( m_iPad, IDS_TOOLTIPS_SELECT,IDS_TOOLTIPS_BACK);
}

void UIScene_HowToPlayMenu::updateComponents()
{
	bool bNotInGame=(Minecraft::GetInstance()->level==NULL);
	if(bNotInGame)
	{
		m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,true);
		m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
	}
	else
	{
		m_parentLayer->showComponent(m_iPad,eUIComponent_Panorama,false);

		if( app.GetLocalPlayerCount() == 1 ) m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,true);
		else m_parentLayer->showComponent(m_iPad,eUIComponent_Logo,false);
	}
}

void UIScene_HowToPlayMenu::handleReload()
{
	for(unsigned int i = 0; i < eHTPButton_Max; ++i)
	{
#ifdef __PS3__
		if(StorageManager.GetBootTypeDisc() || (app.GetProductSKU()==e_sku_SCEJ))
		{
			if(!(i==eHTPButton_WhatsNew) )
			{
				m_buttonListHowTo.addItem( app.GetString(m_uiHTPButtonNameA[i]) , i);
			}
		}
		else
#elif defined(__ORBIS__) || defined(_DURANGO) || defined(__PSVITA__)
		if(true)
		{
			if(!(i==eHTPButton_WhatsNew) )
			{
				m_buttonListHowTo.addItem( app.GetString(m_uiHTPButtonNameA[i]) , i);
			}
		}
		else
#endif
		{
			m_buttonListHowTo.addItem( app.GetString(m_uiHTPButtonNameA[i]) , i);
		}
	}
}

void UIScene_HowToPlayMenu::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	// FIX: Reset the double-fire guard at the start of each new press.
	if(pressed) m_bHandlePressCalledThisTick = false;

	switch(key)
	{
	case ACTION_MENU_CANCEL:
		if(pressed)
		{
			navigateBack();
		}
		break;

	case ACTION_MENU_OK:
#ifdef __ORBIS__
	case ACTION_MENU_TOUCHPAD_PRESS:
#endif
		if(pressed)
		{
			// Send the keypress to Flash as normal so gamepad/keyboard keep working.
			sendInputToMovie(key, repeat, pressed, released);

			// FIX: Mouse click fallback.
			// When focus was set via mouse, Flash may not fire handlePress in the
			// same tick. We use the public getters getControlFocus() and
			// getControlChildFocus() from UIScene base class to avoid accessing
			// private members directly.
			int focusControl = getControlFocus();
			int focusChild   = getControlChildFocus();

			if( !m_bHandlePressCalledThisTick   &&
				focusControl == eControl_Buttons &&
				focusChild   >= 0                &&
				focusChild   <  (int)eHTPButton_Max )
			{
				m_bHandlePressCalledThisTick = true;
				handlePress((F64)focusControl, (F64)focusChild);
			}
		}
		else
		{
			// Always forward the release so Flash can reset button visual states.
			sendInputToMovie(key, repeat, pressed, released);
		}
		break;

	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
	case ACTION_MENU_PAGEUP:
	case ACTION_MENU_PAGEDOWN:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_HowToPlayMenu::handlePress(F64 controlId, F64 childId)
{
	// FIX: Mark that handlePress ran this tick to block the fallback from re-firing.
	m_bHandlePressCalledThisTick = true;

	if( (int)controlId == eControl_Buttons)
	{
		//CD - Added for audio
		ui.PlayUISFX(eSFX_Press);

		unsigned int uiInitData;
		uiInitData = ( ( 1 <<  31 )  | ( m_uiHTPSceneA[(int)childId]  << 16 ) | ( short )( m_iPad ) );
		ui.NavigateToScene(m_iPad, eUIScene_HowToPlay, ( void* )( uiInitData ) );
	}
}