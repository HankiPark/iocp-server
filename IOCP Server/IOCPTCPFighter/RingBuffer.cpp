#include <iostream>
#include <windows.h>
#include "RingBuffer.h"


RingBuffer::RingBuffer() : RingBuffer(szRingBuffer_DEFAULT) {}


RingBuffer::RingBuffer(int size)
	: bufferSize(size)
{
	this->inBuffer = (char*)malloc(sizeof(char) * this->bufferSize);
}

RingBuffer::~RingBuffer() noexcept
{
	free(this->inBuffer);
}

RingBuffer::RingBuffer(const RingBuffer& ref)
{
	this->inBuffer = (char*)malloc(sizeof(char) * this->bufferSize);
	memcpy_s(this->inBuffer, this->bufferSize, ref.inBuffer, this->bufferSize);
	this->bufferSize = ref.bufferSize;
	this->head = ref.head;
	this->tail = ref.tail;
}

RingBuffer& RingBuffer::operator=(const RingBuffer& ref)
{
	if (this != &ref)
	{
		free(this->inBuffer);
		this->inBuffer = (char*)malloc(sizeof(char) * this->bufferSize);
		memcpy_s(this->inBuffer, this->bufferSize, ref.inBuffer, this->bufferSize);
		this->bufferSize = ref.bufferSize;
		this->head = ref.head;
		this->tail = ref.tail;
	}
	return *this;
}


void RingBuffer::Resize(int size) 
{
	char* temp = this->inBuffer;
	if (NULL == (this->inBuffer = (char*)realloc(this->inBuffer, size)))
	{}
	this->bufferSize = size;
}

int RingBuffer::GetBufferSize(void) const noexcept
{
	return this->bufferSize;
}

int RingBuffer::GetUseSize(void) const noexcept
{
	size_t heads = this->head;
	size_t tails = this->tail;
	if (tails >= heads)
		return tails - heads;
	else
		return this->bufferSize - (heads - tails);
}

int RingBuffer::GetFreeSize(void) const noexcept
{
	size_t heads = this->head;
	size_t tails = this->tail;

	return (heads + this->bufferSize - tails - 1) % this->bufferSize;
}

int RingBuffer::Enqueue(char* data, int size) noexcept
{
	if (head == (tail + 1) % this->bufferSize)
	{
		return 0;
	}

	int enableSize = this->GetFreeSize();
	int index = 0;
	int targetSize;
	size_t temp = this->tail;
	if (enableSize < size)
	{
		targetSize = enableSize;
	}
	else
	{
		targetSize = size;
	}
	

	while (index < targetSize)
	{

		this->inBuffer[temp++] = data[index++];
		if (temp == this->bufferSize)
		{
			temp = 0;

		}
	}
	this->tail = temp;
	return index;
}

int RingBuffer::Dequeue(char* dest, int size) noexcept
{
	if (head == tail)
	{
		// 데이터가 없습니다.
		return 0;
	}
	int index = 0;
	int targetSize;
	size_t temp = this->head;

	if (size < this->GetUseSize())
	{
		targetSize = size;
	}
	else
	{
		targetSize = this->GetUseSize();
	}

	while (index < targetSize)
	{

		dest[index++] = this->inBuffer[temp++];
		if (temp == this->bufferSize)
		{
			temp = 0;

		}
	}
	this->head = temp;

	return index;
}

int RingBuffer::Peek(char* dest, int size) noexcept
{
	if (head == tail)
	{
		// 데이터가 없습니다.
		return 0;
	}

	int index = 0;
	size_t peekIndex = this->head;
	int targetSize;

	if (size < this->GetUseSize())
	{
		targetSize = size;

	}
	else
	{
		targetSize = this->GetUseSize();
	}

	while (index < targetSize)
	{

		dest[index++] = this->inBuffer[peekIndex++];
		if (peekIndex == this->bufferSize)
		{
			peekIndex = 0;
		}
	}

	return index;
}

void RingBuffer::ClearBuffer() noexcept
{
	this->head = 0;
	this->tail = 0;
}


int RingBuffer::DirectEnqueueSize(void) const noexcept
{
	size_t headVal = this->head;
	size_t tailVal = this->tail;
	if (this->GetFreeSize() == 0)
	{
		return 0;
	}
	if (headVal > tailVal)
	{
		return headVal - tailVal - 1;
	}
	if (headVal == 0)
	{
		return this->bufferSize - tailVal - 1;
	}
	return (int)(this->bufferSize - tailVal);
}

int RingBuffer::DirectDequeueSize(void) const noexcept
{
	size_t headVal = this->head;
	size_t tailVal = this->tail;
	if (headVal > tailVal)
	{
		return this->bufferSize - headVal;
	}

	return (int)(tailVal - headVal);

}
//음...... 끝에 도달하면 초기화되게끔하자.
int RingBuffer::MoveHead(int size) noexcept
{
	int useSize = GetUseSize();
	size_t headVal = this->head;
	size_t tailVal = this->tail;

	if (size > useSize)
	{
		return 0;
	}
	this->head = (headVal + size) % this->bufferSize;

	return size;
}

int RingBuffer::MoveTail(int size) noexcept
{
	int freeSize = GetFreeSize();
	size_t headVal = this->head;
	size_t tailVal = this->tail;

	if (size > freeSize)
	{
		return 0;
	}
	this->tail = (tailVal + size) % this->bufferSize;

	return size;
}

char* RingBuffer::GetHeadBufferPtr(void) noexcept
{
	return (char*)(this->inBuffer) + this->head;
}

char* RingBuffer::GetTailBufferPtr(void) noexcept
{
	return (char*)(this->inBuffer) + this->tail;
}

char* RingBuffer::GetRingBufferStartPtr(void) noexcept
{
	return (char*)(this->inBuffer);
}