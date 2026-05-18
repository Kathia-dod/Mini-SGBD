#include "../src/storage/StorageManager.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace std;

static const char* DB = "test_persist.db";

void test1_next_page_id_persiste() {
    cout << "next_page_id persiste tras cerrar el StorageManager... ";

    {
        StorageManager sm(DB);
        // Archivo nuevo: next_page_id = 1 (reserva pagina 0 para meta)
        assert(sm.getNumPages() == 1);
        sm.allocatePage();  // pid=1 → next_page_id=2
        sm.allocatePage();  // pid=2 → next_page_id=3
    }
    // Reabrir
    {
        StorageManager sm(DB);
        assert(sm.getNumPages() == 3 && "next_page_id no persistió tras cerrar");
    }
    cout << "OK\n";
}

void test2_multiples_aperturas() {
    cout << "Múltiples aperturas incrementan el contador correctamente... ";

    const char* db = "test_multiabierto.db";
    for (int sesion = 0; sesion < 4; sesion++) {
        StorageManager sm(db);
        uint32_t antes = sm.getNumPages();
        sm.allocatePage();
        assert(sm.getNumPages() == antes + 1);
        // al destruirse, persiste
    }
    {
        StorageManager sm(db);
        // 4 sesiones x 1 alloc + pagina 0 inicial = 5
        assert(sm.getNumPages() == 5 && "Contador incorrecto tras múltiples sesiones");
    }
    cout << "OK\n";
}

void test3_datos_sobreviven_reinicio() {
    cout << "Datos en área de datos sobreviven al reinicio... ";

    const char* db = "test_datos.db";
    uint32_t pid_guardado;

    {
        StorageManager sm(db);
        uint32_t pid = sm.allocatePage();
        pid_guardado = pid;
        Page* p = sm.fetchPage(pid);

        // Escribir datos en el area de datos (despues del header)
        const char* mensaje = "Hola persistencia!";
        memcpy(p->data + PAGE_HEADER_SIZE, mensaje, strlen(mensaje) + 1);
        p->dirty = true;
        sm.flushPage(pid);
    }
    {
        StorageManager sm(db);
        Page* p = sm.fetchPage(pid_guardado);
        const char* leido = p->data + PAGE_HEADER_SIZE;
        assert(strcmp(leido, "Hola persistencia!") == 0 && "Datos no persistieron en área de datos");
    }
    cout << "OK\n";
}

void test4_metapagina_no_sobreescrita() {
    cout << "Metapágina (page_id=0) no es asignada por allocatePage()... ";

    const char* db = "test_meta.db";
    StorageManager sm(db);

    // allocatePage nunca debe devolver 0
    for (int i = 0; i < 10; i++) {
        uint32_t pid = sm.allocatePage();
        assert(pid != META_PAGE_ID && "allocatePage devolvió page_id=0 (metapágina)");
    }
    cout << "OK\n";
}

void test5_fetch_no_reinicializa() {
    cout << "fetchPage no reinicializa una página ya cargada... ";

    const char* db = "test_fetch.db";
    StorageManager sm(db);
    uint32_t pid = sm.allocatePage();

    Page* p = sm.fetchPage(pid);
    // Escribir algo reconocible
    memset(p->data + PAGE_HEADER_SIZE, 0xBE, 16);
    p->dirty = true;

    // Volver a fetchear — debe devolver el mismo objeto, sin limpiar los datos
    Page* p2 = sm.fetchPage(pid);
    for (int i = 0; i < 16; i++)
        assert((unsigned char)p2->data[PAGE_HEADER_SIZE + i] == 0xBE && "fetchPage reinicializó datos de página ya cargada");

    cout << "OK\n";
}

int main() {
    cout << "------ StorageManager — Persistencia y Recuperación ------\n";
    try {
        test1_next_page_id_persiste();
        test2_multiples_aperturas();
        test3_datos_sobreviven_reinicio();
        test4_metapagina_no_sobreescrita();
        test5_fetch_no_reinicializa();
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
