#ifndef __LOGGING__
#define __LOGGING__
#include "Struct.h"

#define LOG_LEVEL_DEBUG		0
#define LOG_LEVEL_ERROR		1
#define LOG_LEVEL_SYSTEM	2

__declspec(thread) static WCHAR tlsLogBuffer[1024];


#define LOG(LogLevel, fmt, ...)								\
do {														\
		if (Log.logLevel <= LogLevel)						\
		{													\
			memset(tlsLogBuffer, 0, sizeof(tlsLogBuffer));	\
			wsprintf(logBuffer, fmt, ##__VA_ARGS__);		\
			log.Logging(logBuffer, Log.logLevel);			\
		}													\
} while (0)													\

#define GAMELOG(fmt, ...)									\
do {														\
		memset(tlsLogBuffer, 0, sizeof(tlsLogBuffer));		\
		wsprintf(tlsLogBuffer, fmt, ##__VA_ARGS__);			\
		log.GameLogging(tlsLogBuffer);						\
} while (0)													\


class Log {
public:
	Log();
	~Log();
	void SettingLog();
	void ReopenLogFile();
	void Logging(WCHAR* str, int logLevel);
	void SyncLogging(WCHAR* str, int logLevel);
	void GameLogging(WCHAR* str);
	void SyncGameLogging(WCHAR* str);
public:
	int logLevel = 2;
	LoggingData logData;
private:
	WCHAR logFileName[255] = { 0, };
	FILE* file = nullptr;
};
//extern Log gLog;

#endif