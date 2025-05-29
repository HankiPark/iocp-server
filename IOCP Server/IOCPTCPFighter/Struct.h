#ifndef __FILESTRUCT__
#define __FILESTRUCT__

#include <winsock2.h>
#include "RingBuffer.h"
#include "BufferClass.h"
#include "Sector.h"
#include "MPSCQueue.h"
#include "LockFreeQueue.h"


//#define SECTOR_SIZE				128
//#define SECTOR_LEN_X			(6400 / SECTOR_SIZE)
//#define SECTOR_LEN_Y			(6400 / SECTOR_SIZE)



#define dfRANGE_MOVE_TOP	0
#define dfRANGE_MOVE_LEFT	0
#define dfRANGE_MOVE_RIGHT	6400
#define dfRANGE_MOVE_BOTTOM	6400

#define dfERROR_RANGE		200

#define dfATTACK1_RANGE_X		80
#define dfATTACK2_RANGE_X		90
#define dfATTACK3_RANGE_X		100
#define dfATTACK1_RANGE_Y		10
#define dfATTACK2_RANGE_Y		10
#define dfATTACK3_RANGE_Y		20

#define dfATTACK1_DAMAGE		1
#define dfATTACK2_DAMAGE		2
#define dfATTACK3_DAMAGE		3

#define dfNETWORK_PACKET_RECV_TIMEOUT	30000

#define dfSPEED_PLAYER_X	6	// 3   50fps
#define dfSPEED_PLAYER_Y	4	// 2   50fps

#define ACTION_MOVE_STOP		0
#define ACTION_MOVING			1

#define DIRECTION_VALUE_LU			0
#define DIRECTION_VALUE_UU			1
#define DIRECTION_VALUE_RU			2
#define DIRECTION_VALUE_LL			3
#define DIRECTION_VALUE_TRASH		4
#define DIRECTION_VALUE_RR			5
#define DIRECTION_VALUE_LD			6
#define DIRECTION_VALUE_DD			7
#define DIRECTION_VALUE_RD			8

#pragma pack(push, 1)
struct PacketHeaderInfo
{
	unsigned char byCode;
	unsigned char bySize;
	unsigned char byType;
};

struct CharacterInfo
{
	unsigned int id;
	unsigned char direction;
	unsigned short x;
	unsigned short y;
	unsigned char hp;
};

struct DeleteInfo {
	unsigned int id;
};

