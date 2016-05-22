#pragma once

#include <map>
#include <winsock2.h>

class ClientSession;

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

	//todo : lock 선언( 매니저단은 여러스레드에서 접근한다! , 반드시 락으로 동기화 해주어야한다)
	volatile long mConnectionCount = 0;
};

extern SessionManager* GSessionManager;
