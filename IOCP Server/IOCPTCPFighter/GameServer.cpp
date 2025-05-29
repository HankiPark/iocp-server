
#pragma comment(lib, "ws2_32")

#include <winsock2.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#include <iostream>
#include <algorithm>
#include <process.h>
#include "Struct.h"
#include <windows.h>
#include "RingBuffer.h"
#include "BufferClass.h"
#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "GameLogic.h"
#include "Logging.h"
#include "GameServer.h"





bool GameServer::StartThread()
{
    HANDLE cp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, setData.threadCount);
    completionPort = cp;
    HANDLE acceptThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)GameServer::RunAcceptThread, this, 0, nullptr);
    workerThreads.push_back(acceptThread);

    for (int i = 0; i < setData.runningThreadCount; i++)
    {
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)GameServer::RunWorkerThread, this, 0, nullptr);
        if (hThread == NULL)
        {
            return false;
        }
        workerThreads.push_back(hThread);
    }
    HANDLE sendThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)GameServer::RunSendThread, this, 0, nullptr);

    workerThreads.push_back(sendThread);    
    HANDLE logicThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)GameServer::RunLogicThread, this, 0, nullptr);

    workerThreads.push_back(logicThread);    
    
    HANDLE deleteThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)GameServer::RunSessionDestroyThread, this, 0, nullptr);

    workerThreads.push_back(deleteThread);

    HANDLE monitorThread = (HANDLE)_beginthreadex(NULL, 0, (_beginthreadex_proc_type)GameServer::RunMonitorThread, this, 0, nullptr);

    workerThreads.push_back(monitorThread);

    return true;
}


DWORD WINAPI GameServer::WorkerThread()
{
    int awake = 0;
    int retval;
    const int MAX_EVENT_COUNT = 500;
    int processCount = 0;
    HANDLE hcp = completionPort;

    while (1)
    {
        DWORD cbTransferred = 0;
        SESSION* session = nullptr;
        OVERLAPPED* overlapped = nullptr;

        retval = GetQueuedCompletionStatus(hcp, &cbTransferred,
            (PULONG_PTR)&session, (LPOVERLAPPED*)&overlapped, INFINITE);
        if (session == nullptr && overlapped == nullptr && cbTransferred == 0)
        {
            break;
        }
        else if (InterlockedCompareExchange(&session->isDeleteRequest, 0, 0) == 1)
        {
        }
        else if (cbTransferred == 0)
        {
            // send thread에서 보낼 것이 없을 때 보내는 0byte send
            if (&session->sendWSAOverlapped.sendWSAOverlapped == overlapped)
            {
                InterlockedExchange(&session->canSend, 1);
            }
        }
        else if (&session->recvWSAOverlapped == overlapped)
        {
            InterlockedIncrement(&log.logData.IocpLoopCount);
            SESSION* ses = sessionMap[session->sessionId.value.index];
            if (ses == nullptr)
            {
                continue;
            }
            if (session->sessionId.originValue != ses->sessionId.originValue)
            {
                continue;
            }
            session->recvRingBuffer.MoveTail(cbTransferred);
            
            OnMessage(session, cbTransferred);
            InterlockedIncrement(&log.logData.RecvCount);
            InterlockedAdd(&log.logData.RecvByte, cbTransferred);

            RecvPost(session);
        }
        else if (&session->sendWSAOverlapped.sendWSAOverlapped == overlapped)
        {
            InterlockedIncrement(&log.logData.IocpLoopCount);
            SESSION* ses = sessionMap[session->sessionId.value.index];
            if (ses == nullptr)
            {
                continue;
            }
            if (session->sessionId.originValue != ses->sessionId.originValue)
            {
                continue;
            }
            
            int count = session->sendWSAOverlapped.count;
            DWORD transfer = 0;
            int i = 0;
            //여기에 overlapped으로 받은 직렬화 버퍼 참조 카운트 감소 + 삭제
            for (i = 0; i < count; i++)
            {
                CPacket* packetBuffer = reinterpret_cast<CPacket*>(session->sendWSAOverlapped.ptr[i]);
                transfer += packetBuffer->GetUsedSize();
                if (transfer > cbTransferred)
                {
                    break;
                }
                session->sendWaitingSize -= packetBuffer->GetUsedSize();
                session->sendWSAOverlapped.count--;
                DESTROY_IF_ZERO(packetBuffer);
            }
            //전송실패한 데이터를 다시 붙여줌. (send queue에 넣지 않음)
            for (int j = i; j < count; j++)
            {
                session->sendWSAOverlapped.ptr[j - i] = session->sendWSAOverlapped.ptr[j];
                session->sendWSAOverlapped.wsabuf[j - i] = session->sendWSAOverlapped.wsabuf[j];
            }

            // EnterCriticalSection(&session->sendRingBufferCSLock);
            if (transfer != cbTransferred || session->sendQueue.GetSize() > 0)
            {
                //InterlockedIncrement(&session->ioCount);
                session->sendQueuingTime = timeGetTime();
                sendRequestQueue.Enqueue(session->sessionId);
            }
            else
            {
                //session->sendQueuingTime = 0;
                InterlockedExchange(&session->canSend, 1);
            }

            //session->sendRingBuffer.MoveHead(cbTransferred);

            // LeaveCriticalSection(&session->sendRingBufferCSLock);
             //printf("send : %d\n", cbTransferred);
            InterlockedAdd(&log.logData.SendByte, cbTransferred);
            
        }
        int ch = 0;
        if ((ch = InterlockedDecrement(&session->ioCount)) == 0)
        {
            if (ch == -1)
            {
                __debugbreak();
            }
            if (InterlockedCompareExchange(&session->isDeleteRequest, 1, 0) == 0)
            {
                destroySessionRequestQueue.Enqueue(session->sessionId);
            }

            //OnRelease(session);
        }

        ++processCount;
        if (processCount >= MAX_EVENT_COUNT)
        {
            Sleep(0); 
            processCount = 0; 
        }

    }
    return 0;
}


