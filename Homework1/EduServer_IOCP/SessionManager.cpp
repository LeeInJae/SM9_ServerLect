#include "stdafx.h"
#include "ClientSession.h"
#include "SessionManager.h"

SessionManager* GSessionManager = nullptr;

ClientSession* SessionManager::CreateClientSession(SOCKET sock)
{
	ClientSession* client = new ClientSession(sock);

	//TODO: lock으로 보호할 것
	//EnterCriticalSection( &mLock );
	FastSpinlockGuard EnterLock( mLock );
	{
		mClientList.insert(ClientList::value_type(sock, client));
	}
	//LeaveCriticalSection( &mLock );

	return client;
}


void SessionManager::DeleteClientSession(ClientSession* client)
{
	//TODO: lock으로 보호할 것
	//세션매니져는 싱글톤으로 되어있고, 여러 스레드에서 접근이 가능하므로 락이 필요하다.
	FastSpinlockGuard EnterLock( mLock );
	{
		mClientList.erase(client->mSocket);
	}

	//세션제거
	delete client;
}