#include "../src/storage/DiskManager.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace std;

int main() {
    DiskManager dm("test.db");

    char write_buf[PAGE_SIZE] = {0};
    strcpy(write_buf, "Hola Base de Datos!");
    dm.writePage(0, write_buf);
    dm.sync();

    char read_buf[PAGE_SIZE] = {0};
    dm.readPage(0, read_buf);

    assert(strcmp(read_buf, "Hola Base de Datos!") == 0);
    cout << "✓ Persistencia básica funciona\n";
    return 0;
}