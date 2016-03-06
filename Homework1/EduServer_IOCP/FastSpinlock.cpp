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
	//Question : Timer의 해상도?
	//Sleepfunction의 정밀도를 높이기 위하여?
	for (int nloops = 0; ; nloops++)
	{
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