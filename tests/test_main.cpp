#include <iostream>
#include <cassert>
#include <cstring>
#include "../src/storage/DiskManager.hpp"
#include "../src/storage/StorageManager.hpp"
using namespace std;

void testDiskManager() {
    cout << "DiskManager\n";

    DiskManager dm("test_s2.db");

    // Escribir datos en la página 0
    char write_buf[PAGE_SIZE] = {0};
    strcpy(write_buf, "Base de datos - avance 2");
    dm.writePage(0, write_buf);
    dm.sync();

    // Leer de vuelta y verificar
    char read_buf[PAGE_SIZE] = {0};
    dm.readPage(0, read_buf);
    assert(strcmp(read_buf, write_buf) == 0);
    cout << "Escritura y lectura correcta\n";

    // Verificar que el contador de páginas se actualiza
    assert(dm.numPages() == 1);
    cout << "numPages() = " << dm.numPages() << "\n";

    // Leer una página que no existe aún: debe devolver ceros, no crashear
    char empty_buf[PAGE_SIZE] = {1}; // inicializar con 1s para detectar si no se limpia
    dm.readPage(5, empty_buf);
    assert(empty_buf[0] == 0);
    cout << "Página inexistente devuelve ceros correctamente\n";

    cout << "Avance 2 terminado\n\n";
}

void testStorageManager() {
    cout << "StorageManager + dirty bit\n";

    {
        StorageManager sm("test_s3.db");
        uint32_t pid = sm.allocatePage();
        Page* p = sm.fetchPage(pid);

        // La página recién asignada debe estar sucia (init la marca dirty)
        assert(p->dirty == true);
        assert(p->header()->page_id == pid);
        assert(p->freeSpace() == PAGE_SIZE - PAGE_HEADER_SIZE);
        cout << "Página asignada, dirty=true, freeSpace correcto\n";

        sm.flushPage(pid);
        // Después de flush, dirty debe ser false
        assert(p->dirty == false);
        cout << "Después de flush, dirty=false\n";
    }

    // Reabrir: next_page_id debe persistir (metapágina)
    {
        StorageManager sm2("test_s3.db");
        // next_page_id=2 porque ya se asignó la página 1 antes
        assert(sm2.getNumPages() == 2);
        cout << "next_page_id persistido correctamente: " << sm2.getNumPages() << "\n";
    }

    cout << "Avance 2 terminado\\n\n";
}

int main() {
    testDiskManager();
    testStorageManager();
    return 0;
}