struct CSMoveInfo {
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct SCMoveInfo {
	unsigned int id;
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct CSStopInfo
{
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct SCStopInfo {
	unsigned int id;
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct CSAttackInfo 
{
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct SCAttackInfo
{
	unsigned int id;
	unsigned char direction;
	unsigned short x;
	unsigned short y;
};

struct SCDamageInfo
{
	unsigned int attackId;
	unsigned int damageId;
	unsigned char hp;
};


#pragma pack(pop)

struct SECTOR_POS
{
	int x;
	int y;
};

struct SECTOR_AROUND
{
	int count;
	SECTOR_POS around[9];
};




typedef union SessionId
{
	struct {
		unsigned long long index : 16;
		unsigned long long uniqueId : 48;
	} value;
	unsigned long long originValue;
} SESSIONID;

typedef union PlayerId
{
	struct {
		unsigned int index;
		unsigned int characterId;
	} value;
	unsigned long long originValue;
} PLAYERID;

typedef struct IncomingPacketInfo
{
	PLAYERID playerId;
	CPacket* packetBuffer;
	int packetType;
} IncomingPacketInfo;

typedef struct SendContext
{
	WSAOVERLAPPED sendWSAOverlapped;
	WSABUF wsabuf[1000];
	int count;
	void* ptr[1000];
} SENDCONTEXT;

struct SESSION
{
	SOCKET sock;
	alignas(64) unsigned long ioCount;
	//alignas(64) unsigned long isDead;
	SESSIONID sessionId;
	bool isDisconnectRequest;
	unsigned int isDeleteRequest;
	PLAYERID playerId;
	IN_ADDR ip;
	alignas(64) unsigned long canSend;
	USHORT port;
	//CRITICAL_SECTION sendRingBufferCSLock;
	RingBuffer recvRingBuffer;
	alignas(64) unsigned int handle;
	//RingBuffer sendRingBuffer;
	LockFreeQueue<CPacket*> sendQueue;
	int sendQueuingTime;
	int sendWaitingSize;
	int lastSendTime;
	SENDCONTEXT sendWSAOverlapped;
	WSAOVERLAPPED recvWSAOverlapped;

	void FlushSendPacket()
	{
		CPacket* packetBuffer;
		while (sendQueue.GetSize() != 0)
		{
			packetBuffer = nullptr;
			if (sendQueue.Dequeue(packetBuffer) != -1)
			{
				DESTROY_IF_ZERO(packetBuffer);
			}
		}

		for (int i = 0; i < sendWSAOverlapped.count; i++)
		{
			packetBuffer = reinterpret_cast<CPacket*>(sendWSAOverlapped.ptr[i]);
			DESTROY_IF_ZERO(packetBuffer);
		}

	}

};

struct Monitor
{
	unsigned int acceptCount;
	unsigned int disconnectCount;
	unsigned long recvByte;
	unsigned long sendByte;
	unsigned int sessionCount;

	Monitor() : acceptCount(0), disconnectCount(0), recvByte(0), sendByte(0), sessionCount(0) {}
};

struct Setting
{
	IN_ADDR openIp;
	USHORT port;
	int threadCount;
	int runningThreadCount;
	bool activateNagle;
	int maxPlayerCount;
	int maxSessionCount;
	//send 관련
	int maxWaitSendTime; // ms
	int maxWaitSendCount; // 개수
	int maxWaitSendSize; // kb
	// 섹터 관련
	int sectorTileSize;
	int sectorRow;
	int sectorCol;

	Setting() : port(0), threadCount(0), runningThreadCount(0), activateNagle(false),
		maxPlayerCount(0), maxSessionCount(0), maxWaitSendTime(0), maxWaitSendCount(0),
		maxWaitSendSize(0), sectorTileSize(0), sectorRow(0), sectorCol(0)
	{
		openIp.s_addr = 0;
	}
};

struct LoggingData
{
	alignas(64) LONG IocpLoopCount = 0;
	alignas(64) LONG LogicLoopCount = 0;
	alignas(64) LONG SessionCount = 0;
	alignas(64) LONG PlayerCount = 0;
	alignas(64) LONG AcceptCount = 0;
	alignas(64) LONG DisconnectCount = 0;

	alignas(64) LONG DelayedPacket = 0;
	alignas(64) LONG RecvByte = 0;
	alignas(64) LONG RecvPacketCount = 0;
	alignas(64) LONG RecvCount = 0;
	alignas(64) LONG CSMoveStartCount = 0;
	alignas(64) LONG CSMoveStopCount = 0;
	alignas(64) LONG CSAttackCount = 0;
	alignas(64) LONG CSEchoCount = 0;

	alignas(64) LONG SendByte = 0;
	alignas(64) LONG SendPacketCount = 0;
	alignas(64) LONG SendCount = 0;
	alignas(64) LONG SCCreateMyCharacterCount = 0;
	alignas(64) LONG SCCreateOtherCharacterCount = 0;
	alignas(64) LONG SCDeleteCharacterCount = 0;
	alignas(64) LONG SCMoveStartCount = 0;
	alignas(64) LONG SCMoveStopCount = 0;
	alignas(64) LONG SCAttackCount = 0;
	alignas(64) LONG SCDamagedCount = 0;
	alignas(64) LONG SCEchoCount = 0;
	alignas(64) LONG SCTotalSyncCount = 0;
};

struct Player
{
	SESSIONID sessionId;
	PLAYERID playerId;
	unsigned short x;
	unsigned short y;

	Sector<Player*>* currentSector;

	//SECTOR_POS curSector;
	//SECTOR_POS oldSector;
	BYTE direction;
	BYTE faceDirection;
	unsigned char hp;
	bool isSynced;
	bool isMoving;
	int lastRecvTime;
	alignas(64) unsigned int isDestroyRequest;
	alignas(64) unsigned int isDead;
};

#endif