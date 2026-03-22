#include "stdafx.h"
#include "Minecraft.h"
#include "GameMode.h"
#include "..\Minecraft.World\net.minecraft.world.entity.player.h"
#include "..\Minecraft.World\net.minecraft.world.level.h"
#include "..\Minecraft.World\net.minecraft.world.level.storage.h"
#include "Input.h"
#include "..\Minecraft.Client\LocalPlayer.h"
#include "Options.h"
#include "KeyMapping.h"

Input::Input()
{
	xa = 0;
	ya = 0;
	wasJumping = false;
	jumping = false;
	sneaking = false;

	lReset = false;
    rReset = false;
}

void Input::tick(LocalPlayer *player)
{
	Minecraft *pMinecraft=Minecraft::GetInstance();
	int iPad=player->GetXboxPad();
	bool keyboardPlayer = (iPad == ProfileManager.GetPrimaryPad()) && pMinecraft->options != NULL;
	bool keyboardGameplay = keyboardPlayer && (pMinecraft->screen == NULL) && !ui.GetMenuDisplayed(iPad);

	// 4J-PB minecraft movement seems to be the wrong way round, so invert x!
	if( pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LEFT) || pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_RIGHT) )
		xa = -InputManager.GetJoypadStick_LX(iPad);
	else
		xa = 0.0f;
	
	if( pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_FORWARD) || pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_BACKWARD) )
		ya = InputManager.GetJoypadStick_LY(iPad);
	else
		ya = 0.0f;

	if (keyboardGameplay)
	{
		float keyboardXA = 0.0f;
		float keyboardYA = 0.0f;

		if (pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LEFT) && Keyboard::isKeyDown(pMinecraft->options->keyLeft->key))
		{
			keyboardXA += 1.0f;
		}
		if (pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_RIGHT) && Keyboard::isKeyDown(pMinecraft->options->keyRight->key))
		{
			keyboardXA -= 1.0f;
		}
		if (pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_FORWARD) && Keyboard::isKeyDown(pMinecraft->options->keyUp->key))
		{
			keyboardYA += 1.0f;
		}
		if (pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_BACKWARD) && Keyboard::isKeyDown(pMinecraft->options->keyDown->key))
		{
			keyboardYA -= 1.0f;
		}

		xa += keyboardXA;
		ya += keyboardYA;

		if (xa > 1.0f) xa = 1.0f;
		if (xa < -1.0f) xa = -1.0f;
		if (ya > 1.0f) ya = 1.0f;
		if (ya < -1.0f) ya = -1.0f;

		// FIX 4: Normalize diagonal movement to avoid speed boost.
		float moveLen = sqrtf(xa*xa + ya*ya);
		if (moveLen > 1.0f)
		{
			xa /= moveLen;
			ya /= moveLen;
		}
	}

#ifndef _CONTENT_PACKAGE
	if (app.GetFreezePlayers())
	{
		xa = ya = 0.0f;
		player->abilities.flying = true;
	}
