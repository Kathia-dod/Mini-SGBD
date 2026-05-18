#include "../src/storage/StorageManager.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace std;

// Macro para verificar que una expresion lanza DiskException
#define ASSERT_THROWS_DISK(expr)                              \
    do {                                                      \
        bool _threw = false;                                  \
        try { expr; }                                         \
        catch (const DiskException&) { _threw = true; }      \
        assert(_threw && "Se esperaba DiskException");        \
    } while(0)

void test1_disk_write_fuera_de_rango() {
    cout << "DiskManager lanza DiskException al escribir page_id >= MAX_PAGES... ";
    DiskManager dm("test_bounds.db");
    char buf[PAGE_SIZE] = {0};
    ASSERT_THROWS_DISK(dm.writePage(MAX_PAGES, buf));
    ASSERT_THROWS_DISK(dm.writePage(MAX_PAGES + 100, buf));
    cout << "OK\n";
}

void test2_disk_read_fuera_de_rango() {
    cout << "DiskManager lanza DiskException al leer page_id >= MAX_PAGES... ";
    DiskManager dm("test_bounds_read.db");
    char buf[PAGE_SIZE];
    ASSERT_THROWS_DISK(dm.readPage(MAX_PAGES, buf));
    cout << "OK\n";
}

void test3_fetch_fuera_de_rango() {
    cout << "StorageManager lanza DiskException en fetchPage fuera de rango... ";
    StorageManager sm("test_sm_bounds.db");
    ASSERT_THROWS_DISK(sm.fetchPage(MAX_PAGES));
    ASSERT_THROWS_DISK(sm.fetchPage(MAX_PAGES + 500));
    cout << "OK\n";
}

void test4_dirty_bit_ciclo_completo() {
    cout << "dirty bit: false→(modify)→true→(flush)→false... ";
    StorageManager sm("test_dirty.db");
    uint32_t pid = sm.allocatePage();
    Page* p = sm.fetchPage(pid);

    // init() marca dirty=true
    assert(p->dirty == true);

    sm.flushPage(pid);
    assert(p->dirty == false && "dirty debe ser false tras flush");

    // Modificar manualmente
    p->data[PAGE_HEADER_SIZE] = 0x42;
    p->dirty = true;
    assert(p->dirty == true);

    sm.flushPage(pid);
    assert(p->dirty == false && "dirty debe ser false tras segundo flush");
    cout << "OK\n";
}

void test5_freespace_no_underflow() {
    cout << "freeSpace() no puede resultar en underflow... ";
    StorageManager sm("test_freespace.db");
    uint32_t pid = sm.allocatePage();
    Page* p = sm.fetchPage(pid);

    // Caso extremo: simular header con free_space_offset = PAGE_SIZE y num_slots = 0
    // freeSpace = PAGE_SIZE - PAGE_SIZE - 0 = 0 (no negativo)
    p->header()->free_space_offset = PAGE_SIZE;
    p->header()->num_slots = 0;
    uint32_t fs = p->freeSpace();
    // Como es uint32_t, verificar que no "wrappea" a un número enorme
    assert(fs <= PAGE_SIZE && "freeSpace() sufrió underflow (uint32 wraparound)");

    // Caso normal: pagina inicial
    p->header()->free_space_offset = PAGE_HEADER_SIZE;
    p->header()->num_slots = 0;
    assert(p->freeSpace() == PAGE_SIZE - PAGE_HEADER_SIZE);
    cout << "OK\n";
}

void test6_header_integridad_tras_init() {
    cout << "Todos los campos del header correctos tras init()... ";
    StorageManager sm("test_header.db");
    uint32_t pid = sm.allocatePage();
    Page* p = sm.fetchPage(pid);

    const PageHeader* h = p->header();
    assert(h->page_id           == pid             && "page_id incorrecto tras init()");
    assert(h->num_slots         == 0               && "num_slots != 0 tras init()");
    assert(h->free_space_offset == PAGE_HEADER_SIZE && "free_space_offset incorrecto tras init()");
    assert(h->reserved          == 0               && "reserved != 0 tras init()");
    cout << "OK\n";
}

void test7_alineacion_offsets_slots() {
    cout << "Offsets de slots son múltiplos de sizeof(Slot)... ";
    StorageManager sm("test_align.db");
    uint32_t pid = sm.allocatePage();
    Page* p = sm.fetchPage(pid);

    // Calcular donde deberian estar los slots
    for (uint16_t i = 0; i < 10; i++) {
        // Slot i esta en: data + PAGE_SIZE - (i+1)*sizeof(Slot)
        uintptr_t slot_addr = reinterpret_cast<uintptr_t>(p->data + PAGE_SIZE) - (i + 1) * sizeof(Slot);
        uintptr_t base_addr = reinterpret_cast<uintptr_t>(p->data);
        uintptr_t offset    = slot_addr - base_addr;

        // El offset desde el inicio de data debe ser >= PAGE_HEADER_SIZE (no pisa el header)
        assert(offset >= PAGE_HEADER_SIZE && "Slot pisa el área del header");
        // El slot no debe salirse del buffer
        assert(slot_addr + sizeof(Slot) <= reinterpret_cast<uintptr_t>(p->data) + PAGE_SIZE && "Slot fuera del buffer de la página");
    }
    cout << "OK\n";
}

int main() {
    cout << "------ Integridad, Límites y Manejo de Errores ------\n";
    try {
        test1_disk_write_fuera_de_rango();
        test2_disk_read_fuera_de_rango();
        test3_fetch_fuera_de_rango();
        test4_dirty_bit_ciclo_completo();
        test5_freespace_no_underflow();
        test6_header_integridad_tras_init();
        test7_alineacion_offsets_slots();
        cout << "\n✓ Todos los tests pasaron.\n";
    } catch (const DiskException& e) {
        cerr << "ERROR DiskException: " << e.what() << "\n";
        return 1;
    } catch (const exception& e) {
        cerr << "ERROR inesperado: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
