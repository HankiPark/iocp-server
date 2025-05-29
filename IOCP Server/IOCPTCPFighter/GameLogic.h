#pragma once
#include "Struct.h"
#include "PacketDefine.h"
#include "Logging.h"
#include "Sector.h"

#define IDX(x, y) ((y % sectorRow) * sectorCol + (x % sectorCol))

#define SEARCH_ATTACK_RANGE(isLeft, xMax, xMin, yMax, yMin, X_RANGE, Y_RANGE)					\
do {																							\
		if (isLeft)																				\
		{																						\
			xMax = 	player->x;																		\
			xMin = xMax - X_RANGE;																\
		}																						\
		else																					\
		{																						\
			xMin = player->x;																		\
			xMax = xMin + X_RANGE;																\
		}																						\
		yMax = player->y + Y_RANGE;																	\
		yMin = player->y - Y_RANGE;																	\
} while (0)																						\

#define UPDATE_MOVE_CHARACTER(player, LocX, LocY)										\
do {																						\
	if (LocX >= 0 && LocY >= 0 && LocX < dfRANGE_MOVE_RIGHT && LocY < dfRANGE_MOVE_BOTTOM)	\
	{																						\
		player->x = LocX;																\
		player->y = LocY;																\
	}																						\
} while (0)																					\

class GameServer;

class GameLogic
{
public:
	GameLogic(GameServer* owner);
	~GameLogic() {};

	DWORD WINAPI StartLogicThread();

	//int EnqueuePacket(SESSIONID sessionId, CPacket* packet);

	int RequestDestroyPlayer(SESSIONID sessionId);

	int PushNewSession(SESSIONID sessionId);

	int PushIncomingPacket(IncomingPacketInfo ipi);

	int CountAlivePlayers();

private:

	int SetInitValue();

	int CreateNewPlayer(SESSIONID id);

	int ProcessPackets();

	int UpdateGameWorld();
	
	int MakeSectorSpawnPackets(Player* player);

	int SendAOI(Player* player, CPacket* packetBuffer, bool sendMe);

	int SendAOIToSelf(Player* player);
	
	bool SectorUpdate(Player* player, int prevX, int prevY);


	bool PacketProcMoveStart(Player* player, CPacket* packetBuffer);

	bool PacketProcMoveStop(Player* player, CPacket* packetBuffer);

	bool PacketProcAttack1(Player* player, CPacket* packetBuffer);

	bool PacketProcAttack2(Player* player, CPacket* packetBuffer);

	bool PacketProcAttack3(Player* player, CPacket* packetBuffer);

	bool PacketProcEcho(Player* player, CPacket* packetBuffer);

	bool CheckDamagedPlayer(Player* player, int attackType);

	bool FlushFrame();

	

	//int SendUnicast(SESSION* session, CPacket* packetBuffer);

	//int SendAround(SESSION* session, CPacket* packetBuffer, bool sendMe = false);

	__forceinline int GetSectorIndex(int x, int y)
	{
		int xIndex = x / sectorTileSize;
		int yIndex = y / sectorTileSize;
		
		if (x < 0 || y < 0 || xIndex < 0 || yIndex < 0 || xIndex >= sectorCol || yIndex >= sectorRow)
		{
			//__debugbreak();
			return -1;
		}
		return yIndex * sectorCol + xIndex;
	}

	__forceinline bool MakePacketCreateMyCharacter(CPacket* packetBuffer,
		DWORD id, BYTE direction, unsigned short x, unsigned short y, unsigned char hp)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 10;
		header.byType = dfPACKET_SC_CREATE_MY_CHARACTER;
		//packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << id << direction << x << y << hp;

		return true;
	}

	__forceinline bool MakePacketCreateOtherCharacter(CPacket* packetBuffer,
		DWORD id, BYTE direction, unsigned short x, unsigned short y, unsigned char hp)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 10;
		header.byType = dfPACKET_SC_CREATE_OTHER_CHARACTER;
		//packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << id << direction << x << y << hp;

		return true;
	}

	__forceinline bool MakePacketSCMoveStart(CPacket* packetBuffer,
		DWORD id, BYTE direction, unsigned short x, unsigned short y)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 9;
		header.byType = dfPACKET_SC_MOVE_START;
		//packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << id << direction << x << y;

		return true;
	}

	__forceinline bool MakePacketDeleteCharacter(CPacket* packetBuffer, DWORD id)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 4;
		header.byType = dfPACKET_SC_DELETE_CHARACTER;
		//packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << id;
		return true;
	}

	__forceinline bool MakePacketSync(CPacket* packetBuffer, DWORD id, unsigned short x, unsigned short y)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 8;
		header.byType = dfPACKET_SC_SYNC;
		//packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << id << x << y;

		return true;
	}

	__forceinline bool MakePacketSCMoveStop(CPacket* packetBuffer,
		DWORD id, BYTE direction, unsigned short x, unsigned short y)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 9;
		header.byType = dfPACKET_SC_MOVE_STOP;
		//packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << id << direction << x << y;

		return true;
	}

	__forceinline bool MakePacketSCAttck1(CPacket* packetBuffer,
		DWORD id, BYTE direction, unsigned short x, unsigned short y)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 9;
		header.byType = dfPACKET_SC_ATTACK1;
		//packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << id << direction << x << y;
		return true;
	}

	__forceinline bool MakePacketSCAttck2(CPacket* packetBuffer,
		DWORD id, BYTE direction, unsigned short x, unsigned short y)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 9;
		header.byType = dfPACKET_SC_ATTACK2;
		//packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << id << direction << x << y;
		return true;
	}

	__forceinline bool MakePacketSCAttck3(CPacket* packetBuffer,
		DWORD id, BYTE direction, unsigned short x, unsigned short y)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 9;
		header.byType = dfPACKET_SC_ATTACK3;
		//packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << id << direction << x << y;
		return true;
	}

	__forceinline bool MakePacketSCDamage(CPacket* packetBuffer,
		DWORD attackId, DWORD damageId, unsigned char hp)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 9;
		header.byType = dfPACKET_SC_DAMAGE;
		//packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << attackId << damageId << hp;

		return true;
	}

	__forceinline bool MakePacketSCEcho(CPacket* packetBuffer,
		DWORD time)
	{
		PacketHeaderInfo header;
		header.byCode = 0x89;
		header.bySize = 4;
		header.byType = dfPACKET_SC_ECHO;
		packetBuffer->ClearPacket();
		packetBuffer->MoveInsideStructData((char*)&header, sizeof(PacketHeaderInfo));
		*packetBuffer << time;

		return true;
	}



private:
	std::vector<Player*> playerList;
	std::vector<Player*> activePlayerList;
	std::vector<Player*> movingPlayerList;
	std::vector<Sector<Player*>> sectors;
	MPSCQueue<SESSIONID> newSessionQueue;
	MPSCQueue<SESSIONID> destroyPlayerRequestQueue;
	MPSCQueue<IncomingPacketInfo> packetQueue;
	Log* log = nullptr;
	unsigned int uniqueCharacterId;
	int sectorTileSize;
	int sectorRow;
	int sectorCol;

	size_t aliveCheckCursor = 0;
	size_t moveCursor = 0;

	GameServer* server;
};