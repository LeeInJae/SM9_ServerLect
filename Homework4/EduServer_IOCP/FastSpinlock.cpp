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
	/// �� ���� �Ű� �Ƚᵵ �Ǵ� ���� �׳� �н�
	if ( mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Push(this);

	while (true)
	{
		/// �ٸ����� writelock Ǯ���ٶ����� ��ٸ���.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();
		
		//�ٸ� �����忡�� ����� �ɰ� �ִ°ų�? 
		if ((InterlockedAdd(&mLockFlag, LF_WRITE_FLAG) & LF_WRITE_MASK) == LF_WRITE_FLAG)
		{
			/// �ٸ����� readlock Ǯ���ٶ����� ��ٸ���.
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

	/// �� ���� �Ű� �Ƚᵵ �Ǵ� ���� �׳� �н�
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}

void FastSpinlock::EnterReadLock()
{
	//cow : ��״� ThreadLocal Storage�ϱ� Thread���� � ���� �䱸�Ҷ� �׻� ��� lock order check�� ���ؼ� �Ѵ�.
	//��� Thread���� �䱸�ϴ� Lock�� �䱸�� ���״ϱ��̾�.. �������� ���� ������ hieracy�� �� ���Ѿ߰���/
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Push(this);

	while (true)
	{
		/// �ٸ����� writelock Ǯ���ٶ����� ��ٸ���.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();

		//TODO: Readlock ���� ���� (mLockFlag�� ��� ó���ϸ� �Ǵ���?)
		// if ( readlock�� ������ )
			//return;
		// else
			// mLockFlag ����

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
	//TODO: mLockFlag ó�� 
	//cow
	//{
	InterlockedDecrement( &mLockFlag );
	//}
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}