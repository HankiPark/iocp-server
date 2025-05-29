#pragma once
#include "struct.h"
#include "LanServer.h"
#include "MPSCQueue.h"
#include "GameLogic.h"


class GameServer : public LanServer
{
public:
    GameServer() {};
    ~GameServer() {};


    bool OnConnectRequest(IN_ADDR ip, unsigned short port);

    void OnClientJoin(SESSIONID sessionId, IN_ADDR ip, USHORT port);
    int OnAccept(SOCKET clientSock, SOCKADDR_IN addr);

    void OnClientLeave(SESSIONID sessionId);
    int OnRelease(SESSION* session);

    void OnRecv(SESSIONID sessionId, CPacket* packetBuffer);

    int OnMessage(SESSION* session, int cbTransferred);

    void OnWorkerThreadBegin();
    void OnWorkerThreadEnd();

    void OnError(int errorCode, WCHAR* str);



    int GetSectorCols() const { return setData.sectorCol; }
    int GetSectorRows() const { return setData.sectorRow; }
    int GetSectorTileSize() const { return setData.sectorTileSize; }
    int GetMaxSessionSize() const { return setData.maxSessionCount; }
    Log* GetLogPointer() { return &log; }
 
    //void SetRecvQueue(MPSCQueue<IncomingPacketInfo>* queue) { packetQueue = queue; }
    //MPSCQueue<IncomingPacketInfo>* GetRecvQueue() const { return packetQueue; }

    bool RegisterSessionPlayer(unsigned int sessionIndex, PLAYERID playerId);

    bool AddRefSession(SESSIONID id);
    void ReleaseRefSession(SESSIONID id);
    void DelayFreeSessionMapIndex(unsigned int index);

    int CountAliveSessions();


private:
    bool StartThread();
    static DWORD WINAPI RunWorkerThread(LPVOID arg)
    {
        GameServer* pThis = reinterpret_cast<GameServer*>(arg);

        pThis->WorkerThread();
        return 0;
    }
    static DWORD WINAPI RunAcceptThread(LPVOID arg)
    {
        GameServer* pThis = reinterpret_cast<GameServer*>(arg);

        pThis->AcceptThread();
        return 0;
    }
    static DWORD WINAPI RunMonitorThread(LPVOID arg)
    {
        GameServer* pThis = reinterpret_cast<GameServer*>(arg);

        pThis->MonitorThread();
        return 0;
    }
    static DWORD WINAPI RunSendThread(LPVOID arg)
    {
        GameServer* pThis = reinterpret_cast<GameServer*>(arg);

        pThis->SendThread();
        return 0;
    }
    static DWORD WINAPI RunLogicThread(LPVOID arg)
    {
        GameServer* pThis = reinterpret_cast<GameServer*>(arg);

        pThis->StartLogicThread();
        return 0;
    }
    static DWORD WINAPI RunSessionDestroyThread(LPVOID arg)
    {
        GameServer* pThis = reinterpret_cast<GameServer*>(arg);

        pThis->SessionDestroyThread();
        return 0;
    }

    DWORD WINAPI AcceptThread();
    DWORD WINAPI WorkerThread();
    DWORD WINAPI MonitorThread();
    DWORD WINAPI SendThread();
    DWORD WINAPI StartLogicThread();
    DWORD WINAPI SessionDestroyThread();

    bool FlushNeeded(SESSION* session);

    


private:
    unsigned long long sessionId = 0;
    GameLogic logicThread{ this };
};
