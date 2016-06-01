#include "stdafx.h"
#include "ClientSession.h"
#include "SessionManager.h"

SessionManager* GSessionManager = nullptr;
SessionManager::SessionManager() : mCurrentConnectionCount(0)
{ 
}


SessionManager::~SessionManager()
{
}
ClientSession* SessionManager::CreateClientSession(SOCKET sock)
{
	ClientSession* client = new ClientSession(sock);

	FastSpinLockGuard CustomSpinLock(mLock);
	{
		mClientList.insert(ClientList::value_type(sock, client));
	}

	return client;

}

void SessionManager::DeleteClientSession(ClientSession* client)
{

}