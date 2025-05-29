#include "Struct.h"
#include <windows.h>
#include "Sector.h"
#include "GameServer.h"
#include "Struct.h"
#include "Logging.h"
#include "GameLogic.h"


GameLogic::GameLogic(GameServer* owner)
{
	server = owner;
	uniqueCharacterId = rand() % 1000;
	sectorTileSize = 0;
	sectorCol = 0;
	sectorRow = 0;
};
int GameLogic::SetInitValue()
{
	sectorTileSize = server->GetSectorTileSize();
	sectorCol = server->GetSectorCols();
	sectorRow = server->GetSectorRows();
	log = server->GetLogPointer();

	sectors.resize(sectorRow * sectorCol);
	int sessionSize = server->GetMaxSessionSize();
	playerList.resize(sessionSize);
	return 1;
}

DWORD WINAPI GameLogic::StartLogicThread()
{

	SetInitValue();
	const int MAX_FRAME_COUNT = 7;
	unsigned int timer = timeGetTime();

	while (1)
	{
		int processed = 0;
		for (int i = 0; i < 200 && newSessionQueue.GetSize() > 0; ++i)
		{
			// 플레이어 객체 생성
			SESSIONID id;
			newSessionQueue.Dequeue(id);
			CreateNewPlayer(id);
		}
		// 패킷 처리
		ProcessPackets();

		while (timer + 40 <= timeGetTime() && processed < MAX_FRAME_COUNT)
		{
			timer += 40;
			// 프레임에 따른 처리
			UpdateGameWorld();
			InterlockedIncrement(&log->logData.LogicLoopCount);
			processed++;
		}
		// 객체 요청 및 제거
		FlushFrame();
	}



}

int GameLogic::PushNewSession(SESSIONID sessionId)
{
	newSessionQueue.Enqueue(sessionId);
	return 1;
}

int GameLogic::PushIncomingPacket(IncomingPacketInfo ipi)
{
	packetQueue.Enqueue(ipi);
	return 1;
}

int GameLogic::RequestDestroyPlayer(SESSIONID sessionId)
{
	
	Player* player = playerList[sessionId.value.index];
	if (player == nullptr)
	{
		server->DelayFreeSessionMapIndex(sessionId.value.index);
		return 0;
	}
	//AcquireSRWLockExclusive(&player->currentSector->lock);
	//player->currentSector->Leave(player);
	//ReleaseSRWLockExclusive(&player->currentSector->lock);
	InterlockedExchange(&player->isDead, 1);
	InterlockedExchange(&player->isDestroyRequest, 1);
	destroyPlayerRequestQueue.Enqueue(sessionId);
	return 1;
}


int GameLogic::CreateNewPlayer(SESSIONID id)
{
	//Player* player = new Player();
	Player* player = (Player*)_aligned_malloc(sizeof(Player), 64);
	new (player) Player();
	// 값 설정
	player->sessionId.originValue = id.originValue;
	player->playerId.value.characterId = InterlockedIncrement(&uniqueCharacterId);
	player->playerId.value.index = (int)id.value.index;
	player->x = rand() % 6300 + 1;
	//player->x = 3000;
	//player->y = 3000;
	player->y = rand() % 6300 + 1;
	player->hp = 100;
	player->isDead = 0;
	player->isMoving = false;
	player->isSynced = false;
	player->isDestroyRequest = 0;
	player->lastRecvTime = timeGetTime();
	if (player->x >= 3150)
	{
		player->direction = dfPACKET_MOVE_DIR_LL;
		player->faceDirection = dfPACKET_MOVE_DIR_LL;
	}
	else
	{
		player->direction = dfPACKET_MOVE_DIR_RR;
		player->faceDirection = dfPACKET_MOVE_DIR_RR;
	}
	if (server->RegisterSessionPlayer(id.value.index, player->playerId) == false)
	{
		_aligned_free(player);
		return false;
	}

	sectors[GetSectorIndex(player->x, player->y)].Enter(player);

	//sector와 characterMap에 등록
	playerList[player->playerId.value.index] = player;
	activePlayerList.push_back(player);


	// session에 패킷 전달
	CPacket* packetBuffer = new CPacket();
	MakePacketCreateMyCharacter(packetBuffer, player->playerId.value.characterId, player->direction, player->x, player->y, player->hp);
	//packetBuffer->IncrementReferenceCount();
	server->SendPacket(id, packetBuffer);
	DESTROY_IF_ZERO(packetBuffer);
	InterlockedIncrement(&log->logData.PlayerCount);
	InterlockedIncrement(&log->logData.SCCreateMyCharacterCount);

	// sector내의 캐릭터 생성
	MakeSectorSpawnPackets(player);
	return 1;
}


