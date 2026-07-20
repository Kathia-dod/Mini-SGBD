#include <iostream>
#include <cassert>
#include <cstdio>
#include <vector>

#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/index/BTreeIndex.hpp"
#include "../src/query/CsvLoader.hpp"
#include "../src/query/ScanOperator.hpp"
#include "../src/query/SelectOperator.hpp"
#include "../src/query/SortOperator.hpp"
#include "../src/query/JoinOperator.hpp"
#include "../src/query/IndexScanOperator.hpp"

using namespace std;

void separador(const string& titulo) {
    cout << "\n---------------------------------------------\n"
         << titulo << "\n---------------------------------------------\n";
}

void test_pool_lleno() {
    cout << "[Edge] Pool lleno con todos los frames pineados... ";

    remove("edge_pool_lleno.bin");
    StorageManager sm("edge_pool_lleno.bin");
    BufferManager bm(2, sm);

    uint32_t p1 = sm.allocatePage();
    uint32_t p2 = sm.allocatePage();
    uint32_t p3 = sm.allocatePage();

    Page* f1 = bm.fetchPage(p1); // sin unpin
    Page* f2 = bm.fetchPage(p2); // sin unpin
    assert(f1 && f2);

    Page* f3 = bm.fetchPage(p3);
    assert(f3 == nullptr && "pool lleno y pineado debe devolver nullptr");

    bm.unpinPage(p1, false);
    f3 = bm.fetchPage(p3);
    assert(f3 != nullptr && "tras liberar deberia funcionar");
    bm.unpinPage(p3, false);
    bm.unpinPage(p2, false);

    cout << "OK\n";
}

void test_btree_con_pool_pequeno() {
    cout << "[Edge] BTree con pool minimo (evicciones constantes)... ";

    remove("edge_btree_pool.bin");
    StorageManager sm("edge_btree_pool.bin");
    BufferManager bm(2, sm);
    BTreeIndex tree(&bm);

    vector<int> keys;
    for (int i = 1; i <= 50; i++) keys.push_back(i * 3);
    for (int k : keys) tree.insert(k, k * 100);

    int value;
    for (int k : keys) {
        assert(tree.search(k, value) && value == k * 100);
    }
    cout << "OK (50 claves, pool=2)\n";
}

void test_clave_inexistente() {
    cout << "[Edge] Clave inexistente (search y remove)... ";

    remove("edge_clave.bin");
    StorageManager sm("edge_clave.bin");
    BufferManager bm(10, sm);
    BTreeIndex tree(&bm);

    int value;
    assert(!tree.search(42, value));
    tree.remove(42); // arbol vacio, no debe lanzar

    tree.insert(10, 100);
    tree.insert(20, 200);
    assert(!tree.search(15, value));
    tree.remove(999);

    assert(tree.search(10, value) && value == 100);
    assert(tree.search(20, value) && value == 200);
    cout << "OK\n";
}

void test_indexscan_clave_inexistente() {
    cout << "[Edge] IndexScanOperator con clave inexistente... ";

    remove("edge_indexscan.bin");
    StorageManager sm("edge_indexscan.bin");
    BufferManager bm(10, sm);
    BTreeIndex index(&bm);

    int inserted = CsvLoader::load("data/datos1.csv", bm, &index);
    assert(inserted > 0);

    IndexScanOperator scan(bm, index, 9999);
    scan.open();
    Tuple t;
    bool found = scan.next(t);
    scan.close();

    assert(!found);
    cout << "OK\n";
}

void test_join_vacio() {
    cout << "[Edge] JOIN con resultado vacio... ";

    remove("edge_join_personas.bin");
    remove("edge_join_ciudades.bin");

    StorageManager smA("edge_join_personas.bin");
    BufferManager bmA(5, smA);
    assert(CsvLoader::load("data/datos1.csv", bmA) > 0);

    StorageManager smB("edge_join_ciudades.bin");
    BufferManager bmB(5, smB);
    assert(CsvLoader::load("data/ciudades.csv", bmB) > 0);

    Operator* join = new JoinOperator(
        new ScanOperator(bmA, smA.getNumPages()),
        new ScanOperator(bmB, smB.getNumPages()),
        [](const Tuple& persona, const Tuple&) {
            return persona.values[2] == "atlantida_inexistente";
        }
    );

    join->open();
    Tuple t;
    int rows = 0;
    while (join->next(t)) rows++;
    join->close();

    assert(rows == 0);
    delete join;
    cout << "OK (0 filas, sin errores)\n";
}

void test_join_una_tabla_vacia() {
    cout << "[Edge] JOIN con una relacion sin registros... ";

    remove("edge_join_vacia_a.bin");
    remove("edge_join_vacia_b.bin");

    StorageManager smA("edge_join_vacia_a.bin");
    BufferManager bmA(5, smA);
    assert(CsvLoader::load("data/datos1.csv", bmA) > 0);

    // smB nunca recibe CsvLoader::load -> sin paginas de datos
    StorageManager smB("edge_join_vacia_b.bin");
    BufferManager bmB(5, smB);

    Operator* join = new JoinOperator(
        new ScanOperator(bmA, smA.getNumPages()),
        new ScanOperator(bmB, smB.getNumPages()),
        [](const Tuple&, const Tuple&) { return true; }
    );

    join->open();
    Tuple t;
    int rows = 0;
    while (join->next(t)) rows++;
    join->close();

    assert(rows == 0);
    delete join;
    cout << "OK\n";
}

void test_orderby_vacio() {
    cout << "[Edge] ORDER BY sobre resultado vacio... ";

    remove("edge_orderby.bin");
    StorageManager sm("edge_orderby.bin");
    BufferManager bm(5, sm);
    assert(CsvLoader::load("data/datos1.csv", bm) > 0);

    Operator* plan = new SortOperator(
        new SelectOperator(
            new ScanOperator(bm, sm.getNumPages()),
            [](const Tuple& t) { return t.values[1] == "-1"; } // imposible
        ),
        1, true
    );

    plan->open();
    Tuple t;
    int rows = 0;
    while (plan->next(t)) rows++;
    plan->close();

    assert(rows == 0);
    delete plan;
    cout << "OK\n";
}

int main() {
    separador("PRUEBAS DE CASOS LIMITE");
    try {
        test_pool_lleno();
        test_btree_con_pool_pequeno();
        test_clave_inexistente();
        test_indexscan_clave_inexistente();
        test_join_vacio();
        test_join_una_tabla_vacia();
        test_orderby_vacio();

        separador("TODOS LOS CASOS LIMITE PASARON");
    } catch (const exception& e) {
        cerr << "\n[FALLO] Excepcion inesperada: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
