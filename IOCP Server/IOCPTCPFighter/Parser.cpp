#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include "Parser.h"
#include <math.h>

Parser Parser::parser;

Parser::Parser() : buffer(nullptr) {};

Parser::~Parser()
{
    free(buffer);
};



bool Parser::ReadFile(CONST WCHAR* fileName)
{
    FILE* file;
    bool res;
    int fileSize;
    res = _wfopen_s(&file, fileName, L"rt, ccs=UTF-8");
    if (res || file == 0) {

        return false;               //need logging
    }
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    this->buffer = (WCHAR*)malloc(sizeof(WCHAR) * fileSize);
    if (this->buffer == nullptr)
    {
        return false;
    }
    fseek(file, 0, SEEK_SET);
    fread_s(this->buffer, sizeof(WCHAR) * fileSize, 1, sizeof(WCHAR) * fileSize, file);

    fclose(file);

    return true;
};

bool Parser::GetValue(CONST WCHAR* key, int& value) const
{
    //   LoggingManager* log = LoggingManager::GetInstance();
    if (this->buffer == nullptr)
    {
        //        log->Logging("읽어올 파일이 존재하지 않습니다.\n");
        return false;
    }

    bool doubleSlashFlag = false; // //
    bool singleSlashFlag = false; // /**/
    bool inTagFlag = false; // tag내의 데이터 패스
    int keyLen = (int)wcslen(key);
    int len = (int)wcslen(this->buffer);
    int seekKey = -1;
    value = 0;

    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            //           log->Logging("파일을 읽던 도중 종료문자가 확인되었습니다. (0x00)\n");
            return false;               //need logging
        }

        // tag 처리 시작
        if (inTagFlag)
        {
            if (this->buffer[i] == '}')
            {
                inTagFlag = false;
            }
            continue;
        }

        if (this->buffer[i] == '{')
        {
            inTagFlag = true;
            continue;
        }

        //tag 처리 종료

        //줄바꿈
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //주석 처리
        if (doubleSlashFlag)
        {
            continue;
        }

        if (singleSlashFlag)
        {
            if (i + 1 >= len)
            {
                continue;
            }
            if (this->buffer[i] == '*' && this->buffer[i + 1] == '/')
            {
                singleSlashFlag = false;
                i++;
            }
            continue;
        }

        if (i + 1 < len)
        {
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '*')
            {
                singleSlashFlag = true;
                i++;
                continue;
            }
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '/')
            {
                doubleSlashFlag = true;
                i++;
                continue;
            }
        }

        //주석 처리 끝

        //띄워쓰기, 탭 처리
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //텍스트 값
        if (this->buffer[i] == key[0])
        {
            if (i + keyLen > len)
            {
                break;
            }
            bool same = true;
            for (int j = i; j < i + keyLen; j++)
            {
                if (this->buffer[j] != key[j - i])
                {
                    same = false;
                    break;
                }
            }

            // key 탐색 성공 시
            if (same)
            {
                seekKey = i + keyLen + 1;
                break;
            }
        }
        continue;
    }
    //문서에서 해당값을 찾지못함
    if (seekKey == -1)
    {
        //       log->Logging("문서에서 해당 key를 찾지 못했습니다.\n");
        return false;               //need logging
    }

    bool seekSign = false;
    bool numberSign = false;
    bool minusSign = false;
    //key를 찾았으니 해당 줄에서 value 탐색
    for (int i = seekKey; i < len; i++)
    {
        // 해당 줄에 값이 있다면 함수 종료 아니라면 실패 반환
        if (this->buffer[i] == 0x0a)
        {
            if (seekSign && numberSign)
            {
                if (minusSign)
                {
                    value = -value;
                }
                return true;
            }
            //          log->Logging("해당 key에 대한 value가 존재하지 않습니다. \n");
            return false;               //need logging
        }
        if (seekSign == false)
        {
            if (this->buffer[i] == '=')
            {
                seekSign = true;
            }
            continue;
        }

        //텍스트 값 처리가 들어왔기에 false
        if (this->buffer[i] == '"')
        {
            //          log->Logging("읽으려는 데이터의 형(char)이 얻고자 하는 형(int)과 다릅니다.\n");
            return false;               // need logging
        }

        if (this->buffer[i] >= '0' && this->buffer[i] <= '9')
        {
            numberSign = true;
            value *= 10;
            value += this->buffer[i] - '0';
            continue;
        }
        if (this->buffer[i] == '-')
        {
            minusSign = true;
            continue;
        }

        if (numberSign)
        {
            if (minusSign)
            {
                value = -value;
            }
            return true;
        }
    }


    return false;               //need logging
}