bool GameServer::OnConnectRequest(IN_ADDR ip, unsigned short port)
{
    return true;
}

int GameServer::OnAccept(SOCKET clientSock, SOCKADDR_IN addr)
{

    //DWORD flags = 0;
    int o = 0;
    //int retval;
    setsockopt(clientSock, SOL_SOCKET, SO_SNDBUF, (const char*)&o, sizeof(int));

    SESSION* session = (SESSION*)_aligned_malloc(sizeof(SESSION), 64);
    new (session) SESSION();

    if (session == NULL)
    {
        __debugbreak();
    }
    ZeroMemory(&session->sendWSAOverlapped, sizeof(session->sendWSAOverlapped));
    ZeroMemory(&session->recvWSAOverlapped, sizeof(session->recvWSAOverlapped));
    session->sock = clientSock;
    session->ip = addr.sin_addr;
    session->port = ntohs(addr.sin_port);
    session->ioCount = 0;
    session->canSend = 1;
    session->handle = 0;
    session->isDisconnectRequest = false;
    session->isDeleteRequest = 0;
    session->lastSendTime = 0;
    session->sessionId.value.uniqueId = InterlockedIncrement(&sessionId) % 0x1FFFF;
    session->sessionId.value.index = emptySessionMapStack.Pop();
    sessionMap[session->sessionId.value.index] = session;
    logicThread.PushNewSession(session->sessionId);
    //newSessionQueue->Enqueue(session->sessionId);
    InterlockedIncrement(&log.logData.SessionCount);
    
    CreateIoCompletionPort((HANDLE)clientSock, completionPort, (ULONG_PTR)session, 0);

    RecvPost(session);

    return 1;
}

