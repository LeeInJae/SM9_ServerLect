#include "stdafx.h"
#include "Exception.h"
#include "MemoryPool.h"

MemoryPool* GMemoryPool = nullptr;

//allocSize만큼의 메모리를 할당할 수 있는 메모리풀 정보를 관리하는 Class
SmallSizeMemoryPool::SmallSizeMemoryPool(DWORD allocSize) : mAllocSize(allocSize)
{
	//allocSize가 8Byte보다는 커야함(최소크기)
	CRASH_ASSERT(allocSize > MEMORY_ALLOCATION_ALIGNMENT);

	//SList는 InitializeSListHead로 초기화를 시켜주어야 한다.
	InitializeSListHead(&mFreeList);
}

MemAllocInfo* SmallSizeMemoryPool::Pop()
{
	//새로운 메모리 할당을 원하므로 사용가능한 메모리에 대한 정보를 획득하여야한다.

	//SList는 멀티프로세서에서 프로그래밍할때 동기화를 보장해주는 자료구조
	//InterlockedPopEntrySList는 List의 맨 앞 엘리먼트를 Pop한다
	MemAllocInfo* mem = (MemAllocInfo*)InterlockedPopEntrySList(&mFreeList);

	//현재 사용할 수 있는 메모리가 없는경우, 새로 할당한다.
	if (NULL == mem)
	{
		//주의해야할 점은 SList를 사용할경우 엘리먼트는 만드시 _alighned_malloc을 활용하여 MEMORY_ALLOCATION_ALIGNMENT를 맞춰주어야한다.
		mem = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(mAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
	}
	else
	{
		//정보가 있는데 allocsize가 할당이 안되어있으면 에러 처리.
		CRASH_ASSERT(mem->mAllocSize == 0);
	}

	//할당 카운트를 증가시킨다.
	InterlockedIncrement(&mAllocCount);
	return mem;
}

void SmallSizeMemoryPool::Push(MemAllocInfo* ptr)
{
	//메모리가 다 쓰고 반환되면 다서 List에 추가해준다.

	//mFreeList->할당가능한 곳( 헤더1 )->할당가능한 곳( 헤더2 )
	//즉 이전에 생성되었던 것들은 메모리 해제를 하지 않고, 재사용할 수 있도록 List에 다시 삽입
	//Question:
	//이렇게하면 즉 생성할때는 어쨋든 새로 생성을 하므로, 100개를 생성하면 100개는 여기저기에 생성될 가능성이 있으므로 단편화의 위험성이 있지않나?
	InterlockedPushEntrySList(&mFreeList, (PSLIST_ENTRY)ptr);
	InterlockedDecrement(&mAllocCount);
}

/////////////////////////////////////////////////////////////////////

MemoryPool::MemoryPool()
{
	memset(mSmallSizeMemoryPoolTable, 0, sizeof(mSmallSizeMemoryPoolTable));

	int recent = 0;

	//32byte 단위로 메모리 풀 먼저 생성 
	//즉 1~32Byte까지는 AllocationSize가 32인 SmallSizeMemoryPool정보를 이용
	//33~64Byte까지는 AllocationSize가 64인 SmallSizeMemoryPool정보를 이용
	//1~1024 까지 32Byte 단위로 메모리풀을 생성
	for (int i = 32; i < 1024; i+=32)
	{
		//i번쨰 byte를 할당할 수 있는 MemoryPool 생성
		SmallSizeMemoryPool* pool = new SmallSizeMemoryPool(i);
		for (int j = recent+1; j <= i; ++j)
		{
			mSmallSizeMemoryPoolTable[j] = pool;
		}
		recent = i;
	}

	//위와 동일하지만 128Byte 단위로 생성
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
	//우리가 Custom하게 Allocate를 하므로 해당 메모리에 어떤 Instance를 할당했는지에 대한 정보가 필요하다.
	//즉 우리는 그 정보를 MemAllocInfo에 담아 활용하고 있다.
	//따라서 우리가 A라는 클래스를 메모리에 할당하기위해서는 sizeof(A) 이외에 sizeof(MemAllocInfo)만큼의 추가 메모리 공간이 필요하다.
	// 메모리정보 + 실제할당되는인스턴스정보
	MemAllocInfo* header = nullptr;
	
	//실제 할당되어야할 메모리 사이즈 계산
	int realAllocSize = size + sizeof(MemAllocInfo);

	//메모리풀을 활용할 수 있는 최대 사이즈를 넘는다면 직접할당
	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		header = reinterpret_cast<MemAllocInfo*>(_aligned_malloc(realAllocSize, MEMORY_ALLOCATION_ALIGNMENT));
	}
	else
	{
		//메모리풀을 활용할 수 있다면 커스텀하게 할당해놓은 메모리 풀을 이용.
		header = mSmallSizeMemoryPoolTable[realAllocSize]->Pop();
	}

	//실제 메모리 할당하러가자
	//할당하는 메모리 앞에는 해당 메모리에 무엇이 할당되었는지에 대한 정보를 붙여주자
	return AttachMemAllocInfo(header, realAllocSize);
}

void MemoryPool::Deallocate(void* ptr, long extraInfo)
{
	//Custom하게 메모리 해제.
	//해제하려는 메모리의 헤더정보를 얻어옴
	MemAllocInfo* header = DetachMemAllocInfo(ptr);
	header->mExtraInfo = extraInfo; ///< 최근 할당에 관련된 정보 힌트
	
	//mAllocSize를 0으로 만든다.
	//해제하기 이전 사이즈가 0이면 중복제거의 위험이 있다.
	long realAllocSize = InterlockedExchange(&header->mAllocSize, 0); ///< 두번 해제 체크 위해
	
	//이미 size가 0인것을 해제하려고 하는 것은 중복 해제를 한다는 의미이므로 에러처리
	CRASH_ASSERT(realAllocSize> 0);

	//만약 최대값보다 더큰 메모리를 할당했을 경우 직접해제
	if (realAllocSize > MAX_ALLOC_SIZE)
	{
		_aligned_free(header);
	}
	else
	{
		//SmallSizeMemoryPool에서 할당한 것이라면, 다시 메모리 반환해줌
		mSmallSizeMemoryPoolTable[realAllocSize]->Push(header);
	}
}