bool Parser::GetValue(CONST WCHAR* key, double& value) const
{
    //   LoggingManager* log = LoggingManager::GetInstance();
    if (this->buffer == nullptr)
    {
        //        log->Logging("읽어올 파일이 존재하지 않습니다.\n");
        return false;
    }

    bool doubleSlashFlag = false; // //
    bool singleSlashFlag = false; // /**/
    bool inTagFlag = false; // tag내의 데이터 패스
    int keyLen = (int)wcslen(key);
    int len = (int)wcslen(this->buffer);
    int seekKey = -1;
    value = 0;

    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            //           log->Logging("파일을 읽던 도중 종료문자가 확인되었습니다. (0x00)\n");
            return false;               //need logging
        }

        // tag 처리 시작
        if (inTagFlag)
        {
            if (this->buffer[i] == '}')
            {
                inTagFlag = false;
            }
            continue;
        }

        if (this->buffer[i] == '{')
        {
            inTagFlag = true;
            continue;
        }

        //tag 처리 종료

        //줄바꿈
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //주석 처리
        if (doubleSlashFlag)
        {
            continue;
        }

        if (singleSlashFlag)
        {
            if (i + 1 >= len)
            {
                continue;
            }
            if (this->buffer[i] == '*' && this->buffer[i + 1] == '/')
            {
                singleSlashFlag = false;
                i++;
            }
            continue;
        }

        if (i + 1 < len)
        {
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '*')
            {
                singleSlashFlag = true;
                i++;
                continue;
            }
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '/')
            {
                doubleSlashFlag = true;
                i++;
                continue;
            }
        }

        //주석 처리 끝

        //띄워쓰기, 탭 처리
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //텍스트 값
        if (this->buffer[i] == key[0])
        {
            if (i + keyLen > len)
            {
                break;
            }
            bool same = true;
            for (int j = i; j < i + keyLen; j++)
            {
                if (this->buffer[j] != key[j - i])
                {
                    same = false;
                    break;
                }
            }

            // key 탐색 성공 시
            if (same)
            {
                seekKey = i + keyLen + 1;
                break;
            }
        }
        continue;
    }
    //문서에서 해당값을 찾지못함
    if (seekKey == -1)
    {
        //       log->Logging("문서에서 해당 key를 찾지 못했습니다.\n");
        return false;               //need logging
    }

    bool seekSign = false;
    bool numberSign = false;
    bool minusSign = false;
    bool demicalSign = false;
    int demicalDepth = 1;
    //key를 찾았으니 해당 줄에서 value 탐색
    for (int i = seekKey; i < len; i++)
    {
        // 해당 줄에 값이 있다면 함수 종료 아니라면 실패 반환
        if (this->buffer[i] == 0x0a)
        {
            if (seekSign && numberSign)
            {
                if (minusSign)
                {
                    value = -value;
                }
                return true;
            }
            //          log->Logging("해당 key에 대한 value가 존재하지 않습니다. \n");
            return false;               //need logging
        }
        if (seekSign == false)
        {
            if (this->buffer[i] == '=')
            {
                seekSign = true;
            }
            continue;
        }

        //텍스트 값 처리가 들어왔기에 false
        if (this->buffer[i] == '"')
        {
            //          log->Logging("읽으려는 데이터의 형(char)이 얻고자 하는 형(int)과 다릅니다.\n");
            return false;               // need logging
        }

        if (this->buffer[i] >= '0' && this->buffer[i] <= '9')
        {
            numberSign = true;
            if (demicalSign)
            {
                value += (this->buffer[i] - '0') / pow(10, demicalDepth++);
            }
            else
            {
                value *= 10;
                value += this->buffer[i] - '0';
            }
            continue;
        }
        if (this->buffer[i] == '-')
        {
            minusSign = true;
            continue;
        }
        if (this->buffer[i] == '.')
        {
            demicalSign = true;
            continue;
        }

        if (numberSign)
        {
            if (minusSign)
            {
                value = -value;
            }
            return true;
        }
    }


    return false;               //need logging
}

