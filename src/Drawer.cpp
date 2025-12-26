#include "../include/Drawer.h"
void write_bmp(const char* filename, const vector<vector<uint32_t>>& pixels) {
    int width = pixels[0].size();
    int height = pixels.size();
    int row_padding = (4 - (width * 3) % 4) % 4; // 计算每行填充字节数
    uint32_t pixel_data_size = (width * 3 + row_padding) * height;

    // 填充文件头
    BitmapFileHeader file_header{};
    file_header.bfType = 0x4D42; // 'BM'
    file_header.bfSize = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + pixel_data_size;
    file_header.bfOffBits = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);

    // 填充信息头
    BitmapInfoHeader info_header{};
    info_header.biSize = sizeof(BitmapInfoHeader);
    info_header.biWidth = width;
    info_header.biHeight = height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 24; // 24位色（BGR格式）
    info_header.biSizeImage = pixel_data_size;

    // 写入文件
    ofstream file(filename, ios::binary);
    file.write(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    file.write(reinterpret_cast<char*>(&info_header), sizeof(info_header));

    // 写入像素数据（从最后一行开始）
    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            uint32_t color = pixels[y][x];
            // 分解为B、G、R三个字节
            uint8_t blue = color & 0xFF;
            uint8_t green = (color >> 8) & 0xFF;
            uint8_t red = (color >> 16) & 0xFF;
            file.write(reinterpret_cast<const char*>(&blue), 1);
            file.write(reinterpret_cast<const char*>(&green), 1);
            file.write(reinterpret_cast<const char*>(&red), 1);
        }
        // 写入填充字节
        uint8_t padding[3] = { 0 };
        file.write(reinterpret_cast<const char*>(padding), row_padding);
    }
}