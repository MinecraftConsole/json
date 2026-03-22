#pragma once

#include "..\Common\Network\NetworkPlayerInterface.h"

class IQNetPlayer;

class NetworkPlayerXbox : public INetworkPlayer
{
public:
	NetworkPlayerXbox(IQNetPlayer *qnetPlayer);

	virtual unsigned char GetSmallId();
	virtual void SendData(INetworkPlayer *player, const void *pvData, int dataSize, bool lowPriority);
	virtual bool IsSameSystem(INetworkPlayer *player);
	virtual int GetSendQueueSizeBytes(INetworkPlayer *player, bool lowPriority);
	virtual int GetSendQueueSizeMessages(INetworkPlayer *player, bool lowPriority);
	virtual int GetCurrentRtt();
	virtual bool IsHost();
	virtual bool IsGuest();
	virtual bool IsLocal();
	virtual int GetSessionIndex();
	virtual bool IsTalking();
	virtual bool IsMutedByLocalUser(int userIndex);
	virtual bool HasVoice();
	virtual bool HasCamera();
	virtual int GetUserIndex();
	virtual void SetSocket(Socket *pSocket);
	virtual Socket *GetSocket();
	virtual const wchar_t *GetOnlineName();
	virtual wstring GetDisplayName();
	virtual PlayerUID GetUID();

	IQNetPlayer *GetQNetPlayer(void);

private:
	IQNetPlayer *m_qnetPlayer;
	Socket *m_pSocket;
};