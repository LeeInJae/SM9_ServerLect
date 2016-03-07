#include "stdafx.h"
#include "FastSpinlock.h"


FastSpinlock::FastSpinlock() : mLockFlag(0)
{
}


FastSpinlock::~FastSpinlock()
{
}


void FastSpinlock::EnterLock()
{
	//Question : Timer�� �ػ�?
	//Sleepfunction�� ���е��� ���̱� ���Ͽ�?
	for (int nloops = 0; ; nloops++)
	{
		//mLockFlag �� �ʱⰪ�� 0�̴�. InterlockedExchange�� ��ȯ�ϴ� ����mLockFlag�� ����Ǿ��ִ� ���� ���̴�. �� �������� 0�̰����� 1�� ü������ �����ϴٸ� ���ҽ���(�� �ɰ� ������) ȹ���� �� �ִ�.
		if ( InterlockedExchange(&mLockFlag, 1) == 0 )
			return;
	
		UINT uTimerRes = 1;
		timeBeginPeriod(uTimerRes); 
		Sleep((DWORD)min(10, nloops));
		timeEndPeriod(uTimerRes);
	}

}

void FastSpinlock::LeaveLock()
{
	InterlockedExchange(&mLockFlag, 0);
}