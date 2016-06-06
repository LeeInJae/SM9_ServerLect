#pragma once



/// 커스텀하게 힙에서 할당 받는 애들은 전부 메모리 정보 붙여주기
__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
struct MemAllocInfo : SLIST_ENTRY
{
	MemAllocInfo(int size) : mAllocSize(size), mExtraInfo(-1)
	{}
	
	long mAllocSize; ///< MemAllocInfo가 포함된 크기
	long mExtraInfo; ///< 기타 추가 정보 (예: 타입 관련 정보 등)

}; ///< total 16 바이트

inline void* AttachMemAllocInfo(MemAllocInfo* header, int size)
{
	//ReplaceNew이용하여, 메모리를 새로 할당하지는 않고 직접 지정한 메모리에 생성자만 호출하여 인스턴스화
	//header가 가리키는 곳(MemAlloInfo + sizeof( ABC...) 만큼의 메모리 )에 먼저 Memory할당 정보를 인스턴스화	
	new (header)MemAllocInfo(size);

	//Memory정보에 해당하는 MemAllocInfo를 할당했으니 이 이후위치를 반환하여 실제 인스턴스화할 메모리 시작위치를 구하기 위해 ++header
	return reinterpret_cast<void*>(++header);
}

inline MemAllocInfo* DetachMemAllocInfo(void* ptr)
{
	//ptr은 실제 Object를 가리킨다.
	//하지만 ptr의 앞에는 header 정보인 MemAllocInfo도 할당되어있으므로 이 헤더 정보를 reset하기 위해 위치값계산을 하고있다.
	//즉 header + object 이므로 object를 가리키는 위치에서 header크기 만큼 빼주면 header를 가리키게된다.
	MemAllocInfo* header = reinterpret_cast<MemAllocInfo*>(ptr);
	--header;
	return header;
}

__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
class SmallSizeMemoryPool
{
public:
	SmallSizeMemoryPool(DWORD allocSize);

	MemAllocInfo* Pop();
	void Push(MemAllocInfo* ptr);
	

private:
	//현재 비어있는 메모리 청크에 관한 정보에 대한 List 의 Header
	SLIST_HEADER mFreeList; ///< 반드시 첫번째 위치

	//몇 Byte를 할당할 수 있는지에 대한 정보
	const DWORD mAllocSize;

	//현재 몇개를 할당했는지에 대한 정보
	//cowspirit : 왜 volatile을 써야할까? 
	volatile long mAllocCount = 0;
};

class MemoryPool
{
public:
	MemoryPool();

	void* Allocate(int size);
	void Deallocate(void* ptr, long extraInfo);

private:
	enum Config
	{
		/// 함부로 바꾸면 안됨. 철저히 계산후 바꿀 것
		MAX_SMALL_POOL_COUNT = 1024/32 + 1024/128 + 2048/256, ///< ~1024까지 32단위, ~2048까지 128단위, ~4096까지 256단위
		MAX_ALLOC_SIZE = 4096
	};

	/// 원하는 크기의 메모리를 가지고 있는 풀에 O(1) access를 위한 테이블
	SmallSizeMemoryPool* mSmallSizeMemoryPoolTable[MAX_ALLOC_SIZE+1];

};

extern MemoryPool* GMemoryPool;


/// 요놈을 상속 받아야만 xnew/xdelete 사용할 수 있게...
struct PooledAllocatable {};


template <class T, class... Args>
T* xnew(Args... arg)
{
	//std::is_convertible을 찾아볼것(두 타입이 같은지 다른지 확인하는 api)
	static_assert(true == std::is_convertible<T, PooledAllocatable>::value, "only allowed when PooledAllocatable");

	//memorypool에서 Allocate하여사용
	void* alloc = GMemoryPool->Allocate(sizeof(T));
	//실제 얻어온 메모리에 할당(생성자에 인자가있으면 해당 인자를 이용하여 생성)
	new (alloc)T(arg...);
	return reinterpret_cast<T*>(alloc);
}

template <class T>
void xdelete(T* object)
{
	//타입체크(PooledAllocatable을 상속받은 것만 메모리풀 사용가능)
	static_assert(true == std::is_convertible<T, PooledAllocatable>::value, "only allowed when PooledAllocatable");

	//object의 소멸자를 호출하여 인스턴스 삭제
	object->~T();
	//메모리풀에 메모리 반환
	GMemoryPool->Deallocate(object, sizeof(T));	
}