bool Parser::GetValue(CONST WCHAR* key, WCHAR** CONST value) const
{
    //  LoggingManager* log = LoggingManager::GetInstance();
    if (this->buffer == nullptr)
    {
        //    log->Logging("읽어올 파일이 존재하지 않습니다.\n");
        return false;
    }

    bool doubleSlashFlag = false; // //
    bool singleSlashFlag = false; // /**/
    bool inTagFlag = false;
    int keyLen = (int)wcslen(key);
    int len = (int)wcslen(this->buffer);
    int seekKey = -1;

    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            //         log->Logging("파일에 종료문자가 발견되었습니다. (0x00)\n");
            return false;               //need logging
        }


        // tag 처리 시작
        if (inTagFlag)
        {
            if (this->buffer[i] == '}')
            {
                inTagFlag = false;
            }
            continue;
        }

        if (this->buffer[i] == '{')
        {
            inTagFlag = true;
            continue;
        }

        //tag 처리 종료

        //줄바꿈
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //주석 처리
        if (doubleSlashFlag)
        {
            continue;
        }

        if (singleSlashFlag)
        {
            if (i + 1 >= len)
            {
                continue;
            }
            if (this->buffer[i] == '*' && this->buffer[i + 1] == '/')
            {
                singleSlashFlag = false;
                i++;
            }
            continue;
        }

        if (i + 1 < len)
        {
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '*')
            {
                singleSlashFlag = true;
                i++;
                continue;
            }
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '/')
            {
                doubleSlashFlag = true;
                i++;
                continue;
            }
        }

        //주석 처리 끝

        //띄워쓰기, 탭 처리
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //텍스트 값
        if (this->buffer[i] == key[0])
        {
            if (i + keyLen > len)
            {
                break;
            }
            bool same = true;
            for (int j = i; j < i + keyLen; j++)
            {
                if (this->buffer[j] != key[j - i])
                {
                    same = false;
                    break;
                }
            }

            // key 탐색 성공 시
            if (same)
            {

                seekKey = i + keyLen;
                break;
            }
        }
        continue;
    }
    //문서에서 해당값을 찾지못함
    if (seekKey == -1)
    {
        //      log->Logging("문서에서 해당 key를 찾지 못했습니다.\n");
        return false;               //need logging
    }

    bool seekSign = false;
    bool charStartSign = false;
    bool charEndSign = false;
    int startIndex = 0;
    int endIndex = 0;
    //key를 찾았으니 해당 줄에서 value 탐색
    for (int i = seekKey; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            //          log->Logging("문서에서 종료 문자가 발견되었습니다. (0x00)\n");
            return false;               //need logging
        }
        // 해당 줄이 다음 줄로 넘어가면 실패 반환
        if (this->buffer[i] == 0x0a)
        {
            if (seekSign == false)
            {
                //              log->Logging("해당 key에 대해 그 줄에서 = 를 발견하지 못했습니다.\n");
                return false;
            }

            if (charStartSign == false)
            {
                //             log->Logging("해당 key에 대해 그 줄에서 value의 시작지점을 찾지 못했습니다.\n");
                return false;
            }

            if (charEndSign == false)
            {
                //             log->Logging("해당 key에 대해 그 줄에서 value의 종료지점을 찾지 못했습니다.\n");
                return false;
            }                             //need logging
        }

        if (seekSign == false)
        {
            if (this->buffer[i] == '=')
            {
                seekSign = true;
            }
            continue;
        }

        if (charStartSign == false)
        {
            if (this->buffer[i] == '"')
            {
                charStartSign = true;
                startIndex = i + 1;
                continue;
            }
        }

        if (charEndSign == false)
        {
            if (this->buffer[i] == '"')
            {
                charEndSign = true;
                endIndex = i;
                continue;
            }
        }

        if (charStartSign && charEndSign)
        {
            break;
        }
    }
    *value = (WCHAR*)malloc(sizeof(WCHAR) * (endIndex - startIndex + 1));
    wmemset(*value, 0x00, (endIndex - startIndex + 1));
    wmemcpy_s(*value, (endIndex - startIndex + 1), this->buffer + startIndex, endIndex - startIndex);

    return true;               //need logging
}

