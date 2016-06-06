#include "stdafx.h"
#include "FastSpinlock.h"
#include "MemoryPool.h"
#include "ThreadLocal.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"


SessionManager* GSessionManager = nullptr;

//mCurrentIssueCount (현재 세션 연결이 된 녀석의 카운트)
//mCurrentReturnCount(현재 세션 연결이 안된 녀석의 카운트)

SessionManager::SessionManager() : mCurrentIssueCount(0), mCurrentReturnCount(0), mLock(LO_FIRST_CLASS)
{
}

SessionManager::~SessionManager()
{
	//Session정보 싹 정리(해제)
	for (auto it : mFreeSessionList)
	{
		xdelete(it);
	}
}

void SessionManager::PrepareSessions()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);


	//최대 커넥션(연결) 갯수만큼 미리 ClientSession을 만들어놓음
	for (int i = 0; i < MAX_CONNECTION; ++i)
	{
		ClientSession* client = xnew<ClientSession>();
		
		//List로 세션 관리.
		mFreeSessionList.push_back(client);
	}
}





void SessionManager::ReturnClientSession(ClientSession* client)
{
	FastSpinlockGuard guard(mLock);

	CRASH_ASSERT(client->mConnected == 0 && client->mRefCount == 0);

	//client 정보 리셋
	client->SessionReset();

	//이용가능한 clientSession으로 반환
	mFreeSessionList.push_back(client);

	++mCurrentReturnCount;
}

bool SessionManager::AcceptSessions()
{
	FastSpinlockGuard guard(mLock);

	//MAX_CONNECTION(최대 접속 클라이언트 수만큼 세션 할당)
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