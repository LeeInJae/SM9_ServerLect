#pragma once

class FastSpinlock
{
public:
	FastSpinlock();
	~FastSpinlock();

	//락 에 들어감
	void EnterLock();
	//락 해제
	void LeaveLock();
	
private:
	FastSpinlock(const FastSpinlock& rhs);
	FastSpinlock& operator=(const FastSpinlock& rhs);

	//락여부를 결정지을 값(0이면 획득 가능, 1이면 이미 어떤 스레드에서 자원 사용하고 있음)
	volatile long mLockFlag;
};

//스핀락을 이용하여, 생성자에서부터 락을 걸어준다.
//해당 부분이 끝나고나면, 소멸자에서 락을 해제한다.
class FastSpinlockGuard
{
public:
	FastSpinlockGuard(FastSpinlock& lock) : mLock(lock)
	{
		mLock.EnterLock();
	}

	~FastSpinlockGuard()
	{
		mLock.LeaveLock();
	}

private:
	//생성자로부터 초기화하고 있다.
	FastSpinlock& mLock;
};

template <class TargetClass>
class ClassTypeLock
{
public:
	struct LockGuard
	{
		LockGuard()
		{
			TargetClass::mLock.EnterLock();
		}

		~LockGuard()
		{
			TargetClass::mLock.LeaveLock();
		}

	};

private:
	static FastSpinlock mLock;
	
	//friend struct LockGuard;
};

template <class TargetClass>
FastSpinlock ClassTypeLock<TargetClass>::mLock;