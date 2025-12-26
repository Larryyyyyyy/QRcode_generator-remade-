#include "../include/Parser.h"
#include "../include/Utils.h"
#include <windows.h>
QRcode::QRcode(int scale, int errorCorrectionLevel, int mask, string text)
    : scale(scale), errorCorrectionLevel(errorCorrectionLevel), mask(mask), text(text) {
	memset(filled, 0, sizeof(filled));
    index = errorCorrectionLevel == 'H' ? 0 : errorCorrectionLevel == 'Q' ? 1 : errorCorrectionLevel == 'M' ? 2 : 3;
	mode = judgeMode();
	version = confirmVersion();
	width = ((version - 1) * 4 + 21), height = ((version - 1) * 4 + 21);
	vector<uint32_t> temp(this->width + 2, 2);
	for (int i = 0; i <= this->height + 2; ++i) pixels.push_back(temp);
	rwidth = ((this->width + 2) * this->scale);
	rheight = ((this->height + 2) * this->scale);
	vector<uint32_t> rtemp(this->rwidth, 2);
	for (int i = 0; i <= this->rheight; ++i) rpixels.push_back(rtemp);
}
QRcode::~QRcode() {}
vector<vector<uint32_t>> QRcode::drawAll() {
	int len = getData();
	len = getExtraData();
	drawPositionDetectionPattern();
	drawTimingPattern();
	drawAlignmentPattern();
	drawFormatInformation();
	drawVersionInformation();
	drawDataInformation();
	drawMaskcode();
	for (int i = 0; i <= width + 1; ++i) {
		for (int j = 0; j <= height + 1; ++j) {
			for (int k = i * scale; k <= i * scale + scale - 1; ++k) {
				for (int t = j * scale; t <= j * scale + scale - 1; ++t) {
					rpixels[k][t] = (!pixels[i][j]) ? 0x00000000 : 0x00FFFFFF;
				}
			}
		}
	}
	return rpixels;
}
void QRcode::drawPositionDetectionPattern() {
	/*
	绘制定位图案(也就是三个大方块)
	*/
	for (int i = 1; i <= 7; ++i) {
		for (int j = 1; j <= 7; ++j) {
			if ((i==2&&2<=j&&j<=6)||(i>=3&&i<=5&&(j==2||j==6))||(i==6&&2<=j&&j<=6)) pixels[i][j] = pixels[i + width - 7][j] = pixels[i][j + width - 7] = 1;
			else pixels[i][j] = pixels[i + width - 7][j] = pixels[i][j + width - 7] = 0;
		}
	}
	for (int i = 1; i <= 8; ++i) {
		pixels[i][8] = pixels[i][width - 8 + 1] = pixels[width - i + 1][8] = 1;
		pixels[8][i] = pixels[8][width - i + 1] = pixels[width - 8 + 1][i] = 1;
	}
}
void QRcode::drawTimingPattern() {
	/*
	绘制时序图案(也就是横纵各一条的交替黑白线)
	*/
	uint32_t x = 0;
	for (int i = 9; i <= width - 8; ++i) {
		pixels[7][i] = x;
		x ^= 1;
	}
	x = 0;
	for (int i = 9; i <= width - 8; ++i) {
		pixels[i][7] = x;
		x ^= 1;
	}
}
void QRcode::drawAlignmentPattern() {
	/*
	绘制对齐图案(小方块)
	*/
	if (2 <= version && version <= 6) {
		int pos = version * 4 + 11;
		pixels[pos][pos] = 0;
		for (int i = pos - 2; i <= pos + 2; ++i) {
			pixels[pos - 2][i] = pixels[i][pos - 2] = pixels[pos + 2][i] = pixels[i][pos + 2] = 0;
		}
		for (int i = pos - 1; i <= pos + 1; ++i) {
			pixels[pos - 1][i] = pixels[i][pos - 1] = pixels[pos + 1][i] = pixels[i][pos + 1] = 1;
		}
	}
	if (7 <= version && version <= 13) {
		int pos[4] = { 0,7,version * 2 + 9, version * 4 + 11};
		for (int i = 1; i <= 3; ++i) {
			for (int j = 1; j <= 3; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 3) || (i == 3 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (14 <= version && version <= 16) {
		int pos[5] = {0,7,27,version * 2 + 19, version * 4 + 11};
		for (int i = 1; i <= 4; ++i) {
			for (int j = 1; j <= 4; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 4) || (i == 4 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (17 <= version && version <= 19) {
		int pos[5] = {0, 7, 30, version * 2 + 21, version * 4 + 11};
		for (int i = 1; i <= 4; ++i) {
			for (int j = 1; j <= 4; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 4) || (i == 4 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (version == 20) {
		int pos[5] = {0, 7, 35, 63, 91};
		for (int i = 1; i <= 4; ++i) {
			for (int j = 1; j <= 4; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 4) || (i == 4 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (version == 21) {
		int pos[6] = {0, 7, 29, 51, 73, 95};
		for (int i = 1; i <= 5; ++i) {
			for (int j = 1; j <= 5; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 5) || (i == 5 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (version == 22) {
		int pos[6] = {0, 7, 27, 51, 75, 99};
		for (int i = 1; i <= 5; ++i) {
			for (int j = 1; j <= 5; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 5) || (i == 5 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (version == 23) {
		int pos[6] = {0, 7, 31, 55, 79, 103};
		for (int i = 1; i <= 5; ++i) {
			for (int j = 1; j <= 5; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 5) || (i == 5 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (version == 24) {
		int pos[6] = {0, 7, 29, 55, 81, 107};
		for (int i = 1; i <= 5; ++i) {
			for (int j = 1; j <= 5; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 5) || (i == 5 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (version == 25) {
		int pos[6] = {0, 7, 33, 59, 85, 111};
		for (int i = 1; i <= 5; ++i) {
			for (int j = 1; j <= 5; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 5) || (i == 5 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (version == 26) {
		int pos[6] = {0, 7, 31, 59, 87, 115};
		for (int i = 1; i <= 5; ++i) {
			for (int j = 1; j <= 5; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 5) || (i == 5 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (version == 27) {
		int pos[6] = {0, 7, 35, 63, 91, 119};
		for (int i = 1; i <= 5; ++i) {
			for (int j = 1; j <= 5; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 5) || (i == 5 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (28 <= version && version <= 29) {
		int pos[7] = {0, 7, version * 4 - 85, version * 4 - 61, version * 4 - 37, version * 4 - 13, version * 4 + 11};
		for (int i = 1; i <= 6; ++i) {
			for (int j = 1; j <= 6; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 6) || (i == 6 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (30 <= version && version <= 32) {
		int pos[7] = {0, 7, version * 4 - 93, version * 4 - 67, version * 4 - 41, version * 4 - 15, version * 4 + 11};
		for (int i = 1; i <= 6; ++i) {
			for (int j = 1; j <= 6; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 6) || (i == 6 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (33 <= version && version <= 34) {
		int pos[7] = {0, 7, version * 4 - 101, version * 4 - 73, version * 4 - 45, version * 4 - 17, version * 4 + 11};
		for (int i = 1; i <= 6; ++i) {
			for (int j = 1; j <= 6; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 6) || (i == 6 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (version == 35) {
		int pos[8] = {0, 7, 31, 55, 79, 103, 127, 151};
		for (int i = 1; i <= 7; ++i) {
			for (int j = 1; j <= 7; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 7) || (i == 7 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (36 <= version && version <= 38) {
		int pos[8] = {0, 7, version * 4 - 113, version * 4 - 93, version * 4 - 68, version * 4 - 41, version * 4 - 15, version * 4 + 11};
		for (int i = 1; i <= 7; ++i) {
			for (int j = 1; j <= 7; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 7) || (i == 7 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
	if (39 <= version && version <= 40) {
		int pos[8] = {0, 7, version * 4 - 129, version * 4 - 101, version * 4 - 73, version * 4 - 45, version * 4 - 17, version * 4 + 11};
		for (int i = 1; i <= 7; ++i) {
			for (int j = 1; j <= 7; ++j) {
				if ((i == 1 && j == 1) || (i == 1 && j == 7) || (i == 7 && j == 1)) continue;
				pixels[pos[i]][pos[j]] = 0;
				for (int k = pos[i] - 2; k <= pos[i] + 2; ++k) {
					pixels[pos[i] - 2][k - pos[i] + pos[j]] = pixels[k][pos[j] - 2] = pixels[pos[i] + 2][k - pos[i] + pos[j]] = pixels[k][pos[j] + 2] = 0;
				}
				for (int k = pos[i] - 1; k <= pos[i] + 1; ++k) {
					pixels[pos[i] - 1][k - pos[i] + pos[j]] = pixels[k][pos[j] - 1] = pixels[pos[i] + 1][k - pos[i] + pos[j]] = pixels[k][pos[j] + 1] = 1;
				}
			}
		}
	}
}
void QRcode::drawFormatInformation() {
	/*
	绘制格式信息(也就是纠错等级+掩码模式)
	*/
	int x = 0, rec = 0x537, code = 0;
	if (errorCorrectionLevel == 'M') x = 0;
	if (errorCorrectionLevel == 'L') x = 1;
	if (errorCorrectionLevel == 'H') x = 2;
	if (errorCorrectionLevel == 'Q') x = 3;
	code = x = ((x << 3) + mask) << 10;
	for (int i = 4; i >= 0; --i) {
		if ((x & (1 << (i + 10))) != 0) {
			x ^= rec << i;
		}
	}
	code += x;
	code ^= 21522;
	x = code;
	for (int i = width; i >= width - 7; --i) {
		pixels[9][i] = !(x & 1);
		x >>= 1;
	}
	pixels[height - 7][9] = 0;
	for (int i = height - 6; i <= height; ++i) {
		pixels[i][9] = !(x & 1);
		x >>= 1;
	}
	x = code;
	for (int i = 1; i <= 6; ++i) {
		pixels[i][9] = !(x & 1);
		x >>= 1;
	}
	pixels[8][9] = !(x & 1), x >>= 1;
	pixels[9][9] = !(x & 1), x >>= 1;
	pixels[9][8] = !(x & 1), x >>= 1;
	for (int i = 6; i >= 1; --i) {
		pixels[9][i] = !(x & 1);
		x >>= 1;
	}
}
void QRcode::drawVersionInformation() {
	/*
	绘制版本信息(版本7及以上才有)
	*/
	if (version < 7 || version>40) return;
	int x = version << 12, rec = 0x1f25;
	for (int i = 5; i >= 0; --i) {
		if ((x & (1 << (i + 12))) != 0) {
			x ^= rec << i;
		}
	}
	rec = x = ((version << 12) + x);
	for (int j = 1; j <= 6; ++j) {
		for (int i = height - 10; i <= height - 8; ++i) {
			pixels[i][j] = !(x & 1);
			x >>= 1;
		}
	}
	x = rec;
	for (int i = 1; i <= 6; ++i) {
		for (int j = width - 10; j <= width - 8; ++j) {
			pixels[i][j] = !(x & 1);
			x >>= 1;
		}
	}
}
void QRcode::drawDataInformation() {
	/*
	蛇形绘制数据信息(也就是实际存储的信息)
	*/
	int range = 1;
	bool up = 1;
	for (int j = width - 1; j >= 1; j -= 2) {
		if (j == 6)--j;
		if (up) {
			for (int i = height; i >= 1; --i) {
				if (pixels[i][j + 1] == 2)pixels[i][j + 1] = rdata[range++] == '1' ? 1 : 0, filled[i][j + 1] = 1;
				if (pixels[i][j] == 2)pixels[i][j] = rdata[range++] == '1' ? 1 : 0, filled[i][j] = 1;
			}
			up = 0;
			continue;
		}
		if (!up) {
			for (int i = 1; i <= height; ++i) {
				if (pixels[i][j + 1] == 2)pixels[i][j + 1] = rdata[range++] == '1' ? 1 : 0, filled[i][j + 1] = 1;
				if (pixels[i][j] == 2)pixels[i][j] = rdata[range++] == '1' ? 1 : 0, filled[i][j] = 1;
			}
			up = 1;
			continue;
		}
	}
}
void QRcode::drawMaskcode() {
	/*
	应用掩码模式(使得二维码更易于扫描)
	*/
	for (int i = 1; i <= height; ++i) {
		for (int j = 1; j <= width; ++j) {
			if (filled[i][j]) {
				if (mask == 0) {
					if ((i - 1 + j - 1) % 2 == 0) pixels[i][j] ^= 0;
					else pixels[i][j] ^= 1;
				}
				if (mask == 1) {
					if ((i - 1) % 2 == 0) pixels[i][j] ^= 0;
					else pixels[i][j] ^= 1;
				}
				if (mask == 2) {
					if ((j - 1) % 3 == 0) pixels[i][j] ^= 0;
					else pixels[i][j] ^= 1;
				}
				if (mask == 3) {
					if ((i - 1 + j - 1) % 3 == 0) pixels[i][j] ^= 0;
					else pixels[i][j] ^= 1;
				}
				if (mask == 4) {
					if (((i - 1) / 2 + (j - 1) / 3) % 2 == 0) pixels[i][j] ^= 0;
					else pixels[i][j] ^= 1;
				}
				if (mask == 5) {
					if ((i - 1) * (j - 1) % 2 + (i - 1) * (j - 1) % 3 == 0) pixels[i][j] ^= 0;
					else pixels[i][j] ^= 1;
				}
				if (mask == 6) {
					if (((i - 1) * (j - 1) % 2 + (i - 1) * (j - 1) % 3) % 2 == 0) pixels[i][j] ^= 0;
					else pixels[i][j] ^= 1;
				}
				if (mask == 7) {
					if (((i - 1) + (j - 1) % 2 + (i - 1) * (j - 1) % 3) % 2 == 0) pixels[i][j] ^= 0;
					else pixels[i][j] ^= 1;
				}
			}
		}
	}
}
int QRcode::judgeMode() {
    /*
    模式判断规则:
    1. 数字模式: 仅包含数字0-9
    2. 数字字母模式: 包含数字0-9和大写字母A-Z及部分特殊字符(空格、$、%、*、+、-、.、/、:)
    3. 字节模式: 包含除数字字母模式外的其他ASCII字符
    4. 汉字模式: 包含汉字字符(通常为UTF-8编码)
    */
	for (unsigned char c : text) {
		if ((c & 0xE0) == 0xE0) {  // 判断是否为UTF-8编码的汉字
            for (int i = 0; i < text.size();) {
                string oneChar;
                if ((unsigned char)text[i] < 0x80) {
                    oneChar = text.substr(i, 1); i += 1;
                } else {
                    oneChar = text.substr(i, 3); i += 3;
                }
                auto num = Utf8ToSjis(oneChar);
                if (num < 0x8140 || num > 0xFEFE || (num >= 0xA0A0 && num <= 0xDFDF) || (num % 256) < 0x40 || (num % 256) == 0x7F || (num % 256) > 0xFC) {
                    text = Utf8ToGbk(text);
                    return 4; // 汉字模式不支持的汉字字符, 返回字节模式
                }
            }
			return mode = 8;
		}
	}
	int rmode = 1; // 默认数字模式
	bool f = 0;
	for (unsigned char c : text) {
		if (c >= '0' && c <= '9') continue;
		if (c >= 'A' && c <= 'Z') {
			if (rmode == 1) rmode = 2;
			continue;
		}
		f = 0;
		for (auto alpha : alphaMode) {
			if (alpha == c) {
				if (rmode == 1) rmode = 2;
				f = 1;
				break;
			}
		}
		if (f) continue;
		return 4;
	}
	return rmode;
}
int QRcode::confirmVersion() {
    /*
    不同纠错等级, 不同模式下, 不同版本的最大数据容量不同
    */
	if (mode == 1) { // 数字模式
		for (int i = 0; i < dataCapacity[0][index].size(); ++i) {
			if (dataCapacity[0][index][i] >= text.size()) return i + 1;
		}
	}
	if (mode == 2) { // 字符模式
		for (int i = 0; i < dataCapacity[1][index].size(); ++i) {
			if (dataCapacity[1][index][i] >= text.size()) return i + 1;
		}
	}
	if (mode == 4) { // 字节模式
		for (int i = 0; i < dataCapacity[2][index].size(); ++i) {
			if (dataCapacity[2][index][i] >= text.size()) return i + 1;
		}
	}
	if (mode == 8) { // 汉字模式
		for (int i = 0; i < dataCapacity[3][index].size(); ++i) {
			if (dataCapacity[3][index][i] >= text.size()) return i + 1;
		}
	}
	return 0;
}
int QRcode::getData() {
    /*
    解析输入数据, 生成对应的二进制数据流
    */
	int len = text.size(), range = 0;
	int rec = 0, tot = bitLength[index][version] * 8;
	if (mode == 1) {
		/*
		数字模式
		1. 模式指示符: 4位, 数字模式为0001
		2. 字符计数指示符: 10/12/14位, 取决于版本号
		3. 数据编码: 每3位数字编码为10位二进制, 最后剩余1位编码为4位, 剩余2位编码为7位
		*/
		data[1] = '0', data[2] = '0', data[3] = '0', data[4] = '1';
		if (version <= 9) range = 14;
		if (version > 9 && version <= 26) range = 16;
		if (version >= 27) range = 18;
		for (int i = range; i >= 5; --i) {
			data[i] = len % 2 + 48;
			len >>= 1;
		}
		len = text.size();
		for (int i = 0; i < len; i+=3) {
			if (i == len - 1) { // 只剩一位
				rec = text[i] - 48;
				for (int j = range + 4; j > range; --j) {
					data[j] = (rec >> 1) + 48;
					rec >>= 1;
				}
				range += 4;
				break;
			}
			if (i == len - 2) { // 只剩两位
				rec = (text[i] - 48) * 10 + (text[i + 1] - 48);
				for (int j = range + 7; j > range; --j) {
					data[j] = rec % 2 + 48;
					rec >>= 1;
				}
				range += 7;
				break;
			}
			rec = (text[i] - 48) * 100 + (text[i + 1] - 48) * 10 + (text[i + 2] - 48);
			for (int j = range + 10; j > range; --j) {
				data[j] = rec % 2 + 48;
				rec >>= 1;
			}
			range += 10;
		}
	}
	if (mode == 2) {
		/*
		字符模式
		1. 模式指示符: 4位, 字符模式为0010
		2. 字符计数指示符: 9/11/13位, 取决于版本号
		3. 数据编码: 每2位字符编码为11位二进制, 最后剩余1位编码为6位
		*/
		data[1] = '0', data[2] = '0', data[3] = '1', data[4] = '0';
		if (version <= 9) range = 13;
		if (version > 9 && version <= 26) range = 15;
		if (version >= 27) range = 17;
		for (int i = range; i >= 5; --i) {
			data[i] = len % 2 + 48;
			len >>= 1;
		}
		len = text.size();
		for (int i = 0, x, y; i < len; i += 2) {
			x = text[i];
			if (isdigit(x)) x = x - 48;
			else if (x >= 'A' && x <= 'Z') x -= 55;
			else if (x == ' ') x = 36;
			else if (x == '$') x = 37;
			else if (x == '%') x = 38;
			else if (x == '*') x = 39;
			else if (x == '+') x = 40;
			else if (x == '-') x = 41;
			else if (x == '.') x = 42;
			else if (x == '/') x = 43;
			else if (x == ':') x = 44;
			if (i + 1 == len) { // 只剩一位
				rec = x;
				for (int i = range + 6; i > range; --i) {
					data[i] = rec % 2 + 48;
					rec >>= 1;
				}
				range += 6;
				break;
			}
			y = text[i + 1];
			if (isdigit(y)) y = y - 48;
			else if (y >= 'A' && y <= 'Z') y -= 55;
			else if (y == ' ') y = 36;
			else if (y == '$') y = 37;
			else if (y == '%') y = 38;
			else if (y == '*') y = 39;
			else if (y == '+') y = 40;
			else if (y == '-') y = 41;
			else if (y == '.') y = 42;
			else if (y == '/') y = 43;
			else if (y == ':') y = 44;
			rec = x * 45 + y;
			for (int i = range + 11; i > range; --i) {
				data[i] = rec % 2 + 48;
				rec >>= 1;
			}
			range += 11;
		}
	}
	if (mode == 4) {
		/*
		字节模式
		1. 模式指示符: 4位, 字节模式为0100
		2. 字符计数指示符: 8/16位, 取决于版本号
		3. 数据编码: 每个字符编码为8位二进制
		*/
        data[1]='0', data[2]='1', data[3]='0', data[4]='0';
        if (version <= 9) range = 12;
        if (version > 9) range = 20;
        len = text.length();
        for (int i = range; i >= 5; --i) {
            data[i] = (len % 2) + 48;
            len >>= 1;
        }
		len = text.length();
        for (unsigned char c : text) {
            for (int j = range + 8; j > range; --j) {
                data[j] = (c % 2) + 48;
                c >>= 1;
            }
            range += 8;
        }
        for (int i = 1; i <= 4; ++i) { // 我也忘记为什么要加这个循环, 去掉也能跑
            data[++range] = 48;
        }
    }
	if (mode == 8) {
		/*
		汉字模式
		1. 模式指示符: 4位, 汉字模式为1000
		2. 字符计数指示符: 8/16位, 取决于版本号
		3. 数据编码: 每个汉字编码为13位二进制(经过特殊压缩)
		*/
        data[1] = '1', data[2] = '0', data[3] = '0', data[4] = '0';
        int charCount = 0;
        for(size_t i=0; i<text.length(); ) {
            unsigned char c = text[i];
            if(c < 0x80) i++;
            else i += 3;
            charCount++;
        }
        if (version <= 9) range = 12;
        if (version > 9 && version <= 26) range = 14;
        if (version > 26) range = 16;
        for (int i = range; i >= 5; --i) {
            data[i] = charCount % 2 + 48;
            charCount >>= 1;
        }
        for (size_t i = 0; i < text.length(); ) {
            string oneChar;
            if ((unsigned char)text[i] < 0x80) {
                oneChar = text.substr(i, 1); i += 1;
            } else {
                oneChar = text.substr(i, 3); i += 3;
            }
            uint16_t gbk = Utf8ToSjis(oneChar);
            if (gbk >= 0x8140 && gbk <= 0x9FFC) gbk -= 0x8140;
            else if (gbk >= 0xE040 && gbk <= 0xEBBF) gbk -= 0xC140;
            uint16_t high = gbk >> 8;
            uint16_t low = gbk & 0xFF;
            uint16_t compressed = high * 0xC0 + low;
            for (int j = range + 13; j > range; --j) {
                data[j] = (compressed % 2) + 48;
                compressed >>= 1;
            }
            range += 13;
        }
	}
	/*
	特殊的末尾填充规则
	1. 如果剩余位数大于等于4, 则在末尾填充4个0, 然后补齐到字节边界, 再用交替的11101100和00010001填充
	2. 如果剩余位数小于4, 则在末尾填充0
	*/
	int extra[17] = {0,1,1,1,0,1,1,0,0,0,0,0,1,0,0,0,1};
	if (tot - range >= 4) {
		for (int i = 1; i <= 4; ++i) {
			data[++range] = 48;
		}
	}
	else {
		for (int i = 1; i <= 4; ++i) {
			data[++range] = 48;
			if (range == tot) return range;
		}
	}
	while (range % 8) {
		data[++range] = 48;
	}
	if (range < tot) {
		int t = 1;
		while (range < tot) {
			data[++range] = extra[(t++ - 1) % 16 + 1] + 48;
		}
	}
	return range;
}
int QRcode::getExtraData() {
	/*
	纠错码生成与数据交织
	1. 根据版本号和纠错等级, 确定数据块的数量和每个数据块的长度
	2. 对每个数据块进行Reed-Solomon编码, 生成纠错码
	3. 将数据块和纠错码进行交织, 生成最终的数据流
	*/
	vector<int> temp;
	vector<vector<int>> rec;
	int range = 1;
	for (int i = 1; i <= blockNum1[index][version]; ++i) {
		temp.clear();
		for (int j = 1; j <= (blockLength1[index][version] << 3); ++j) {
			temp.push_back(data[j + (i - 1) * (blockLength1[index][version] << 3)]);
		}
		rec.push_back(solveBlockData(temp));
	}
	if (blockNum2[index][version]) { // 不止一组
		for (int i = 1; i <= blockNum2[index][version]; ++i) {
			temp.clear();
			for (int j = 1; j <= (blockLength2[index][version] << 3); ++j) {
				temp.push_back(data[j + (i - 1) * (blockLength2[index][version] << 3) + blockNum1[index][version] * (blockLength1[index][version] << 3)]);
			}
			rec.push_back(solveBlockData(temp));
		}
		for (int i = 0; i < blockLength2[index][version]; ++i) {
			for (int j = 0; j < blockNum1[index][version] + blockNum2[index][version]; ++j) {
				if (j < blockNum1[index][version]) {
					if (i == blockLength1[index][version]) continue;
				}
				for (int k = range + 7, x = rec[j][i]; k >= range; --k) rdata[k] = x % 2 + 48, x >>= 1;
				range += 8;
			}
		}
		for (int i = blockLength1[index][version]; i < rec[0].size(); ++i) {
			for (int j = 0; j < blockNum1[index][version]; ++j) {
				for (int k = range + 7, x = rec[j][i]; k >= range; --k) rdata[k] = x % 2 + 48, x >>= 1;
				range += 8;
			}
			for (int j = blockNum1[index][version]; j < blockNum1[index][version] + blockNum2[index][version]; ++j) {
				for (int k = range + 7, x = rec[j][i + 1]; k >= range; --k) rdata[k] = x % 2 + 48, x >>= 1;
				range += 8;
			}
		}
	}
	else {
		temp.clear();
		for (int i = 0; i < rec[0].size(); ++i) {
			for (int j = 0; j < blockNum1[index][version]; ++j) {
				for (int k = range + 7, x = rec[j][i]; k >= range; --k) {
					rdata[k] = x % 2 + 48;
					x >>= 1;
				}
				range += 8;
			}
		}
	}
	if (14 <= version && version <= 20) {
		for (int i = 1; i <= 3; ++i) rdata[range++] = 0;
	}
	if (2 <= version && version <= 6) {
		for (int i = 1; i <= 7; ++i) rdata[range++] = 0;
	}
	return range - 1;
}
vector<int> QRcode::solveBlockData(vector<int> v) {
	/*
	对单个数据块进行Reed-Solomon编码, 生成纠错码
	*/
	vector<int> blockData;
	int len1 = v.size() / 8, len2 = errorLength[index][version];
	for (int i = 0, range = 0, x; i < len1; ++i) {
		x = 0;
		for (int j = range; j <= range + 7; ++j) x = (x << 1) + v[j] - 48;
		range += 8;
		blockData.push_back(x);
	}
	ReedSolomonEncoder encoder(len1 + len2, len1); // 总长度和数据码长度
	return encoder.encode(blockData);
}