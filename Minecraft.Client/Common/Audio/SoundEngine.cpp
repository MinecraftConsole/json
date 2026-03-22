#include "stdafx.h"

#include "SoundEngine.h"
#include "..\Consoles_App.h"
#include "..\..\MultiplayerLocalPlayer.h"
#include "..\..\..\Minecraft.World\net.minecraft.world.level.h"
#include "..\..\Minecraft.World\leveldata.h"
#include "..\..\Minecraft.World\mth.h"
#include "..\..\TexturePackRepository.h"
#include "..\..\DLCTexturePack.h"
#include "Common\DLC\DLCAudioFile.h"

#ifdef _WINDOWS64
#include "..\..\Minecraft.Client\Windows64\Windows64_App.h"
#include "..\..\Minecraft.Client\Windows64\Miles\include\imssapi.h"
#endif

// ---------------------------------------------------------------------------
// Windows64-only build — all console platform code removed.
// ---------------------------------------------------------------------------

static bool g_windowsMilesDisabled = false;

// ---------------------------------------------------------------------------
// TEMP WINDOWS64 AUDIO LOGGING BEGIN
// Este bloque abarca helpers de log temporales para depurar Miles/audio:
//   _GetWindowsSoundLogPath
//   _ClearWindowsSoundLog
//   _AppendWindowsSoundLog
//   _AppendWindowsSoundLogPair
//   _AppendWindowsSoundLogInt
// Y también las llamadas de instrumentacion en:
//   SoundEngine::init
//   SoundEngine::OpenStreamThreadProc
//   SoundEngine::playMusicUpdate
// Cuando ya no se necesite depuracion, borrar este bloque y sus llamadas.
// ---------------------------------------------------------------------------

// Fixed Windows64 layout expected next to the exe:
//   Sound\Minecraft.msscmp
//   Sound\music\
//   redist64\mss64.dll
char SoundEngine::m_szSoundPath[]  = "Sound\\";
char SoundEngine::m_szMusicPath[]  = "Sound\\music\\";
char SoundEngine::m_szRedistName[] = "redist64";

// ---------------------------------------------------------------------------
// Sound bank path resolution — searches common relative locations so the
// binary can be run from different working directories.
// ---------------------------------------------------------------------------
// Returns the directory containing the running exe, with trailing backslash.
// e.g. "C:\MyProject\x64\Release\"
static void _GetExeDir(char *outDir, size_t outDirSize)
{
	char exePath[MAX_PATH] = {0};
	GetModuleFileNameA(NULL, exePath, MAX_PATH);
	// Strip the filename, keep the trailing backslash
	char *lastSlash = strrchr(exePath, '\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = '\0';
	}
	strcpy_s(outDir, outDirSize, exePath);
}

static void _GetWindowsSoundLogPath(char *outPath, size_t outPathSize)
{
	char exeDir[MAX_PATH];
	_GetExeDir(exeDir, MAX_PATH);
	strcpy_s(outPath, outPathSize, exeDir);
	strcat_s(outPath, outPathSize, "sound_init.txt");
}

static void _ClearWindowsSoundLog()
{
	char logPath[MAX_PATH];
	_GetWindowsSoundLogPath(logPath, MAX_PATH);
	HANDLE hFile = CreateFileA(logPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
	}
}