int GameLogic::MakeSectorSpawnPackets(Player* player)
{
	CPacket* packetBuffer = new CPacket();
	MakePacketCreateOtherCharacter(packetBuffer, player->playerId.value.characterId, player->direction, player->x, player->y, player->hp);
	int count = SendAOI(player, packetBuffer, false);
	DESTROY_IF_ZERO(packetBuffer);
	SendAOIToSelf(player);
	InterlockedAdd(&log->logData.SCCreateOtherCharacterCount, count);
	
	return 1;
}

int GameLogic::SendAOI(Player* player, CPacket* packetBuffer, bool sendMe)
{
	const std::unordered_set<Player*>* players = player->currentSector->GetSectorPlayers();
	int count = 0;
	if (players != nullptr)
	{
		for (Player* p : *players)
		{
			if (InterlockedCompareExchange(&player->isDead, 0, 0) == 1 || (!sendMe && p == player))
			{
				continue;
			}
			//packetBuffer->IncrementReferenceCount();
			server->SendPacket(p->sessionId, packetBuffer);
			count++;
		}
	}


	for (int h = -1; h <= 1; h++)
	{
		for (int w = -1; w <= 1; w++)
		{
			if (h == 0 && w == 0)
			{
				continue;
			}
			int hv = h * sectorTileSize;
			int wv = w * sectorTileSize;
			int coordinate = GetSectorIndex((player->x + wv), (player->y + hv));
			if (coordinate == -1)
			{
				continue;
			}
			players = sectors[coordinate].GetSectorPlayers();
			if (players == nullptr)
			{
				continue;
			}
			for (Player* p : *players)
			{
				if (InterlockedCompareExchange(&player->isDead, 0, 0) == 1)
				{
					continue;
				}
				//packetBuffer->IncrementReferenceCount();
				server->SendPacket(p->sessionId, packetBuffer);
				count++;
			}
		
		}
	}


	return count;
}

int GameLogic::SendAOIToSelf(Player* player)
{
	const std::unordered_set<Player*>* players = player->currentSector->GetSectorPlayers();
	int createCount = 0;
	int moveCount = 0;
	if (players != nullptr)
	{
		for (Player* p : *players)
		{
			if (InterlockedCompareExchange(&p->isDead, 0, 0) == 1 || p == player)
			{
				continue;
			}
			CPacket* packetBuffer = new CPacket();
			MakePacketCreateOtherCharacter(packetBuffer,
				p->playerId.value.characterId, p->direction, p->x, p->y, p->hp);
			//packetBuffer->IncrementReferenceCount();
			server->SendPacket(player->sessionId, packetBuffer);
			createCount++;
			DESTROY_IF_ZERO(packetBuffer);
			if (p->isMoving == ACTION_MOVING)
			{
				CPacket* packetBuffer2 = new CPacket();
				MakePacketSCMoveStart(packetBuffer2, p->playerId.value.characterId, p->direction, p->x, p->y);
				//packetBuffer2->IncrementReferenceCount();
				server->SendPacket(player->sessionId, packetBuffer2);
				moveCount++;
				DESTROY_IF_ZERO(packetBuffer2);
			}
		}
	}


	for (int h = -1; h <= 1; h++)
	{
		for (int w = -1; w <= 1; w++)
		{
			if (h == 0 && w == 0)
			{
				continue;
			}
			int hv = h * sectorTileSize;
			int wv = w * sectorTileSize;
			int coordinate = GetSectorIndex((player->x + wv), (player->y + hv));
			if (coordinate == -1)
			{
				continue;
			}
			players = sectors[coordinate].GetSectorPlayers();
			if (players == nullptr)
			{
				continue;
			}
			for (Player* p : *players)
			{
				if (InterlockedCompareExchange(&p->isDead, 0, 0) == 1)
				{
					continue;
				}
				CPacket* packetBuffer = new CPacket();
				MakePacketCreateOtherCharacter(packetBuffer,
					p->playerId.value.characterId, p->direction, p->x, p->y, p->hp);
				createCount++;
				//packetBuffer->IncrementReferenceCount();
				server->SendPacket(player->sessionId, packetBuffer);
				DESTROY_IF_ZERO(packetBuffer);
				if (p->isMoving == ACTION_MOVING)
				{
					CPacket* packetBuffer2 = new CPacket();
					MakePacketSCMoveStart(packetBuffer2, p->playerId.value.characterId, p->direction, p->x, p->y);
					//packetBuffer2->IncrementReferenceCount();
					server->SendPacket(player->sessionId, packetBuffer2);
					moveCount++;
					DESTROY_IF_ZERO(packetBuffer2);
				}

			}
		
		}
	}
	InterlockedAdd(&log->logData.SCCreateOtherCharacterCount, createCount);
	InterlockedAdd(&log->logData.SCMoveStartCount, moveCount);

	return 1;
}


