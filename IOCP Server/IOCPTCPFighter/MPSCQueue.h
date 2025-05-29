#pragma once
template <typename T>
class MPSCQueue
{
public:
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


	MPSCQueue()
	{
		Node* dummyNode = new Node;
		//dummyNode->data = 0;
		dummyNode->next.raw = 0;
		head.part.index = 0;
		tail.part.index = 0;
		head.part.address = reinterpret_cast<UINT_PTR>(dummyNode);
		tail.part.address = reinterpret_cast<UINT_PTR>(dummyNode);

	}

	~MPSCQueue()
	{
		Node* node = reinterpret_cast<Node*>(head.part.address);
		while (node)
		{
			NodePtr nextPtr;
			nextPtr.raw = node->next.raw;
			Node* next = reinterpret_cast<Node*>(nextPtr.part.address);
			delete node;
			node = next;
		}
	}

	void Enqueue(T value)
	{
		Node* newNode = new Node;
		newNode->data = value;
		newNode->next.raw = 0;
		unsigned long long idx = InterlockedIncrement(&index) & 0x1FFFF;
		NodePtr newNodePtr;
		newNodePtr.part.index = idx;
		newNodePtr.part.address = reinterpret_cast<UINT_PTR>(newNode);
		NodePtr oldTailNode;
		Node* oldNode;
		NodePtr tailNextNode;

		while (1)
		{
			oldTailNode.raw = tail.raw;
			oldNode = reinterpret_cast<Node*>(oldTailNode.part.address);
			tailNextNode.raw = oldNode->next.raw;

			if (tailNextNode.part.address == 0)
			{

				if (InterlockedCompareExchange64(&oldNode->next.raw, newNodePtr.raw, tailNextNode.raw)
					== tailNextNode.raw)
				{

					InterlockedCompareExchange64(&tail.raw, newNodePtr.raw, oldTailNode.raw);
					break;
				}
			}
			else
			{
				InterlockedCompareExchange64(&tail.raw, tailNextNode.raw, oldTailNode.raw);
			}
		}
		InterlockedIncrement(&count);
	}

	int Dequeue(T& t)
	{
		NodePtr toBeDeletedNode;
		NodePtr HeadDummyNode;
		Node* delNode;
		Node* delNextNode;
		/*if (InterlockedDecrement(reinterpret_cast<LONG*>(&count)) < 0)
		{
			InterlockedIncrement(reinterpret_cast<LONG*>(&count));
			return -1;
		}*/


		while (true)
		{
			toBeDeletedNode.raw = head.raw;
			delNode = reinterpret_cast<Node*>(toBeDeletedNode.part.address);
			HeadDummyNode.raw = delNode->next.raw;
			delNextNode = reinterpret_cast<Node*>(HeadDummyNode.part.address);

			if (delNextNode == nullptr)
			{	
				return - 1;
			}
			if (InterlockedCompareExchange64(&head.raw, HeadDummyNode.raw, toBeDeletedNode.raw) == toBeDeletedNode.raw)
			{
				t = delNextNode->data;
				delete delNode;
				InterlockedDecrement(&count);
				return 0;
			}
		}
	}
	int GetSize()
	{
		return InterlockedCompareExchange(&count, 0, 0);
	}
private:
	alignas(64) NodePtr head;
	alignas(64) NodePtr tail;
	alignas(64) unsigned int count = 0;
	alignas(64) unsigned long long index = 0;

};