void GameServer::OnClientJoin(SESSIONID sessionId, IN_ADDR ip, USHORT port)
{

}
//long k = 0;
int GameServer::OnRelease(SESSION* session)
{
    //k++;
    InterlockedDecrement(&log.logData.SessionCount);
    InterlockedIncrement(&log.logData.DisconnectCount);

    int index = session->sessionId.value.index;
    SESSIONID sessionId = session->sessionId;
    SOCKET socket = session->sock;
    //sessionMap[index] = nullptr;
    InterlockedExchangePointer((void**) & sessionMap[index], nullptr);
    
    session->sock = INVALID_SOCKET;
    session->FlushSendPacket();
    session->~SESSION();
    _aligned_free(session);

    //emptySessionMapStack.Push(index);
    if (closesocket(socket) != 0)
    {
        int v = WSAGetLastError();
        __debugbreak();
    }
    logicThread.RequestDestroyPlayer(sessionId);
    return 0;
}

void GameServer::OnClientLeave(SESSIONID sessionId)
{
    SESSION* session = sessionMap[sessionId.value.index];
    if (session == nullptr)
    {
        return;
    }
    if (session->sessionId.originValue != sessionId.originValue)
    {
        return;
    }
    if (InterlockedCompareExchange(&session->isDeleteRequest, 1, 0) == 0)
    {
        destroySessionRequestQueue.Enqueue(sessionId);
    }
    return;
}


void GameServer::OnRecv(SESSIONID sessionId, CPacket* packetBuffer)
{
    /*char buf[10000] = { 0, };
    int dataLen = packetBuffer->GetUsedSize();
    packetBuffer->GetStructData(buf, dataLen);
    packetBuffer->ClearPacket();
    packetBuffer->MoveInsideStructData(buf, dataLen);
    SendPacket(sessionId, packetBuffer);*/
    return;
}

int GameServer::OnMessage(SESSION* session, int cbTransferred)
{
    
    while (1)
    {
        if (session->recvRingBuffer.GetUseSize() < sizeof(PacketHeaderInfo))
        {
            break;
        }
       
        //헤더 검사
        PacketHeaderInfo phi;
        int retval;
        retval = session->recvRingBuffer.Peek((char*)&phi, sizeof(PacketHeaderInfo));
        if (retval != sizeof(PacketHeaderInfo))
        {
            //LOG(LOG_LEVEL_ERROR, L"[%s] %s", L"ERROR", L"recvbuffer peek Fail");
            __debugbreak();
        }
        //헤더가 잘못됨
        if (phi.byCode != 0x89)
        {
            break;
        }
        // 헤더에서 명시한 길이만큼 충분히 recv했는지 확인
        if (sizeof(PacketHeaderInfo) + phi.bySize > session->recvRingBuffer.GetUseSize())
        {
            break;
        }
        int recvDequeueRetval = session->recvRingBuffer.MoveHead(sizeof(PacketHeaderInfo));
        if (recvDequeueRetval != sizeof(PacketHeaderInfo))
        {
           // LOG(LOG_LEVEL_ERROR, L"[%s] %s", L"ERROR", L"recvbuffer movehead Fail");
            __debugbreak();
        }
        InterlockedIncrement(&log.logData.RecvPacketCount);
        CPacket* packetBuffer = new CPacket();
        IncomingPacketInfo ipi;
        if (session->recvRingBuffer.DirectDequeueSize() >= phi.bySize)
        {
            retval = packetBuffer->MoveInsideStructData(session->recvRingBuffer.GetHeadBufferPtr(), session->recvRingBuffer.DirectDequeueSize());
            if (retval != session->recvRingBuffer.DirectDequeueSize())
            {
                __debugbreak();
            }
            session->recvRingBuffer.MoveHead(retval);
        }
        else
        {
            int temp = session->recvRingBuffer.DirectDequeueSize();
            retval = packetBuffer->MoveInsideStructData(session->recvRingBuffer.GetHeadBufferPtr(), temp);
            session->recvRingBuffer.MoveHead(retval);
            retval = packetBuffer->MoveInsideStructData(session->recvRingBuffer.GetHeadBufferPtr(), phi.bySize - retval);
            session->recvRingBuffer.MoveHead(retval);
        }
        ipi.packetBuffer = packetBuffer;
        ipi.packetType = phi.byType;
        ipi.playerId = session->playerId;
        logicThread.PushIncomingPacket(ipi);
        //packetQueue->Enqueue(ipi);
    }


    return 1;
}

