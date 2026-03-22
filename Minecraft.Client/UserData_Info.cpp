#include "stdafx.h"
#include "UserData_Info.h"

#include <cstdlib>

#include "..\Minecraft.World\Definitions.h"
#include "Common\Consoles_App.h"

namespace
{
	const wchar_t *kUserDataInfoFileName = L"UserData_Info.userdat";
	const wchar_t *kDefaultPlayerName = L"Steve";
	const unsigned int kDefaultSkinId = eDefaultSkins_Skin0;
	const unsigned int kUserDataInfoVersion = 2;

#pragma pack(push, 1)
	struct UserDataInfoFile
	{
		char magic[4];
		unsigned int version;
		char playerName[17];
		unsigned char modelType;
		unsigned char languageId;
		unsigned char reserved[2];
		unsigned int skinId;
	};
#pragma pack(pop)

	bool g_userDataLoaded = false;
	UserData_Info::Data g_userData = { L"Steve", false, MINECRAFT_LANGUAGE_SPANISH, kDefaultSkinId };
	char g_userDataPlayerNameAnsi[17] = "Steve";

	bool IsAllowedPlayerNameChar(wchar_t ch)
	{
		return ((ch >= L'0' && ch <= L'9') ||
				(ch >= L'A' && ch <= L'Z') ||
				(ch >= L'a' && ch <= L'z'));
	}

	std::wstring SanitisePlayerName(const std::wstring &playerName)
	{
		if(playerName.length() < 3 || playerName.length() > 16)
		{
			return kDefaultPlayerName;
		}

		for(size_t i = 0; i < playerName.length(); ++i)
		{
			if(!IsAllowedPlayerNameChar(playerName[i]))
			{
				return kDefaultPlayerName;
			}
		}

		return playerName;
	}

	std::wstring GetUserDataInfoPath()
	{
		wchar_t modulePath[MAX_PATH] = {0};
		if(GetModuleFileNameW(NULL, modulePath, MAX_PATH) == 0)
		{
			return kUserDataInfoFileName;
		}

		wchar_t *lastSlash = wcsrchr(modulePath, L'\\');
		if(lastSlash == NULL)
		{
			lastSlash = wcsrchr(modulePath, L'/');
		}

		if(lastSlash != NULL)
		{
			*(lastSlash + 1) = L'\0';
		}
		else
		{
			modulePath[0] = L'\0';
		}

		return std::wstring(modulePath) + kUserDataInfoFileName;
	}

	void SyncPlayerNameAnsi()
	{
		memset(g_userDataPlayerNameAnsi, 0, sizeof(g_userDataPlayerNameAnsi));

		const size_t copyLength = min((size_t)16, g_userData.playerName.length());
		for(size_t i = 0; i < copyLength; ++i)
		{
			g_userDataPlayerNameAnsi[i] = (char)g_userData.playerName[i];
		}
	}

	bool SkinExists(unsigned int skinId)
	{
		if(skinId == kDefaultSkinId)
		{
			return true;
		}

		wstring skinPath = app.getSkinPathFromId((DWORD)skinId);
		return !skinPath.empty();
	}

	bool ValidateData(bool validateSkin)
	{
		bool changed = false;

		const std::wstring validatedName = SanitisePlayerName(g_userData.playerName);
		if(validatedName != g_userData.playerName)
		{
			g_userData.playerName = validatedName;
			changed = true;
		}

		if(validateSkin && !SkinExists(g_userData.skinId))
		{
			g_userData.skinId = kDefaultSkinId;
			g_userData.modelType = false;
			changed = true;
		}

		SyncPlayerNameAnsi();
		return changed;
	}

	bool ReadBinaryFile(UserDataInfoFile &fileOut)
	{
		const std::wstring path = GetUserDataInfoPath();
		HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		LARGE_INTEGER fileSize = {0};
		if(!GetFileSizeEx(hFile, &fileSize) || fileSize.QuadPart != sizeof(UserDataInfoFile))
		{
			CloseHandle(hFile);
			return false;
		}

		DWORD bytesRead = 0;
		if(!ReadFile(hFile, &fileOut, sizeof(UserDataInfoFile), &bytesRead, NULL))
		{
			CloseHandle(hFile);
			return false;
		}

		CloseHandle(hFile);
		return (bytesRead == sizeof(UserDataInfoFile));
	}

