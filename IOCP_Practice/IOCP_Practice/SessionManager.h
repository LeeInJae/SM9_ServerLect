#pragma once
#include <map>
#include "FastSpinLock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager();
	~SessionManager();

	ClientSession* CreateClientSession(SOCKET sock);

	void DeleteClientSession(ClientSession* client);
	
	int IncreaseConnectionCount() { return InterlockedIncrement(&mCurrentConnectionCount); }
	int DecreaseConnectionCount() { return InterlockedDecrement(&mCurrentConnectionCount); }

private:
	using ClientList = std::map<SOCKET, ClientSession*>;
	ClientList mClientList;

	FastSpinLock	mLock;
	volatile long mCurrentConnectionCount;
};

extern SessionManager* GSessionManager;