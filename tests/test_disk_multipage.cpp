#include "../src/storage/DiskManager.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <array>
using namespace std;

// Rellena un buffer con un patron repetido basado en page_id
static void fillPattern(char* buf, uint32_t page_id) {
    for (uint32_t i = 0; i < PAGE_SIZE; i++)
        buf[i] = static_cast<char>((page_id * 37 + i) & 0xFF);
}

// Verifica que el buffer tiene el patron esperado
static bool checkPattern(const char* buf, uint32_t page_id) {
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        char expected = static_cast<char>((page_id * 37 + i) & 0xFF);
        if (buf[i] != expected) return false;
    }
    return true;
}

void test1_escritura_lectura_no_contigua() {
    cout << "Escritura/lectura en páginas no contiguas... ";
    DiskManager dm("test_noncontiguous.db");

    uint32_t pids[] = {0, 3, 7, 50};
    char write_buf[PAGE_SIZE];

    // Escribir patron unico en cada pagina
    for (uint32_t pid : pids) {
        fillPattern(write_buf, pid);
        dm.writePage(pid, write_buf);
    }
    dm.sync();

    // Leer y verificar cada pagina
    char read_buf[PAGE_SIZE];
    for (uint32_t pid : pids) {
        dm.readPage(pid, read_buf);
        assert(checkPattern(read_buf, pid) && "Patron corrupto en pagina no contigua");
    }
    cout << "OK\n";
}

void test2_no_contaminacion_entre_paginas() {
    cout << "No contaminación entre páginas adyacentes... ";
    DiskManager dm("test_contamination.db");

    char buf_a[PAGE_SIZE], buf_b[PAGE_SIZE];
    fillPattern(buf_a, 10);
    fillPattern(buf_b, 11);

    dm.writePage(10, buf_a);
    dm.writePage(11, buf_b);
    dm.sync();

    char read_a[PAGE_SIZE], read_b[PAGE_SIZE];
    dm.readPage(10, read_a);
    dm.readPage(11, read_b);

    assert(checkPattern(read_a, 10) && "Página 10 contaminada por página 11");
    assert(checkPattern(read_b, 11) && "Página 11 contaminada por página 10");
    cout << "OK\n";
}

void test3_sobreescritura() {
    cout << "Sobreescritura de página existente... ";
    DiskManager dm("test_overwrite.db");

    char original[PAGE_SIZE] = {0};
    strcpy(original, "DATOS ORIGINALES");
    dm.writePage(2, original);
    dm.sync();

    char nuevo[PAGE_SIZE] = {0};
    strcpy(nuevo, "DATOS NUEVOS - SOBREESCRITOS");
    dm.writePage(2, nuevo);
    dm.sync();

    char leido[PAGE_SIZE] = {0};
    dm.readPage(2, leido);
    assert(strcmp(leido, "DATOS NUEVOS - SOBREESCRITOS") == 0 && "Sobreescritura fallida");
    cout << "OK\n";
}

void test4_pagina_no_escrita_devuelve_ceros() {
    cout << "Página nunca escrita devuelve ceros... ";
    DiskManager dm("test_zeros.db");

    // Aseguramos que existe al menos la pagina 0
    char dummy[PAGE_SIZE] = {0};
    dm.writePage(0, dummy);

    // Pagina 20 nunca escrita
    char buf[PAGE_SIZE];
    memset(buf, 0xAB, PAGE_SIZE);  // rellenar con basura para detectar si no se limpia
    dm.readPage(20, buf);

    for (uint32_t i = 0; i < PAGE_SIZE; i++)
        assert(buf[i] == 0 && "Página inexistente no fue rellenada con ceros");
    cout << "OK\n";
}

void test5_num_pages() {
    cout << "numPages() refleja páginas escritas... ";
    DiskManager dm("test_numpages.db");

    assert(dm.numPages() == 0);

    char buf[PAGE_SIZE] = {0};
    dm.writePage(0, buf);
    assert(dm.numPages() == 1);

    dm.writePage(4, buf);
    // writePage solo actualiza numPages si page_id >= num_pages_
    // page_id=4 >= 1, entonces numPages = 5
    assert(dm.numPages() == 5 && "numPages no refleja el máximo page_id+1");
    cout << "OK\n";
}

int main() {
    cout << "------ DiskManager — Multi-página ------\n";
    try {
        test1_escritura_lectura_no_contigua();
        test2_no_contaminacion_entre_paginas();
        test3_sobreescritura();
        test4_pagina_no_escrita_devuelve_ceros();
        test5_num_pages();
        cout << "\n✓ Todos los tests de lecturas y escrituras multi-pagina pasaron.\n";
    } catch (const DiskException& e) {
        cerr << "ERROR DiskException: " << e.what() << "\n";
        return 1;
    } catch (const exception& e) {
        cerr << "ERROR inesperado: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
