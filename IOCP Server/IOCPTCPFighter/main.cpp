#pragma comment(lib, "ws2_32")
#pragma comment(lib, "Dbghelp.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <queue>
#include <process.h>
#include <conio.h>
#include <windows.h>
#include <crtdbg.h>
#include <wchar.h>
#include <DbgHelp.h>
#include "CrashDump.h"
#include "LanServer.h"
#include "GameServer.h"

long dump::CrashDump::dumpCount;

int main()
{
    LanServer* server = (LanServer*)_aligned_malloc(sizeof(GameServer), 64);
    new (server) GameServer();
    dump::CrashDump dump;
    server->Start();

    while (1)
    {
        if (_kbhit())
        {
            char c = _getch();
            if (c == 'q' || c == 'Q')
            {
                server->Stop();
                break;
            }
            if (c == 'w' || c == 'W')
            {
                printf("%d \n", server->CountAliveSessions());
                __debugbreak();
            }
        }
    }
    
    server->~LanServer();
    _aligned_free(server);

    //CloseHandle(endEvent);



    WSACleanup();
    return 0;
}