int GameLogic::ProcessPackets()
{
	while (packetQueue.GetSize() != 0)
	{
		IncomingPacketInfo p;
		int k = packetQueue.Dequeue(p);
		Player* player = playerList[p.playerId.value.index];
		if (player == nullptr || player->playerId.originValue != p.playerId.originValue)
		{
			InterlockedIncrement(&log->logData.DelayedPacket);
			continue;
		}
		server->AddRefSession(player->sessionId);

		if (player->playerId.value.characterId != p.playerId.value.characterId)
		{
			DESTROY_IF_ZERO(p.packetBuffer);
			continue;
		}
		player->lastRecvTime = timeGetTime();
		switch (p.packetType)
		{
		case dfPACKET_CS_MOVE_START:
			InterlockedIncrement(&log->logData.CSMoveStartCount);
			PacketProcMoveStart(player, p.packetBuffer);
			break;
		case dfPACKET_CS_MOVE_STOP:
			InterlockedIncrement(&log->logData.CSMoveStopCount);
			PacketProcMoveStop(player, p.packetBuffer);
			break;
		case dfPACKET_CS_ATTACK1:
			InterlockedIncrement(&log->logData.CSAttackCount);
			PacketProcAttack1(player, p.packetBuffer);
			break;
		case dfPACKET_CS_ATTACK2:
			InterlockedIncrement(&log->logData.CSAttackCount);
			PacketProcAttack2(player, p.packetBuffer);
			break;
		case dfPACKET_CS_ATTACK3:
			InterlockedIncrement(&log->logData.CSAttackCount);
			PacketProcAttack3(player, p.packetBuffer);
			break;
		case dfPACKET_CS_ECHO:
			InterlockedIncrement(&log->logData.CSEchoCount);
			PacketProcEcho(player, p.packetBuffer);
			break;
		default:
			DESTROY_IF_ZERO(p.packetBuffer);
			break;
		}
		server->ReleaseRefSession(player->sessionId);
	}
	return 1;
}



bool GameLogic::PacketProcMoveStart(Player* player, CPacket* packetBuffer)
{
	unsigned char direction;
	unsigned short x;
	unsigned short y;	
	unsigned short prevX = player->x;
	unsigned short prevY = player->y;
	*packetBuffer >> direction >> x >> y;
	DESTROY_IF_ZERO(packetBuffer);
	//LOG(LOG_LEVEL_DEBUG, L"[%s] MoveStart > Session ID : %d / Direction : %d / X : %d / Y : %d",
	//	L"DEBUG", si->sessionId, direction, x, y);


	if ((abs(player->x - x) > dfERROR_RANGE ||
		abs(player->y - y) > dfERROR_RANGE))
	{
		CPacket* packetBuffer2 = new CPacket();
		//printf("start sync error session : %d, direction : %d, now x : %d , now y : %d, packet x : %d, packet y : %d \n"
		//	, player->playerId.value.characterId, player->direction, player->x, player->y, x, y);
		MakePacketSync(packetBuffer2, player->playerId.value.characterId, player->x, player->y);
		//packetBuffer2->IncrementReferenceCount();
		server->SendPacket(player->sessionId, packetBuffer2);
		DESTROY_IF_ZERO(packetBuffer2);
		InterlockedIncrement(&log->logData.SCTotalSyncCount);
		//syncCnt += SendUnicast(si, packetBuffer);
		x = player->x;
		y = player->y;
	}

	player->x = x;
	player->y = y;
	player->direction = direction;
	if (player->isMoving == false)
	{
		movingPlayerList.push_back(player);
	}
	player->isMoving = true;
	
	//moveSetGroup.insert(si->sessionId);

	//얼굴 방향 전환
	if (direction != dfPACKET_MOVE_DIR_DD && direction != dfPACKET_MOVE_DIR_UU)
	{
		player->faceDirection = direction;
	}
	int index = GetSectorIndex(player->x, player->y);
	if (player->currentSector != &sectors[index])
	{
		SectorUpdate(player, prevX, prevY);
	}
	CPacket* packetBuffer3 = new CPacket();
	MakePacketSCMoveStart(packetBuffer3, player->playerId.value.characterId, player->direction, player->x, player->y);
	int count = SendAOI(player, packetBuffer3, false);
	DESTROY_IF_ZERO(packetBuffer3);
	InterlockedAdd(&log->logData.SCMoveStartCount, count);
	return true;
}

