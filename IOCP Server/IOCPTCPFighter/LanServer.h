#pragma once
#include "BufferClass.h"
#include "LockFreeStack.h"
#include "MPSCQueue.h"
#include "Struct.h"
#include "Logging.h"

class LanServer
{
public:
    LanServer() {};
    virtual ~LanServer() {};

    bool Start();
    void Stop();
    int GetSessionCount() const;

    bool Disconnect(SESSIONID sessionId);
    bool SendPacket(SESSIONID sessionId, CPacket* packetBuffer);

    virtual bool OnConnectRequest(IN_ADDR ip, unsigned short port) = 0;

    virtual void OnClientJoin(SESSIONID sessionId, IN_ADDR ip, USHORT port) = 0;
    virtual int OnAccept(SOCKET clientSock, SOCKADDR_IN addr) = 0;


    virtual void OnClientLeave(SESSIONID sessionId) = 0;
    virtual int OnRelease(SESSION* session) = 0;

    virtual void OnRecv(SESSIONID sessionId, CPacket* packetBuffer) = 0;

    virtual int OnMessage(SESSION* session, int cbTransferred) = 0;

    virtual void OnWorkerThreadBegin() = 0;
    virtual void OnWorkerThreadEnd() = 0;

    virtual void OnError(int errorCode, WCHAR* str) = 0;

   // int GetAcceptTPS();
   // int GetRecvMessageTPS();
   // int GetSendMessageTPS();

    int RecvPost(SESSION* session);
    int SendPost(SESSION* session);

    int CountAliveSessions();
    SESSION* TryAcquireSession(SESSIONID id)
    {
        SESSION* s = (SESSION*)InterlockedCompareExchangePointer((void**)&sessionMap[id.value.index], nullptr, nullptr);
        if (!s || s->sessionId.originValue != id.originValue)
            return nullptr;

        if (InterlockedIncrement(&s->ioCount) <= 0)
        {
            // 이미 삭제 직전
            InterlockedDecrement(&s->ioCount);

            return nullptr;
        }

        // double-check는 생략 가능 (순서상 안전)
        return s;
    }
protected:
    virtual bool StartThread() = 0;
protected:
    std::vector<HANDLE> workerThreads;
    //std::unordered_map<int, SESSION*> sessionMap;
    std::vector<SESSION*> sessionMap;
    LockFreeStack<int> emptySessionMapStack;
    MPSCQueue<SESSIONID> sendRequestQueue;
    MPSCQueue<SESSIONID> destroySessionRequestQueue;
    Log log;
    //MPSCQueue<SESSIONID>* newSessionQueue = nullptr;
    //MPSCQueue<IncomingPacketInfo>* packetQueue = nullptr;
    //CRITICAL_SECTION sessionMapStackCSLock;

    Setting setData;
    HANDLE endEvent = 0;
    SOCKET listenSock = 0;
    HANDLE completionPort = 0;
    
};
