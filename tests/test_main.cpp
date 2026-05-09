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

void testSlotDirectoryPartA() {
    cout << "Slot Directory\n";
 
    StorageManager sm("test_s4a.db");
    uint32_t pid = sm.allocatePage();
    Page* p = sm.fetchPage(pid);
 
    // La pagina recien inicializada no tiene slots ni datos
    assert(p->header()->num_slots == 0);
    cout << "✓ num_slots inicial = 0\n";
 
    // El espacio libre empieza justo despues del header
    assert(p->header()->free_space_offset == PAGE_HEADER_SIZE);
    cout << "✓ free_space_offset inicial = " << p->header()->free_space_offset
         << " (= PAGE_HEADER_SIZE)\n";
 
    // Todo el espacio de la pagina menos el header esta libre
    assert(p->freeSpace() == PAGE_SIZE - PAGE_HEADER_SIZE);
    cout << "✓ freeSpace() inicial = " << p->freeSpace() << " bytes\n";
 
    // Verificar que sizeof(Slot) es exactamente 4 bytes (2 bytes offset + 2 bytes length, sin padding gracias a #pragma pack)
    assert(sizeof(Slot) == 4);
    cout << "✓ sizeof(Slot) = " << sizeof(Slot) << " bytes (correcto)\n";
 
    // Verificar que sizeof(PageHeader) es exactamente 12 bytes
    assert(sizeof(PageHeader) == 12);
    cout << "✓ sizeof(PageHeader) = " << sizeof(PageHeader) << " bytes (correcto)\n";
 
    // Verificar que la pagina esta marcada dirty tras init()
    assert(p->dirty == true);
    cout << "✓ dirty=true tras init()\n";
 
    // Calcular cuantos slots cabrian en teoria en una pagina vacia (sin contar datos, solo el espacio del slot directory)
    uint32_t max_slots = (PAGE_SIZE - PAGE_HEADER_SIZE) / sizeof(Slot);
    cout << "✓ Slots maximos teoricos en pagina vacia: " << max_slots << "\n";
 
    // Verificar que freeSpace() descuenta correctamente cuando simulamos manualmente que hay slots (sin insertar datos todavía)
    // Para esto manipulamos num_slots directamente como prueba del layout
    p->header()->num_slots = 3;
    uint32_t esperado = PAGE_SIZE - PAGE_HEADER_SIZE - (3 * sizeof(Slot));
    assert(p->freeSpace() == esperado);
    cout << "✓ freeSpace() con 3 slots simulados = " << p->freeSpace()
         << " bytes (descuenta " << 3 * sizeof(Slot) << " bytes de slots)\n";
 
    // Restaurar para no afectar otros tests
    p->header()->num_slots = 0;
    p->header()->free_space_offset = PAGE_HEADER_SIZE;
 
    cout << "Avance 4: parte a completada\n\n";
}
 
int main() {
    try {
        testDiskManager();
        testStorageManager();
        testSlotDirectoryPartA();
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