static void _AppendWindowsSoundLog(const char *text)
{
	char logPath[MAX_PATH];
	_GetWindowsSoundLogPath(logPath, MAX_PATH);
	HANDLE hFile = CreateFileA(logPath, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	DWORD written = 0;
	if (text != NULL)
	{
		WriteFile(hFile, text, (DWORD)strlen(text), &written, NULL);
	}
	WriteFile(hFile, "\r\n", 2, &written, NULL);
	CloseHandle(hFile);
}

static void _AppendWindowsSoundLogPair(const char *label, const char *value)
{
	char line[1024];
	sprintf_s(line, "%s%s", label, value ? value : "<null>");
	_AppendWindowsSoundLog(line);
}

static void _AppendWindowsSoundLogInt(const char *label, int value)
{
	char line[256];
	sprintf_s(line, "%s%d", label, value);
	_AppendWindowsSoundLog(line);
}

// TEMP WINDOWS64 AUDIO LOGGING END

static bool _SoundEngineFileExists(const char *path)
{
	DWORD attr = GetFileAttributesA(path);
	return (attr != INVALID_FILE_ATTRIBUTES) && ((attr & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

static bool _ResolveWindowsSoundBankPath(char *outPath, size_t outPathSize)
{
	char exeDir[MAX_PATH];
	_GetExeDir(exeDir, MAX_PATH);

	strcpy_s(outPath, outPathSize, exeDir);
	strcat_s(outPath, outPathSize, "Sound\\Minecraft.msscmp");

	return _SoundEngineFileExists(outPath);
}

// ---------------------------------------------------------------------------

F32 AILCALLBACK custom_falloff_function(HSAMPLE S,
                                        F32 distance,
                                        F32 rolloff_factor,
                                        F32 min_dist,
                                        F32 max_dist);

char *SoundEngine::m_szStreamFileA[eStream_Max] =
{
	"calm1",
	"calm2",
	"calm3",
	"hal1",
	"hal2",
	"hal3",
	"hal4",
	"nuance1",
	"nuance2",
	// new music tracks
	"creative1",
	"creative2",
	"creative3",
	"creative4",
	"creative5",
	"creative6",
	"menu1",
	"menu2",
	"menu3",
	"menu4",
	"piano1",
	"piano2",
	"piano3",
	// Nether
	"nether1",
	"nether2",
	"nether3",
	"nether4",
	// The End
	"the_end_dragon_alive",
	"the_end_end",
	// CDs
	"11",
	"13",
	"blocks",
	"cat",
	"chirp",
	"far",
	"mall",
	"mellohi",
	"stal",
	"strad",
	"ward",
	"where_are_we_now"
};

/////////////////////////////////////////////
//
//	ErrorCallback
//
/////////////////////////////////////////////
void AILCALL ErrorCallback(S64 i_Id, char const* i_Details)
{
	char *pchLastError = AIL_last_error();
	if (pchLastError[0] != 0)
		app.DebugPrintf("\rErrorCallback Error Category: %s\n", pchLastError);
	if (i_Details)
		app.DebugPrintf("ErrorCallback - Details: %s\n", i_Details);
}

/////////////////////////////////////////////
//
//	init
//
/////////////////////////////////////////////
void SoundEngine::init(Options *pOptions)
{
	app.DebugPrintf("---SoundEngine::init\n");
	g_windowsMilesDisabled = false;
	m_hBank   = NULL;
	m_hDriver = NULL;
	m_hStream = NULL;
	_ClearWindowsSoundLog();
	_AppendWindowsSoundLog("SoundEngine::init begin");

	// Build exe-relative paths for redist and music
	char exeDir[MAX_PATH];
	_GetExeDir(exeDir, MAX_PATH);
	_AppendWindowsSoundLogPair("exeDir=", exeDir);

	char redistPath[MAX_PATH];
	strcpy_s(redistPath, MAX_PATH, exeDir);
	strcat_s(redistPath, MAX_PATH, m_szRedistName);

	char redistDllPath[MAX_PATH];
	strcpy_s(redistDllPath, MAX_PATH, redistPath);
	strcat_s(redistDllPath, MAX_PATH, "\\mss64.dll");

	char binkPluginPath[MAX_PATH];
	strcpy_s(binkPluginPath, MAX_PATH, redistPath);
	strcat_s(binkPluginPath, MAX_PATH, "\\binkawin64.asi");

	char exeDllPath[MAX_PATH];
	strcpy_s(exeDllPath, MAX_PATH, exeDir);
	strcat_s(exeDllPath, MAX_PATH, "mss64.dll");

	// Set the Miles redist directory so it can find a consistent runtime set.
	// Prefer <exe>\redist64 so mss64.dll and its .flt/.asi plugins stay together.
	bool hasRedistDll = _SoundEngineFileExists(redistDllPath);
	bool hasExeDll = _SoundEngineFileExists(exeDllPath);
	bool hasBinkPlugin = _SoundEngineFileExists(binkPluginPath);
	_AppendWindowsSoundLogPair("redistDir=", redistPath);
	_AppendWindowsSoundLogPair("redistDll=", redistDllPath);
	_AppendWindowsSoundLogInt("hasRedistDll=", hasRedistDll ? 1 : 0);
	_AppendWindowsSoundLogPair("binkPlugin=", binkPluginPath);
	_AppendWindowsSoundLogInt("hasBinkPlugin=", hasBinkPlugin ? 1 : 0);
	_AppendWindowsSoundLogPair("exeDll=", exeDllPath);
	_AppendWindowsSoundLogInt("hasExeDll=", hasExeDll ? 1 : 0);
	if (hasRedistDll)
	{
		SetDllDirectoryA(redistPath);
		AIL_set_redist_directory(redistPath);
		_AppendWindowsSoundLogPair("SetDllDirectoryA=", redistPath);
		_AppendWindowsSoundLogPair("AIL_set_redist_directory=", redistPath);
	}
	else if (hasExeDll)
	{
		SetDllDirectoryA(exeDir);
		AIL_set_redist_directory(exeDir);
		_AppendWindowsSoundLogPair("SetDllDirectoryA=", exeDir);
		_AppendWindowsSoundLogPair("AIL_set_redist_directory=", exeDir);
	}
	else
	{
		_AppendWindowsSoundLog("Miles runtime not found");
		g_windowsMilesDisabled = true;
		return;
	}

	app.DebugPrintf("---SoundEngine::init - AIL_startup\n");
	S32 ret = AIL_startup();
	_AppendWindowsSoundLogInt("AIL_startup_ret=", ret);
	if (ret == 0)
	{
		app.DebugPrintf("SoundEngine::init - AIL_startup failed: %s\n", AIL_last_error());
		_AppendWindowsSoundLogPair("AIL_startup_error=", AIL_last_error());
		g_windowsMilesDisabled = true;
		_AppendWindowsSoundLog("g_windowsMilesDisabled=1");
		return;
	}

	m_hDriver = AIL_open_digital_driver(44100, 16, MSS_MC_USE_SYSTEM_CONFIG, 0);
	if (m_hDriver == 0)
	{
		app.DebugPrintf("Couldn't open digital sound driver with system config: %s\n", AIL_last_error());
		_AppendWindowsSoundLogPair("AIL_open_digital_driver_error=", AIL_last_error());

		m_hDriver = AIL_open_digital_driver(44100, 16, MSS_MC_STEREO, 0);
		if (m_hDriver == 0)
		{
			app.DebugPrintf("Couldn't open digital sound driver in stereo fallback: %s\n", AIL_last_error());
			_AppendWindowsSoundLogPair("AIL_open_digital_driver_fallback_error=", AIL_last_error());
			AIL_shutdown();
			g_windowsMilesDisabled = true;
			_AppendWindowsSoundLog("g_windowsMilesDisabled=1");
			return;
		}

		_AppendWindowsSoundLog("AIL_open_digital_driver_fallback=OK");
	}
	app.DebugPrintf("---SoundEngine::init - driver opened\n");
	_AppendWindowsSoundLog("AIL_open_digital_driver=OK");

	AIL_set_event_error_callback(ErrorCallback);
	AIL_set_3D_rolloff_factor(m_hDriver, 1.0);

	if (AIL_startup_event_system(m_hDriver, 1024*20, 0, 1024*128) == 0)
	{
		app.DebugPrintf("Couldn't init event system: %s\n", AIL_last_error());
		_AppendWindowsSoundLogPair("AIL_startup_event_system_error=", AIL_last_error());
		AIL_close_digital_driver(m_hDriver);
		AIL_shutdown();
		g_windowsMilesDisabled = true;
		_AppendWindowsSoundLog("g_windowsMilesDisabled=1");
		return;
	}
	_AppendWindowsSoundLog("AIL_startup_event_system=OK");

	char szBankName[MAX_PATH];
	bool hasResolvedBankPath = _ResolveWindowsSoundBankPath(szBankName, sizeof(szBankName));
	_AppendWindowsSoundLogPair("soundbankPath=", szBankName);
	_AppendWindowsSoundLogInt("soundbankExists=", _SoundEngineFileExists(szBankName) ? 1 : 0);
	_AppendWindowsSoundLogInt("soundbankResolved=", hasResolvedBankPath ? 1 : 0);
	if (!hasResolvedBankPath)
	{
		char cwd[MAX_PATH];
		DWORD cwdLen = GetCurrentDirectoryA(MAX_PATH, cwd);
		if (cwdLen > 0 && cwdLen < MAX_PATH)
			app.DebugPrintf("SoundEngine::init - current dir: %s\n", cwd);
		app.DebugPrintf("SoundEngine::init - couldn't locate soundbank, using default: %s\n", szBankName);
	}

	m_hBank = AIL_add_soundbank(szBankName, 0);
	if (m_hBank == NULL)
	{
		app.DebugPrintf("Couldn't open soundbank: %s (%s)\n", szBankName, AIL_last_error());
		_AppendWindowsSoundLogPair("AIL_add_soundbank_error=", AIL_last_error());
		AIL_close_digital_driver(m_hDriver);
		AIL_shutdown();
		g_windowsMilesDisabled = true;
		_AppendWindowsSoundLog("g_windowsMilesDisabled=1");
		return;
	}
	_AppendWindowsSoundLog("AIL_add_soundbank=OK");

	// Enumerate events for debugging
	HMSSENUM token = MSS_FIRST;
	char const* Events[1] = {0};
	S32 EventCount = 0;
	while (AIL_enumerate_events(m_hBank, &token, 0, &Events[0]))
	{
		app.DebugPrintf(4, "%d - %s\n", EventCount, Events[0]);
		EventCount++;
	}

	U64 u64Result;
	u64Result = AIL_enqueue_event_by_name("Minecraft/CacheSounds");

	m_MasterMusicVolume   = 1.0f;
	m_MasterEffectsVolume = 1.0f;
	m_bSystemMusicPlaying = false;
	m_openStreamThread    = NULL;

	app.DebugPrintf("---SoundEngine::init - complete\n");
	_AppendWindowsSoundLog("SoundEngine::init complete");
}

void SoundEngine::SetStreamingSounds(int iOverworldMin, int iOverWorldMax, int iNetherMin, int iNetherMax, int iEndMin, int iEndMax, int iCD1)
{
	m_iStream_Overworld_Min = iOverworldMin;
	m_iStream_Overworld_Max = iOverWorldMax;
	m_iStream_Nether_Min    = iNetherMin;
	m_iStream_Nether_Max    = iNetherMax;
	m_iStream_End_Min       = iEndMin;
	m_iStream_End_Max       = iEndMax;
	m_iStream_CD_1          = iCD1;

	if (m_bHeardTrackA)
		delete[] m_bHeardTrackA;

	m_bHeardTrackA = new bool[iEndMax + 1];
	memset(m_bHeardTrackA, 0, sizeof(bool) * (iEndMax + 1));
}

/////////////////////////////////////////////
//
//	updateMiles
//
/////////////////////////////////////////////
void SoundEngine::updateMiles()
{
	if (g_windowsMilesDisabled || m_hDriver == NULL)
		return;

	if (m_validListenerCount == 1)
	{
		for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
		{
			if (m_ListenerA[i].bValid)
			{
				AIL_set_listener_3D_position(m_hDriver,
					m_ListenerA[i].vPosition.x,
					m_ListenerA[i].vPosition.y,
					-m_ListenerA[i].vPosition.z);
				AIL_set_listener_3D_orientation(m_hDriver,
					-m_ListenerA[i].vOrientFront.x,
					m_ListenerA[i].vOrientFront.y,
					m_ListenerA[i].vOrientFront.z,
					0, 1, 0);
				break;
			}
		}
	}
	else
	{
		AIL_set_listener_3D_position(m_hDriver, 0, 0, 0);
		AIL_set_listener_3D_orientation(m_hDriver, 0, 0, 1, 0, 1, 0);
	}

	AIL_begin_event_queue_processing();

	HMSSENUM token = MSS_FIRST;
	MILESEVENTSOUNDINFO SoundInfo;
	while (AIL_enumerate_sound_instances(0, &token, 0, 0, 0, &SoundInfo))
	{
		AUDIO_INFO *game_data = (AUDIO_INFO *)(SoundInfo.UserBuffer);

		if (SoundInfo.Status != MILESEVENT_SOUND_STATUS_COMPLETE)
		{
			bool isThunder = (game_data->volume == 10000.0f);
			if (game_data->volume > 1) game_data->volume = 1;

			AIL_set_sample_volume_levels(SoundInfo.Sample,
				game_data->volume * m_MasterEffectsVolume,
				game_data->volume * m_MasterEffectsVolume);

			float distanceScaler = 16.0f;

			switch (SoundInfo.Status)
			{
			case MILESEVENT_SOUND_STATUS_PENDING:
				AIL_register_falloff_function_callback(SoundInfo.Sample, &custom_falloff_function);

				if (game_data->bIs3D)
				{
					AIL_set_sample_is_3D(SoundInfo.Sample, 1);

					int iSound = game_data->iSound - eSFX_MAX;
					switch (iSound)
					{
					case eSoundType_MOB_ENDERDRAGON_GROWL:
					case eSoundType_MOB_ENDERDRAGON_MOVE:
					case eSoundType_MOB_ENDERDRAGON_END:
					case eSoundType_MOB_ENDERDRAGON_HIT:
						distanceScaler = 100.0f;
						break;
					case eSoundType_MOB_GHAST_MOAN:
					case eSoundType_MOB_GHAST_SCREAM:
					case eSoundType_MOB_GHAST_DEATH:
					case eSoundType_MOB_GHAST_CHARGE:
					case eSoundType_MOB_GHAST_FIREBALL:
						distanceScaler = 30.0f;
						break;
					}
					if (isThunder) distanceScaler = 10000.0f;
				}
				else
				{
					AIL_set_sample_is_3D(SoundInfo.Sample, 0);
				}

				AIL_set_sample_3D_distances(SoundInfo.Sample, distanceScaler, 1, 0);

				if (!game_data->bUseSoundsPitchVal)
					AIL_set_sample_playback_rate_factor(SoundInfo.Sample, game_data->pitch);

				if (game_data->bIs3D)
				{
					if (m_validListenerCount > 1)
					{
						float fClosest = 10000.0f, fClosestX = 0, fClosestY = 0, fClosestZ = 0, fDist;
						for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
						{
							if (m_ListenerA[i].bValid)
							{
								float x = fabs(m_ListenerA[i].vPosition.x - game_data->x);
								float y = fabs(m_ListenerA[i].vPosition.y - game_data->y);
								float z = fabs(m_ListenerA[i].vPosition.z - game_data->z);
								fDist = x + y + z;
								if (fDist < fClosest) { fClosest = fDist; fClosestX = x; fClosestY = y; fClosestZ = z; }
							}
						}
						fDist = sqrtf(fClosestX*fClosestX + fClosestY*fClosestY + fClosestZ*fClosestZ);
						AIL_set_sample_3D_position(SoundInfo.Sample, 0, 0, fDist);
					}
					else
					{
						AIL_set_sample_3D_position(SoundInfo.Sample, game_data->x, game_data->y, -game_data->z);
					}
				}
				break;

			default:
				if (game_data->bIs3D)
				{
					if (m_validListenerCount > 1)
					{
						float fClosest = 10000.0f, fClosestX = 0, fClosestY = 0, fClosestZ = 0, fDist;
						for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
						{
							if (m_ListenerA[i].bValid)
							{
								float x = fabs(m_ListenerA[i].vPosition.x - game_data->x);
								float y = fabs(m_ListenerA[i].vPosition.y - game_data->y);
								float z = fabs(m_ListenerA[i].vPosition.z - game_data->z);
								fDist = x + y + z;
								if (fDist < fClosest) { fClosest = fDist; fClosestX = x; fClosestY = y; fClosestZ = z; }
							}
						}
						fDist = sqrtf(fClosestX*fClosestX + fClosestY*fClosestY + fClosestZ*fClosestZ);
						AIL_set_sample_3D_position(SoundInfo.Sample, 0, 0, fDist);
					}
					else
					{
						AIL_set_sample_3D_position(SoundInfo.Sample, game_data->x, game_data->y, -game_data->z);
					}
				}
				break;
			}
		}
	}

	AIL_complete_event_queue_processing();
}

/////////////////////////////////////////////
//
//	tick
//
/////////////////////////////////////////////
void SoundEngine::tick(shared_ptr<Mob> *players, float a)
{
	if (g_windowsMilesDisabled || m_hDriver == NULL)
		return;

	int listenerCount = 0;

	if (players)
	{
		for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
		{
			if (players[i] != NULL)
			{
				m_ListenerA[i].bValid = true;
				F32 x = players[i]->xo + (players[i]->x - players[i]->xo) * a;
				F32 y = players[i]->yo + (players[i]->y - players[i]->yo) * a;
				F32 z = players[i]->zo + (players[i]->z - players[i]->zo) * a;

				float yRot = players[i]->yRotO + (players[i]->yRot - players[i]->yRotO) * a;
				float yCos = (float)cos(-yRot * Mth::RAD_TO_GRAD - PI);
				float ySin = (float)sin(-yRot * Mth::RAD_TO_GRAD - PI);

				m_ListenerA[i].vPosition.x    = x;
				m_ListenerA[i].vPosition.y    = y;
				m_ListenerA[i].vPosition.z    = z;
				m_ListenerA[i].vOrientFront.x = ySin;
				m_ListenerA[i].vOrientFront.y = 0;
				m_ListenerA[i].vOrientFront.z = yCos;
				listenerCount++;
			}
			else
			{
				m_ListenerA[i].bValid = false;
			}
		}
	}

	if (listenerCount == 0)
	{
		m_ListenerA[0].vPosition.x    = 0;
		m_ListenerA[0].vPosition.y    = 0;
		m_ListenerA[0].vPosition.z    = 0;
		m_ListenerA[0].vOrientFront.x = 0;
		m_ListenerA[0].vOrientFront.y = 0;
		m_ListenerA[0].vOrientFront.z = 1.0f;
		listenerCount++;
	}
	m_validListenerCount = listenerCount;

	updateMiles();
}

/////////////////////////////////////////////
//
//	SoundEngine constructor
//
/////////////////////////////////////////////
SoundEngine::SoundEngine()
{
	random         = new Random();
	m_hStream      = 0;
	m_StreamState  = eMusicStreamState_Idle;
	m_iMusicDelay  = 0;
	m_validListenerCount = 0;
	m_bHeardTrackA = NULL;

	SetStreamingSounds(
		eStream_Overworld_Calm1, eStream_Overworld_piano3,
		eStream_Nether1,         eStream_Nether4,
		eStream_end_dragon,      eStream_end_end,
		eStream_CD_1);

	m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);

	m_StreamingAudioInfo.bIs3D  = false;
	m_StreamingAudioInfo.x      = 0;
	m_StreamingAudioInfo.y      = 0;
	m_StreamingAudioInfo.z      = 0;
	m_StreamingAudioInfo.volume = 1;
	m_StreamingAudioInfo.pitch  = 1;

	memset(CurrentSoundsPlaying, 0, sizeof(int) * (eSoundType_MAX + eSFX_MAX));
	memset(m_ListenerA, 0, sizeof(AUDIO_LISTENER) * XUSER_MAX_COUNT);
}

void SoundEngine::destroy() {}

#ifdef _DEBUG
void SoundEngine::GetSoundName(char *szSoundName, int iSound)
{
	strcpy((char *)szSoundName, "Minecraft/");
	wstring name    = wchSoundNames[iSound];
	char *SoundName = (char *)ConvertSoundPathToName(name);
	strcat((char *)szSoundName, SoundName);
}
#endif

/////////////////////////////////////////////
//
//	play
//
/////////////////////////////////////////////
void SoundEngine::play(int iSound, float x, float y, float z, float volume, float pitch)
{
	if (g_windowsMilesDisabled || m_hDriver == NULL)
		return;

	if (iSound == -1)
	{
		app.DebugPrintf(6, "PlaySound with sound of -1 !!!!!!!!!!!!!!!\n");
		return;
	}

	U8 szSoundName[256];
	strcpy((char *)szSoundName, "Minecraft/");
	wstring name    = wchSoundNames[iSound];
	char *SoundName = (char *)ConvertSoundPathToName(name);
	strcat((char *)szSoundName, SoundName);

	AUDIO_INFO AudioInfo;
	AudioInfo.x               = x;
	AudioInfo.y               = y;
	AudioInfo.z               = z;
	AudioInfo.volume          = volume;
	AudioInfo.pitch           = pitch;
	AudioInfo.bIs3D           = true;
	AudioInfo.bUseSoundsPitchVal = false;
	AudioInfo.iSound          = iSound + eSFX_MAX;
#ifdef _DEBUG
	strncpy(AudioInfo.chName, (char *)szSoundName, 64);
#endif

	S32 token = AIL_enqueue_event_start();
	AIL_enqueue_event_buffer(&token, &AudioInfo, sizeof(AUDIO_INFO), 0);
	AIL_enqueue_event_end_named(token, (char *)szSoundName);
}

/////////////////////////////////////////////
//
//	playUI
//
/////////////////////////////////////////////
void SoundEngine::playUI(int iSound, float volume, float pitch)
{
	if (g_windowsMilesDisabled || m_hDriver == NULL)
		return;

	U8 szSoundName[256];
	wstring name;

	if (iSound >= eSFX_MAX)
	{
		strcpy((char *)szSoundName, "Minecraft/");
		name = wchSoundNames[iSound];
	}
	else
	{
		strcpy((char *)szSoundName, "Minecraft/UI/");
		name = wchUISoundNames[iSound];
	}

	char *SoundName = (char *)ConvertSoundPathToName(name);
	strcat((char *)szSoundName, SoundName);

	AUDIO_INFO AudioInfo;
	memset(&AudioInfo, 0, sizeof(AUDIO_INFO));
	AudioInfo.volume           = volume;
	AudioInfo.pitch            = pitch;
	AudioInfo.bUseSoundsPitchVal = true;
	AudioInfo.iSound           = (iSound >= eSFX_MAX) ? iSound + eSFX_MAX : iSound;
#ifdef _DEBUG
	strncpy(AudioInfo.chName, (char *)szSoundName, 64);
#endif

	S32 token = AIL_enqueue_event_start();
	AIL_enqueue_event_buffer(&token, &AudioInfo, sizeof(AUDIO_INFO), 0);
	AIL_enqueue_event_end_named(token, (char *)szSoundName);
}

/////////////////////////////////////////////
//
//	playStreaming
//
/////////////////////////////////////////////
void SoundEngine::playStreaming(const wstring& name, float x, float y, float z, float volume, float pitch, bool bMusicDelay)
{
	if (g_windowsMilesDisabled || m_hDriver == NULL)
		return;

	m_StreamingAudioInfo.x      = x;
	m_StreamingAudioInfo.y      = y;
	m_StreamingAudioInfo.z      = z;
	m_StreamingAudioInfo.volume = volume;
	m_StreamingAudioInfo.pitch  = pitch;

	if (m_StreamState == eMusicStreamState_Playing)
		m_StreamState = eMusicStreamState_Stop;
	else if (m_StreamState == eMusicStreamState_Opening)
		m_StreamState = eMusicStreamState_OpeningCancel;

	if (name.empty())
	{
		m_StreamingAudioInfo.bIs3D = false;
		m_iMusicDelay = random->nextInt(20 * 60 * 3);
#ifdef _DEBUG
		m_iMusicDelay = 0;
#endif
		Minecraft *pMinecraft = Minecraft::GetInstance();
		bool playerInEnd    = false;
		bool playerInNether = false;

		for (unsigned int i = 0; i < MAX_LOCAL_PLAYERS; i++)
		{
			if (pMinecraft->localplayers[i] != NULL)
			{
				if      (pMinecraft->localplayers[i]->dimension == LevelData::DIMENSION_END)    playerInEnd    = true;
				else if (pMinecraft->localplayers[i]->dimension == LevelData::DIMENSION_NETHER) playerInNether = true;
			}
		}

		if      (playerInEnd)    m_musicID = getMusicID(LevelData::DIMENSION_END);
		else if (playerInNether) m_musicID = getMusicID(LevelData::DIMENSION_NETHER);
		else                     m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD);
	}
	else
	{
		m_StreamingAudioInfo.bIs3D = true;
		m_musicID     = getMusicID(name);
		m_iMusicDelay = 0;
	}
}

int SoundEngine::GetRandomishTrack(int iStart, int iEnd)
{
	bool bAllTracksHeard = true;
	int iVal = iStart;

	for (int i = iStart; i <= iEnd; i++)
	{
		if (m_bHeardTrackA[i] == false) { bAllTracksHeard = false; break; }
	}

	if (bAllTracksHeard)
	{
		for (int i = iStart; i <= iEnd; i++) m_bHeardTrackA[i] = false;
	}

	for (int i = 0; i <= ((iEnd - iStart) / 2); i++)
	{
		iVal = random->nextInt((iEnd - iStart) + 1) + iStart;
		if (m_bHeardTrackA[iVal] == false)
		{
			m_bHeardTrackA[iVal] = true;
			break;
		}
	}

	return iVal;
}

/////////////////////////////////////////////
//
//	getMusicID
//
/////////////////////////////////////////////
int SoundEngine::getMusicID(int iDomain)
{
	Minecraft *pMinecraft = Minecraft::GetInstance();

	if (pMinecraft == NULL)
		return GetRandomishTrack(m_iStream_Overworld_Min, m_iStream_Overworld_Max);

	if (pMinecraft->skins->isUsingDefaultSkin())
	{
		switch (iDomain)
		{
		case LevelData::DIMENSION_END:    return m_iStream_End_Min;
		case LevelData::DIMENSION_NETHER: return GetRandomishTrack(m_iStream_Nether_Min, m_iStream_Nether_Max);
		default:                          return GetRandomishTrack(m_iStream_Overworld_Min, m_iStream_Overworld_Max);
		}
	}
	else
	{
		switch (iDomain)
		{
		case LevelData::DIMENSION_END:    return GetRandomishTrack(m_iStream_End_Min, m_iStream_End_Max);
		case LevelData::DIMENSION_NETHER: return GetRandomishTrack(m_iStream_Nether_Min, m_iStream_Nether_Max);
		default:                          return GetRandomishTrack(m_iStream_Overworld_Min, m_iStream_Overworld_Max);
		}
	}
}

int SoundEngine::getMusicID(const wstring& name)
{
	int iCD = 0;
	char *SoundName = (char *)ConvertSoundPathToName(name, true);

	for (int i = 0; i < 12; i++)
	{
		if (strcmp(SoundName, m_szStreamFileA[i + eStream_CD_1]) == 0)
		{
			iCD = i;
			break;
		}
	}

	return iCD + m_iStream_CD_1;
}

float SoundEngine::getMasterMusicVolume()
{
	return m_bSystemMusicPlaying ? 0.0f : m_MasterMusicVolume;
}

void SoundEngine::updateMusicVolume(float fVal)      { m_MasterMusicVolume   = fVal; }
void SoundEngine::updateSoundEffectVolume(float fVal){ m_MasterEffectsVolume = fVal; }
void SoundEngine::updateSystemMusicPlaying(bool b)   { m_bSystemMusicPlaying = b;    }

void SoundEngine::add(const wstring& name, File *file)          {}
void SoundEngine::addMusic(const wstring& name, File *file)     {}
void SoundEngine::addStreaming(const wstring& name, File *file) {}
bool SoundEngine::isStreamingWavebankReady()                    { return true; }

int SoundEngine::OpenStreamThreadProc(void *lpParameter)
{
	SoundEngine *soundEngine = (SoundEngine *)lpParameter;
	_AppendWindowsSoundLogPair("AIL_open_stream_path=", soundEngine->m_szStreamName);
	soundEngine->m_hStream = AIL_open_stream(soundEngine->m_hDriver, soundEngine->m_szStreamName, 0);
	if (soundEngine->m_hStream == NULL)
	{
		_AppendWindowsSoundLogPair("AIL_open_stream_error=", AIL_last_error());
	}
	else
	{
		_AppendWindowsSoundLog("AIL_open_stream=OK");
	}
	return 0;
}

/////////////////////////////////////////////
//
//	playMusicTick / playMusicUpdate
//
/////////////////////////////////////////////
void SoundEngine::playMusicTick()
{
	if (g_windowsMilesDisabled || m_hDriver == NULL)
		return;
	playMusicUpdate();
}

void SoundEngine::playMusicUpdate()
{
	static bool  firstCall  = true;
	static float fMusicVol  = 0.0f;
	if (firstCall) { fMusicVol = getMasterMusicVolume(); firstCall = false; }

	switch (m_StreamState)
	{
	case eMusicStreamState_Idle:
		if (m_iMusicDelay > 0) { m_iMusicDelay--; return; }

		if (m_musicID != -1)
		{
		// Build exe-relative music base path
		char exeDir[MAX_PATH];
		_GetExeDir(exeDir, MAX_PATH);
		strcpy_s((char *)m_szStreamName, sizeof(m_szStreamName), exeDir);
		strcat_s((char *)m_szStreamName, sizeof(m_szStreamName), m_szMusicPath);

			if (Minecraft::GetInstance()->skins->getSelected()->hasAudio())
			{
				// Mash-up pack
				TexturePack    *pTexPack    = Minecraft::GetInstance()->skins->getSelected();
				DLCTexturePack *pDLCTexPack = (DLCTexturePack *)pTexPack;
				DLCPack        *pack        = pDLCTexPack->getDLCInfoParentPack();
				DLCAudioFile   *dlcAudioFile = (DLCAudioFile *)pack->getFile(DLCManager::e_DLCType_Audio, 0);

				if (m_musicID < m_iStream_CD_1)
				{
					SetIsPlayingStreamingGameMusic(true);
					SetIsPlayingStreamingCDMusic(false);
					m_MusicType                = eMusicType_Game;
					m_StreamingAudioInfo.bIs3D = false;

					wstring &wstrSoundName = dlcAudioFile->GetSoundName(m_musicID);
					char szName[255];
					wcstombs(szName, wstrSoundName.c_str(), 255);
					string strFile   = "TPACK:\\Data\\" + string(szName) + ".binka";
					string mountedPath = StorageManager.GetMountedPath(strFile);
					strcpy(m_szStreamName, mountedPath.c_str());
				}
				else
				{
					SetIsPlayingStreamingGameMusic(false);
					SetIsPlayingStreamingCDMusic(true);
					m_MusicType                = eMusicType_CD;
					m_StreamingAudioInfo.bIs3D = true;
					strcat((char *)m_szStreamName, "cds\\");
					strcat((char *)m_szStreamName, m_szStreamFileA[m_musicID - m_iStream_CD_1 + eStream_CD_1]);
					strcat((char *)m_szStreamName, ".binka");
				}
			}
			else
			{
				if (m_musicID < m_iStream_CD_1)
				{
					SetIsPlayingStreamingGameMusic(true);
					SetIsPlayingStreamingCDMusic(false);
					m_MusicType                = eMusicType_Game;
					m_StreamingAudioInfo.bIs3D = false;
					strcat((char *)m_szStreamName, "music\\");
				}
				else
				{
					SetIsPlayingStreamingGameMusic(false);
					SetIsPlayingStreamingCDMusic(true);
					m_MusicType                = eMusicType_CD;
					m_StreamingAudioInfo.bIs3D = true;
					strcat((char *)m_szStreamName, "cds\\");
				}
				strcat((char *)m_szStreamName, m_szStreamFileA[m_musicID]);
				strcat((char *)m_szStreamName, ".binka");
			}

			app.DebugPrintf("Starting streaming - %s\n", m_szStreamName);
			_AppendWindowsSoundLogPair("Starting streaming - ", m_szStreamName);
			m_openStreamThread = new C4JThread(OpenStreamThreadProc, this, "OpenStreamThreadProc");
			m_openStreamThread->Run();
			m_StreamState = eMusicStreamState_Opening;
		}
		break;

	case eMusicStreamState_Opening:
		if (!m_openStreamThread->isRunning())
		{
			delete m_openStreamThread;
			m_openStreamThread = NULL;

			HSAMPLE hSample = AIL_stream_sample_handle(m_hStream);
			AIL_register_falloff_function_callback(hSample, &custom_falloff_function);

			if (m_StreamingAudioInfo.bIs3D)
			{
				AIL_set_sample_3D_distances(hSample, 64.0f, 1, 0);
				if (m_validListenerCount > 1)
				{
					float fClosest = 10000.0f, fClosestX = 0, fClosestY = 0, fClosestZ = 0, fDist;
					for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
					{
						if (m_ListenerA[i].bValid)
						{
							float x = fabs(m_ListenerA[i].vPosition.x - m_StreamingAudioInfo.x);
							float y = fabs(m_ListenerA[i].vPosition.y - m_StreamingAudioInfo.y);
							float z = fabs(m_ListenerA[i].vPosition.z - m_StreamingAudioInfo.z);
							float d = x + y + z;
							if (d < fClosest) { fClosest = d; fClosestX = x; fClosestY = y; fClosestZ = z; }
						}
					}
					fDist = sqrtf(fClosestX*fClosestX + fClosestY*fClosestY + fClosestZ*fClosestZ);
					AIL_set_sample_3D_position(hSample, 0, 0, fDist);
				}
				else
				{
					AIL_set_sample_3D_position(hSample, m_StreamingAudioInfo.x, m_StreamingAudioInfo.y, -m_StreamingAudioInfo.z);
				}
			}
			else
			{
				AIL_set_sample_is_3D(hSample, 0);
			}

			AIL_set_sample_playback_rate_factor(hSample, m_StreamingAudioInfo.pitch);
			AIL_set_sample_volume_levels(hSample,
				m_StreamingAudioInfo.volume * getMasterMusicVolume(),
				m_StreamingAudioInfo.volume * getMasterMusicVolume());
			AIL_start_stream(m_hStream);
			m_StreamState = eMusicStreamState_Playing;
		}
		break;

	case eMusicStreamState_OpeningCancel:
		if (!m_openStreamThread->isRunning())
		{
			delete m_openStreamThread;
			m_openStreamThread = NULL;
			m_StreamState = eMusicStreamState_Stop;
		}
		break;

	case eMusicStreamState_Stop:
		AIL_pause_stream(m_hStream, 1);
		AIL_close_stream(m_hStream);
		m_hStream = 0;
		SetIsPlayingStreamingCDMusic(false);
		SetIsPlayingStreamingGameMusic(false);
		m_StreamState = eMusicStreamState_Idle;
		break;

	case eMusicStreamState_Stopping:
	case eMusicStreamState_Play:
		break;

	case eMusicStreamState_Playing:
		if (GetIsPlayingStreamingGameMusic())
		{
			Minecraft *pMinecraft  = Minecraft::GetInstance();
			bool playerInEnd    = false;
			bool playerInNether = false;

			for (unsigned int i = 0; i < MAX_LOCAL_PLAYERS; ++i)
			{
				if (pMinecraft->localplayers[i] != NULL)
				{
					if      (pMinecraft->localplayers[i]->dimension == LevelData::DIMENSION_END)    playerInEnd    = true;
					else if (pMinecraft->localplayers[i]->dimension == LevelData::DIMENSION_NETHER) playerInNether = true;
				}
			}

			if (playerInEnd && !GetIsPlayingEndMusic())
			{
				m_StreamState = eMusicStreamState_Stop;
				m_musicID     = getMusicID(LevelData::DIMENSION_END);
				SetIsPlayingEndMusic(true); SetIsPlayingNetherMusic(false);
			}
			else if (!playerInEnd && GetIsPlayingEndMusic())
			{
				m_StreamState = eMusicStreamState_Stop;
				if (playerInNether) { m_musicID = getMusicID(LevelData::DIMENSION_NETHER); SetIsPlayingEndMusic(false); SetIsPlayingNetherMusic(true); }
				else                { m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD); SetIsPlayingEndMusic(false); SetIsPlayingNetherMusic(false); }
			}
			else if (playerInNether && !GetIsPlayingNetherMusic())
			{
				m_StreamState = eMusicStreamState_Stop;
				m_musicID     = getMusicID(LevelData::DIMENSION_NETHER);
				SetIsPlayingNetherMusic(true); SetIsPlayingEndMusic(false);
			}
			else if (!playerInNether && GetIsPlayingNetherMusic())
			{
				m_StreamState = eMusicStreamState_Stop;
				if (playerInEnd) { m_musicID = getMusicID(LevelData::DIMENSION_END); SetIsPlayingNetherMusic(false); SetIsPlayingEndMusic(true); }
				else             { m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD); SetIsPlayingNetherMusic(false); SetIsPlayingEndMusic(false); }
			}

			if (fMusicVol != getMasterMusicVolume())
			{
				fMusicVol = getMasterMusicVolume();
				HSAMPLE hSample = AIL_stream_sample_handle(m_hStream);
				AIL_set_sample_volume_levels(hSample, fMusicVol, fMusicVol);
			}
		}
		else if (m_StreamingAudioInfo.bIs3D && m_validListenerCount > 1)
		{
			float fClosest = 10000.0f, fClosestX = 0, fClosestY = 0, fClosestZ = 0, fDist;
			for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
			{
				if (m_ListenerA[i].bValid)
				{
					float x = fabs(m_ListenerA[i].vPosition.x - m_StreamingAudioInfo.x);
					float y = fabs(m_ListenerA[i].vPosition.y - m_StreamingAudioInfo.y);
					float z = fabs(m_ListenerA[i].vPosition.z - m_StreamingAudioInfo.z);
					float d = x + y + z;
					if (d < fClosest) { fClosest = d; fClosestX = x; fClosestY = y; fClosestZ = z; }
				}
			}
			fDist = sqrtf(fClosestX*fClosestX + fClosestY*fClosestY + fClosestZ*fClosestZ);
			HSAMPLE hSample = AIL_stream_sample_handle(m_hStream);
			AIL_set_sample_3D_position(hSample, 0, 0, fDist);
		}
		break;

	case eMusicStreamState_Completed:
		{
			m_iMusicDelay = random->nextInt(20 * 60 * 3);
			Minecraft *pMinecraft  = Minecraft::GetInstance();
			bool playerInEnd    = false;
			bool playerInNether = false;

			for (unsigned int i = 0; i < MAX_LOCAL_PLAYERS; i++)
			{
				if (pMinecraft->localplayers[i] != NULL)
				{
					if      (pMinecraft->localplayers[i]->dimension == LevelData::DIMENSION_END)    playerInEnd    = true;
					else if (pMinecraft->localplayers[i]->dimension == LevelData::DIMENSION_NETHER) playerInNether = true;
				}
			}

			if      (playerInEnd)    { m_musicID = getMusicID(LevelData::DIMENSION_END);      SetIsPlayingEndMusic(true);  SetIsPlayingNetherMusic(false); }
			else if (playerInNether) { m_musicID = getMusicID(LevelData::DIMENSION_NETHER);   SetIsPlayingNetherMusic(true); SetIsPlayingEndMusic(false); }
			else                     { m_musicID = getMusicID(LevelData::DIMENSION_OVERWORLD); SetIsPlayingNetherMusic(false); SetIsPlayingEndMusic(false); }

			m_StreamState = eMusicStreamState_Idle;
		}
		break;
	}

	if (m_hStream != 0 && AIL_stream_status(m_hStream) == SMP_DONE)
	{
		AIL_close_stream(m_hStream);
		m_hStream = 0;
		SetIsPlayingStreamingCDMusic(false);
		SetIsPlayingStreamingGameMusic(false);
		m_StreamState = eMusicStreamState_Completed;
	}
}

/////////////////////////////////////////////
//
//	ConvertSoundPathToName
//
/////////////////////////////////////////////
char *SoundEngine::ConvertSoundPathToName(const wstring& name, bool bConvertSpaces)
{
	static char buf[256];
	assert(name.length() < 256);
	for (unsigned int i = 0; i < name.length(); i++)
	{
		wchar_t c = name[i];
		if (c == '.') c = '/';
		if (bConvertSpaces && c == ' ') c = '_';
		buf[i] = (char)c;
	}
	buf[name.length()] = 0;
	return buf;
}

// ---------------------------------------------------------------------------
//  custom_falloff_function — linear falloff emulating Xbox 360 XACT behaviour
// ---------------------------------------------------------------------------
F32 AILCALLBACK custom_falloff_function(HSAMPLE S,
                                        F32 distance,
                                        F32 rolloff_factor,
                                        F32 min_dist,
                                        F32 max_dist)
{
	if (max_dist == 10000.0f) return 1.0f;   // thunder: no attenuation
	F32 result = 1.0f - (distance / max_dist);
	if (result < 0.0f) result = 0.0f;
	if (result > 1.0f) result = 1.0f;
	return result;
}
