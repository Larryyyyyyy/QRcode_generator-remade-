#define STB_IMAGE_IMPLEMENTATION
#include "../include/Parser.h"
#include "../include/Drawer.h"
#include "../include/Utils.h"
#include "../include/Image.h"
#include "../include/Stb_image.h"
#include <windows.h>
#include <string>
#include <vector>
int main() {
    string s = "https://github.com/Larryyyyyyy/QRcode_generator-remade-";
    encoder x(10, 'L', 7, s);
    write_bmp("recording.bmp", x.drawAll());
    imageHandler p("recording.bmp");
    decoder y(p.pixels);
    return 0;
}