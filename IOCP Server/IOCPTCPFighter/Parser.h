#ifndef __PARSER__
#define __PARSER__

class Parser {
private:
    static Parser parser;
public:
    static Parser* GetInstance()
    {
        return &parser;
    };
    bool ReadFile(CONST WCHAR* fileName);
    bool GetValue(CONST WCHAR* key, int& value) const; // ���⼭�� �� ������ ������⸦ �������� ����
    bool GetValue(CONST WCHAR* key, double& value) const; // ���⼭�� �� ������ ������⸦ �������� ����
    bool GetValue(CONST WCHAR* key, WCHAR** CONST value) const; // ���⼭ {} ������� ������ �����ϰ� �ʹٸ�, key�տ� ���������� �ϳ� ���־�߰ڴ�.
    bool GetValue(CONST WCHAR* tag, CONST WCHAR* key, int& value) const; // tag�� �ִ� ����� number
    bool GetValue(CONST WCHAR* tag, CONST WCHAR* key, double& value) const; // tag�� �ִ� ����� number
    bool GetValue(CONST WCHAR* tag, CONST WCHAR* key, WCHAR** CONST value) const; // tag �� �ִ� ����� string
    // ������� �о ����ü�� ���� �ִ� ��� (key�� value�� �̻��� ��� logging)
private:
    Parser();
    ~Parser();
    WCHAR* buffer;
}; 



#endif
