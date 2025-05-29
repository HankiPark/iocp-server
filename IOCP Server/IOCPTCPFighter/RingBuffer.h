#ifndef __RINGBUFFER__
#define __RINGBUFFER__



class RingBuffer
{
public:
	enum sz_RingBuffer
	{
		szRingBuffer_DEFAULT = 3000
	};

	RingBuffer();
	RingBuffer(int size);
	~RingBuffer() noexcept;

	RingBuffer(const RingBuffer& ref);
	RingBuffer& operator=(const RingBuffer& ref);


	void Resize(int size);
	int GetBufferSize(void) const noexcept; // 가용가능한 크기(bufferSize - 1)를 줄지, bufferSize 값을 전부 줄지 고민
	int GetUseSize(void) const noexcept; // 현재 사용중인 용량 얻기
	int GetFreeSize(void) const noexcept;// 현재 버퍼에 남은 크기 얻기

	int Enqueue(char* data, int size) noexcept;
	int Dequeue(char* dest, int size) noexcept;
	int Peek(char* dest, int size) noexcept;
	void ClearBuffer(void) noexcept;

	int DirectEnqueueSize(void) const noexcept; // tail - ring buffer의 끝까지의 크기 혹은 tail - head의 크기
	int DirectDequeueSize(void) const noexcept; // head - tail의 크기 혹은 head - ring buffer의 끝까지의 크기

	int MoveHead(int size) noexcept; // GetHeadBufferPtr과 연계
	int MoveTail(int size) noexcept; // GetTailBufferPtr과 연계

	char* GetHeadBufferPtr(void) noexcept; // for delete
	char* GetTailBufferPtr(void) noexcept; // for insert
	char* GetRingBufferStartPtr(void) noexcept;
private:
	int bufferSize = 4000;
	char* inBuffer;
	size_t head = 0;
	size_t tail = 0;

};
#endif