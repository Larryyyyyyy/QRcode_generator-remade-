#ifndef UTILS_H
#define UTILS_H
#include <windows.h>
#include <assert.h>
#include<iostream>
#include <vector>
#include <string>
using namespace std;
class GF256 {
public:
    vector<int> exp_table, log_table;
    GF256();
    int exp(int a);
    int log(int a);
    int add(int a, int b);
    int mul(int a, int b);
    int inv(int n);
};
class ReedSolomonEncoder {
private:
    int n, k;
    GF256 gf;
public:
    ReedSolomonEncoder(int n, int k);
    vector<int> encode(const vector<int>& data);
};
class ReedSolomonDecoder {
private:
    int n, k;
    GF256 gf;
public:
    ReedSolomonDecoder(int n, int k);
    void decode(vector<int>& data);
    vector<vector<int>> constructMatrix(const vector<int>& syndromes); // 得到判定矩阵
    vector<int> detectErrorPos(const vector<vector<int>>& matrix); // 计算得到错误位置
    vector<vector<int>> constructEquations(vector<vector<int>>& matrix, const vector<int>& pos, const vector<int>& data);
    vector<int> calculateSyndromes(const vector<int>& data); // 计算伴随式
    void gaussianElimination(vector<vector<int>>& v); // 高斯消元
    vector<int> solveLinearEquations(vector<vector<int>>& matrix); // 解线性方程组
};
uint16_t Utf8ToSjis(const string& utf8char); // 获取Sjis编码
string SjisToUtf8(const uint16_t sjis); // 获取UTF-8编码
string Utf8ToGbk(const string& utf8Str); // UTF-8转GBK编码
#endif // !UTILS_H