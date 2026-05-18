#include "../src/storage/StorageManager.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include <string>
using namespace std;

// ---- Helpers de slot directory (mismos que commit 3, copiados para autonomía) ----

static uint16_t insertRecord(Page* p, const char* rec, uint16_t len) {
    assert(p->freeSpace() >= len + (uint32_t)sizeof(Slot));
    uint16_t slot_id = p->header()->num_slots;
    uint16_t offset  = p->header()->free_space_offset;
    memcpy(p->data + offset, rec, len);
    Slot* slot = reinterpret_cast<Slot*>(p->data + PAGE_SIZE) - (slot_id + 1);
    slot->offset = offset;
    slot->length = len;
    p->header()->free_space_offset += len;
    p->header()->num_slots++;
    p->dirty = true;
    return slot_id;
}

static const char* readRecord(Page* p, uint16_t slot_id) {
    if (slot_id >= p->header()->num_slots) return nullptr;
    Slot* slot = reinterpret_cast<Slot*>(p->data + PAGE_SIZE) - (slot_id + 1);
    if (slot->offset == SLOT_DELETED) return nullptr;
    return p->data + slot->offset;
}

// ---- Tests ----

void test1_ciclo_sesion1_sesion2() {
    cout << "Sesión 1→2: datos persisten byte a byte... ";

    const char* db = "test_end2end.db";
    vector<string> datos = {"Tupla Alpha", "Tupla Beta", "Tupla Gamma"};
    uint32_t pid_guardado;

    // Sesion 1: escribir
    {
        StorageManager sm(db);
        uint32_t pid = sm.allocatePage();
        pid_guardado = pid;
        Page* p = sm.fetchPage(pid);
        for (const auto& d : datos)
            insertRecord(p, d.c_str(), d.size() + 1);
        sm.flushPage(pid);
        assert(p->dirty == false);
    }

    // Sesion 2: leer y verificar
    {
        StorageManager sm(db);
        Page* p = sm.fetchPage(pid_guardado);
        assert((size_t)p->header()->num_slots == datos.size() &&
               "num_slots no persistió correctamente");
        for (uint16_t i = 0; i < datos.size(); i++) {
            const char* leido = readRecord(p, i);
            assert(leido != nullptr && "Registro no encontrado tras reabrir");
            assert(strcmp(leido, datos[i].c_str()) == 0 &&
                   "Contenido del registro corrupto tras reabrir");
        }
    }
    cout << "OK\n";
}

void test2_modificacion_no_corrompe_otros() {
    cout << "Sesión 3→4: modificación parcial no corrompe otros registros... ";

    const char* db = "test_modify.db";
    uint32_t pid_guardado;

    // Sesion 3: insertar 3 registros, modificar el del medio
    {
        StorageManager sm(db);
        uint32_t pid = sm.allocatePage();
        pid_guardado = pid;
        Page* p = sm.fetchPage(pid);

        insertRecord(p, "Primero", 8);
        uint16_t sid_medio = insertRecord(p, "Medio ORIGINAL", 15);
        insertRecord(p, "Tercero", 8);

        // Modificar el registro del medio in-place (mismo tamaño)
        Slot* s = reinterpret_cast<Slot*>(p->data + PAGE_SIZE) - (sid_medio + 1);
        memcpy(p->data + s->offset, "Medio EDITADO!", 15);
        p->dirty = true;
        sm.flushPage(pid);
    }

    // Sesion 4: verificar
    {
        StorageManager sm(db);
        Page* p = sm.fetchPage(pid_guardado);

        assert(strcmp(readRecord(p, 0), "Primero") == 0 && "Registro 0 fue corrompido");
        assert(strcmp(readRecord(p, 1), "Medio EDITADO!") == 0 && "Modificación no persistió");
        assert(strcmp(readRecord(p, 2), "Tercero") == 0 && "Registro 2 fue corrompido");
    }
    cout << "OK\n";
}

