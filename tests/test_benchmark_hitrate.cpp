#include <iostream>
#include <cstdio>
#include <vector>
#include <iomanip>
#include <cstdlib>

#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/query/CsvLoader.hpp"
#include "../src/query/ScanOperator.hpp"

using namespace std;

void hitRateScanSecuencial(size_t poolSize) {
    string db = "hitrate_scan_pool" + to_string(poolSize) + ".bin";
    remove(db.c_str());

    StorageManager sm(db);
    BufferManager bm(poolSize, sm);
    int inserted = CsvLoader::load("data/datos1.csv", bm);

    // Dos pasadas completas: cada una toca cada pagina una sola vez,
    // por lo que el hit rate sera bajo salvo que el pool retenga
    // paginas de la pasada anterior hasta que la siguiente las repita.
    for (int pasada = 0; pasada < 2; pasada++) {
        ScanOperator scan(bm, sm.getNumPages());
        scan.open();
        Tuple t;
        while (scan.next(t)) {}
        scan.close();
    }

    uint64_t hits = bm.getHits(), misses = bm.getMisses();
    double hitRate = 100.0 * hits / (hits + misses);

    cout << "[Scan secuencial]  pool=" << setw(4) << poolSize
         << " | registros=" << inserted
         << " | hits=" << hits << " misses=" << misses
         << " | hit rate=" << fixed << setprecision(2) << hitRate << "%\n";
}

void hitRateAccesoAleatorio(size_t poolSize) {
    string db = "hitrate_rand_pool" + to_string(poolSize) + ".bin";
    remove(db.c_str());

    StorageManager sm(db);
    BufferManager bm(poolSize, sm);
    CsvLoader::load("data/datos1.csv", bm);
    uint32_t maxPage = sm.getNumPages();

    // 500 accesos aleatorios simulando un workload real con localidad:
    // mientras mas grande el pool, mas paginas "calientes" caben en cache.
    srand(42);
    for (int i = 0; i < 500; i++) {
        uint32_t pid = 1 + (rand() % (maxPage - 1)); // saltar metapagina (0)
        Page* p = bm.fetchPage(pid);
        if (p) bm.unpinPage(pid, false);
    }

    uint64_t hits = bm.getHits(), misses = bm.getMisses();
    double hitRate = 100.0 * hits / (hits + misses);

    cout << "[Acceso aleatorio] pool=" << setw(4) << poolSize
         << " | hits=" << hits << " misses=" << misses
         << " | hit rate=" << fixed << setprecision(2) << hitRate << "%\n";
}

int main() {
    cout << "--------------------- HIT RATE del Buffer Manager segun tamano de pool ---------------------\n\n";

    vector<size_t> poolSizes = {2, 3, 5, 10, 20, 50, 100};

    cout << "--- Escenario 1: Scan secuencial (2 pasadas completas) ---\n";
    for (size_t ps : poolSizes) hitRateScanSecuencial(ps);

    cout << "\n--- Escenario 2: Acceso aleatorio con reuso (localidad) ---\n";
    for (size_t ps : poolSizes) hitRateAccesoAleatorio(ps);

    cout << "\n[benchmark_hitrate] Finalizado\n";
    return 0;
}
