#include "../include/Utils.h"
GF256::GF256() {
    exp_table.resize(512);
    log_table.resize(256);
    int x = 1;
    for (int i = 0; i < 255; ++i) {
        exp_table[i] = x;
        log_table[x] = i;
        int p = x << 1;
        if (p & 0x100) p ^= 0x11D;
        x = p;
    }
    for (int i = 255; i < 511; ++i) {
        exp_table[i] = exp_table[i - 255];
    }
}
int GF256::exp(int a) { 
    return exp_table[a];
}
int GF256::log(int a) {
    return log_table[a];
}
int GF256::add(int a, int b) { return a ^ b; }
int GF256::mul(int a, int b) {
    if (a == 0 || b == 0) return 0;
    return exp_table[log_table[a] + log_table[b]];
}
int GF256::inv(int n) {
    return exp_table[255 - log_table[n]];
}
ReedSolomonEncoder::ReedSolomonEncoder(int n, int k) : n(n), k(k) {}
vector<int> ReedSolomonEncoder::encode(const vector<int>& data) {
    vector<int> msg = data;
    vector<int> generator_poly(n - k + 1, 0);
    generator_poly[0] = 1;
    for (int i = 0; i < n - k; ++i) {
        int root = gf.exp(i);
        for (int j = n - k; j > 0; --j) {
            generator_poly[j] = gf.add(generator_poly[j], gf.mul(generator_poly[j - 1], root));
        }
    }
    vector<int> codeword(n, 0);
    for (int i = 0; i < k; ++i) {
        codeword[i] = msg[i];
    }
    for (int i = 0; i < k; ++i) {
        int coef = codeword[i];
        for (int j = 1; j < n - k + 1; ++j) {
            codeword[i + j] = gf.add(codeword[i + j], gf.mul(generator_poly[j], coef));
        }
    }
    vector<int> encoded_msg(n, 0);
    for (int i = 0; i < k; ++i) {
        encoded_msg[i] = msg[i];
    }
    for (int i = 0; i < n - k; ++i) {
        encoded_msg[k + i] = codeword[k + i];
    }
    return encoded_msg;
}
ReedSolomonDecoder::ReedSolomonDecoder(int n, int k) : n(n), k(k) {}
void ReedSolomonDecoder::decode(vector<int>& data) {
    auto syndromes = calculateSyndromes(data);
    // 如果伴随式都是 0, 说明没有发生错误
    bool contaminated = false;
    for(auto p : syndromes) {
        if (p) {
            contaminated = true;
            break;
        }
    }
    if (!contaminated) return;
    auto matrix = constructMatrix(syndromes);
    gaussianElimination(matrix);
    auto pos = detectErrorPos(matrix);
    matrix = constructEquations(matrix, pos, data);
    auto ans = solveLinearEquations(matrix);
    assert(!ans.empty());
    for(int i = 0; i < ans.size(); ++i) {
        data[pos[i]] = ans[i];
    }
}
vector<int> ReedSolomonDecoder::calculateSyndromes(const vector<int>& data) {
    vector<int> syndromes(n - k);
    for (int i = 0; i < n - k; ++i) {
        int s = 0;
        for (auto y : data) {
            s = gf.add(gf.mul(s, gf.exp_table[i]), y);
        }
        syndromes[i] = s;
    }
    return syndromes;
}
vector<vector<int>> ReedSolomonDecoder::constructMatrix(const vector<int>& syndromes) {
    vector<vector<int>> matrix;
    matrix.resize((n - k) >> 1);
    for(int i = 0; i < (n - k) >> 1; ++i) {
        matrix[i].resize(((n - k) >> 1) + 1);
        for(int j = 0; j < ((n - k) >> 1) + 1; ++j) {
            matrix[i][j] = syndromes[i + j];
        }
    }
    return matrix;
}
vector<int> ReedSolomonDecoder::detectErrorPos(const vector<vector<int>>& matrix) {
    int nums; // 解的个数
    int res;
    vector<int> pos;
    for(nums = 0; ; ++nums) {
        if (nums == (n - k) >> 1 || !matrix[nums][nums]) break;
    }
    for(int j = n - 1; j >= 0; --j) {
        res = 0;
        for(int i = 0; i < nums; ++i) {
            if (i == 0) {
                res = gf.add(res, matrix[i][nums]);
                continue;
            }
            int temp = gf.exp_table[j];
            for(int p = 0; p < i - 1; ++p) {
                temp = gf.mul(temp, gf.exp_table[j]);
            }
            res = gf.add(res, gf.mul(temp, matrix[i][nums]));
        }
        int temp = gf.exp_table[j];
        for(int p = 0; p < nums - 1; ++p) {
            temp = gf.mul(temp, gf.exp_table[j]);
        }
        res = gf.add(res, temp);
        if (!res) {
            pos.push_back(n - j - 1);
        }
    }
    assert(pos.size() == nums);
    return pos;
}
vector<vector<int>> ReedSolomonDecoder::constructEquations(vector<vector<int>>& matrix, const vector<int>& pos, const vector<int>& data) {
    matrix.clear();
    matrix.resize(pos.size());
    for(auto &p:matrix) {
        p.resize(pos.size() + 1);
    }
    int test, cnt, step;
    for(int i = 0 ; i < pos.size(); ++i) {
        cnt = test = step = 0;
        for(int j = 0; j < data.size(); ++j) {
            if(step < pos.size() && j == pos[step]) {
                matrix[i][step] = gf.exp_table[(n - cnt - 1) * i];
                ++step;
            }
            else test = gf.add(gf.mul(data[j], gf.exp_table[(n - cnt - 1) * i]), test);
            ++cnt;
        }
        matrix[i][pos.size()] = test;
    }
    return matrix;
}
void ReedSolomonDecoder::gaussianElimination(vector<vector<int>>& v) {
    int rows = v.size();
    int cols = v[0].size();
    int pivotRow = 0;
    for (int j = 0; j < cols && pivotRow < rows; ++j) {
        int sel = pivotRow;
        while (sel < rows && v[sel][j] == 0) {
            sel++;
        }
        if (sel == rows) continue;
        swap(v[sel], v[pivotRow]);
        int inv = gf.inv(v[pivotRow][j]);
        for (int k = j; k < cols; ++k) {
            v[pivotRow][k] = gf.mul(v[pivotRow][k], inv);
        }
        for (int i = 0; i < rows; ++i) {
            if (i != pivotRow && v[i][j] != 0) {
                int factor = v[i][j];
                for (int k = j; k < cols; ++k) {
                    v[i][k] ^= gf.mul(factor, v[pivotRow][k]);
                }
            }
        }
        pivotRow++;
    }
}
vector<int> ReedSolomonDecoder::solveLinearEquations(vector<vector<int>>& matrix) {
    int n = matrix.size();
    int cols = matrix[0].size();
    gaussianElimination(matrix);
    vector<int> result;
    result.resize(n);
    int pivotRow = 0;
    vector<int> pivotPos(n, -1);
    for (int i = 0; i < n; ++i) {
        int j = 0;
        while (j < n && matrix[i][j] == 0) j++;
        if (j < n) {
            pivotPos[i] = j;
            result[j] = matrix[i][n];
        } else {
            if (matrix[i][n] != 0) {
                return {};
            }
        }
    }
    for(int i = 0; i < n; ++i) {
        bool isPivotCol = false;
        for(int p : pivotPos) if (p == i) isPivotCol = true;
    }
    return result;
}