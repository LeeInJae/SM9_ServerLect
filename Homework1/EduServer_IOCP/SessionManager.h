#pragma once
#include <map>
#include <WinSock2.h>
#include "FastSpinlock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager( ) : mCurrentConnectionCount( 0 )	{ //InitializeCriticalSection( &mLock ); 
	}
	~SessionManager( ) { //DeleteCriticalSection( &mLock ); 
	}

	ClientSession* CreateClientSession(SOCKET sock);

	void DeleteClientSession(ClientSession* client);

	int IncreaseConnectionCount() { return InterlockedIncrement(&mCurrentConnectionCount); }
	int DecreaseConnectionCount() { return InterlockedDecrement(&mCurrentConnectionCount); }


private:
	typedef std::map<SOCKET, ClientSession*> ClientList;
	ClientList	mClientList;

	//TODO: mLock; ����
	//CRITICAL_SECTION mLock;
	FastSpinlock	mLock;
	volatile long mCurrentConnectionCount;
};

extern SessionManager* GSessionManager;