bool GameLogic::SectorUpdate(Player* player, int prevX, int prevY)
{
	int prevIndex = GetSectorIndex(prevX, prevY);
	int currIndex = GetSectorIndex(player->x, player->y);
	std::unordered_set<Sector<Player*>*> prevSet;
	std::unordered_set<Sector<Player*>*> currSet;
	sectors[prevIndex].Leave(player);
	sectors[currIndex].Enter(player);
	
	for (int h = -1; h <= 1; h++)
	{
		for (int w = -1; w <= 1; w++)
		{
			/*if (h == 0 && w == 0)
			{
				continue;
			}*/
			int hv = h * sectorTileSize;
			int wv = w * sectorTileSize;
			prevIndex = GetSectorIndex(prevX + wv, prevY + hv);
			if (prevIndex != -1)
			{
				if (currSet.find(&sectors[prevIndex]) != currSet.end())
				{
					currSet.erase(&sectors[prevIndex]);
				}
				else
				{
					prevSet.insert(&sectors[prevIndex]);
				}
			}


			currIndex = GetSectorIndex(player->x + wv, player->y + hv);
			if (currIndex != -1)
			{
				if (prevSet.find(&sectors[currIndex]) != prevSet.end())
				{
					prevSet.erase(&sectors[currIndex]);
				}
				else
				{
					currSet.insert(&sectors[currIndex]);
				}
			}
		}
	}

	int createCount = 0;
	int moveCount = 0;
	int deleteCount = 0;
	CPacket* packetBuffer = new CPacket();
	MakePacketCreateOtherCharacter(packetBuffer, player->playerId.value.characterId, player->direction, player->x, player->y, player->hp);
	for (std::unordered_set<Sector<Player*>*>::iterator iter = currSet.begin(); iter != currSet.end(); ++iter)
	{
		Sector<Player*>* sector = *iter;
		const auto* players = sector->GetSectorPlayers();
		if (players == nullptr)
		{
			continue;
		}
		for (Player* otherPlayer : *players)
		{
			if (InterlockedCompareExchange(&otherPlayer->isDead, 0, 0) == 1)
			{
				continue;
			}
			CPacket* packetBuffer2 = new CPacket();
			MakePacketCreateOtherCharacter(packetBuffer2, otherPlayer->playerId.value.characterId, otherPlayer->direction, otherPlayer->x, otherPlayer->y, otherPlayer->hp);
			//packetBuffer2->IncrementReferenceCount();
			//packetBuffer->IncrementReferenceCount();
			server->SendPacket(player->sessionId, packetBuffer2);
			server->SendPacket(otherPlayer->sessionId, packetBuffer);
			createCount += 2;
			DESTROY_IF_ZERO(packetBuffer2);
			if (player->isMoving)
			{
				CPacket* packetBuffer3 = new CPacket();
				MakePacketSCMoveStart(packetBuffer3, player->playerId.value.characterId, player->direction, player->x, player->y);
				//packetBuffer3->IncrementReferenceCount();
				server->SendPacket(otherPlayer->sessionId, packetBuffer3);
				moveCount++;
				DESTROY_IF_ZERO(packetBuffer3);
			}
			if (otherPlayer->isMoving)
			{
				CPacket* packetBuffer4 = new CPacket();
				MakePacketSCMoveStart(packetBuffer4, otherPlayer->playerId.value.characterId, otherPlayer->direction, otherPlayer->x, otherPlayer->y);
				//packetBuffer4->IncrementReferenceCount();
				server->SendPacket(player->sessionId, packetBuffer4);
				moveCount++;
				DESTROY_IF_ZERO(packetBuffer4);
			}
		}
	}
	DESTROY_IF_ZERO(packetBuffer);
	

	CPacket* packetBuffer5 = new CPacket();
	MakePacketDeleteCharacter(packetBuffer5, player->playerId.value.characterId);

	for (std::unordered_set<Sector<Player*>*>::iterator iter = prevSet.begin(); iter != prevSet.end(); ++iter)
	{
		Sector<Player*>* sector = *iter;
		const auto* players = sector->GetSectorPlayers();
		if (players == nullptr)
		{
			continue;
		}
		for (Player* otherPlayer : *players)
		{
			if (InterlockedCompareExchange(&otherPlayer->isDead, 0, 0) == 1)
			{
				continue;
			}
			CPacket* packetBuffer6 = new CPacket();
			MakePacketDeleteCharacter(packetBuffer6, otherPlayer->playerId.value.characterId);
			//packetBuffer5->IncrementReferenceCount();
			//packetBuffer6->IncrementReferenceCount();
			deleteCount += 2;
			server->SendPacket(player->sessionId, packetBuffer6);
			server->SendPacket(otherPlayer->sessionId, packetBuffer5);
			DESTROY_IF_ZERO(packetBuffer6);
		}
	}
	DESTROY_IF_ZERO(packetBuffer5);

	InterlockedAdd(&log->logData.SCMoveStartCount, moveCount);
	InterlockedAdd(&log->logData.SCCreateOtherCharacterCount, createCount);
	InterlockedAdd(&log->logData.SCDeleteCharacterCount, deleteCount);

	return 1;
}

