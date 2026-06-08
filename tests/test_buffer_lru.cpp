// test_buffer_lru.cpp
// Semana 7 — Pruebas del Buffer Manager con política LRU

//  1. Hit/miss básico
//  2. LRU evicta el menos recientemente usado (no el primero insertado)
//  3. Página dirty se escribe a disco antes de ser evictada
//  4. Página pineada NO puede ser evictada
//  5. newPage() integrado con StorageManager
//  6. Métricas de hit/miss
//  7. flushAll() persiste todas las dirty pages

#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace std;



//  TEST 1 — Hit básico: fetchPage dos veces devuelve el mismo frame
void test1_hit_basico() {
    cout << "T1  Hit basico: segunda fetch es hit... ";
 
    StorageManager sm("t7_hit.db");
    BufferManager  bm(4, sm);
 
    uint32_t pid = sm.allocatePage();
 
    Page* p1 = bm.fetchPage(pid);
    assert(p1 != nullptr);
    bm.unpinPage(pid, false);
 
    assert(bm.getMisses() == 1 && bm.getHits() == 0);
 
    Page* p2 = bm.fetchPage(pid);
    assert(p2 != nullptr);
    assert(p1 == p2 && "fetchPage deberia devolver el mismo frame en hit");
    bm.unpinPage(pid, false);
 
    assert(bm.getHits() == 1 && bm.getMisses() == 1);
    cout << "OK\n";
}
 
//  TEST 2 — LRU evicta el menos recientemente usado
//  Pool size = 3, páginas A B C D
//  Orden de acceso: A B C  →  toca A  →  evicta B (LRU)
//  Al pedir D con pool lleno y A pineada:
//    candidatos son B y C; B es el menos reciente → victim = B
void test2_lru_eviccion_correcta() {
    cout << "T2  LRU evicta el menos recientemente usado... ";
 
    StorageManager sm("t7_lru.db");
    BufferManager  bm(3, sm);
 
    uint32_t pA = sm.allocatePage();
    uint32_t pB = sm.allocatePage();
    uint32_t pC = sm.allocatePage();
    uint32_t pD = sm.allocatePage();
 
    // Llenar el pool: A B C (orden: C es MRU, A es LRU)
    bm.fetchPage(pA); bm.unpinPage(pA, false);  // uso 1
    bm.fetchPage(pB); bm.unpinPage(pB, false);  // uso 2
    bm.fetchPage(pC); bm.unpinPage(pC, false);  // uso 3
 
    // Retocar A: ahora A es MRU, B es LRU
    bm.fetchPage(pA); bm.unpinPage(pA, false);
 
    // Pool lleno: A(MRU) C B(LRU)
    // Pedir D → debe evictar B
    Page* pD_ptr = bm.fetchPage(pD);
    assert(pD_ptr != nullptr && "fetchPage(D) falló con pool lleno");
    bm.unpinPage(pD, false);
 
    // Ahora B ya no está en pool; pedirla debe ser un miss
    uint64_t misses_antes = bm.getMisses();
    bm.fetchPage(pB); bm.unpinPage(pB, false);
    assert(bm.getMisses() == misses_antes + 1 && "B deberia haberse evictado (miss esperado)");
 
    cout << "OK\n";
}
 
//  TEST 3 — Página dirty se escribe a disco antes de evictarse
void test3_dirty_flush_al_evictar() {
    cout << "T3  Dirty page se escribe a disco al evictarse... ";
 
    const char* db = "t7_dirty.db";
    uint32_t pid_dirty;
 
    {
        StorageManager sm(db);
        BufferManager  bm(2, sm);  // pool pequeño para forzar eviccion
 
        pid_dirty = sm.allocatePage();
        uint32_t pid2 = sm.allocatePage();
        uint32_t pid3 = sm.allocatePage();
 
        // Escribir dato identificable en la página dirty
        Page* p = bm.fetchPage(pid_dirty);
        memset(p->data + PAGE_HEADER_SIZE, 0xAB, 8);
        bm.unpinPage(pid_dirty, true);   // marca dirty=true
 
        // Llenar pool con otras páginas para forzar eviccion de pid_dirty
        bm.fetchPage(pid2); bm.unpinPage(pid2, false);
        // En este punto pid_dirty es LRU y dirty; al pedir pid3 debe evictarse con flush
        bm.fetchPage(pid3); bm.unpinPage(pid3, false);
    }  // StorageManager destructor hace sync final
 
    // Reabrir y verificar que los datos persisten
    {
        StorageManager sm2(db);
        Page* p = sm2.fetchPage(pid_dirty);
        for (int i = 0; i < 8; i++) {
            assert((unsigned char)p->data[PAGE_HEADER_SIZE + i] == 0xAB &&
                   "Dirty page no se flusheó al disco antes de evictarse");
        }
    }
    cout << "OK\n";
}
 
