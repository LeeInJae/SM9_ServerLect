#pragma once



/// Ŀ�����ϰ� ������ �Ҵ� �޴� �ֵ��� ���� �޸� ���� �ٿ��ֱ�
__declspec(align(MEMORY_ALLOCATION_ALIGNMENT))
struct MemAllocInfo : SLIST_ENTRY
{
	MemAllocInfo(int size) : mAllocSize(size), mExtraInfo(-1)
	{}
	
	long mAllocSize; ///< MemAllocInfo�� ���Ե� ũ��
	long mExtraInfo; ///< ��Ÿ �߰� ���� (��: Ÿ�� ���� ���� ��)

}; ///< total 16 ����Ʈ

inline void* AttachMemAllocInfo(MemAllocInfo* header, int size)
{
	//ReplaceNew�̿��Ͽ�, �޸𸮸� ���� �Ҵ������� �ʰ� ���� ������ �޸𸮿� �����ڸ� ȣ���Ͽ� �ν��Ͻ�ȭ
	//header�� ����Ű�� ��(MemAlloInfo + sizeof( ABC...) ��ŭ�� �޸� )�� ���� Memory�Ҵ� ������ �ν��Ͻ�ȭ	
	new (header)MemAllocInfo(size);

	//Memory������ �ش��ϴ� MemAllocInfo�� �Ҵ������� �� ������ġ�� ��ȯ�Ͽ� ���� �ν��Ͻ�ȭ�� �޸� ������ġ�� ���ϱ� ���� ++header
	return reinterpret_cast<void*>(++header);
}

inline MemAllocInfo* DetachMemAllocInfo(void* ptr)
{
	//ptr�� ���� Object�� ����Ų��.
	//������ ptr�� �տ��� header ������ MemAllocInfo�� �Ҵ�Ǿ������Ƿ� �� ��� ������ reset�ϱ� ���� ��ġ������� �ϰ��ִ�.
	//�� header + object �̹Ƿ� object�� ����Ű�� ��ġ���� headerũ�� ��ŭ ���ָ� header�� ����Ű�Եȴ�.
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
	//���� ����ִ� �޸� ûũ�� ���� ������ ���� List �� Header
	SLIST_HEADER mFreeList; ///< �ݵ�� ù��° ��ġ

	//�� Byte�� �Ҵ��� �� �ִ����� ���� ����
	const DWORD mAllocSize;

	//���� ��� �Ҵ��ߴ����� ���� ����
	//cowspirit : �� volatile�� ����ұ�? 
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
		/// �Ժη� �ٲٸ� �ȵ�. ö���� ����� �ٲ� ��
		MAX_SMALL_POOL_COUNT = 1024/32 + 1024/128 + 2048/256, ///< ~1024���� 32����, ~2048���� 128����, ~4096���� 256����
		MAX_ALLOC_SIZE = 4096
	};

	/// ���ϴ� ũ���� �޸𸮸� ������ �ִ� Ǯ�� O(1) access�� ���� ���̺�
	SmallSizeMemoryPool* mSmallSizeMemoryPoolTable[MAX_ALLOC_SIZE+1];

};

extern MemoryPool* GMemoryPool;


/// ����� ��� �޾ƾ߸� xnew/xdelete ����� �� �ְ�...
struct PooledAllocatable {};


template <class T, class... Args>
T* xnew(Args... arg)
{
	//std::is_convertible�� ã�ƺ���(�� Ÿ���� ������ �ٸ��� Ȯ���ϴ� api)
	static_assert(true == std::is_convertible<T, PooledAllocatable>::value, "only allowed when PooledAllocatable");

	//memorypool���� Allocate�Ͽ����
	void* alloc = GMemoryPool->Allocate(sizeof(T));
	//���� ���� �޸𸮿� �Ҵ�(�����ڿ� ���ڰ������� �ش� ���ڸ� �̿��Ͽ� ����)
	new (alloc)T(arg...);
	return reinterpret_cast<T*>(alloc);
}

template <class T>
void xdelete(T* object)
{
	//Ÿ��üũ(PooledAllocatable�� ��ӹ��� �͸� �޸�Ǯ ��밡��)
	static_assert(true == std::is_convertible<T, PooledAllocatable>::value, "only allowed when PooledAllocatable");

	//object�� �Ҹ��ڸ� ȣ���Ͽ� �ν��Ͻ� ����
	object->~T();
	//�޸�Ǯ�� �޸� ��ȯ
	GMemoryPool->Deallocate(object, sizeof(T));	
}