#ifndef __BUFFERCLASS__
#define __BUFFERCLASS__
#include <iostream>

#define DESTROY_IF_ZERO(packetBuffer)						\
do {														\
	if (packetBuffer->DecrementReferenceCount() == 0)		\
	{														\
		delete packetBuffer;								\
	}														\
} while (0)													\


class CPacket
{
public:
	__forceinline CPacket()
	{
		this->buffer = (char*)malloc(sizeof(char) * this->bufferSize);
		referenceCount = 1;
	}
	__forceinline CPacket(unsigned int bufferSize)
	{
		if (bufferSize <= 0)
		{
			__debugbreak();
		}
		this->bufferSize = bufferSize;
		referenceCount = 1;
		//this->buffer = (char*)bufferPool.Alloc();
		this->buffer = (char*)malloc(sizeof(char) * this->bufferSize);
	}
	virtual __forceinline ~CPacket() noexcept
	{
		//bufferPool.Free((BS*&)(this->buffer));
		free(this->buffer);
	}

	//버퍼 비우기
	__forceinline void ClearPacket() noexcept
	{
		this->readPos = 0;
		this->writePos = 0;
	}

	//버퍼 사이즈 얻기
	__forceinline unsigned int GetBufferSize() const noexcept
	{
		return this->bufferSize;
	}

	//현재 사용중인 크기 얻기
	__forceinline int GetUsedSize() const noexcept
	{
		return this->writePos - this->readPos;
	}

	//buffer pointer 얻기
	__forceinline char* GetBufferPointer() noexcept
	{
		return this->buffer;
	}	
	
	//buffer reference count 증가
	__forceinline int IncrementReferenceCount() noexcept
	{
		
		return InterlockedIncrement(&this->referenceCount);
	}	
	
	//buffer reference count 감소
	__forceinline int DecrementReferenceCount() noexcept
	{
		return InterlockedDecrement(&this->referenceCount);
	}	
	//buffer reference count 얻기
	__forceinline int GetReferenceCount() noexcept
	{
		return InterlockedCompareExchange(&this->referenceCount, 0, 0);
	}

	__forceinline bool Resize()
	{
		if (this->isExpand)
		{
			__debugbreak();
			return false;
		}
		char* temp = (char*)malloc(this->bufferSize * 2 + 1);
		memcpy_s(temp, this->bufferSize * 2, this->buffer, this->bufferSize);
		free(this->buffer);
		this->buffer = temp;
		this->bufferSize *= 2;
		this->isExpand = true;
		return true;

	}

	// buffer pos 이동
	__forceinline int MoveWritePos(unsigned int size) noexcept
	{
		if (this->writePos + size >= this->bufferSize)
		{
			int retval = this->bufferSize - this->writePos;
			this->writePos = this->bufferSize;
			return retval;
		}

		this->writePos += size;

		return size;
	}

	__forceinline int MoveReadPos(unsigned int size) noexcept
	{
		if (this->readPos + size >= this->bufferSize)
		{
			int retval = this->bufferSize - this->readPos;
			this->readPos = this->bufferSize;
			return retval;
		}

		this->readPos += size;

		return size;
	}

	__forceinline int GetStructData(char* dest, unsigned int size)
	{
		unsigned int wpos = this->writePos;
		unsigned int rpos = this->readPos;
		char* pbuffer = this->buffer;
		if (wpos - rpos < size)
		{
			int retval;
			retval = wpos - rpos;
			memcpy_s(dest, retval, pbuffer + rpos, retval);
			this->readPos = wpos;
			return retval;
		}

		memcpy_s(dest, size, pbuffer + rpos, size);
		this->readPos += size;
		return size;
	}

	__forceinline int MoveInsideStructData(char* src, unsigned int size)
	{
		unsigned int bsz = this->bufferSize;
		unsigned int wpos = this->writePos;
		char* pbuffer = this->buffer;
		if (bsz - wpos < size)
		{
			int retval;
			retval = bsz - wpos;
			memcpy_s(pbuffer + wpos, retval, src, retval);
			this->writePos = bsz;
			return retval;
		}

		memcpy_s(pbuffer + wpos, size, src, size);
		this->writePos += size;
		return size;
	}