bool Parser::GetValue(CONST WCHAR* tag, CONST WCHAR* key, int& value) const
{
    if (this->buffer == nullptr)
    {
        return false;
    }

    bool doubleSlashFlag = false; // //
    bool singleSlashFlag = false; // /**/
    int keyLen = (int)wcslen(key);
    int tagLen = (int)wcslen(tag);
    int len = (int)wcslen(this->buffer);
    int seekKey = -1;
    int seekTag = -1;
    value = 0;

    //태그 찾기
    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        //줄바꿈
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //주석 처리
        if (doubleSlashFlag)
        {
            continue;
        }

        if (singleSlashFlag)
        {
            if (i + 1 >= len)
            {
                continue;
            }
            if (this->buffer[i] == '*' && this->buffer[i + 1] == '/')
            {
                singleSlashFlag = false;
                i++;
            }
            continue;
        }

        if (i + 1 < len)
        {
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '*')
            {
                singleSlashFlag = true;
                i++;
                continue;
            }
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '/')
            {
                doubleSlashFlag = true;
                i++;
                continue;
            }
        }

        //주석 처리 끝

        //띄워쓰기, 탭 처리
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        if (this->buffer[i] == ':')
        {
            bool same = true;
            if (i + 1 + tagLen >= len)
            {
                break;
            }
            for (int j = i + 1; j < i + 1 + tagLen; j++)
            {
                if (this->buffer[j] != tag[j - (i + 1)])
                {
                    same = false;
                    break;
                }
            }

            if (same)
            {
                i += tagLen;
                // { 찾기
                for (int j = i + 1; j < len; j++)
                {
                    if (this->buffer[j] == '{')
                    {
                        seekTag = j + 1;
                        break;
                    }
                }
                break;
            }
        }
    }

    //태그 찾기 끝
    if (seekTag == -1)
    {
        return false; // need logging
    }

    // 키 찾기 시작

    for (int i = seekTag; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        // tag 닫힘
        if (this->buffer[i] == '}')
        {
            return false;               // need logging
        }
        //줄바꿈
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //주석 처리
        if (doubleSlashFlag)
        {
            continue;
        }

        if (singleSlashFlag)
        {
            if (i + 1 >= len)
            {
                continue;
            }
            if (this->buffer[i] == '*' && this->buffer[i + 1] == '/')
            {
                singleSlashFlag = false;
                i++;
            }
            continue;
        }

        if (i + 1 < len)
        {
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '*')
            {
                singleSlashFlag = true;
                i++;
                continue;
            }
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '/')
            {
                doubleSlashFlag = true;
                i++;
                continue;
            }
        }

        //주석 처리 끝

        //띄워쓰기, 탭 처리
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //텍스트 값
        if (this->buffer[i] == key[0])
        {
            if (i + keyLen > len)
            {
                break;
            }
            bool same = true;
            for (int j = i; j < i + keyLen; j++)
            {
                if (this->buffer[j] != key[j - i])
                {
                    same = false;

                    break;
                }
            }

            // key 탐색 성공 시
            if (same)
            {

                seekKey = i + keyLen + 1;
                break;
            }
        }
        continue;
    }
    //문서에서 해당값을 찾지못함
    if (seekKey == -1)
    {
        return false;               //need logging
    }

    bool seekSign = false;
    bool numberSign = false;
    bool minusSign = false;
    //key를 찾았으니 해당 줄에서 value 탐색
    for (int i = seekKey; i < len; i++)
    {
        // 해당 줄에 값이 있다면 함수 종료 아니라면 실패 반환
        if (this->buffer[i] == 0x0a)
        {
            if (seekSign && numberSign)
            {
                if (minusSign)
                {
                    value = -value;
                }
                return true;
            }
            return false;               //need logging
        }
        if (seekSign == false)
        {
            if (this->buffer[i] == '=')
            {
                seekSign = true;
            }
            continue;
        }

        //텍스트 값 처리가 들어왔기에 false
        if (this->buffer[i] == '"')
        {
            return false;               // need logging
        }

        if (this->buffer[i] >= '0' && this->buffer[i] <= '9')
        {
            numberSign = true;
            value *= 10;
            value += this->buffer[i] - '0';
            continue;
        }
        if (this->buffer[i] == '-')
        {
            minusSign = true;
            continue;
        }

        if (numberSign)
        {
            if (minusSign)
            {
                value = -value;
            }
            return true;
        }
    }


    return false;               //need logging
}

