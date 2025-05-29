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
        //        log->Logging("�о�� ������ �������� �ʽ��ϴ�.\n");
        return false;
    }

    bool doubleSlashFlag = false; // //
    bool singleSlashFlag = false; // /**/
    bool inTagFlag = false; // tag���� ������ �н�
    int keyLen = (int)wcslen(key);
    int len = (int)wcslen(this->buffer);
    int seekKey = -1;
    value = 0;

    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            //           log->Logging("������ �д� ���� ���Ṯ�ڰ� Ȯ�εǾ����ϴ�. (0x00)\n");
            return false;               //need logging
        }

        // tag ó�� ����
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

        //tag ó�� ����

        //�ٹٲ�
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //�ּ� ó��
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

        //�ּ� ó�� ��

        //�������, �� ó��
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //�ؽ�Ʈ ��
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

            // key Ž�� ���� ��
            if (same)
            {
                seekKey = i + keyLen + 1;
                break;
            }
        }
        continue;
    }
    //�������� �ش簪�� ã������
    if (seekKey == -1)
    {
        //       log->Logging("�������� �ش� key�� ã�� ���߽��ϴ�.\n");
        return false;               //need logging
    }

    bool seekSign = false;
    bool numberSign = false;
    bool minusSign = false;
    //key�� ã������ �ش� �ٿ��� value Ž��
    for (int i = seekKey; i < len; i++)
    {
        // �ش� �ٿ� ���� �ִٸ� �Լ� ���� �ƴ϶�� ���� ��ȯ
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
            //          log->Logging("�ش� key�� ���� value�� �������� �ʽ��ϴ�. \n");
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

        //�ؽ�Ʈ �� ó���� ���Ա⿡ false
        if (this->buffer[i] == '"')
        {
            //          log->Logging("�������� �������� ��(char)�� ����� �ϴ� ��(int)�� �ٸ��ϴ�.\n");
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
        //        log->Logging("�о�� ������ �������� �ʽ��ϴ�.\n");
        return false;
    }

    bool doubleSlashFlag = false; // //
    bool singleSlashFlag = false; // /**/
    bool inTagFlag = false; // tag���� ������ �н�
    int keyLen = (int)wcslen(key);
    int len = (int)wcslen(this->buffer);
    int seekKey = -1;
    value = 0;

    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            //           log->Logging("������ �д� ���� ���Ṯ�ڰ� Ȯ�εǾ����ϴ�. (0x00)\n");
            return false;               //need logging
        }

        // tag ó�� ����
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

        //tag ó�� ����

        //�ٹٲ�
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //�ּ� ó��
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

        //�ּ� ó�� ��

        //�������, �� ó��
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //�ؽ�Ʈ ��
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

            // key Ž�� ���� ��
            if (same)
            {
                seekKey = i + keyLen + 1;
                break;
            }
        }
        continue;
    }
    //�������� �ش簪�� ã������
    if (seekKey == -1)
    {
        //       log->Logging("�������� �ش� key�� ã�� ���߽��ϴ�.\n");
        return false;               //need logging
    }

    bool seekSign = false;
    bool numberSign = false;
    bool minusSign = false;
    bool demicalSign = false;
    int demicalDepth = 1;
    //key�� ã������ �ش� �ٿ��� value Ž��
    for (int i = seekKey; i < len; i++)
    {
        // �ش� �ٿ� ���� �ִٸ� �Լ� ���� �ƴ϶�� ���� ��ȯ
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
            //          log->Logging("�ش� key�� ���� value�� �������� �ʽ��ϴ�. \n");
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

        //�ؽ�Ʈ �� ó���� ���Ա⿡ false
        if (this->buffer[i] == '"')
        {
            //          log->Logging("�������� �������� ��(char)�� ����� �ϴ� ��(int)�� �ٸ��ϴ�.\n");
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
        //    log->Logging("�о�� ������ �������� �ʽ��ϴ�.\n");
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
            //         log->Logging("���Ͽ� ���Ṯ�ڰ� �߰ߵǾ����ϴ�. (0x00)\n");
            return false;               //need logging
        }


        // tag ó�� ����
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

        //tag ó�� ����

        //�ٹٲ�
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //�ּ� ó��
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

        //�ּ� ó�� ��

        //�������, �� ó��
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //�ؽ�Ʈ ��
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

            // key Ž�� ���� ��
            if (same)
            {

                seekKey = i + keyLen;
                break;
            }
        }
        continue;
    }
    //�������� �ش簪�� ã������
    if (seekKey == -1)
    {
        //      log->Logging("�������� �ش� key�� ã�� ���߽��ϴ�.\n");
        return false;               //need logging
    }

    bool seekSign = false;
    bool charStartSign = false;
    bool charEndSign = false;
    int startIndex = 0;
    int endIndex = 0;
    //key�� ã������ �ش� �ٿ��� value Ž��
    for (int i = seekKey; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            //          log->Logging("�������� ���� ���ڰ� �߰ߵǾ����ϴ�. (0x00)\n");
            return false;               //need logging
        }
        // �ش� ���� ���� �ٷ� �Ѿ�� ���� ��ȯ
        if (this->buffer[i] == 0x0a)
        {
            if (seekSign == false)
            {
                //              log->Logging("�ش� key�� ���� �� �ٿ��� = �� �߰����� ���߽��ϴ�.\n");
                return false;
            }

            if (charStartSign == false)
            {
                //             log->Logging("�ش� key�� ���� �� �ٿ��� value�� ���������� ã�� ���߽��ϴ�.\n");
                return false;
            }

            if (charEndSign == false)
            {
                //             log->Logging("�ش� key�� ���� �� �ٿ��� value�� ���������� ã�� ���߽��ϴ�.\n");
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

    //�±� ã��
    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        //�ٹٲ�
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //�ּ� ó��
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

        //�ּ� ó�� ��

        //�������, �� ó��
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
                // { ã��
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

    //�±� ã�� ��
    if (seekTag == -1)
    {
        return false; // need logging
    }

    // Ű ã�� ����

    for (int i = seekTag; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        // tag ����
        if (this->buffer[i] == '}')
        {
            return false;               // need logging
        }
        //�ٹٲ�
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //�ּ� ó��
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

        //�ּ� ó�� ��

        //�������, �� ó��
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //�ؽ�Ʈ ��
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

            // key Ž�� ���� ��
            if (same)
            {

                seekKey = i + keyLen + 1;
                break;
            }
        }
        continue;
    }
    //�������� �ش簪�� ã������
    if (seekKey == -1)
    {
        return false;               //need logging
    }

    bool seekSign = false;
    bool numberSign = false;
    bool minusSign = false;
    //key�� ã������ �ش� �ٿ��� value Ž��
    for (int i = seekKey; i < len; i++)
    {
        // �ش� �ٿ� ���� �ִٸ� �Լ� ���� �ƴ϶�� ���� ��ȯ
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

        //�ؽ�Ʈ �� ó���� ���Ա⿡ false
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

    //�±� ã��
    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        //�ٹٲ�
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //�ּ� ó��
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

        //�ּ� ó�� ��

        //�������, �� ó��
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
                // { ã��
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

    //�±� ã�� ��
    if (seekTag == -1)
    {
        return false; // need logging
    }

    // Ű ã�� ����

    for (int i = seekTag; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        // tag ����
        if (this->buffer[i] == '}')
        {
            return false;               // need logging
        }
        //�ٹٲ�
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //�ּ� ó��
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

        //�ּ� ó�� ��

        //�������, �� ó��
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //�ؽ�Ʈ ��
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

            // key Ž�� ���� ��
            if (same)
            {

                seekKey = i + keyLen + 1;
                break;
            }
        }
        continue;
    }
    //�������� �ش簪�� ã������
    if (seekKey == -1)
    {
        return false;               //need logging
    }

    bool seekSign = false;
    bool numberSign = false;
    bool minusSign = false;
    bool demicalSign = false;
    int demicalDepth = 1;
    //key�� ã������ �ش� �ٿ��� value Ž��
    for (int i = seekKey; i < len; i++)
    {
        // �ش� �ٿ� ���� �ִٸ� �Լ� ���� �ƴ϶�� ���� ��ȯ
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

        //�ؽ�Ʈ �� ó���� ���Ա⿡ false
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

    //�±� ã��
    for (int i = 0; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        //�ٹٲ�
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //�ּ� ó��
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

        //�ּ� ó�� ��

        //�������, �� ó��
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
                // { ã��
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

    //�±� ã�� ��
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

        // tag ����
        if (this->buffer[i] == '}')
        {
            return false;               // need logging
        }

        //�ٹٲ�
        if (this->buffer[i] == 0x0a)
        {
            doubleSlashFlag = false;
            continue;
        }

        //�ּ� ó��
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

        //�ּ� ó�� ��

        //�������, �� ó��
        if (this->buffer[i] == 0x20 || this->buffer[i] == 0x08 || this->buffer[i] == 0x09)
        {
            continue;
        }

        //�ؽ�Ʈ ��
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

            // key Ž�� ���� ��
            if (same)
            {
                seekKey = i + keyLen + 1;
                break;
            }
        }
        continue;
    }
    //�������� �ش簪�� ã������
    if (seekKey == -1)
    {
        return false;               //need logging
    }

    bool seekSign = false;
    bool charStartSign = false;
    bool charEndSign = false;
    int startIndex = 0;
    int endIndex = 0;
    //key�� ã������ �ش� �ٿ��� value Ž��
    for (int i = seekKey; i < len; i++)
    {
        if (this->buffer[i] == 0x00)
        {
            return false;               //need logging
        }
        // �ش� ���� ���� �ٷ� �Ѿ�� ���� ��ȯ
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