#endif
	
    if (!lReset)
    {
        if (xa*xa+ya*ya==0.0f)
        {
            lReset = true;
        }
        xa = ya = 0.0f;
    }

	// 4J - in flying mode, don't actually toggle sneaking
	if(!player->abilities.flying)
	{
		if((player->ullButtonsPressed&(1LL<<MINECRAFT_ACTION_SNEAK_TOGGLE)) && pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_SNEAK_TOGGLE))
		{
			sneaking=!sneaking;
		}
	}

	if(sneaking)
	{
		xa*=0.3f;
		ya*=0.3f;
	}

    float turnSpeed = 50.0f;

	float tx = 0.0f;
	float ty = 0.0f;
	float mouseTx = 0.0f;
	float mouseTy = 0.0f;

	// --- GAMEPAD LOOK (unchanged) ---
	if( pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LOOK_LEFT) || pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LOOK_RIGHT) )
		tx = InputManager.GetJoypadStick_RX(iPad) * (((float)app.GetGameSettings(iPad,eGameSetting_Sensitivity_InGame)) / 100.0f);

	if( pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LOOK_UP) || pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LOOK_DOWN) )
		ty = InputManager.GetJoypadStick_RY(iPad) * (((float)app.GetGameSettings(iPad,eGameSetting_Sensitivity_InGame)) / 100.0f);

	// --- MOUSE LOOK ---
	if (keyboardGameplay)
	{
		float rawMouseDX = 0.0f;
		float rawMouseDY = 0.0f;

		if( pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LOOK_LEFT) || pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LOOK_RIGHT) )
		{
			rawMouseDX = (float)Mouse::getDX();
		}

		if( pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LOOK_UP) || pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_LOOK_DOWN) )
		{
			rawMouseDY = (float)-Mouse::getDY();
		}

		// FIX 1: Raise the clamp ceiling so fast mouse movements aren't cut off.
		const float maxDeltaPerTick = 1500.0f;
		if(rawMouseDX >  maxDeltaPerTick) rawMouseDX =  maxDeltaPerTick;
		if(rawMouseDX < -maxDeltaPerTick) rawMouseDX = -maxDeltaPerTick;
		if(rawMouseDY >  maxDeltaPerTick) rawMouseDY =  maxDeltaPerTick;
		if(rawMouseDY < -maxDeltaPerTick) rawMouseDY = -maxDeltaPerTick;

		// FIX 2: Use the in-game sensitivity setting to match gamepad.
		int mouseSensitivitySetting = app.GetGameSettings(iPad, eGameSetting_Sensitivity_InGame);
		if(mouseSensitivitySetting > 120) mouseSensitivitySetting = 120;

		// FIX 3: Linear scale — mouse deltas are pixels, not analog -1..1 values.
		float mouseSensitivity    = (float)mouseSensitivitySetting / 100.0f;
		const float mouseTurnScale = 0.025f;

		mouseTx = rawMouseDX * mouseSensitivity * mouseTurnScale;
		mouseTy = rawMouseDY * mouseSensitivity * mouseTurnScale;
	}

#ifndef _CONTENT_PACKAGE
	if (app.GetFreezePlayers())	tx = ty = mouseTx = mouseTy = 0.0f;
#endif

	// 4J: WESTY : Invert look Y if required.
	if ( app.GetGameSettings(iPad,eGameSetting_ControlInvertLook) )
	{
		ty    = -ty;
		mouseTy = -mouseTy;
	}

	// Gamepad uses quadratic curve (tx*|tx|) for analog feel.
	// Mouse path is already linear, added directly without extra curve.
	float turnX = (tx * abs(tx)) + mouseTx;
	float turnY = (ty * abs(ty)) + mouseTy;

	// FIX 5: Mouse input already starts at 0 each tick so rReset is meaningless
	// for it — skip the guard when using mouse to avoid freeze+jerk on click.
	if (!rReset)
	{
		if (turnX*turnX+turnY*turnY==0.0f)
		{
			rReset = true;
		}
		// Only zero out turn if there is no mouse contribution.
		// Gamepad path: mouseTx/mouseTy are both 0.0f here.
		if (mouseTx == 0.0f && mouseTy == 0.0f)
		{
			turnX = turnY = 0.0f;
		}
		else
		{
			rReset = true;
		}
	}

	player->interpolateTurn(turnX * turnSpeed, turnY * turnSpeed);

	unsigned int jump = InputManager.GetValue(iPad, MINECRAFT_ACTION_JUMP);
	bool keyJump = keyboardGameplay && Keyboard::isKeyDown(pMinecraft->options->keyJump->key);
	if( (jump > 0 || keyJump) && pMinecraft->localgameModes[iPad]->isInputAllowed(MINECRAFT_ACTION_JUMP) )
		jumping = true;
	else
 		jumping = false;

#ifndef _CONTENT_PACKAGE
	if (app.GetFreezePlayers())	jumping = false;
#endif
}