bool GameLogic::PacketProcMoveStop(Player* player, CPacket* packetBuffer)
{

	unsigned char direction;
	unsigned short x;
	unsigned short y;	
	unsigned short prevX;
	unsigned short prevY;
	*packetBuffer >> direction >> x >> y;
	DESTROY_IF_ZERO(packetBuffer);
	//LOG(LOG_LEVEL_DEBUG, L"[%s] MoveStop > Session ID : %d / Direction : %d / X : %d / Y : %d",
	//	L"DEBUG", si->sessionId, direction, x, y);

	if (player->isSynced == false && (abs(player->x - x) > dfERROR_RANGE ||
		abs(player->y - y) > dfERROR_RANGE))
	{
		CPacket* packetBuffer2 = new CPacket();
		//printf("stop sync error session : %d, direction : %d, now x : %d , now y : %d, packet x : %d, packet y : %d \n"
		//	, player->playerId.value.characterId, player->direction, player->x, player->y, x, y);
		MakePacketSync(packetBuffer2, player->playerId.value.characterId, player->x, player->y);
		//packetBuffer2->IncrementReferenceCount();
		server->SendPacket(player->sessionId, packetBuffer2);
		DESTROY_IF_ZERO(packetBuffer2);
		InterlockedIncrement(&log->logData.SCTotalSyncCount);
		player->isSynced = true;
		x = player->x;
		y = player->y;
	}
	prevX = player->x;
	prevY = player->y;
	player->x = x;
	player->y = y;
	player->direction = direction;
	player->isMoving = false;
	//movingPlayerList.erase(std::remove(movingPlayerList.begin(), movingPlayerList.end(), player), movingPlayerList.end());
	//moveSetGroup.erase(si->sessionId);
	int index = GetSectorIndex(player->x, player->y);
	if (player->currentSector != &sectors[index])
	{
		SectorUpdate(player, prevX, prevY);
	}
	CPacket* packetBuffer3 = new CPacket();
	MakePacketSCMoveStop(packetBuffer3, player->playerId.value.characterId, player->direction, player->x, player->y);
	//scmoveStopCnt += SendAround(si, packetBuffer, false);
	int count = SendAOI(player, packetBuffer3, false);
	DESTROY_IF_ZERO(packetBuffer3);
	InterlockedAdd(&log->logData.SCMoveStopCount, count);
	return true;
}


bool GameLogic::PacketProcAttack1(Player* player, CPacket* packetBuffer)
{
	unsigned char direction;
	unsigned short x;
	unsigned short y;
	*packetBuffer >> direction >> x >> y;
	DESTROY_IF_ZERO(packetBuffer);
	//LOG(LOG_LEVEL_DEBUG, L"[%s] Attack1 > Session ID : %d / Direction : %d / X : %d / Y : %d",
	//	L"DEBUG", si->sessionId, direction, x, y);

	if (player->isSynced == false && (abs(player->x - x) > dfERROR_RANGE ||
		abs(player->y - y) > dfERROR_RANGE))
	{
		CPacket* packetBuffer2 = new CPacket();
		//printf("attack1 sync error session : %d, direction : %d, now x : %d , now y : %d, packet x : %d, packet y : %d \n"
		//	, player->playerId.value.characterId, player->direction, player->x, player->y, x, y);
		MakePacketSync(packetBuffer2, player->playerId.value.characterId, player->x, player->y);
		//packetBuffer2->IncrementReferenceCount();
		server->SendPacket(player->sessionId, packetBuffer2);
		DESTROY_IF_ZERO(packetBuffer2);
		InterlockedIncrement(&log->logData.SCTotalSyncCount);
		//syncCnt += SendUnicast(si, packetBuffer);
		player->isSynced = true;
		x = player->x;
		y = player->y;
	}

	player->x = x;
	player->y = y;
	player->direction = direction;
	player->isMoving = false;

	CPacket* packetBuffer3 = new CPacket();
	MakePacketSCAttck1(packetBuffer3, player->playerId.value.characterId, player->direction, player->x, player->y);
	int count = SendAOI(player, packetBuffer3, false);
	DESTROY_IF_ZERO(packetBuffer3);
	InterlockedAdd(&log->logData.SCAttackCount, count);
	CheckDamagedPlayer(player, dfPACKET_SC_ATTACK1);

	return true;
}


