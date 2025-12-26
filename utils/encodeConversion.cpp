#include "../include/Utils.h"
uint16_t Utf8ToSjis(const string& utf8char) {
    /*
	将输入的 UTF-8 字符转为 Sjis 码值
	*/
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8char.c_str(), -1, NULL, 0);
    wchar_t* wstr = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, utf8char.c_str(), -1, wstr, len);
    char sjis[3];
    WideCharToMultiByte(932, 0, wstr, -1, sjis, sizeof(sjis), NULL, NULL);
    delete[] wstr;
    return ((uint8_t)sjis[0] << 8) | (uint8_t)sjis[1];
}
string Utf8ToGbk(const string& utf8Str) {
	/*
	将输入的 UTF-8 字符串转为 GBK 编码字符串
	*/
    if (utf8Str.empty()) return "";
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
    wstring wideStr(wideLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &wideStr[0], wideLen);
    int gbkLen = WideCharToMultiByte(936, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    string gbkStr(gbkLen, 0);
    WideCharToMultiByte(936, 0, wideStr.c_str(), -1, &gbkStr[0], gbkLen, nullptr, nullptr);
    if (!gbkStr.empty() && gbkStr.back() == '\0') gbkStr.pop_back();
    return gbkStr;
}