void GameServer::OnWorkerThreadBegin() {};

void GameServer::OnWorkerThreadEnd() {};

void GameServer::OnError(int errorCode, WCHAR* str) {};




DWORD WINAPI GameServer::MonitorThread()
{
    long long timer = timeGetTime();
    
    log.SettingLog();
    while (1)
    {
        if (timeGetTime() - timer > 1000)
        {
            timer += 1000;
            log.ReopenLogFile();
            GAMELOG(L"--------------------------------------------------------\n");
            GAMELOG(L"Network Loop : %d       Logic Loop : %d\n", 
                InterlockedExchange(&log.logData.IocpLoopCount, 0), InterlockedExchange(&log.logData.LogicLoopCount, 0));
            GAMELOG(L"Total Session : %d       Total Player : %d       Total Sync : %d\n", 
                log.logData.SessionCount, log.logData.PlayerCount, log.logData.SCTotalSyncCount);
            GAMELOG(L"Accept : %d       Disconnect : %d       Total DelayedPacket : %d\n\n", 
                InterlockedExchange(&log.logData.AcceptCount, 0), InterlockedExchange(&log.logData.DisconnectCount, 0), log.logData.DelayedPacket);
            GAMELOG(L"recv Byte : %d       recv Count : %d       recv Packet Count : %d\n", 
                InterlockedExchange(&log.logData.RecvByte, 0), InterlockedExchange(&log.logData.RecvCount, 0), InterlockedExchange(&log.logData.RecvPacketCount, 0));
            GAMELOG(L"[Client -> Server]\n");
            GAMELOG(L"MoveStart Packet : %d       MoveStop Packet : %d\n", 
                InterlockedExchange(&log.logData.CSMoveStartCount, 0), InterlockedExchange(&log.logData.CSMoveStopCount, 0));
            GAMELOG(L"Attack Packet : %d       Echo Packet : %d\n\n", 
                InterlockedExchange(&log.logData.CSAttackCount, 0), InterlockedExchange(&log.logData.CSEchoCount, 0));
            GAMELOG(L"send Byte : %d       send Count : %d       send Packet Count : %d\n", 
                InterlockedExchange(&log.logData.SendByte, 0), InterlockedExchange(&log.logData.SendCount, 0), InterlockedExchange(&log.logData.SendPacketCount, 0));
            GAMELOG(L"[Server -> Client]\n");
            GAMELOG(L"CreateMyCharacter Packet : %d       CreateOtherCharacter Packet : %d       DeleteCharacter Packet : %d\n",
                InterlockedExchange(&log.logData.SCCreateMyCharacterCount, 0), InterlockedExchange(&log.logData.SCCreateOtherCharacterCount, 0),
                InterlockedExchange(&log.logData.SCDeleteCharacterCount, 0));
            GAMELOG(L"MoveStart Packet : %d       MoveStop Packet : %d\n", 
                InterlockedExchange(&log.logData.SCMoveStartCount, 0), InterlockedExchange(&log.logData.SCMoveStopCount, 0));
            GAMELOG(L"Attack Packet : %d       Damaged Packet : %d       Echo Packet : %d\n", 
                InterlockedExchange(&log.logData.SCAttackCount, 0), InterlockedExchange(&log.logData.SCDamagedCount, 0), 
                InterlockedExchange(&log.logData.SCEchoCount, 0));
            GAMELOG(L"--------------------------------------------------------\n");

        }
        if (WaitForSingleObject(endEvent, 1000) == WAIT_OBJECT_0)
        {
            break;
        }


    }
    return 1;
}

