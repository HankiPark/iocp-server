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
    bool GetValue(CONST WCHAR* key, int& value) const; // 여기서는 값 사이의 띄워쓰기를 인정하지 않음
    bool GetValue(CONST WCHAR* key, double& value) const; // 여기서는 값 사이의 띄워쓰기를 인정하지 않음
    bool GetValue(CONST WCHAR* key, WCHAR** CONST value) const; // 여기서 {} 블록으로 구간을 구별하고 싶다면, key앞에 구간구별을 하나 더넣어야겠다.
    bool GetValue(CONST WCHAR* tag, CONST WCHAR* key, int& value) const; // tag가 있는 경우의 number
    bool GetValue(CONST WCHAR* tag, CONST WCHAR* key, double& value) const; // tag가 있는 경우의 number
    bool GetValue(CONST WCHAR* tag, CONST WCHAR* key, WCHAR** CONST value) const; // tag 가 있는 경우의 string
    // 순서대로 읽어서 구조체의 값에 넣는 방식 (key나 value가 이상할 경우 logging)
private:
    Parser();
    ~Parser();
    WCHAR* buffer;
}; 



#endif
