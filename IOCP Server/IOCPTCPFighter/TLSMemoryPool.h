#pragma once
#include <iostream>
#include <vector>
#include "MPSCQueue.h"
static int globalTemplateCnt = 1;

// ���� ��, �޸� Ǯ�� ��� (alloc�Ǿ��ִ�) �޸� �����ǵ��� ����
template <typename T>
class TLSMemoryPool
{
public:
#pragma pack(push, 1)
	struct Node
	{
		int frontSafe;
		char data[sizeof(T)];
		Node* next;
		TLSMemoryPool<T>* ownerPool;
		int nodeGeneration;
		int rearSafe;
		int uniqueId;
		bool isFree;
		Node(int front, int rear, int id)
		{
			
			frontSafe = front;
			next = NULL;
			rearSafe = rear;
			uniqueId = id;
			isFree = false;
			nodeGeneration = 0;

		}
	};
#pragma pack(pop)
	static TLSMemoryPool& GetInstance()
	{
		thread_local static TLSMemoryPool objPool;
		
		return objPool;
	}
	TLSMemoryPool(const TLSMemoryPool&) = delete;
	TLSMemoryPool& operator=(const TLSMemoryPool&) = delete;




	T* Alloc()
	{
		//printf("size : %d \n", sizeof(T));
		if (_node == NULL)
		{
			total++;
			Node* n = new Node(this->frontSafe, this->rearSafe, this->uniqueId);
			if (this->toUseAddressList)
			{
				this->addressList.push_back(reinterpret_cast<uintptr_t>(n));
			}
			if (this->initConstructionFlag)
			{
				new (&(n->data)) T;
			}
			//InterlockedIncrement(&allocCount);
			n->next = nullptr;
			n->ownerPool = this;
			return reinterpret_cast<T*> (reinterpret_cast<char*>(n) + sizeof(this->frontSafe));
		}
		Node* temp = _node;
		_node = _node->next;
		temp->next = nullptr;
		this->now--;
		temp->ownerPool = this;
		if (!this->toUseReplacementFlag)
		{
			new(&(temp->data)) T;
		}
		return reinterpret_cast<T*> (reinterpret_cast<char*>(temp) + sizeof(this->frontSafe));

	}

	bool Free(T*& t)
	{
		Node* temp = reinterpret_cast<Node*>(reinterpret_cast<char*>(t) - sizeof(this->frontSafe));

		if (this->toUseFreeFlag)
		{
			if (temp->isFree == true)
			{
				//error
				__debugbreak();
			}
		}
		if (this->toUseUniqueId)
		{
			if (temp->uniqueId != this->uniqueId)
			{
				//error
				__debugbreak();
			}
		}
		if (this->toUseSafeFlag)
		{
			if (temp->frontSafe != this->frontSafe)
			{
				//error
				__debugbreak();
			}
			if (temp->rearSafe != this->rearSafe)
			{
				//error
				__debugbreak();
			}
		}
		//�� ����ߴٸ� ���� ������
		if (!this->toUseReplacementFlag)
		{
			t->~T();
		}

		// �� �����忡�� �Ҵ��� ���������� Ȯ��
		if (temp->ownerPool == this)
		{
			this->now++;
			
			if (_node == nullptr)
			{
				temp->next = nullptr;
				_node = temp;
				return true;
			}
			temp->next = _node;
			_node = temp;
		}
		else
		{
			if (temp->ownerPool == nullptr)
			{
				__debugbreak();
			}
			temp->ownerPool->PushReclaimQueue(temp);
		}

		return true;
	}
	int PopReclaimQueue()
	{
		Node* node;
		int count = 0;
		generation++;
		while (reclaimQueue.Dequeue(node) != -1)
		{
			if (node->nodeGeneration + 3 <= generation)
			{
				if (node->next != nullptr)
				{
					reclaimQueue.Enqueue(node);
					continue;
				}
				this->now++;

				if (_node == nullptr)
				{
					node->next = nullptr;
				}
				else
				{
					node->next = _node;
				}
				
				_node = node;
				count++;
				//InterlockedIncrement(&freeCount);
			}
			else
			{
				reclaimQueue.Enqueue(node);
				break;
			}

		}

		return count;
	}

private:
	TLSMemoryPool()
	{
		uniqueId = globalTemplateCnt++;
		_node = nullptr;
	};
	TLSMemoryPool(bool toUseReplacement, bool initConstruction = true, bool toUseAddressList = false,
		bool toUseSafeFlag = false, bool toUseFree = false, bool toUseUniqueId = false)
	{
		this->initConstructionFlag = initConstruction;
		this->toUseReplacementFlag = toUseReplacement;
		this->toUseAddressList = toUseAddressList;
		this->toUseSafeFlag = toUseSafeFlag;
		this->toUseFreeFlag = toUseFree;
		this->toUseUniqueId = toUseUniqueId;
		uniqueId = globalTemplateCnt++;
	};

	~TLSMemoryPool()
	{
		/*
		if (!toUseReplacementFlag)
		{
			for (int i = 0; i < this->addressList.size(); i++)
			{
				delete (reinterpret_cast<T*>(this->addressList[i]));
			}
		}
		else
		{
			for (int i = 0; i < this->addressList.size(); i++)
			{
				free(reinterpret_cast<T*>(this->addressList[i]));
			}
		}
		*/
	};
	void PushReclaimQueue(Node* node)
	{
		node->nodeGeneration = generation;
		reclaimQueue.Enqueue(node);
	}



private:
	int total = 0;
	int now = 0;

	//��� ��� ����

	bool toUseReplacementFlag = true; // replacement new ��� ���� / �ܺο��� ��ü�� �˾Ƽ� �ٷ� ��
	bool initConstructionFlag = true; // �ʱ� �Ҵ�� ������ ȣ�� ����
	bool toUseAddressList = false; // addresslist ��� ����
	bool toUseSafeFlag = false; // safe �� ��� ����
	bool toUseFreeFlag = false; // alloc, free ǥ�� ���� 
	bool toUseUniqueId = false; // unique id ��� ����

	unsigned int allocCount = 0;
	unsigned int freeCount = 0;

	int generation = 0;
	//�޸� Ǯ�� �ִٸ� true, ���ٸ� false
	bool isFree = true;
	int frontSafe = 0xDEC0ADDE; // 0xDEADC0DE
	int rearSafe = 0x01F0BABA; // 0xBABAF001
	int uniqueId = 0; // �ܺο��� ���� �ο�
	//int additionalDataSize;
	std::vector<uintptr_t> addressList;

	Node* _node = nullptr;
	MPSCQueue<Node*> reclaimQueue;
};