bool GameLogic::PacketProcAttack2(Player* player, CPacket* packetBuffer)
{
	unsigned char direction;
	unsigned short x;
	unsigned short y;
	*packetBuffer >> direction >> x >> y;
	DESTROY_IF_ZERO(packetBuffer);
	//LOG(LOG_LEVEL_DEBUG, L"[%s] Attack2 > Session ID : %d / Direction : %d / X : %d / Y : %d",
	//	L"DEBUG", si->sessionId, direction, x, y);

	if (player->isSynced == false && (abs(player->x - x) > dfERROR_RANGE ||
		abs(player->y - y) > dfERROR_RANGE))
	{
		CPacket* packetBuffer2 = new CPacket();
		//printf("attack2 sync error session : %d, direction : %d, now x : %d , now y : %d, packet x : %d, packet y : %d \n"
		//	, player->playerId.value.characterId, player->direction, player->x, player->y, x, y);
		MakePacketSync(packetBuffer2, player->playerId.value.characterId, player->x, player->y);
		//packetBuffer2->IncrementReferenceCount();
		server->SendPacket(player->sessionId, packetBuffer2);
		DESTROY_IF_ZERO(packetBuffer2);
		InterlockedIncrement(&log->logData.SCTotalSyncCount);
		//syncCnt += SendUnicast(si, packetBuffer);
		player->isSynced = true;
		x = player->x;
		y = player->y;
	}

	player->x = x;
	player->y = y;
	player->direction = direction;
	player->isMoving = false;

	CPacket* packetBuffer3 = new CPacket();
	MakePacketSCAttck2(packetBuffer3, player->playerId.value.characterId, player->direction, player->x, player->y);
	int count = SendAOI(player, packetBuffer3, false);
	DESTROY_IF_ZERO(packetBuffer3);
	InterlockedAdd(&log->logData.SCAttackCount, count);
	CheckDamagedPlayer(player, dfPACKET_SC_ATTACK2);

	return true;
}


bool GameLogic::PacketProcAttack3(Player* player, CPacket* packetBuffer)
{
	unsigned char direction;
	unsigned short x;
	unsigned short y;
	*packetBuffer >> direction >> x >> y;
	DESTROY_IF_ZERO(packetBuffer);
	//LOG(LOG_LEVEL_DEBUG, L"[%s] Attack3 > Session ID : %d / Direction : %d / X : %d / Y : %d",
	//	L"DEBUG", si->sessionId, direction, x, y);

	if (player->isSynced == false && (abs(player->x - x) > dfERROR_RANGE ||
		abs(player->y - y) > dfERROR_RANGE))
	{
		CPacket* packetBuffer2 = new CPacket();
		//printf("attack3 sync error session : %d, direction : %d, now x : %d , now y : %d, packet x : %d, packet y : %d \n"
		//	, player->playerId.value.characterId, player->direction, player->x, player->y, x, y);
		MakePacketSync(packetBuffer2, player->playerId.value.characterId, player->x, player->y);
		//packetBuffer2->IncrementReferenceCount();
		server->SendPacket(player->sessionId, packetBuffer2);
		DESTROY_IF_ZERO(packetBuffer2);
		InterlockedIncrement(&log->logData.SCTotalSyncCount);
		//syncCnt += SendUnicast(si, packetBuffer);
		player->isSynced = true;
		x = player->x;
		y = player->y;
	}

	player->x = x;
	player->y = y;
	player->direction = direction;
	player->isMoving = false;

	CPacket* packetBuffer3 = new CPacket();
	MakePacketSCAttck3(packetBuffer3, player->playerId.value.characterId, player->direction, player->x, player->y);
	int count = SendAOI(player, packetBuffer3, false);
	DESTROY_IF_ZERO(packetBuffer3);
	InterlockedAdd(&log->logData.SCAttackCount, count);
	CheckDamagedPlayer(player, dfPACKET_SC_ATTACK3);

	return true;
}


bool GameLogic::PacketProcEcho(Player* player, CPacket* packetBuffer)
{
	DWORD time;
	*packetBuffer >> time;
	//LOG(LOG_LEVEL_DEBUG, L"[%s] Echo Message : %d time : %d",
	//	L"DEBUG", si->sessionId, time);
	DESTROY_IF_ZERO(packetBuffer);

	CPacket* packetBuffer2 = new CPacket();
	MakePacketSCEcho(packetBuffer2, time);
	//packetBuffer2->IncrementReferenceCount();
	server->SendPacket(player->sessionId, packetBuffer2);
	DESTROY_IF_ZERO(packetBuffer2);
	InterlockedIncrement(&log->logData.SCEchoCount);
	return true;
}