	//연산자 오버로딩
	__forceinline CPacket& operator=(const CPacket& srcPacket)
	{
		if (this != &srcPacket)
		{
			if (this->bufferSize != srcPacket.bufferSize)
			{
				this->bufferSize = srcPacket.bufferSize;
				free(this->buffer);
				this->buffer = (char*)malloc(this->bufferSize);
			}
			memcpy_s(this->buffer, this->bufferSize, srcPacket.buffer, this->bufferSize);
			this->writePos = srcPacket.writePos;
			this->readPos = srcPacket.readPos;
			this->referenceCount = srcPacket.referenceCount;
		}
		return *this;

	}

	// <<
	__forceinline CPacket& operator<<(unsigned char value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		this->buffer[this->writePos++] = value;
		return *this;
	}
	__forceinline CPacket& operator<<(char value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		this->buffer[this->writePos++] = value;
		return *this;
	}

	__forceinline CPacket& operator<<(unsigned short value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<unsigned short*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);
		return *this;
	}
	__forceinline CPacket& operator<<(short value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<short*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);

		return *this;
	}

	__forceinline CPacket& operator<<(unsigned int value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<unsigned int*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);

		return *this;
	}
	__forceinline CPacket& operator<<(int value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<int*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);

		return *this;
	}

	__forceinline CPacket& operator<<(unsigned __int64 value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<unsigned __int64*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);

		return *this;
	}
	__forceinline CPacket& operator<<(__int64 value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<__int64*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);

		return *this;
	}

	__forceinline CPacket& operator<<(unsigned long value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<unsigned long*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);

		return *this;
	}
	__forceinline CPacket& operator<<(long value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<long*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);

		return *this;
	}

	__forceinline CPacket& operator<<(float value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<float*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);
		return *this;
	}
	__forceinline CPacket& operator<<(double value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<double*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);

		return *this;
	}

	__forceinline CPacket& operator<<(wchar_t value)
	{
		if (this->writePos + sizeof(value) >= this->bufferSize)
		{
			if (this->Resize() == false)
			{
				this->errorFlag = true;
				return *this;
			}
		}
		*reinterpret_cast<wchar_t*> ((char*)(this->buffer) + this->writePos) = value;
		this->writePos += sizeof(value);

		return *this;
	}

	// >>
	__forceinline CPacket& operator>>(unsigned char& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<unsigned char*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}

	__forceinline CPacket& operator>>(char& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<char*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}

	__forceinline CPacket& operator>>(unsigned short& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<unsigned short*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}
	__forceinline CPacket& operator>>(short& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<short*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}

	__forceinline CPacket& operator>>(unsigned int& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<unsigned int*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}
	__forceinline CPacket& operator>>(int& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<int*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}

	__forceinline CPacket& operator>>(unsigned __int64& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<unsigned __int64*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}
	__forceinline CPacket& operator>>(__int64& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<__int64*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}

	__forceinline CPacket& operator>>(unsigned long& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<unsigned long*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}
	__forceinline CPacket& operator>>(long& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<long*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}

	__forceinline CPacket& operator>>(float& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<float*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}
	__forceinline CPacket& operator>>(double& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<double*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}

	__forceinline CPacket& operator>>(wchar_t& value)
	{
		if (this->readPos + sizeof(value) > this->writePos)
		{
			this->errorFlag = true;
			return *this;
		}
		value = *reinterpret_cast<wchar_t*> ((char*)(this->buffer) + this->readPos);
		this->readPos += sizeof(value);
		return *this;
	}

private:
	char* buffer;
	unsigned int readPos = 0; // Pos를 읽으면 된다.
	unsigned int writePos = 0; // Pos까지 썼다.
	unsigned int bufferSize = 1400;
	unsigned int referenceCount = 1;
	bool isExpand = false;
	bool errorFlag = false;
};





#endif