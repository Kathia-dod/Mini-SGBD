#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/index/BTreeIndex.hpp"
#include "../src/query/ScanOperator.hpp"
#include "../src/query/SelectOperator.hpp"
#include "../src/query/IndexScanOperator.hpp"
#include "../src/query/QueryOptimizer.hpp"
#include "../src/query/QueryStatement.hpp"
#include "../src/query/Tuple.hpp"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <sstream>
#include <vector>

// Genera y carga N registros sinteticos ("id|nombre|edad|ciudad"), indexando por "id"
// (unico, para no chocar con el upsert del B+ Tree ante claves duplicadas).
static uint32_t cargarDatasetSintetico(BufferManager& bm, BTreeIndex& index, int n) {
    static const std::vector<std::string> ciudades = {
        "arequipa", "lima", "cusco", "puno", "tacna", "moquegua"
    };

    // --- Fase A: insertar SOLO los datos, guardando (id -> pageId/slot) en memoria ---
    std::vector<std::pair<int,int>> ridPorId; // (id, rid codificado)
    ridPorId.reserve(n);

    uint32_t pageId;
    Page* page = bm.newPage(pageId);

    for (int id = 0; id < n; id++) {
        std::ostringstream oss;
        oss << id << "|usuario_" << id << "|" << (18 + (id % 63)) << "|"
            << ciudades[id % ciudades.size()];
        std::string row = oss.str();

        int slot = page->insertRecord(row.c_str(), static_cast<uint16_t>(row.size()));
        if (slot == -1) {
            bm.unpinPage(pageId, true);
            page = bm.newPage(pageId);
            slot = page->insertRecord(row.c_str(), static_cast<uint16_t>(row.size()));
        }
        if (slot == -1) {
            std::cerr << "[demo] fila " << id << " demasiado grande, se omite\n";
            continue;
        }

        int rid = static_cast<int>(pageId) * 1000 + slot;
        ridPorId.push_back({id, rid});
    }
    bm.unpinPage(pageId, true);

    uint32_t maxDataPageId = pageId + 1; // primera pagina de datos NO usada (exclusive)

    // --- Fase B: recien ahora construir el indice (sus paginas van DESPUES de las de datos) ---
    for (auto& [id, rid] : ridPorId)
        index.insert(id, rid);

    return maxDataPageId;
}

int main() {
    const int N = 10000;
    std::remove("demo_live.bin");

    std::cout << "__________________________________________________________\n";
    std::cout << " DEMOSTRACION EN VIVO - Mini-SGBD (" << N << " registros)\n";
    std::cout << "__________________________________________________________\n\n";

    StorageManager sm("demo_live.bin");
    BufferManager  bm(20, sm);      // pool deliberadamente pequeno frente a N registros
    BTreeIndex     index(&bm);

    // --- 1) Carga masiva + construccion del indice B+ Tree ---
    auto t0 = std::chrono::steady_clock::now();
    uint32_t maxDataPageId = cargarDatasetSintetico(bm, index, N);
    auto t1 = std::chrono::steady_clock::now();
    double loadMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "[1] Carga de " << N << " registros e indexacion por 'id'\n";
    std::cout << "    Paginas de datos : " << (maxDataPageId - 1) << "\n";
    std::cout << "    Paginas totales  : " << sm.getNumPages()
               << " (incluye paginas del indice B+ Tree)\n";
    std::cout << "    Tiempo de carga  : " << loadMs << " ms\n";
    std::cout << "    Buffer hits      : " << bm.getHits()   << "\n";
    std::cout << "    Buffer misses    : " << bm.getMisses() << "\n\n";

    int idBuscado = N / 2;

    // El QueryStatement: SELECT * FROM t WHERE id = idBuscado
    QueryStatement stmt;
    stmt.select_columns = {"*"};
    stmt.from_table = "t";
    stmt.where_clause = {true, "id", "=", std::to_string(idBuscado)};

    // --- 2) QueryOptimizer decide la ruta con el indice registrado ---
    QueryOptimizer optimizerConIndice;
    optimizerConIndice.registerIndex("id", &index);

    Operator* planConIndice = optimizerConIndice.chooseAccessPath(stmt, bm, maxDataPageId);

    planConIndice->open();
    Tuple resultadoIdx;
    bool encontradoIdx = planConIndice->next(resultadoIdx);
    planConIndice->close();

    assert(encontradoIdx);
    std::cout << "[2] QueryOptimizer -- indice 'id' registrado (id = " << idBuscado << ")\n";
    std::cout << "    Ruta elegida     : " << planConIndice->name() << "\n";
    std::cout << "    Encontrado       : " << (encontradoIdx ? "si" : "no") << "\n";
    std::cout << "    Paginas leidas   : " << planConIndice->pagesRead()   << "\n";
    std::cout << "    Tiempo           : " << planConIndice->elapsedMs()   << " ms\n";
    std::cout << "    Registro         : ";
    for (auto& v : resultadoIdx.values) std::cout << v << " ";
    std::cout << "\n\n";
    delete planConIndice;

    // --- 3) query con un optimizer que no tiene el indice registrado ---
    QueryOptimizer optimizerSinIndice; 

    Operator* accessSinIndice = optimizerSinIndice.chooseAccessPath(stmt, bm, maxDataPageId);
    Operator* planSinIndice = new SelectOperator(accessSinIndice, [idBuscado](const Tuple& t) { 
        return std::stoi(t.values[0]) == idBuscado; 
    }
    );

    planSinIndice->open();
    Tuple resultadoScan;
    bool encontradoScan = false;
    while (planSinIndice->next(resultadoScan)) encontradoScan = true;
    planSinIndice->close();

    assert(encontradoScan);
    std::cout << "[3] QueryOptimizer -- misma query, sin indice registrado\n";
    std::cout << "    Ruta elegida     : Scan -> Select (el optimizer no encontro indice)\n";
    std::cout << "    Encontrado       : " << (encontradoScan ? "si" : "no") << "\n";
    std::cout << "    Paginas leidas   : " << planSinIndice->pagesRead() << "\n";
    std::cout << "    Tiempo           : " << planSinIndice->elapsedMs() << " ms\n";

    std::cout << "\n--- explain() ---\n";
    planSinIndice->explain(std::cout);
    delete planSinIndice;

    std::cout << "\n[4] Resumen final del Buffer Manager tras toda la demo\n";
    std::cout << "    Buffer hits      : " << bm.getHits()   << "\n";
    std::cout << "    Buffer misses    : " << bm.getMisses() << "\n";

    std::cout << "\n[test_live_demo] OK\n";
    return 0;
}
