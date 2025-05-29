
#include <iostream>

#include <Strsafe.h>
#include <conio.h>
#include "Logging.h"
#include <windows.h>

Log::Log() {};
void Log::SettingLog()
{
    int res;
    SYSTEMTIME sysTime;
    

    GetLocalTime(&sysTime);
    if (GetFileAttributes(L"log") == INVALID_FILE_ATTRIBUTES)
    {
        CreateDirectory(L"Log", NULL);
    }
    StringCchPrintf(logFileName, wcslen(L"Log/log%02d%02d.txt"), L"Log/log%02d%02d.txt", sysTime.wMonth, sysTime.wDay);

    errno_t err = _wfopen_s(&file, logFileName, L"ab");
    if (err != 0 || file == NULL) {
        _wfopen_s(&file, logFileName, L"wb");
        if (file) {
            wchar_t bom = 0xFEFF; // UTF-16 BOM
            fwrite(&bom, sizeof(wchar_t), 1, file);
            fclose(file);

            res = _wfopen_s(&file, logFileName, L"ab");
            if (res != 0)
            {
                __debugbreak();
            }
        }
    }


}

Log::~Log()
{
    fclose(file);
}
void Log::ReopenLogFile()
{
    if (file)
    {
        fclose(file);
    }
    errno_t err = _wfopen_s(&file, logFileName, L"ab");
    if (err != 0 || file == NULL) {
        _wfopen_s(&file, logFileName, L"wb");
        if (file) {
            wchar_t bom = 0xFEFF; // UTF-16 BOM
            fwrite(&bom, sizeof(wchar_t), 1, file);
            fclose(file);

            err = _wfopen_s(&file, logFileName, L"ab");
            if (err != 0)
            {
                __debugbreak();
            }
        }
    }
}


void Log::Logging(WCHAR* str, int logLevel)
{
    wprintf(L"%s\n", str);
    SYSTEMTIME sysTime;
    WCHAR logData[1024];
    GetLocalTime(&sysTime);
    StringCchPrintf(logData, wcslen(L"[%02d:%02d:%02d:%03d] ") + wcslen(str) + wcslen(L" \n"), L"[%02d:%02d:%02d:%03d] %s \n",
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds, str);
    fwrite(logData, sizeof(WCHAR), wcslen(logData), file);

}



void Log::GameLogging(WCHAR* str)
{
    wprintf(L"%s\n", str);
    SYSTEMTIME sysTime;
    WCHAR logData[1024];
    GetLocalTime(&sysTime);
    StringCchPrintf(logData, wcslen(L"[%02d:%02d:%02d:%03d] ") + wcslen(str) + wcslen(L" \n"), L"[%02d:%02d:%02d:%03d] %s \n",
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds, str);
    fwrite(logData, sizeof(WCHAR), wcslen(logData), file);
}

void Log::SyncLogging(WCHAR* str, int logLevel)
{
    wprintf(L"%s\n", str);
    SYSTEMTIME sysTime;
    FILE* file;
    WCHAR logFileName[255];
    WCHAR logData[1024];
    bool res;

    GetLocalTime(&sysTime);
    if (GetFileAttributes(L"log") == INVALID_FILE_ATTRIBUTES)
    {
        CreateDirectory(L"Log", NULL);
    }
    StringCchPrintf(logFileName, wcslen(L"Log/log%02d%02d.txt"), L"Log/log%02d%02d.txt", sysTime.wMonth, sysTime.wDay);



    errno_t err = _wfopen_s(&file, logFileName, L"ab");
    if (err != 0 || file == NULL) {
        _wfopen_s(&file, logFileName, L"wb");
        if (file) {
            wchar_t bom = 0xFEFF; // UTF-16 BOM
            fwrite(&bom, sizeof(wchar_t), 1, file);
            fclose(file);

            res = _wfopen_s(&file, logFileName, L"ab");
            if (res != 0)
            {
                __debugbreak();
            }

        }
    }

    StringCchPrintf(logData, wcslen(L"[%02d:%02d:%02d:%03d] ") + wcslen(str) + wcslen(L" \n"), L"[%02d:%02d:%02d:%03d] %s \n",
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds, str);

    fwrite(logData, sizeof(WCHAR), wcslen(logData), file);


    fclose(file);
}



void Log::SyncGameLogging(WCHAR* str)
{
    wprintf(L"%s\n", str);
    SYSTEMTIME sysTime;
    FILE* file;
    WCHAR logFileName[255];
    WCHAR logData[1024];
    bool res;

    GetLocalTime(&sysTime);
    if (GetFileAttributes(L"log") == INVALID_FILE_ATTRIBUTES)
    {
        CreateDirectory(L"Log", NULL);
    }
    StringCchPrintf(logFileName, wcslen(L"Log/log%02d.txt"), L"Log/log%02d.txt", sysTime.wMonth);



    errno_t err = _wfopen_s(&file, logFileName, L"rb");
    if (err != 0 || file == NULL) {
        _wfopen_s(&file, logFileName, L"wb");
        if (file) {
            wchar_t bom = 0xFEFF; // UTF-16 BOM
            fwrite(&bom, sizeof(wchar_t), 1, file);
            fclose(file);
            res = _wfopen_s(&file, logFileName, L"ab");
            if (res != 0)
            {
                __debugbreak();
            }
        }
    }

    StringCchPrintf(logData, wcslen(L"[%02d:%02d:%02d:%03d] ") + wcslen(str) + wcslen(L" \n"), L"[%02d:%02d:%02d:%03d] %s \n",
        sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds, str);

    fwrite(logData, sizeof(WCHAR), wcslen(logData), file);


    fclose(file);
}