//  TEST 4 — Página pineada NO puede ser evictada
void test4_pinned_no_evictable() {
    cout << "T4  Pagina pineada no puede ser evictada... ";
 
    StorageManager sm("t7_pin.db");
    BufferManager  bm(2, sm);
 
    uint32_t p1 = sm.allocatePage();
    uint32_t p2 = sm.allocatePage();
    uint32_t p3 = sm.allocatePage();
 
    // Pinear ambas páginas del pool (sin unpin)
    bm.fetchPage(p1);  // pin_count=1, NO se hace unpin
    bm.fetchPage(p2);  // pin_count=1, NO se hace unpin
 
    // Pool lleno y todo pineado → fetchPage(p3) debe retornar nullptr
    Page* result = bm.fetchPage(p3);
    assert(result == nullptr && "Pool lleno con todo pineado deberia retornar nullptr");
 
    // Despin y volver a intentar
    bm.unpinPage(p1, false);
    result = bm.fetchPage(p3);
    assert(result != nullptr && "Tras despin, fetchPage(p3) deberia funcionar");
    bm.unpinPage(p3, false);
    bm.unpinPage(p2, false);
 
    cout << "OK\n";
}
 
//  TEST 5 — newPage() asigna y carga en pool
void test5_new_page() {
    cout << "T5  newPage() asigna pagina y la carga en pool... ";
 
    StorageManager sm("t7_newpage.db");
    BufferManager  bm(4, sm);
 
    uint32_t new_pid;
    Page* p = bm.newPage(new_pid);
    assert(p != nullptr && "newPage() devolvio nullptr");
    assert(new_pid >= 1 && "page_id debe ser >= 1 (0 es metapagina)");
 
    // Escribir algo y verificar
    memcpy(p->data + PAGE_HEADER_SIZE, "NEWPAGE", 8);
    bm.unpinPage(new_pid, true);
 
    // Leer de vuelta desde el pool (debe ser hit)
    uint64_t hits_antes = bm.getHits();
    Page* p2 = bm.fetchPage(new_pid);
    assert(bm.getHits() == hits_antes + 1 && "Deberia ser hit tras newPage");
    assert(memcmp(p2->data + PAGE_HEADER_SIZE, "NEWPAGE", 8) == 0 &&
           "Datos de newPage no coinciden");
    bm.unpinPage(new_pid, false);
 
    cout << "OK\n";
}
 
//  TEST 6 — Métricas de hit rate
void test6_metricas_hit_rate() {
    cout << "T6  Metricas de hit/miss son correctas... ";
 
    StorageManager sm("t7_metrics.db");
    BufferManager  bm(4, sm);
 
    uint32_t pid = sm.allocatePage();
 
    // 1 miss, luego 3 hits
    bm.fetchPage(pid); bm.unpinPage(pid, false);
    bm.fetchPage(pid); bm.unpinPage(pid, false);
    bm.fetchPage(pid); bm.unpinPage(pid, false);
    bm.fetchPage(pid); bm.unpinPage(pid, false);
 
    assert(bm.getMisses() == 1 && "Deberia haber exactamente 1 miss");
    assert(bm.getHits()   == 3 && "Deberia haber exactamente 3 hits");
 
    cout << "OK  (hits=" << bm.getHits() << " misses=" << bm.getMisses() << ")\n";
}
 
//  TEST 7 — flushAll persiste dirty pages sin evictarlas
void test7_flush_all() {
    cout << "T7  flushAll() persiste dirty pages sin evictarlas... ";
 
    const char* db = "t7_flushall.db";
    uint32_t pid;
 
    {
        StorageManager sm(db);
        BufferManager  bm(4, sm);
 
        pid = sm.allocatePage();
        Page* p = bm.fetchPage(pid);
        memset(p->data + PAGE_HEADER_SIZE, 0xCC, 16);
        bm.unpinPage(pid, true);
 
        bm.flushAll();  // flush sin evictar
 
        // La página debe seguir en el pool (hit esperado)
        uint64_t hits_antes = bm.getHits();
        bm.fetchPage(pid);
        assert(bm.getHits() == hits_antes + 1 && "Pagina debe seguir en pool tras flushAll");
        bm.unpinPage(pid, false);
    }
 
    // Verificar persistencia
    {
        StorageManager sm2(db);
        Page* p = sm2.fetchPage(pid);
        for (int i = 0; i < 16; i++) {
            assert((unsigned char)p->data[PAGE_HEADER_SIZE + i] == 0xCC &&
                   "flushAll no persistió los datos correctamente");
        }
    }
    cout << "OK\n";
}
 
//  MAIN
int main() {
    cout << "Semana 7: LRU + Dirty Pages\n\n";
    try {
        test1_hit_basico();
        test2_lru_eviccion_correcta();
        test3_dirty_flush_al_evictar();
        test4_pinned_no_evictable();
        test5_new_page();
        test6_metricas_hit_rate();
        test7_flush_all();
        cout << "\nTodos los tests de Semana 7 pasaron.\n";
    } catch (const DiskException& e) {
        cerr << "ERROR DiskException: " << e.what() << "\n";
        return 1;
    } catch (const exception& e) {
        cerr << "ERROR inesperado: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

