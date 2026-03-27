#include "stdafx.h"
#include "ShutdownManager.h"
#include "NetworkPlayerXbox.h"

void ShutdownManager::HasStarted(EThreadId threadId)
{
	(void)threadId;
}

void ShutdownManager::HasStarted(EThreadId threadId, C4JThread::EventArray *startEvent)
{
	(void)threadId;
	(void)startEvent;
}

bool ShutdownManager::ShouldRun(EThreadId threadId)
{
	(void)threadId;
	return true;
}

void ShutdownManager::HasFinished(EThreadId threadId)
{
	(void)threadId;
}

NetworkPlayerXbox::NetworkPlayerXbox(IQNetPlayer *qnetPlayer)
{
	m_qnetPlayer = qnetPlayer;
	m_pSocket = NULL;
}

unsigned char NetworkPlayerXbox::GetSmallId()
{
	return 0;
}

void NetworkPlayerXbox::SendData(INetworkPlayer *player, const void *pvData, int dataSize, bool lowPriority)
{
	(void)player;
	(void)pvData;
	(void)dataSize;
	(void)lowPriority;
}

bool NetworkPlayerXbox::IsSameSystem(INetworkPlayer *player)
{
	(void)player;
	return false;
}

int NetworkPlayerXbox::GetSendQueueSizeBytes(INetworkPlayer *player, bool lowPriority)
{
	(void)player;
	(void)lowPriority;
	return 0;
}

int NetworkPlayerXbox::GetSendQueueSizeMessages(INetworkPlayer *player, bool lowPriority)
{
	(void)player;
	(void)lowPriority;
	return 0;
}

int NetworkPlayerXbox::GetCurrentRtt()
{
	return 0;
}

bool NetworkPlayerXbox::IsHost()
{
	return false;
}

bool NetworkPlayerXbox::IsGuest()
{
	return false;
}

bool NetworkPlayerXbox::IsLocal()
{
	return false;
}

int NetworkPlayerXbox::GetSessionIndex()
{
	return -1;
}

bool NetworkPlayerXbox::IsTalking()
{
	return false;
}

bool NetworkPlayerXbox::IsMutedByLocalUser(int userIndex)
{
	(void)userIndex;
	return false;
}

bool NetworkPlayerXbox::HasVoice()
{
	return false;
}

bool NetworkPlayerXbox::HasCamera()
{
	return false;
}

int NetworkPlayerXbox::GetUserIndex()
{
	return -1;
}

void NetworkPlayerXbox::SetSocket(Socket *pSocket)
{
	m_pSocket = pSocket;
}

Socket *NetworkPlayerXbox::GetSocket()
{
	return m_pSocket;
}

const wchar_t *NetworkPlayerXbox::GetOnlineName()
{
	return L"";
}

wstring NetworkPlayerXbox::GetDisplayName()
{
	return L"";
}

PlayerUID NetworkPlayerXbox::GetUID()
{
	return INVALID_XUID;
}

IQNetPlayer *NetworkPlayerXbox::GetQNetPlayer(void)
{
	return m_qnetPlayer;
}