void test3_page_id_en_header_correcto_tras_releer() {
    cout << "page_id en header correcto tras leer del disco... ";

    const char* db = "test_pageid.db";
    uint32_t pids[3];

    {
        StorageManager sm(db);
        for (int i = 0; i < 3; i++) pids[i] = sm.allocatePage();
    }
    {
        StorageManager sm(db);
        for (int i = 0; i < 3; i++) {
            Page* p = sm.fetchPage(pids[i]);
            assert(p->header()->page_id == pids[i] &&
                   "page_id en header incorrecto tras leer del disco");
        }
    }
    cout << "OK\n";
}

void test4_num_slots_persiste() {
    cout << "num_slots persiste en header de página... ";

    const char* db = "test_numslots.db";
    uint32_t pid;

    {
        StorageManager sm(db);
        pid = sm.allocatePage();
        Page* p = sm.fetchPage(pid);
        insertRecord(p, "A", 2);
        insertRecord(p, "BB", 3);
        insertRecord(p, "CCC", 4);
        assert(p->header()->num_slots == 3);
        sm.flushPage(pid);
    }
    {
        StorageManager sm(db);
        Page* p = sm.fetchPage(pid);
        assert(p->header()->num_slots == 3 && "num_slots no persistió");
    }
    cout << "OK\n";
}

void test5_free_space_offset_persiste() {
    cout << "free_space_offset persiste tras insertar y reabrir... ";

    const char* db = "test_fso.db";
    uint32_t pid;
    uint16_t fso_esperado;

    {
        StorageManager sm(db);
        pid = sm.allocatePage();
        Page* p = sm.fetchPage(pid);
        insertRecord(p, "Registro largo de prueba 123", 29);
        fso_esperado = p->header()->free_space_offset;
        sm.flushPage(pid);
    }
    {
        StorageManager sm(db);
        Page* p = sm.fetchPage(pid);
        assert(p->header()->free_space_offset == fso_esperado &&
               "free_space_offset no persistió");
    }
    cout << "OK\n";
}

void test6_multiples_paginas_sin_contaminacion() {
    cout << "Múltiples páginas: A no contamina B tras reinicio... ";

    const char* db = "test_multi.db";
    uint32_t pidA, pidB;

    {
        StorageManager sm(db);
        pidA = sm.allocatePage();
        pidB = sm.allocatePage();
        Page* pA = sm.fetchPage(pidA);
        Page* pB = sm.fetchPage(pidB);

        insertRecord(pA, "PAGINA_A_DATOS", 15);
        insertRecord(pB, "PAGINA_B_DATOS", 15);

        sm.flushPage(pidA);
        sm.flushPage(pidB);
    }
    {
        StorageManager sm(db);
        Page* pA = sm.fetchPage(pidA);
        Page* pB = sm.fetchPage(pidB);

        assert(strcmp(readRecord(pA, 0), "PAGINA_A_DATOS") == 0 && "Datos de A corrompidos");
        assert(strcmp(readRecord(pB, 0), "PAGINA_B_DATOS") == 0 && "Datos de B corrompidos");

        // Verificar que A no tiene datos de B y viceversa
        assert(strstr(pA->data + PAGE_HEADER_SIZE, "PAGINA_B") == nullptr &&
               "Página A contiene datos de B");
        assert(strstr(pB->data + PAGE_HEADER_SIZE, "PAGINA_A") == nullptr &&
               "Página B contiene datos de A");
    }
    cout << "OK\n";
}

int main() {
    cout << "------ Integración End-to-End — Ciclo completo de vida de datos ------\n";
    try {
        test1_ciclo_sesion1_sesion2();
        test2_modificacion_no_corrompe_otros();
        test3_page_id_en_header_correcto_tras_releer();
        test4_num_slots_persiste();
        test5_free_space_offset_persiste();
        test6_multiples_paginas_sin_contaminacion();
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
