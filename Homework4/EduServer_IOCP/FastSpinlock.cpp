#include "stdafx.h"
#include "Exception.h"
#include "FastSpinlock.h"
#include "LockOrderChecker.h"
#include "ThreadLocal.h"

FastSpinlock::FastSpinlock(const int lockOrder) : mLockFlag(0), mLockOrder(lockOrder)
{
}


FastSpinlock::~FastSpinlock()
{
}


void FastSpinlock::EnterWriteLock()
{
	/// 락 순서 신경 안써도 되는 경우는 그냥 패스
	if ( mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Push(this);

	while (true)
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();
		
		//다른 스레드에서 쓰기락 걸고 있는거냥? 
		if ((InterlockedAdd(&mLockFlag, LF_WRITE_FLAG) & LF_WRITE_MASK) == LF_WRITE_FLAG)
		{
			/// 다른놈이 readlock 풀어줄때까지 기다린다.
			while (mLockFlag & LF_READ_MASK)
				YieldProcessor();

			return;
		}

		InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);
	}

}

void FastSpinlock::LeaveWriteLock()
{
	InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);

	/// 락 순서 신경 안써도 되는 경우는 그냥 패스
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}

void FastSpinlock::EnterReadLock()
{
	//cow : 얘네는 ThreadLocal Storage니까 Thread별로 어떤 락을 요구할때 항상 요놈 lock order check를 통해서 한다.
	//어쩃든 Thread별로 요구하는 Lock을 요구를 할테니깐말이야.. 여러개의 락을 가질때 hieracy를 잘 지켜야겠지/
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Push(this);

	while (true)
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();

		//TODO: Readlock 진입 구현 (mLockFlag를 어떻게 처리하면 되는지?)
		// if ( readlock을 얻으면 )
			//return;
		// else
			// mLockFlag 원복

		//cow
		//{
		if ( InterlockedIncrement( &mLockFlag) & LF_WRITE_MASK  == 0)
			return;
		else
			mLockFlag = InterlockedDecrement( &mLockFlag );
		//}	
	}
}

void FastSpinlock::LeaveReadLock()
{
	//TODO: mLockFlag 처리 
	//cow
	//{
	InterlockedDecrement( &mLockFlag );
	//}
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}