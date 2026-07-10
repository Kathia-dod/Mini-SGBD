#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/query/ScanOperator.hpp"
#include "../src/query/SelectOperator.hpp"
#include "../src/query/ProjectOperator.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>

int main() {
    std::remove("test_query.bin");

    StorageManager sm("test_query.bin");
    BufferManager bm(3, sm);

    uint32_t pid;
    Page* p = bm.newPage(pid);
    auto insertRow = [&](const char* row) {
        p->insertRecord(row, static_cast<uint16_t>(std::strlen(row)));
    };
    insertRow("juan|20|lima");
    insertRow("maria|25|arequipa");
    insertRow("pedro|20|cusco");
    bm.unpinPage(pid, true);

    // Plan: Project[0,2]( Select(edad == "20")( Scan(tabla_prueba) ) )
    Operator* plan = new ProjectOperator(
        new SelectOperator(
            new ScanOperator(bm, sm.getNumPages()),
            [](const Tuple& t){ return t.values[1] == "20"; }
        ),
        {0, 2}
    );

    plan->open();
    Tuple t;
    int rows = 0;
    std::cout << "Resultado (nombre, ciudad) con edad == 20:\n";
    while (plan->next(t)) {
        for (auto& v : t.values) std::cout << v << " ";
        std::cout << "\n";
        rows++;
    }
    plan->close();

    std::cout << "\n--- explain() ---\n";
    plan->explain(std::cout);

    // Verificaciones basicas:
    assert(rows == 2);

    delete plan;
    std::cout << "\n[test_query_scan] OK\n";
    return 0;
}