DWORD WINAPI GameServer::SendThread()
{
    unsigned int timer = timeGetTime();
    while (1)
    {
        if (timer + 40 >= timeGetTime())
        {
            continue;
        }
        timer += 40;

        SESSIONID sessionId;
        SESSION* session;
        int queueSize = sendRequestQueue.GetSize();
        int processCount = 0;
        if (queueSize == 0)
        {
            Sleep(0);
            continue;
        }
        if (queueSize > 1000)
        {
            queueSize = 1000;
        }
        while (++processCount <= queueSize)
        {
            int k = sendRequestQueue.Dequeue(sessionId);
            if (k == -1)
            {
                __debugbreak();
            }
            session = sessionMap[sessionId.value.index];
            if (session == nullptr)
            {
                continue;
            }
            // 이미 재사용된 session
            // session의 isSend도 초기화되었을테니 무시
            if (session->sessionId.originValue != sessionId.originValue)
            {
                continue;
            }


            // 보낼 데이터가 없는 상황 발생
            if (session->sendQueue.GetSize() + session->sendWSAOverlapped.count == 0)
            {
                DWORD flags = 0;
                WSABUF dummyBuf = { 0, nullptr };
                // 0byte send
                InterlockedIncrement(&session->ioCount);
                //session->lastSendTime = timeGetTime();
                int retval = WSASend(session->sock, &dummyBuf, 0, NULL,
                    flags, &session->sendWSAOverlapped.sendWSAOverlapped, NULL);
                if (retval == SOCKET_ERROR)
                {
                    if (WSAGetLastError() != ERROR_IO_PENDING)
                    {
                        int v = WSAGetLastError();
                        if (WSAGetLastError() == WSAECONNABORTED || WSAGetLastError() == WSAECONNRESET
                            || WSAGetLastError() == WSAENOTSOCK)
                        {
                        }
                        else
                        {
                            __debugbreak();
                        }
                        int ch = 0;
                        if ((ch = InterlockedDecrement(&session->ioCount)) == 0)
                        {
                            if (ch == -1)
                            {
                                __debugbreak();
                            }
                            if (InterlockedCompareExchange(&session->isDeleteRequest, 1, 0) == 0)
                            {
                                destroySessionRequestQueue.Enqueue(session->sessionId);
                            }

                        }
                    }
                }
                continue;
            }
            int timer = timeGetTime();
            //timer > session->lastSendTime + 1000 &&
            if ((timer >= session->sendQueuingTime + setData.maxWaitSendTime
                || session->sendWaitingSize >= setData.maxWaitSendSize
                || session->sendWSAOverlapped.count + session->sendQueue.GetSize() >= setData.maxWaitSendCount))
            {
                InterlockedIncrement(&session->ioCount);
                SendPost(session);
            }
            else
            {
                sendRequestQueue.Enqueue(session->sessionId);
            }
        }
        Sleep(5);
        //if (WaitForSingleObject(endEvent, 0) == WAIT_OBJECT_0)
        //{
        //    break;
        //}
    }
    return 1;
}

DWORD WINAPI GameServer::StartLogicThread()
{
    logicThread.StartLogicThread();
    return 1;
}
//long qq = 0;
//int cnt = 0;
//int mismatch = 0;
DWORD WINAPI GameServer::SessionDestroyThread()
{
    
    while (1)
    {
        int deleteCount = destroySessionRequestQueue.GetSize();
        int processCount = 0;
        while (++processCount <= deleteCount)
        {
            if (deleteCount != 0)
            {
                SESSIONID sessionId;
                int res = destroySessionRequestQueue.Dequeue(sessionId);
                SESSION* session = sessionMap[sessionId.value.index];
                if (session == nullptr || session->sessionId.originValue != sessionId.originValue)
                {
                   // printf("Release error(null), index : %d Count : %d\n", sessionId.value.index, ++cnt);
                    continue;
                }             
                //if (session == nullptr)
                //{
                //   // printf("Release error(null), index : %d Count : %d\n", sessionId.value.index, ++cnt);
                //    continue;
                //}                
                //if (session->sessionId.originValue != sessionId.originValue)
                //{
                //    //printf("Release error(value exchange), index : %d Count : %d\n", sessionId.value.index, ++mismatch);
                //    continue;
                //}
                if (session->isDisconnectRequest == false)
                {
                    session->isDisconnectRequest = true;
                    //InterlockedIncrement(&session->ioCount);
                    //logicThread.RequestDestroyPlayer(session->sessionId);
                    if (CancelIoEx((HANDLE)session->sock, &session->recvWSAOverlapped) == 0)
                    {
                        int v = GetLastError();
                        if (v != ERROR_NOT_FOUND)
                        {
                            __debugbreak();
                        }
                    }
                }
                if (InterlockedCompareExchange(&session->ioCount, 0, 0) != 0)
                {
                    destroySessionRequestQueue.Enqueue(session->sessionId);
                    continue;
                }
                OnRelease(session);
            }
        }
        Sleep(10);
    }
    return 1;
}

