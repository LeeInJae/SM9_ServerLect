#pragma once

class FastSpinlock
{
public:
	FastSpinlock();
	~FastSpinlock();

	//�� �� ��
	void EnterLock();
	//�� ����
	void LeaveLock();
	
private:
	FastSpinlock(const FastSpinlock& rhs);
	FastSpinlock& operator=(const FastSpinlock& rhs);

	//�����θ� �������� ��(0�̸� ȹ�� ����, 1�̸� �̹� � �����忡�� �ڿ� ����ϰ� ����)
	volatile long mLockFlag;
};

//���ɶ��� �̿��Ͽ�, �����ڿ������� ���� �ɾ��ش�.
//�ش� �κ��� ��������, �Ҹ��ڿ��� ���� �����Ѵ�.
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
	//�����ڷκ��� �ʱ�ȭ�ϰ� �ִ�.
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