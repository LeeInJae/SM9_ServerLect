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
		//mLockFlag 가 초기값이 0이다. InterlockedExchange가 반환하는 값은mLockFlag에 저장되어있던 이전 값이다. 즉 이전값이 0이고현재 1로 체인지가 가능하다면 리소스를(락 걸고 들어가버림) 획득할 수 있다.
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