bool GameLogic::CheckDamagedPlayer(Player* player, int attackType)
{
	int xMin;
	int xMax;
	int yMin;
	int yMax;
	int damage;
	int count = 0;
	bool isLeft;

	switch (player->faceDirection)
	{
	case dfPACKET_MOVE_DIR_LL:
	case dfPACKET_MOVE_DIR_LU:
	case dfPACKET_MOVE_DIR_LD:
		isLeft = true;
		break;
	case dfPACKET_MOVE_DIR_RU:
	case dfPACKET_MOVE_DIR_RR:
	case dfPACKET_MOVE_DIR_RD:
		isLeft = false;
		break;
	default:
		return false;
		break;
	}

	switch (attackType)
	{
	case dfPACKET_SC_ATTACK1:
	{
		SEARCH_ATTACK_RANGE(isLeft, xMax, xMin, yMax, yMin, dfATTACK1_RANGE_X, dfATTACK1_RANGE_Y);
		damage = dfATTACK1_DAMAGE;
	}
	break;
	case dfPACKET_SC_ATTACK2:
	{
		SEARCH_ATTACK_RANGE(isLeft, xMax, xMin, yMax, yMin, dfATTACK2_RANGE_X, dfATTACK2_RANGE_Y);
		damage = dfATTACK2_DAMAGE;
	}
	break;
	case dfPACKET_SC_ATTACK3:
	{
		SEARCH_ATTACK_RANGE(isLeft, xMax, xMin, yMax, yMin, dfATTACK3_RANGE_X, dfATTACK3_RANGE_Y);
		damage = dfATTACK3_DAMAGE;
	}
	break;
	default:
		return false;
		break;
	}

	// 공격 시 내 섹터와 내 정면의 섹터만 범위로 한정하도록 변경
	// 내 섹터
	const std::unordered_set<Player*>* players = player->currentSector->GetSectorPlayers();
	if (players != nullptr)
	{
		for (Player* p : *players)
		{
			if (p == player)
			{
				continue;
			}
			if (p->x < xMin || p->x > xMax ||
				p->y < yMin || p->y > yMax)
			{
				continue;
			}
			if (p->hp < damage)
			{
				p->hp = 0;
			}
			else
			{
				p->hp -= damage;
			}
			//LOG(LOG_LEVEL_DEBUG, L"[%s] Attacked > Session ID : %d / left hp : %d / X : %d / Y : %d",
			//	L"DEBUG", (*iter)->sessionId, (*iter)->hp, (*iter)->x, (*iter)->y);
			CPacket* packetBuffer = new CPacket();
			MakePacketSCDamage(packetBuffer, player->playerId.value.characterId, p->playerId.value.characterId, p->hp);
			//packetBuffer->IncrementReferenceCount();
			count = SendAOI(p, packetBuffer, true);
			//server->SendPacket();
			DESTROY_IF_ZERO(packetBuffer);
			InterlockedAdd(&log->logData.SCDamagedCount, count);
			return true;
		}
	}

	if (isLeft)
	{
		int index = GetSectorIndex((player->x - sectorTileSize), player->y);
		if (index == -1)
		{
			return true;
		}
		players = sectors[index].GetSectorPlayers();
	}
	else
	{
		int index = GetSectorIndex((player->x + sectorTileSize), player->y);
		if (index == -1)
		{
			return true;
		}
		players = sectors[index].GetSectorPlayers();
	}
	if (players != nullptr)
	{
		for (Player* p : *players)
		{
			if (p->x < xMin || p->x > xMax ||
				p->y < yMin || p->y > yMax)
			{
				continue;
			}
			if (p->hp < damage)
			{
				p->hp = 0;
			}
			else
			{
				p->hp -= damage;
			}
			//LOG(LOG_LEVEL_DEBUG, L"[%s] Attacked > Session ID : %d / left hp : %d / X : %d / Y : %d",
			//	L"DEBUG", (*iter)->sessionId, (*iter)->hp, (*iter)->x, (*iter)->y);
			CPacket* packetBuffer = new CPacket();
			MakePacketSCDamage(packetBuffer, player->playerId.value.characterId, p->playerId.value.characterId, p->hp);
			//packetBuffer->IncrementReferenceCount();
			count = SendAOI(p, packetBuffer, true);
			DESTROY_IF_ZERO(packetBuffer);
			InterlockedAdd(&log->logData.SCDamagedCount, count);
			return true;
		}
	}

	return true;
}

