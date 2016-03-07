#include "stdafx.h"
#include "ClientSession.h"
#include "SessionManager.h"

SessionManager* GSessionManager = nullptr;

ClientSession* SessionManager::CreateClientSession(SOCKET sock)
{
	ClientSession* client = new ClientSession(sock);

	//TODO: lock���� ��ȣ�� ��
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
	//TODO: lock���� ��ȣ�� ��
	//���ǸŴ����� �̱������� �Ǿ��ְ�, ���� �����忡�� ������ �����ϹǷ� ���� �ʿ��ϴ�.
	FastSpinlockGuard EnterLock( mLock );
	{
		mClientList.erase(client->mSocket);
	}

	//��������
	delete client;
}