DWORD WINAPI GameServer::AcceptThread()
{
    HANDLE hcp = completionPort;
    int retval;
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET)
        __debugbreak();

    listenSock = listenSocket;

    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(setData.openIp.s_addr);
    serveraddr.sin_port = htons(setData.port);
    retval = bind(listenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR)
    {
        int v = WSAGetLastError();
        __debugbreak();
    }

    linger li = { 1, 0 };
    setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, (const char*)&li, sizeof(linger));
    retval = listen(listenSocket, SOMAXCONN_HINT(65535));
    if (retval == SOCKET_ERROR)
        __debugbreak();

    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen;
    int bufSize = 0;
    int processCount = 0;
    const int MAX_COUNT = 200;
    while (1)
    {
        addrlen = sizeof(clientaddr);
        client_sock = accept(listenSocket, (SOCKADDR*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET)
        {
            printf("accept error : %d \n", WSAGetLastError());
            break;
        }
        InterlockedIncrement(&log.logData.AcceptCount);
        OnAccept(client_sock, clientaddr);
        if (++processCount >= MAX_COUNT)
        {
            processCount = 0;
            Sleep(5);
        }
    }

    return 1;
}

bool GameServer::FlushNeeded(SESSION* session)
{
    int timer = timeGetTime();
    if (timer >= session->sendQueuingTime + setData.maxWaitSendTime
        || session->sendWaitingSize >= setData.maxWaitSendSize
        || session->sendWSAOverlapped.count + session->sendQueue.GetSize() >= setData.maxWaitSendCount)
    {
        return true;
    }
    return false;
}

bool GameServer::RegisterSessionPlayer(unsigned int sessionIndex, PLAYERID playerId)
{
    SESSION* session = sessionMap[sessionIndex];
    if (session == nullptr)
    {
        // 세션이 이미 종료되었거나 재사용됨 → player 폐기
        
        return false;
    }
    sessionMap[sessionIndex]->playerId.originValue = playerId.originValue;
    return true;
}

bool GameServer::AddRefSession(SESSIONID id)
{
    SESSION* s = sessionMap[id.value.index];
    if (!s || s->sessionId.originValue != id.originValue)
    {
        return false;
    }

    if (InterlockedIncrement(&s->ioCount) <= 1)
    {
        int ch = 0;
        ch = InterlockedDecrement(&s->ioCount);
        if (ch == -1)
        {
            __debugbreak();
        }
        return false;
    }

    return true;
}

void GameServer::ReleaseRefSession(SESSIONID id)
{
    SESSION* s = sessionMap[id.value.index];
    if (!s || s->sessionId.originValue != id.originValue)
        return;
    int ch = 0;
    if ((ch = InterlockedDecrement(&s->ioCount)) == 0)
    {
        if (ch == -1)
        {
            __debugbreak();
        }
        // 세션 삭제 요청 큐에 enqueue
        destroySessionRequestQueue.Enqueue(s->sessionId);  // or sessionId
    }


    return;
}

void GameServer::DelayFreeSessionMapIndex(unsigned int index)
{
    emptySessionMapStack.Push(index);
    return;
}

