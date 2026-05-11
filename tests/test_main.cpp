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

    cout << "Avance 3 terminado\\n\n";
}

void testSlotDirectory() {
    cout << "Slot Directory\n";

    // layout inicial
    {
        StorageManager sm("test_s4a.db");
        uint32_t pid = sm.allocatePage();
        Page* p = sm.fetchPage(pid);

        assert(p->header()->num_slots == 0);
        assert(p->header()->free_space_offset == PAGE_HEADER_SIZE);
        assert(p->freeSpace() == PAGE_SIZE - PAGE_HEADER_SIZE);
        cout << "Layout inicial: sin slots, freeSpace="
             << p->freeSpace() << " bytes\n";
    }

    // inserción y recuperación
    {
        StorageManager sm("test_s4b.db");
        uint32_t pid = sm.allocatePage();
        Page* p = sm.fetchPage(pid);

        // Registros de longitud variable 
        const char* r0 = "Alice|25|Arequipa";    // 17 bytes
        const char* r1 = "Bob|17|Puno";           // 11 bytes
        const char* r2 = "Carol|22|Cusco";        // 14 bytes

        int s0 = p->insertRecord(r0, strlen(r0));
        int s1 = p->insertRecord(r1, strlen(r1));
        int s2 = p->insertRecord(r2, strlen(r2));

        assert(s0 == 0 && s1 == 1 && s2 == 2);
        assert(p->header()->num_slots == 3);
        cout << "Inserción: 3 registros de largo variable (slots 0,1,2)\n";

        // Recuperación por slot_id
        char buf[256]; uint16_t len;

        assert(p->getRecord(0, buf, len));
        buf[len] = '\0';
        assert(strcmp(buf, r0) == 0);
        cout << "getRecord(0): " << buf << "\n";

        assert(p->getRecord(1, buf, len));
        buf[len] = '\0';
        assert(strcmp(buf, r1) == 0);
        cout << "getRecord(1): " << buf << "\n";

        assert(p->getRecord(2, buf, len));
        buf[len] = '\0';
        assert(strcmp(buf, r2) == 0);
        cout << "getRecord(2): " << buf << "\n";

        // updateRecord: mismo tamaño exacto
        const char* r0_v2 = "Alice|26|Arequipa";
        assert(strlen(r0_v2) == strlen(r0));
        assert(p->updateRecord(0, r0_v2, strlen(r0_v2)));
        p->getRecord(0, buf, len);
        buf[len] = '\0';
        assert(strcmp(buf, r0_v2) == 0);
        cout << "updateRecord(0): " << buf << "\n";

        // deleteRecord: eliminación lógica
        assert(p->deleteRecord(1));
        assert(!p->getRecord(1, buf, len));  // ya no accesible
        assert(p->getRecord(2, buf, len));  // slot 2 intacto
        buf[len] = '\0';
        assert(strcmp(buf, r2) == 0);
        cout << "deleteRecord(1): slot 1 eliminado, slot 2 intacto\n";

        // Slot fuera de rango
        assert(!p->getRecord(99, buf, len));
        cout << "slot_id inexistente retorna false\n";

        // Flush y verificar dirty
        sm.flushPage(pid);
        assert(p->dirty == false);
        cout << "flushPage: dirty=false\n";
    }

    // Persistencia: cerrar y reabrir
    {
        StorageManager sm("test_s4b.db");
        Page* p = sm.fetchPage(1);  // pid=1 (pid=0 es metapágina)
        char buf[256]; uint16_t len;

        assert(p->getRecord(0, buf, len));
        buf[len] = '\0';
        assert(strcmp(buf, "Alice|26|Arequipa") == 0);
        cout << "Persistencia: slot 0 = " << buf << "\n";

        assert(!p->getRecord(1, buf, len));
        cout << "Persistencia: slot 1 sigue eliminado\n";

        assert(p->getRecord(2, buf, len));
        buf[len] = '\0';
        assert(strcmp(buf, "Carol|22|Cusco") == 0);
        cout << "Persistencia: slot 2 = " << buf << "\n";
    }

    cout << "Avance 4 hecho\n\n";
}
 
int main() {
    try {
        testDiskManager();
        testStorageManager();
        testSlotDirectory();
        cout << "Pruebas superadas\n";
    } catch (const DiskException& e) {
        cerr << "ERROR de disco: " << e.what() << "\n";
        return 1;
    } catch (const exception& e) {
        cerr << "ERROR inesperado: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