	void ParseBinaryFile(const UserDataInfoFile &fileData)
	{
		UserData_Info::Data parsed = { kDefaultPlayerName, false, kDefaultSkinId };

		if(memcmp(fileData.magic, "UDAT", 4) != 0 || (fileData.version != 1 && fileData.version != kUserDataInfoVersion))
		{
			g_userData = parsed;
			return;
		}

		parsed.playerName.clear();
		for(size_t i = 0; i < 16 && fileData.playerName[i] != '\0'; ++i)
		{
			parsed.playerName.push_back((wchar_t)(unsigned char)fileData.playerName[i]);
		}
		parsed.modelType = (fileData.modelType != 0);
		
		if (fileData.version >= 2)
		{
			parsed.languageId = fileData.languageId;
		}
		else
		{
			parsed.languageId = MINECRAFT_LANGUAGE_SPANISH; // Default for v1 files
		}

		parsed.skinId = fileData.skinId;

		g_userData = parsed;
	}
}

void UserData_Info::EnsureLoaded()
{
	if(g_userDataLoaded)
	{
		return;
	}

	g_userDataLoaded = true;

	UserDataInfoFile fileData;
	if(ReadBinaryFile(fileData))
	{
		ParseBinaryFile(fileData);
	}
	else
	{
		g_userData.playerName = kDefaultPlayerName;
		g_userData.modelType = false;
		g_userData.languageId = MINECRAFT_LANGUAGE_SPANISH;
		g_userData.skinId = kDefaultSkinId;
	}

	if(ValidateData(false))
	{
		Save();
	}
}

void UserData_Info::Save()
{
	EnsureLoaded();
	ValidateData(false);

	UserDataInfoFile fileData;
	memset(&fileData, 0, sizeof(fileData));
	memcpy(fileData.magic, "UDAT", 4);
	fileData.version = kUserDataInfoVersion;
	memcpy(fileData.playerName, g_userDataPlayerNameAnsi, sizeof(fileData.playerName) - 1);
	fileData.modelType = g_userData.modelType ? 1 : 0;
	fileData.languageId = g_userData.languageId;
	fileData.skinId = g_userData.skinId;

	const std::wstring path = GetUserDataInfoPath();

	HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return;
	}

	DWORD bytesWritten = 0;
	WriteFile(hFile, &fileData, sizeof(fileData), &bytesWritten, NULL);
	CloseHandle(hFile);
}

const UserData_Info::Data &UserData_Info::GetData()
{
	EnsureLoaded();
	return g_userData;
}

std::wstring UserData_Info::GetPlayerName()
{
	EnsureLoaded();
	return g_userData.playerName;
}

const char *UserData_Info::GetPlayerNameAnsi()
{
	EnsureLoaded();
	return g_userDataPlayerNameAnsi;
}

bool UserData_Info::GetModelType()
{
	EnsureLoaded();
	return g_userData.modelType;
}

unsigned char UserData_Info::GetLanguageId()
{
	EnsureLoaded();
	return g_userData.languageId;
}

unsigned int UserData_Info::GetSkinId()
{
	EnsureLoaded();
	if(ValidateData(true))
	{
		Save();
	}
	return g_userData.skinId;
}

void UserData_Info::SetPlayerName(const std::wstring &playerName)
{
	EnsureLoaded();
	g_userData.playerName = playerName;
	Save();
}

void UserData_Info::SetModelType(bool modelType)
{
	EnsureLoaded();
	g_userData.modelType = modelType;
	Save();
}

void UserData_Info::SetLanguageId(unsigned char languageId)
{
	EnsureLoaded();
	g_userData.languageId = languageId;
	Save();
}

void UserData_Info::SetSkinId(unsigned int skinId)
{
	EnsureLoaded();
	g_userData.skinId = skinId;
	ValidateData(true);
	Save();
}
