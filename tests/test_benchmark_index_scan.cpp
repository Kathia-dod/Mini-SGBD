#include <iostream>
#include <cstdio>
#include <vector>
#include <string>

#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/query/CsvLoader.hpp"
#include "../src/query/ScanOperator.hpp"
#include "../src/query/SelectOperator.hpp"
#include "../src/query/IndexScanOperator.hpp"
#include "../src/index/BTreeIndex.hpp"

using namespace std;

struct BenchResult {
    string metodo;
    int clave;
    long tuplas;
    long paginasLeidas;
    double tiempoMs;
    bool encontrado;
};

void imprimir(const BenchResult& r) {
    cout << "  [" << r.metodo << "] edad=" << r.clave
         << " | encontrado=" << (r.encontrado ? "SI" : "NO")
         << " | tuplas=" << r.tuplas
         << " | paginas_leidas=" << r.paginasLeidas
         << " | tiempo=" << r.tiempoMs << " ms\n";
}

BenchResult benchScanSelect(BufferManager& bm, uint32_t maxPageId, int edad) {
    Operator* plan = new SelectOperator(
        new ScanOperator(bm, maxPageId),
        [edad](const Tuple& t) { return std::stoi(t.values[1]) == edad; }
    );

    plan->open();
    Tuple t;
    bool encontrado = false;
    while (plan->next(t)) encontrado = true;
    plan->close();

    BenchResult r{"Scan+Select (sin indice)", edad,
                   plan->tuplesProduced(), plan->pagesRead(),
                   plan->elapsedMs(), encontrado};
    delete plan;
    return r;
}

BenchResult benchIndexScan(BufferManager& bm, BTreeIndex& index, int edad) {
    IndexScanOperator scan(bm, index, edad);

    scan.open();
    Tuple t;
    bool encontrado = scan.next(t);
    scan.close();

    BenchResult r{"IndexScan (con indice B+)", edad,
                   scan.tuplesProduced(), scan.pagesRead(),
                   scan.elapsedMs(), encontrado};
    return r;
}

int main() {
    remove("bench_no_index.bin");
    remove("bench_with_index.bin");

    StorageManager smA("bench_no_index.bin");
    BufferManager bmA(20, smA);
    int insertedA = CsvLoader::load("data/datos1.csv", bmA);
    cout << "[Sin indice] " << insertedA << " registros cargados\n";

    StorageManager smB("bench_with_index.bin");
    BufferManager bmB(20, smB);
    BTreeIndex index(&bmB);
    int insertedB = CsvLoader::load("data/datos1.csv", bmB, &index);
    cout << "[Con indice] " << insertedB << " registros cargados\n";

    vector<int> claves = {18, 20, 25, 30, 40, 50, 60, 9999}; // 9999 no existe

    cout << "\n------------ BENCHMARK: Scan+Select vs IndexScan -----------\n";
    for (int edad : claves) {
        cout << "\n-- edad = " << edad << " --\n";
        BenchResult sinIdx = benchScanSelect(bmA, smA.getNumPages(), edad);
        imprimir(sinIdx);

        BenchResult conIdx = benchIndexScan(bmB, index, edad);
        imprimir(conIdx);

        if (sinIdx.paginasLeidas > 0) {
            double mejora = 100.0 * (1.0 - (double)conIdx.paginasLeidas / sinIdx.paginasLeidas);
            cout << "  >> Reduccion de paginas leidas con indice: " << mejora << "%\n";
        }
    }

    cout << "\n[benchmark_index_scan] Finalizado\n";
    return 0;
}