bool Parser::GetValue(CONST WCHAR* tag, CONST WCHAR* key, double& value) const
{
    if (this->buffer == nullptr)
    {
        return false;
    }

    bool doubleSlashFlag = false; // //
    bool singleSlashFlag = false; // /**/
    int keyLen = (int)wcslen(key);
    int tagLen = (int)wcslen(tag);
    int len = (int)wcslen(this->buffer);
    int seekKey = -1;
    int seekTag = -1;
    value = 0;

    //태그 찾기
    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        //줄바꿈
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //주석 처리
        if (doubleSlashFlag)
        {
            continue;
        }

        if (singleSlashFlag)
        {
            if (i + 1 >= len)
            {
                continue;
            }
            if (this->buffer[i] == '*' && this->buffer[i + 1] == '/')
            {
                singleSlashFlag = false;
                i++;
            }
            continue;
        }

        if (i + 1 < len)
        {
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '*')
            {
                singleSlashFlag = true;
                i++;
                continue;
            }
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '/')
            {
                doubleSlashFlag = true;
                i++;
                continue;
            }
        }

        //주석 처리 끝

        //띄워쓰기, 탭 처리
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        if (this->buffer[i] == ':')
        {
            bool same = true;
            if (i + 1 + tagLen >= len)
            {
                break;
            }
            for (int j = i + 1; j < i + 1 + tagLen; j++)
            {
                if (this->buffer[j] != tag[j - (i + 1)])
                {
                    same = false;
                    break;
                }
            }

            if (same)
            {
                i += tagLen;
                // { 찾기
                for (int j = i + 1; j < len; j++)
                {
                    if (this->buffer[j] == '{')
                    {
                        seekTag = j + 1;
                        break;
                    }
                }
                break;
            }
        }
    }

    //태그 찾기 끝
    if (seekTag == -1)
    {
        return false; // need logging
    }

    // 키 찾기 시작

    for (int i = seekTag; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        // tag 닫힘
        if (this->buffer[i] == '}')
        {
            return false;               // need logging
        }
        //줄바꿈
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //주석 처리
        if (doubleSlashFlag)
        {
            continue;
        }

        if (singleSlashFlag)
        {
            if (i + 1 >= len)
            {
                continue;
            }
            if (this->buffer[i] == '*' && this->buffer[i + 1] == '/')
            {
                singleSlashFlag = false;
                i++;
            }
            continue;
        }

        if (i + 1 < len)
        {
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '*')
            {
                singleSlashFlag = true;
                i++;
                continue;
            }
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '/')
            {
                doubleSlashFlag = true;
                i++;
                continue;
            }
        }

        //주석 처리 끝

        //띄워쓰기, 탭 처리
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //텍스트 값
        if (this->buffer[i] == key[0])
        {
            if (i + keyLen > len)
            {
                break;
            }
            bool same = true;
            for (int j = i; j < i + keyLen; j++)
            {
                if (this->buffer[j] != key[j - i])
                {
                    same = false;

                    break;
                }
            }

            // key 탐색 성공 시
            if (same)
            {

                seekKey = i + keyLen + 1;
                break;
            }
        }
        continue;
    }
    //문서에서 해당값을 찾지못함
    if (seekKey == -1)
    {
        return false;               //need logging
    }

    bool seekSign = false;
    bool numberSign = false;
    bool minusSign = false;
    bool demicalSign = false;
    int demicalDepth = 1;
    //key를 찾았으니 해당 줄에서 value 탐색
    for (int i = seekKey; i < len; i++)
    {
        // 해당 줄에 값이 있다면 함수 종료 아니라면 실패 반환
        if (this->buffer[i] == 0x0a)
        {
            if (seekSign && numberSign)
            {
                if (minusSign)
                {
                    value = -value;
                }
                return true;
            }
            return false;               //need logging
        }
        if (seekSign == false)
        {
            if (this->buffer[i] == '=')
            {
                seekSign = true;
            }
            continue;
        }

        //텍스트 값 처리가 들어왔기에 false
        if (this->buffer[i] == '"')
        {
            return false;               // need logging
        }

        if (this->buffer[i] >= '0' && this->buffer[i] <= '9')
        {
            numberSign = true;
            if (demicalSign)
            {
                value += (this->buffer[i] - '0') / pow(10, demicalDepth++);
            }
            else
            {
                value *= 10;
                value += this->buffer[i] - '0';
            }

            continue;
        }
        if (this->buffer[i] == '-')
        {
            minusSign = true;
            continue;
        }
        if (this->buffer[i] == '.')
        {
            demicalSign = true;
            continue;
        }

        if (numberSign)
        {
            if (minusSign)
            {
                value = -value;
            }
            return true;
        }
    }


    return false;               //need logging
}

