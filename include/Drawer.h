#ifndef DRAWER_H
#define DRAWER_H
#include <fstream>
#include <vector>
#pragma pack(push, 1) // 禁用内存对齐
using namespace std;
struct BitmapFileHeader {
    uint16_t bfType;          // 文件类型, 必须是0x4D42(即'BM')
    uint32_t bfSize;          // 文件大小(字节)
    uint16_t bfReserved1;     // 保留, 设为0
    uint16_t bfReserved2;     // 保留, 设为0
    uint32_t bfOffBits;       // 从文件头到像素数据的偏移量
};
struct BitmapInfoHeader {
    uint32_t biSize;          // 信息头大小(40字节)
    int32_t  biWidth;         // 图像宽度(像素)
    int32_t  biHeight;        // 图像高度(像素)
    uint16_t biPlanes;        // 颜色平面数, 必须为1
    uint16_t biBitCount;      // 每像素位数(24表示24位色)
    uint32_t biCompression;   // 压缩方式(0表示不压缩)
    uint32_t biSizeImage;     // 像素数据大小(字节)
    int32_t  biXPelsPerMeter; // 水平分辨率(像素/米, 可设为0)
    int32_t  biYPelsPerMeter; // 垂直分辨率(像素/米, 可设为0)
    uint32_t biClrUsed;       // 调色板颜色数(0表示使用所有)
    uint32_t biClrImportant;  // 重要颜色数(0表示所有都重要)
};
#pragma pack(pop)
void write_bmp(const char* filename, const vector<vector<uint32_t>>& pixels);
#endif // !DRAWER_H