
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <process.h>


#include "Sector.h"
#include <windows.h>
#include "RingBuffer.h"
#include "BufferClass.h"
#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "Parser.h"
#include "Struct.h"
#include "Logging.h"
#include "LanServer.h"



bool LanServer::Start()
{
    timeBeginPeriod(1);
    srand((unsigned int)time(nullptr));
    // config 파일 설정 시작
    Parser* parsers = Parser::GetInstance();
    if (!parsers->ReadFile(L"config.txt"))
    {
        return false;
    }
    WCHAR* strings;
    //WCHAR* token;
    parsers->GetValue(L"LanServer", L"IP", &strings);
    
    if (wcslen(strings) == 0)
    {
        setData.openIp.S_un.S_addr = 0;
    }
    else
    {
        /*
        token = wcstok(strings, L".");

        setData.openIp.s_net = _wtoi(token);
        token = wcstok(NULL, L".");
        setData.openIp.s_host = _wtoi(token);
        token = wcstok(NULL, L".");
        setData.openIp.s_lh = _wtoi(token);
        token = wcstok(NULL, L".");
        setData.openIp.s_impno = _wtoi(token);
        */
        setData.openIp.S_un.S_addr = 0;
    }
    free(strings);
    int port;
    parsers->GetValue(L"LanServer", L"Port", port);
    setData.port = (USHORT)port;
    parsers->GetValue(L"LanServer", L"Concurrent Thread", setData.threadCount);
    parsers->GetValue(L"LanServer", L"Running Thread", setData.runningThreadCount);

    WCHAR* strings2;
    parsers->GetValue(L"LanServer", L"Nagle", &strings2);

    if (wcscmp(strings2, L"true") == 0)
    {
        setData.activateNagle = true;
    }
    else
    {
        setData.activateNagle = false;
    }
    free(strings2);
    parsers->GetValue(L"LanServer", L"MaxPlayer", setData.maxPlayerCount);

    setData.maxSessionCount = setData.maxPlayerCount * 2;

    parsers->GetValue(L"Session", L"MaxWaitSendTime", setData.maxWaitSendTime);
    parsers->GetValue(L"Session", L"MaxWaitSendCount", setData.maxWaitSendCount);
    parsers->GetValue(L"Session", L"MaxWaitSendSize", setData.maxWaitSendSize);
    setData.maxWaitSendSize *= 1024;

    parsers->GetValue(L"Sector", L"SectorTileSize", setData.sectorTileSize);
    parsers->GetValue(L"Sector", L"SectorRow", setData.sectorRow);
    parsers->GetValue(L"Sector", L"SectorCol", setData.sectorCol);

    // config 파일 읽기 종료

    sessionMap.resize(setData.maxSessionCount);
    for (int i = setData.maxSessionCount - 1; i >= 0; i--)
    {
        emptySessionMapStack.Push(i);
    }
    


    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return false;
    //InitializeCriticalSection(&sessionMapStackCSLock);
    endEvent = CreateEvent(nullptr, true, false, nullptr);
    if (endEvent == NULL)
    {
        return false;
    }

    StartThread();


    return true;
}

void LanServer::Stop()
{
    // 인원 내쫓기
    // 종료 절차 따르기
    
    //일단 스레드 종료
    closesocket(listenSock);
    SetEvent(endEvent);
    for (int i = 0; i < setData.runningThreadCount; i++)
    {
        PostQueuedCompletionStatus(completionPort, 0, 0, nullptr);
    }

    WaitForMultipleObjects((DWORD)workerThreads.size(), &workerThreads[0], true, INFINITE);

}

int LanServer::GetSessionCount() const
{
    return setData.maxSessionCount - emptySessionMapStack.GetSize();
}

bool LanServer::Disconnect(SESSIONID sessionId)
{
    /*InterlockedIncrement(&monitor.disconnectCount);
    
    SESSION* session = sessionMap[sessionId.value.index];
    if (session->sessionId.originValue != sessionId.originValue)
    {
        return 0;
    }
    int index = sessionId.value.index;

    InterlockedDecrement(&monitor.sessionCount);

    closesocket(session->sock);
    session->sock = INVALID_SOCKET;

    delete session;

    emptySessionMapStack.Push(index);*/


    return true;
}

bool LanServer::SendPacket(SESSIONID sessionId, CPacket* packetBuffer)
{
    
    SESSION* session = TryAcquireSession(sessionId);
    
    if (session == nullptr || session->sessionId.originValue != sessionId.originValue)
    {
        return false;
    }
    packetBuffer->IncrementReferenceCount();
    session->sendQueue.Enqueue(packetBuffer);
    session->sendWaitingSize += packetBuffer->GetUsedSize();

    if (InterlockedExchange(&session->canSend, 0) == 0)
    {
        int ch = 0;
        ch = InterlockedDecrement(&session->ioCount);
        if (ch == -1)
        {
            __debugbreak();
        }
        return true;
    }
    session->sendQueuingTime = timeGetTime();
    sendRequestQueue.Enqueue(session->sessionId);
    int ch = 0;
    ch = InterlockedDecrement(&session->ioCount);
    if (ch == -1)
    {
        __debugbreak();
    }
    return true;
}



// WSARecv 등록
int LanServer::RecvPost(SESSION* session)
{
    int retval;
    WSABUF wsabuf[2];
    DWORD flags = 0;

    InterlockedIncrement(&session->ioCount);
    ZeroMemory(&session->recvWSAOverlapped, sizeof(session->recvWSAOverlapped));
    wsabuf[0].buf = session->recvRingBuffer.GetTailBufferPtr();
    wsabuf[0].len = session->recvRingBuffer.DirectEnqueueSize();
    wsabuf[1].buf = session->recvRingBuffer.GetRingBufferStartPtr();
    wsabuf[1].len = session->recvRingBuffer.GetFreeSize() - session->recvRingBuffer.DirectEnqueueSize();
    retval = WSARecv(session->sock, &wsabuf[0], 2, NULL, &flags, &session->recvWSAOverlapped, NULL);


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

                //return OnRelease(session);
            }
        }
    }

    return 1;
}

int LanServer::SendPost(SESSION* session)
{
    DWORD flags = 0;
    int retval;
    int size = session->sendQueue.GetSize();
    int leftCount = session->sendWSAOverlapped.count;
    if (size + leftCount == 0)
    {
        return -1;
    }

    size = (size + leftCount) > 1000 ? 1000 : (size + leftCount);
    InterlockedAdd(&log.logData.SendPacketCount, size);
    InterlockedIncrement(&log.logData.SendCount);
    //InterlockedIncrement(&session->ioCount);
    CPacket* packetBuffer;
    
    for (int i = leftCount; i < size; i++)
    {
        int k = session->sendQueue.Dequeue(packetBuffer);
        session->sendWSAOverlapped.wsabuf[i].buf = packetBuffer->GetBufferPointer();
        session->sendWSAOverlapped.wsabuf[i].len = packetBuffer->GetUsedSize();
        session->sendWSAOverlapped.count++;
        session->sendWSAOverlapped.ptr[i] = packetBuffer;
    }
    session->lastSendTime = timeGetTime();
    retval = WSASend(session->sock, &session->sendWSAOverlapped.wsabuf[0], session->sendWSAOverlapped.count, NULL,
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

                //return OnRelease(session);
            }
        }
    }
    
    return 1;
}

int LanServer::CountAliveSessions()
{
    int count = 0;
    for (size_t i = 0; i < sessionMap.size(); ++i)
    {
        if (sessionMap[i] != nullptr)
            ++count;
    }
    return count;
}