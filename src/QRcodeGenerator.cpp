#include "../include/Parser.h"
#include "../include/Drawer.h"
#include "../include/Utils.h"
#include <windows.h>
#include <string>
#include <vector>
int main() {
    string s="https://github.com/Larryyyyyyy/QRcode_generator-remade-";
    QRcode x(10, 'H', 3, s);
    write_bmp("../out/1.bmp", x.drawAll());
    return 0;
}