#include "../include/Utils.h"
int gf_add(int a, int b) { return a ^ b; }
int gf_mul(int a, int b) {
    int p = 0;
    while (a && b) {
        if (b & 1) p ^= a;
        a <<= 1;
        if (a & 0x100) a ^= 0x11D;
        b >>= 1;
    }
    return p;
}
GF256::GF256() {
    exp_table.resize(512);
    log_table.resize(256);
    int x = 1;
    for (int i = 0; i < 255; ++i) {
        exp_table[i] = x;
        log_table[x] = i;
        x = gf_mul(x, 2);  // 生成指数表
    }
    for (int i = 255; i < 511; ++i) {
        exp_table[i] = exp_table[i - 255];  // 指数表后面是重复的
    }
}
int GF256::exp(int a) { 
    return exp_table[a];
}
int GF256::log(int a) {
    return log_table[a];
}
ReedSolomonEncoder::ReedSolomonEncoder(int n, int k) : n(n), k(k) {}
vector<int> ReedSolomonEncoder::encode(const vector<int>& data) {
    vector<int> msg = data;
    vector<int> syndromes(n - k, 0);  // 用于存储校验
    // 计算纠错码的多项式
    vector<int> generator_poly(n - k + 1, 0);
    generator_poly[0] = 1;
    for (int i = 0; i < n - k; ++i) {
        int root = gf.exp(i);
        for (int j = n - k; j > 0; --j) {
            generator_poly[j] = gf_add(generator_poly[j], gf_mul(generator_poly[j - 1], root));
        }
    }
    // 计算纠错码: 计算多项式的校验和(除法得到纠错码)
    vector<int> codeword(n, 0);
    for (int i = 0; i < k; ++i) {
        codeword[i] = msg[i];
    }
    for (int i = 0; i < k; ++i) {
        int coef = codeword[i];
        for (int j = 1; j < n - k + 1; ++j) {
            codeword[i + j] = gf_add(codeword[i + j], gf_mul(generator_poly[j], coef));
        }
    }
    // 将生成的纠错码加到最后, 最终得到带有纠错码的完整代码
    vector<int> encoded_msg(n, 0);
    for (int i = 0; i < k; ++i) {
        encoded_msg[i] = msg[i];
    }
    for (int i = 0; i < n - k; ++i) {
        encoded_msg[k + i] = codeword[k + i];
    }
    return encoded_msg;
}