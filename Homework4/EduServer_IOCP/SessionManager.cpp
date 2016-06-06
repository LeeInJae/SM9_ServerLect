#include "stdafx.h"
#include "FastSpinlock.h"
#include "MemoryPool.h"
#include "ThreadLocal.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"


SessionManager* GSessionManager = nullptr;

//mCurrentIssueCount (���� ���� ������ �� �༮�� ī��Ʈ)
//mCurrentReturnCount(���� ���� ������ �ȵ� �༮�� ī��Ʈ)

SessionManager::SessionManager() : mCurrentIssueCount(0), mCurrentReturnCount(0), mLock(LO_FIRST_CLASS)
{
}

SessionManager::~SessionManager()
{
	//Session���� �� ����(����)
	for (auto it : mFreeSessionList)
	{
		xdelete(it);
	}
}

void SessionManager::PrepareSessions()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);


	//�ִ� Ŀ�ؼ�(����) ������ŭ �̸� ClientSession�� ��������
	for (int i = 0; i < MAX_CONNECTION; ++i)
	{
		ClientSession* client = xnew<ClientSession>();
		
		//List�� ���� ����.
		mFreeSessionList.push_back(client);
	}
}





void SessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);

	CRASH_ASSERT(client->mConnected == 0 && client->mRefCount == 0);

	//client ���� ����
	client->SessionReset();

	//�̿밡���� clientSession���� ��ȯ
	mFreeSessionList.push_back(client);

	++mCurrentReturnCount;
}

bool SessionManager::AcceptSessions()
{
	FastSpinlockGuard guard(mLock);

	//MAX_CONNECTION(�ִ� ���� Ŭ���̾�Ʈ ����ŭ ���� �Ҵ�)
	while (mCurrentIssueCount - mCurrentReturnCount < MAX_CONNECTION)
	{
		ClientSession* newClient = mFreeSessionList.back();
		mFreeSessionList.pop_back();

		++mCurrentIssueCount;

		newClient->AddRef(); ///< refcount +1 for issuing 
		
		if (false == newClient->PostAccept())
			return false;
	}

	return true;
}