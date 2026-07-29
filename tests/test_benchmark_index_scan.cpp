#include <iostream>
#include <cstdio>
#include <vector>
#include <string>
#include <iomanip>

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
    string detalle;
};

void imprimir(const BenchResult& r) {
    cout << "  [" << left << setw(24) << r.metodo << "] edad=" << setw(3) << r.clave
         << " | encontrado=" << (r.encontrado ? "SI" : "NO")
         << " | tuplas=" << setw(4) << r.tuplas
         << " | paginas_leidas=" << setw(4) << r.paginasLeidas
         << " | tiempo=" << setw(8) << r.tiempoMs << " ms"
         << "| " << r.detalle << "\n";
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

    string detalle = encontrado ? ("dpto=" + t.values[3] + " salario=" + t.values[4]) : "-";

    BenchResult r{"Scan+Select (sin indice)", edad,
                   plan->tuplesProduced(), plan->pagesRead(),
                   plan->elapsedMs(), encontrado, detalle};
    delete plan;
    return r;
}

BenchResult benchIndexScan(BufferManager& bm, BTreeIndex& index, int edad) {
    IndexScanOperator scan(bm, index, edad);

    scan.open();
    Tuple t;
    bool encontrado = scan.next(t);
    scan.close();

    string detalle = encontrado ? ("dpto=" + t.values[3] + " salario=" + t.values[4]) : "-";

    BenchResult r{"IndexScan (con indice B+)", edad,
                   scan.tuplesProduced(), scan.pagesRead(),
                   scan.elapsedMs(), encontrado, detalle};
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

    vector<int> claves = {18, 20, 22, 25, 28, 30, 33, 35, 38, 40, 43, 45, 48, 50, 53, 55, 58, 60, 65, 9999}; // 9999 no existe

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