int GameLogic::UpdateGameWorld()
{
	int timer = timeGetTime();
	unsigned short prevX;
	unsigned short prevY;
	const size_t chunkSize = 3000;
	size_t totalPlayer = activePlayerList.size();
	size_t totalMove = movingPlayerList.size();
	std::vector<Player*>::iterator iter;
	Player* player;
	int idx;
	size_t processed = 0;
	for (size_t i = 0; processed < chunkSize && i < totalPlayer; ++i)
	{
		idx = (aliveCheckCursor + i) % totalPlayer;
		player = activePlayerList[idx];
		if ((timer > player->lastRecvTime && timer > player->lastRecvTime + dfNETWORK_PACKET_RECV_TIMEOUT)
			|| player->hp <= 0)
		{
			InterlockedExchange(&player->isDead, 1);
			player->isMoving = false;
		}
		++processed;
	}
	if (totalPlayer > 0)
	{
		aliveCheckCursor = (aliveCheckCursor + processed) % totalPlayer;
	}
	
	processed = 0;
	for (size_t i = 0; processed < chunkSize && i < totalMove; ++i)
	{
		idx = (moveCursor + i) % totalMove;
		player = movingPlayerList[idx];
		if (InterlockedCompareExchange(&player->isDead, 0, 0) == 1 || player->isMoving == false)
		{
			continue;
		}
		prevX = player->x;
		prevY = player->y;
		switch (player->direction)
		{
		case dfPACKET_MOVE_DIR_LL:
		{
			UPDATE_MOVE_CHARACTER(player, player->x - dfSPEED_PLAYER_X, player->y);
		}
		break;
		case dfPACKET_MOVE_DIR_LU:
		{
			UPDATE_MOVE_CHARACTER(player, player->x - dfSPEED_PLAYER_X, player->y - dfSPEED_PLAYER_Y);
		}
		break;
		case dfPACKET_MOVE_DIR_UU:
		{
			UPDATE_MOVE_CHARACTER(player, player->x, player->y - dfSPEED_PLAYER_Y);
		}
		break;
		case dfPACKET_MOVE_DIR_RU:
		{
			UPDATE_MOVE_CHARACTER(player, player->x + dfSPEED_PLAYER_X, player->y - dfSPEED_PLAYER_Y);
		}
		break;
		case dfPACKET_MOVE_DIR_RR:
		{
			UPDATE_MOVE_CHARACTER(player, player->x + dfSPEED_PLAYER_X, player->y);
		}
		break;
		case dfPACKET_MOVE_DIR_RD:
		{
			UPDATE_MOVE_CHARACTER(player, player->x + dfSPEED_PLAYER_X, player->y + dfSPEED_PLAYER_Y);
		}
		break;
		case dfPACKET_MOVE_DIR_DD:
		{
			UPDATE_MOVE_CHARACTER(player, player->x, player->y + dfSPEED_PLAYER_Y);
		}
		break;
		case dfPACKET_MOVE_DIR_LD:
		{
			UPDATE_MOVE_CHARACTER(player, player->x - dfSPEED_PLAYER_X, player->y + dfSPEED_PLAYER_Y);
		}
		break;
		default:
			break;
		}

		int index = GetSectorIndex(player->x, player->y);
		if (player->currentSector != &sectors[index])
		{
			SectorUpdate(player, prevX, prevY);
		}
		++processed;
	}
	if (totalMove > 0)
	{
		moveCursor = (moveCursor + processed) % totalMove;
	}
	
	return 1;
}

//long tt = 0;
bool GameLogic::FlushFrame()
{
	std::vector<Player*>::iterator iter;
	Player* player;
	for (iter = activePlayerList.begin(); iter != activePlayerList.end(); ++iter)
	{
		player = *iter;
		if (InterlockedCompareExchange(&player->isDead, 0, 0) == 1 && InterlockedCompareExchange(&player->isDestroyRequest, 1, 0) == 0)
		{
			//printf("logic쪽의 연결끊기 요청 sessionId : %d \n", player->sessionId);
			server->OnClientLeave(player->sessionId);
		}
	}

	movingPlayerList.erase(std::remove_if(movingPlayerList.begin(), movingPlayerList.end(),
		[](Player* p) {return (p == nullptr|| !p->isMoving || p->isDead); }), movingPlayerList.end());

	while (destroyPlayerRequestQueue.GetSize() != 0)
	{
		SESSIONID sessionId;
		int res = destroyPlayerRequestQueue.Dequeue(sessionId);
		player = playerList[sessionId.value.index];
		//삭제 패킷 전송
		CPacket* packetBuffer = new CPacket();
		MakePacketDeleteCharacter(packetBuffer, player->playerId.value.characterId);		
		int count = SendAOI(player, packetBuffer, false);
		InterlockedAdd(&log->logData.SCDeleteCharacterCount, count);
		InterlockedDecrement(&log->logData.PlayerCount);
		//tt++;
		if (player->currentSector)
		{
			player->currentSector->Leave(player);
		}
		DESTROY_IF_ZERO(packetBuffer);
		activePlayerList.erase(std::remove(activePlayerList.begin(), activePlayerList.end(),
			player), activePlayerList.end());
		int idx = player->playerId.value.index;
		playerList[player->playerId.value.index] = nullptr;
		_aligned_free(player);
		server->DelayFreeSessionMapIndex(idx);
	}


	return 1;
}


int GameLogic::CountAlivePlayers()
{
	int count = 0;
	for (size_t i = 0; i < playerList.size(); ++i)
	{
		if (playerList[i] != nullptr)
			++count;
	}
	return count;
}