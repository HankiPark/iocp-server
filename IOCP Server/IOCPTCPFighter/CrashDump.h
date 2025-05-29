#ifndef _Crash_DUMP_
#define _Crash_DUMP_

/*
#include <windows.h>
#include <stdlib.h>
#include <crtdbg.h>
#include <wchar.h>
#include <DbgHelp.h>
*/
namespace dump
{
	class CrashDump
	{
	public:
		CrashDump()
		{
			dumpCount = 0;

			_invalid_parameter_handler oldHandler, newHandler;
			newHandler = myInvalidParameterHandler;
			
			oldHandler = _set_invalid_parameter_handler(newHandler); // crt함수의 nullptr 등
			_CrtSetReportMode(_CRT_WARN, 0);
			_CrtSetReportMode(_CRT_ASSERT, 0);
			_CrtSetReportMode(_CRT_ERROR, 0);

			_CrtSetReportHook(_custom_Report_hook);

			_set_purecall_handler(myPurecallHandler);

			SetHandlerDump();
		}

		static void Crash(void)
		{
			__debugbreak();
		}

		static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
		{
			int iWorkingMemory = 0;
			SYSTEMTIME stNowTime;

			long DumpCount = InterlockedIncrement(&dumpCount);

			WCHAR fileName[MAX_PATH];

			GetLocalTime(&stNowTime);
			wsprintf(fileName, L"Dump_%d%02d%02d_%02d.%02d.%02d_%d.dmp",
				stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond,
				DumpCount);

			wprintf(L"\n\n\n Crash Error  %d.%d.%d / %d:%d:%d\n", stNowTime.wYear, stNowTime.wMonth, stNowTime.wDay, stNowTime.wHour, stNowTime.wMinute, stNowTime.wSecond);
			wprintf(L"Now Save Dump File \n");

			HANDLE dumpFile = CreateFile(fileName,
				GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (dumpFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInformation;

				minidumpExceptionInformation.ThreadId = GetCurrentThreadId();
				minidumpExceptionInformation.ExceptionPointers = pExceptionPointer;
				minidumpExceptionInformation.ClientPointers = TRUE;

				MiniDumpWriteDump(GetCurrentProcess(),
					GetCurrentProcessId(),
					dumpFile,
					MiniDumpWithFullMemory,
					&minidumpExceptionInformation,
					NULL, NULL);
				
				CloseHandle(dumpFile);

				wprintf(L"Crash dump save fin \n");


			}

			return EXCEPTION_EXECUTE_HANDLER;
		}

		static void SetHandlerDump()
		{
			SetUnhandledExceptionFilter(MyExceptionFilter);
		}

		static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function,
			const wchar_t* file, unsigned int line, uintptr_t pReserved)
		{
			Crash();
		}

		static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue)
		{
			Crash();
			return true; // CRT가 추후 처리를 하지 않음
		}

		static void myPurecallHandler(void)
		{
			Crash();
		}

		static long dumpCount;

	};

	
}



#endif
