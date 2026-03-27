#pragma once

#include <string>

namespace UserData_Info
{
	struct Data
	{
		std::wstring playerName;
		bool modelType;
		unsigned char languageId;
		unsigned int skinId;
	};

	void EnsureLoaded();
	void Save();

	const Data &GetData();
	std::wstring GetPlayerName();
	const char *GetPlayerNameAnsi();
	bool GetModelType();
	unsigned char GetLanguageId();
	unsigned int GetSkinId();

	void SetPlayerName(const std::wstring &playerName);
	void SetModelType(bool modelType);
	void SetLanguageId(unsigned char languageId);
	void SetSkinId(unsigned int skinId);
}
