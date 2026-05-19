#include "../src/storage/StorageManager.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace std;

// Helper: simula insertar un registro en la pagina manualmente
// Devuelve el slot_id asignado
static uint16_t insertRecord(Page* p, const char* rec, uint16_t len) {
    assert(p->freeSpace() >= len + sizeof(Slot) && "No hay espacio para insertar");

    uint16_t slot_id = p->header()->num_slots;
    uint16_t offset  = p->header()->free_space_offset;

    // Copiar datos al area de datos
    memcpy(p->data + offset, rec, len);

    // Escribir slot al final de la pagina (crece hacia el inicio)
    // Slot N esta en: data + PAGE_SIZE - (N+1)*sizeof(Slot)
    Slot* slot = reinterpret_cast<Slot*>(p->data + PAGE_SIZE) - (slot_id + 1);
    slot->offset = offset;
    slot->length = len;

    // Actualizar header
    p->header()->free_space_offset += len;
    p->header()->num_slots++;
    p->dirty = true;

    return slot_id;
}

// Helper: leer un registro por slot_id
static const char* readRecord(Page* p, uint16_t slot_id) {
    Slot* slot = reinterpret_cast<Slot*>(p->data + PAGE_SIZE) - (slot_id + 1);
    if (slot->offset == SLOT_DELETED) return nullptr;
    return p->data + slot->offset;
}

void test1_tamanos_estructuras() {
    cout << "sizeof(Slot)==4 y sizeof(PageHeader)==12... ";
    assert(sizeof(Slot)       == 4  && "- Slot tiene padding inesperado");
    assert(sizeof(PageHeader) == 12 && "- PageHeader tiene padding inesperado");
    cout << "OK\n";
}

void test2_freespace_decrementa_correctamente() {
    cout << "freeSpace() decrementa al insertar registros... ";

    StorageManager sm("test_freespace.db");
    uint32_t pid = sm.allocatePage();
    Page* p = sm.fetchPage(pid);

    uint32_t libre_inicial = p->freeSpace();
    assert(libre_inicial == PAGE_SIZE - PAGE_HEADER_SIZE);

    const char* r1 = "- Registro uno";
    uint16_t len1 = strlen(r1) + 1;
    insertRecord(p, r1, len1);

    uint32_t esperado = libre_inicial - len1 - sizeof(Slot);
    assert(p->freeSpace() == esperado && "- freeSpace no decrementó correctamente tras insertar");

    cout << "OK\n";
}

void test3_slot0_al_final_del_buffer() {
    cout << "Slot 0 ocupa los últimos 4 bytes del buffer... ";

    StorageManager sm("test_layout.db");
    uint32_t pid = sm.allocatePage();
    Page* p = sm.fetchPage(pid);

    const char* rec = "ABC";
    insertRecord(p, rec, 4);

    // El slot 0 esta en data[PAGE_SIZE-4 .. PAGE_SIZE-1]
    Slot* slot0 = reinterpret_cast<Slot*>(p->data + PAGE_SIZE - sizeof(Slot));
    assert(slot0->offset == PAGE_HEADER_SIZE && "Offset del slot 0 incorrecto");
    assert(slot0->length == 4 && "Length del slot 0 incorrecto");
    cout << "OK\n";
}

void test4_sin_solapamiento_datos_slots() {
    cout << "Sin solapamiento entre área de datos y slot directory... ";

    StorageManager sm("test_overlap.db");
    uint32_t pid = sm.allocatePage();
    Page* p = sm.fetchPage(pid);

    // Insertar varios registros de 100 bytes cada uno
    char rec[100];
    memset(rec, 'X', 100);
    int insertados = 0;

    while (p->freeSpace() >= 100 + sizeof(Slot)) {
        insertRecord(p, rec, 100);
        insertados++;

        // Verificar que free_space_offset no ha entrado en el area de slots
        uint32_t slots_start = PAGE_SIZE - p->header()->num_slots * sizeof(Slot);
        assert(p->header()->free_space_offset <= slots_start &&
               "Solapamiento: datos se metieron en el area de slots");
    }
    assert(insertados > 0 && "No se insertó ningún registro");
    cout << "OK (insertados " << insertados << " registros de 100 bytes)\n";
}

void test5_slot_deleted_centinela() {
    cout << "SLOT_DELETED tiene valor 0xFFFF... ";
    assert(SLOT_DELETED == 0xFFFF && "SLOT_DELETED no es 0xFFFF");

    // Verificar que marcar un slot como eliminado funciona correctamente
    StorageManager sm("test_deleted.db");
    uint32_t pid = sm.allocatePage();
    Page* p = sm.fetchPage(pid);

    const char* rec = "Para borrar";
    uint16_t sid = insertRecord(p, rec, strlen(rec) + 1);

    // "Eliminar" el slot marcando offset como SLOT_DELETED
    Slot* slot = reinterpret_cast<Slot*>(p->data + PAGE_SIZE) - (sid + 1);
    slot->offset = SLOT_DELETED;
    slot->length = 0;

    // readRecord debe devolver nullptr para slot eliminado
    const char* leido = readRecord(p, sid);
    assert(leido == nullptr && "readRecord no detectó slot eliminado");
    cout << "OK\n";
}

void test6_insercion_y_lectura_multiples_registros() {
    cout << "Inserción y lectura de múltiples registros coherentes... ";

    StorageManager sm("test_multi.db");
    uint32_t pid = sm.allocatePage();
    Page* p = sm.fetchPage(pid);

    const char* registros[] = {"Alpha", "Beta", "Gamma", "Delta", "Epsilon"};
    int n = 5;
    uint16_t sids[5];

    for (int i = 0; i < n; i++) {
        uint16_t len = strlen(registros[i]) + 1;
        sids[i] = insertRecord(p, registros[i], len);
    }

    assert(p->header()->num_slots == n && "num_slots incorrecto");

    for (int i = 0; i < n; i++) {
        const char* leido = readRecord(p, sids[i]);
        assert(leido != nullptr && "Registro no encontrado");
        assert(strcmp(leido, registros[i]) == 0 && "Contenido del registro incorrecto");
    }
    cout << "OK\n";
}

int main() {
    cout << "------ Page — Slot Directory y espacio libre ------\n";
    try {
        test1_tamanos_estructuras();
        test2_freespace_decrementa_correctamente();
        test3_slot0_al_final_del_buffer();
        test4_sin_solapamiento_datos_slots();
        test5_slot_deleted_centinela();
        test6_insercion_y_lectura_multiples_registros();
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
