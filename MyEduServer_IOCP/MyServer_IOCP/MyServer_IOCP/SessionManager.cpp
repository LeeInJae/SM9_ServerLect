#include "stdafx.h"
#include "FastSpinLock.h"
#include "ClientSession.h"
#include "SessionManager.h"

SessionManager* GSessionManager = nullptr;


ClientSession* SessionManager::CreateClientSession( SOCKET InStreamSock )
{
	ClientSession* ClientSessionObject = new ClientSession( InStreamSock );
	//todo : lock
	FastSpinLockGuard EnterLock( mLock );
	mClientList.insert(ClientList::value_type( InStreamSock , ClientSessionObject ));

	return ClientSessionObject;
}

void SessionManager::DeleteClientSession( ClientSession* InClientSession )
{
	//todo : lock
	FastSpinLockGuard EnterLock( mLock );
	mClientList.erase( InClientSession->GetStreamSocket( ) );

	delete InClientSession;
	InClientSession = nullptr;
}

SessionManager::~SessionManager( )
{
}
