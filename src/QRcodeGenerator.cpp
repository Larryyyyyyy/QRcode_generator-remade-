#include "../include/Parser.h"
#include "../include/Drawer.h"
#include "../include/Utils.h"
#include <windows.h>
#include <string>
#include <vector>
int main() {
    string s="https://github.com/Larryyyyyyy/QRcode_generator-remade-";
    encoder x(10, 'L', 7, s);
    write_bmp("recording.bmp", x.drawAll());
    decoder y(x.pixels);
    return 0;
}