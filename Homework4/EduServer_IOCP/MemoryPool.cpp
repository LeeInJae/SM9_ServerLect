#include "stdafx.h"
#include "Exception.h"
#include "MemoryPool.h"

MemoryPool* GMemoryPool = nullptr;

//allocSize��ŭ�� �޸𸮸� �Ҵ��� �� �ִ� �޸�Ǯ ������ �����ϴ� Class
SmallSizeMemoryPool::SmallSizeMemoryPool(DWORD allocSize) : mAllocSize(allocSize)
{
	//allocSize�� 8Byte���ٴ� Ŀ����(�ּ�ũ��)
	CRASH_ASSERT(allocSize > MEMORY_ALLOCATION_ALIGNMENT);

	//SList�� InitializeSListHead�� �ʱ�ȭ�� �����־�� �Ѵ�.
	InitializeSListHead(&mFreeList);
}

MemAllocInfo* SmallSizeMemoryPool::Pop()
{
	//���ο� �޸� �Ҵ��� ���ϹǷ� ��밡���� �޸𸮿� ���� ������ ȹ���Ͽ����Ѵ�.

	//SList�� ��Ƽ���μ������� ���α׷����Ҷ� ����ȭ�� �������ִ� �ڷᱸ��
	//InterlockedPopEntrySList�� List�� �� �� ������Ʈ�� Pop�Ѵ�
	MemAllocInfo* mem = (MemAllocInfo*)InterlockedPopEntrySList(&mFreeList);

	//���� ����� �� �ִ� �޸𸮰� ���°��, ���� �Ҵ��Ѵ�.
	if (NULL == mem)
	{
		//�����ؾ��� ���� SList�� ����Ұ�� ������Ʈ�� ����� _alighned_malloc�� Ȱ���Ͽ� MEMORY_ALLOCATION_ALIGNMENT�� �����־���Ѵ�.
		mem = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(mAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
	}
	else
	{
		//������ �ִµ� allocsize�� �Ҵ��� �ȵǾ������� ���� ó��.
		CRASH_ASSERT(mem->mAllocSize == 0);
	}

	//�Ҵ� ī��Ʈ�� ������Ų��.
	InterlockedIncrement(&mAllocCount);
	return mem;
}

void SmallSizeMemoryPool::Push(MemAllocInfo* ptr)
{
	//�޸𸮰� �� ���� ��ȯ�Ǹ� �ټ� List�� �߰����ش�.

	//mFreeList->�Ҵ簡���� ��( ���1 )->�Ҵ簡���� ��( ���2 )
	//�� ������ �����Ǿ��� �͵��� �޸� ������ ���� �ʰ�, ������ �� �ֵ��� List�� �ٽ� ����
	//Question:
	//�̷����ϸ� �� �����Ҷ��� ��¶�� ���� ������ �ϹǷ�, 100���� �����ϸ� 100���� �������⿡ ������ ���ɼ��� �����Ƿ� ����ȭ�� ���輺�� �����ʳ�?
	InterlockedPushEntrySList(&mFreeList, (PSLIST_ENTRY)ptr);
	InterlockedDecrement(&mAllocCount);
}

/////////////////////////////////////////////////////////////////////

MemoryPool::MemoryPool()
{
	memset(mSmallSizeMemoryPoolTable, 0, sizeof(mSmallSizeMemoryPoolTable));

	int recent = 0;

	//32byte ������ �޸� Ǯ ���� ���� 
	//�� 1~32Byte������ AllocationSize�� 32�� SmallSizeMemoryPool������ �̿�
	//33~64Byte������ AllocationSize�� 64�� SmallSizeMemoryPool������ �̿�
	//1~1024 ���� 32Byte ������ �޸�Ǯ�� ����
	for (int i = 32; i < 1024; i+=32)
	{
		//i���� byte�� �Ҵ��� �� �ִ� MemoryPool ����
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		for (int j = recent+1; j <= i; ++j)
		{
			mSmallSizeMemoryPoolTable[j] = pool;
		}
		recent = i;
	}

	//���� ���������� 128Byte ������ ����
	for (int i = 1024; i < 2048; i += 128)
	{
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		for (int j = recent + 1; j <= i; ++j)
		{
			mSmallSizeMemoryPoolTable[j] = pool;
		}
		recent = i;
	}

	for (int i = 2048; i <= 4096; i += 256)
	{
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		for (int j = recent + 1; j <= i; ++j)
		{
			mSmallSizeMemoryPoolTable[j] = pool;
		}
		recent = i;
	}

}

void* MemoryPool::Allocate(int size)
{
	//�츮�� Custom�ϰ� Allocate�� �ϹǷ� �ش� �޸𸮿� � Instance�� �Ҵ��ߴ����� ���� ������ �ʿ��ϴ�.
	//�� �츮�� �� ������ MemAllocInfo�� ��� Ȱ���ϰ� �ִ�.
	//���� �츮�� A��� Ŭ������ �޸𸮿� �Ҵ��ϱ����ؼ��� sizeof(A) �̿ܿ� sizeof(MemAllocInfo)��ŭ�� �߰� �޸� ������ �ʿ��ϴ�.
	// �޸����� + �����Ҵ�Ǵ��ν��Ͻ�����
	MemAllocInfo* header = nullptr;
	
	//���� �Ҵ�Ǿ���� �޸� ������ ���
	int realAllocSize = size + sizeof(MemAllocInfo);

	//�޸�Ǯ�� Ȱ���� �� �ִ� �ִ� ����� �Ѵ´ٸ� �����Ҵ�
	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		header = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(realAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
	}
	else
	{
		//�޸�Ǯ�� Ȱ���� �� �ִٸ� Ŀ�����ϰ� �Ҵ��س��� �޸� Ǯ�� �̿�.
		header = mSmallSizeMemoryPoolTable[realAllocSize]->Pop();
	}

	//���� �޸� �Ҵ��Ϸ�����
	//�Ҵ��ϴ� �޸� �տ��� �ش� �޸𸮿� ������ �Ҵ�Ǿ������� ���� ������ �ٿ�����
	return AttachMemAllocInfo(header, realAllocSize);
}

void MemoryPool::Deallocate(void* ptr, long extraInfo)
{
	//Custom�ϰ� �޸� ����.
	//�����Ϸ��� �޸��� ��������� ����
	MemAllocInfo* header = DetachMemAllocInfo(ptr);
	header->mExtraInfo = extraInfo; ///< �ֱ� �Ҵ翡 ���õ� ���� ��Ʈ
	
	//mAllocSize�� 0���� �����.
	//�����ϱ� ���� ����� 0�̸� �ߺ������� ������ �ִ�.
	long realAllocSize = InterlockedExchange(&header->mAllocSize, 0); ///< �ι� ���� üũ ����
	
	//�̹� size�� 0�ΰ��� �����Ϸ��� �ϴ� ���� �ߺ� ������ �Ѵٴ� �ǹ��̹Ƿ� ����ó��
	CRASH_ASSERT(realAllocSize> 0);

	//���� �ִ밪���� ��ū �޸𸮸� �Ҵ����� ��� ��������
	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		_aligned_free(header);
	}
	else
	{
		//SmallSizeMemoryPool���� �Ҵ��� ���̶��, �ٽ� �޸� ��ȯ����
		mSmallSizeMemoryPoolTable[realAllocSize]->Push(header);
	}
}