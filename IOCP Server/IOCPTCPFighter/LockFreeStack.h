#pragma once
#include <vector>
#include "TLSMemoryPool.h"
///*


template <typename T>
class LockFreeStack
{
	typedef union
	{
		struct {
			UINT_PTR index : 17;
			UINT_PTR address : 47;
		} part;
		LONG_PTR raw;
	} NodePtr;

	struct alignas(8) Node
	{
		NodePtr next;
		T data;
	};

public:
	LockFreeStack()
	{
		thread_local static int acceptTick = 0;
		if (++acceptTick >= 64)
		{
			acceptTick = 0;
			TLSMemoryPool<Node>::GetInstance().PopReclaimQueue();
		}
		Top.raw = 0;
	};
	void Push(T value)
	{
		thread_local static int tick = 0;
		if (++tick >= 64)
		{
			tick = 0;
			TLSMemoryPool<Node>::GetInstance().PopReclaimQueue();
		}
		Node* newNode = TLSMemoryPool<Node>::GetInstance().Alloc();
		newNode->data = value;
		//newNode->NodePtr.nextAddress.index = InterlockedCompareExchange(&index, 0, 0);
		unsigned long long indexCnt = InterlockedIncrement(&index) & 0x1FFFF;
		//newNode->isDeleted = false;
		NodePtr oldTop;
		NodePtr newTop;
		newTop.part.address = reinterpret_cast<UINT_PTR>(newNode);
		newTop.part.index = indexCnt;
		do {
			oldTop.raw = Top.raw;
			newNode->next.raw = oldTop.raw;
			//printf("push : &Top : %ld, newNode : %ld, top : %ld \n", Top, newNode, top);
		} while (InterlockedCompareExchange64(&Top.raw,
			newTop.raw,
			oldTop.raw)
			!= oldTop.raw
			);

		InterlockedIncrement(&count);
	};
	T Pop()
	{
		NodePtr toBeDeletedTop;
		NodePtr newTop;
		Node* delNode;
		int index;
		T data;
		int oldCount;
		do {
			oldCount = count;
			if (oldCount <= 0)
				return 0;
		} while (InterlockedCompareExchange(&count, oldCount - 1, oldCount) != oldCount);

		do {
			toBeDeletedTop.raw = Top.raw;
			if (toBeDeletedTop.part.address == 0)
			{
				return 0;
			}
			delNode = reinterpret_cast<Node*>(toBeDeletedTop.part.address);
			index = toBeDeletedTop.part.index;
			newTop.raw = delNode->next.raw;
			//printf("pop : &Top : %ld, newNode : %ld, top : %ld \n", Top, node, top);
		} while (InterlockedCompareExchange64(&Top.raw,
			newTop.raw,
			toBeDeletedTop.raw)
			!= toBeDeletedTop.raw
			);
		data = delNode->data;
		//printf("Top : %ld \n", Top);
		//if (InterlockedExchange((unsigned int*) & toBeDeletedTop->isDeleted, 1) == 1)
		//{
		//	__debugbreak();
		//}
		if (index != toBeDeletedTop.part.index)
		{
			__debugbreak();
		}

		TLSMemoryPool<Node>::GetInstance().Free(delNode);
		return data;
	};
	int GetSize() const
	{
		return InterlockedCompareExchange(&count, 0, 0);
	}
private:
	alignas(64) NodePtr Top;
	alignas(64) unsigned int index = 0;
	alignas(64) mutable unsigned int count = 0;
};


//*/