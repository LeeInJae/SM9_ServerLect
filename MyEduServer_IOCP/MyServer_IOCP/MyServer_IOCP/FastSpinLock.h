#pragma once
#pragma comment(lib, "winmm.lib")

class FastSpinLock
{
public:
	FastSpinLock( );
	~FastSpinLock( );

	void EnterLock( );
	void LeaveLock( );
private:
	FastSpinLock( const FastSpinLock& rhs );
	FastSpinLock& operator=( const FastSpinLock& rhs );

	volatile long mLockFlag;
};

class FastSpinLockGuard
{
public:
	FastSpinLockGuard( FastSpinLock& Lock) : mLock(Lock)
	{
		mLock.EnterLock( );
	}

	~FastSpinLockGuard( )
	{
		mLock.LeaveLock( );
	}

private:
	FastSpinLock& mLock;
};
