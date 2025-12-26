#ifndef UTILS_H
#define UTILS_H
#include <windows.h>
#include <vector>
#include <string>
using namespace std;
int gf_add(int a, int b);
int gf_mul(int a, int b);
class GF256 {
public:
    vector<int> exp_table, log_table;
    GF256();
    int exp(int a);
    int log(int a);
};
class ReedSolomonEncoder {
private:
    int n, k;
    GF256 gf;
public:
    ReedSolomonEncoder(int n, int k);
    vector<int> encode(const vector<int>& data);
};
uint16_t Utf8ToSjis(const string& utf8char); // 获取Sjis编码
string Utf8ToGbk(const string& utf8Str); // UTF-8转GBK编码
#endif // !UTILS_H