bool Parser::GetValue(CONST WCHAR* tag, CONST WCHAR* key, WCHAR** CONST value) const
{
    if (this->buffer == nullptr)
    {
        return false;
    }

    bool doubleSlashFlag = false; // //
    bool singleSlashFlag = false; // /**/
    int keyLen = (int)wcslen(key);
    int tagLen = (int)wcslen(tag);
    int len = (int)wcslen(this->buffer);
    int seekKey = -1;
    int seekTag = -1;

    //태그 찾기
    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        //줄바꿈
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //주석 처리
        if (doubleSlashFlag)
        {
            continue;
        }

        if (singleSlashFlag)
        {
            if (i + 1 >= len)
            {
                continue;
            }
            if (this->buffer[i] == '*' && this->buffer[i + 1] == '/')
            {
                singleSlashFlag = false;
                i++;
            }
            continue;
        }

        if (i + 1 < len)
        {
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '*')
            {
                singleSlashFlag = true;
                i++;
                continue;
            }
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '/')
            {
                doubleSlashFlag = true;
                i++;
                continue;
            }
        }

        //주석 처리 끝

        //띄워쓰기, 탭 처리
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        if (this->buffer[i] == ':')
        {
            bool same = true;
            if (i + 1 + tagLen >= len)
            {
                break;
            }
            for (int j = i + 1; j < i + 1 + tagLen; j++)
            {
                if (this->buffer[j] != tag[j - (i + 1)])
                {
                    same = false;
                    break;
                }
            }

            if (same)
            {
                i += tagLen;
                // { 찾기
                for (int j = i + 1; j < len; j++)
                {
                    if (this->buffer[j] == '{')
                    {
                        seekTag = j + 1;
                        break;
                    }
                }
                break;
            }
        }
    }

    //태그 찾기 끝
    if (seekTag == -1)
    {
        return false; // need logging
    }

    for (int i = seekTag; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }

        // tag 닫힘
        if (this->buffer[i] == '}')
        {
            return false;               // need logging
        }

        //줄바꿈
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //주석 처리
        if (doubleSlashFlag)
        {
            continue;
        }

        if (singleSlashFlag)
        {
            if (i + 1 >= len)
            {
                continue;
            }
            if (this->buffer[i] == '*' && this->buffer[i + 1] == '/')
            {
                singleSlashFlag = false;
                i++;
            }
            continue;
        }

        if (i + 1 < len)
        {
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '*')
            {
                singleSlashFlag = true;
                i++;
                continue;
            }
            if (this->buffer[i] == '/' && this->buffer[i + 1] == '/')
            {
                doubleSlashFlag = true;
                i++;
                continue;
            }
        }

        //주석 처리 끝

        //띄워쓰기, 탭 처리
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //텍스트 값
        if (this->buffer[i] == key[0])
        {
            if (i + keyLen > len)
            {
                break;
            }
            bool same = true;
            for (int j = i; j < i + keyLen; j++)
            {
                if (this->buffer[j] != key[j - i])
                {
                    same = false;
                    break;
                }
            }

            // key 탐색 성공 시
            if (same)
            {
                seekKey = i + keyLen + 1;
                break;
            }
        }
        continue;
    }
    //문서에서 해당값을 찾지못함
    if (seekKey == -1)
    {
        return false;               //need logging
    }

    bool seekSign = false;
    bool charStartSign = false;
    bool charEndSign = false;
    int startIndex = 0;
    int endIndex = 0;
    //key를 찾았으니 해당 줄에서 value 탐색
    for (int i = seekKey; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        // 해당 줄이 다음 줄로 넘어가면 실패 반환
        if (this->buffer[i] == 0x0a)
        {
            if (seekSign == false)
            {
                return false;
            }

            if (charStartSign == false)
            {
                return false;
            }

            if (charEndSign == false)
            {
                return false;
            }                             //need logging
        }

        if (seekSign == false)
        {
            if (this->buffer[i] == '=')
            {
                seekSign = true;
            }
            continue;
        }

        if (charStartSign == false)
        {
            if (this->buffer[i] == '"')
            {
                charStartSign = true;
                startIndex = i + 1;
                continue;
            }
        }

        if (charEndSign == false)
        {
            if (this->buffer[i] == '"')
            {
                charEndSign = true;
                endIndex = i;
                continue;
            }
        }

        if (charStartSign && charEndSign)
        {
            break;
        }
    }
    *value = (WCHAR*)malloc(sizeof(WCHAR) * (endIndex - startIndex + 1));
    wmemset(*value, 0x00, (endIndex - startIndex + 1));
    wmemcpy_s(*value, (endIndex - startIndex + 1), this->buffer + startIndex, endIndex - startIndex);

    return true;
}
