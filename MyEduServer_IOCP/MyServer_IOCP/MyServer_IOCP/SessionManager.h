#pragma once

#include <map>
#include "FastSpinLock.h"
#include <winsock2.h>

class ClientSession;
class FastSpinLock;

class SessionManager
{
public:
	SessionManager( ) : mConnectionCount( 0 )
	{

	}
	~SessionManager( );

	ClientSession* CreateClientSession( SOCKET InStreamSock);
	void	DeleteClientSession(ClientSession* InClientSession );

	int IncreaseConnectionCount( ) { return InterlockedIncrement( &mConnectionCount ); }
	int DecreaseConnectionCount( ) { return InterlockedDecrement( &mConnectionCount ); }
private:
	using ClientList = std::map<SOCKET , ClientSession*>;
	ClientList mClientList;

	//todo : lock ����( �Ŵ������� ���������忡�� �����Ѵ�! , �ݵ�� ������ ����ȭ ���־���Ѵ�)
	FastSpinLock	mLock;
	volatile long mConnectionCount = 0;
};

extern SessionManager* GSessionManager;
