#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/query/CsvLoader.hpp"
#include "../src/query/IndexScanOperator.hpp"
#include "../src/index/BTreeIndex.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>

int main() {

    std::remove("test_index_scan.bin");

    StorageManager sm("test_index_scan.bin");

    BufferManager bm(
        5,
        sm
    );

    BTreeIndex index(&bm);

    int inserted = CsvLoader::load(
        "data/datos1.csv",
        bm,
        &index
    );

    assert(inserted > 0);

    //Consulta optimizada usando el índice B+ Tree.
    IndexScanOperator scan(
        bm,
        index,
        20
    );

    scan.open();

    Tuple result;

    bool found = scan.next(result);

    assert(found);

    std::cout << "\nRegistro encontrado mediante IndexScan:\n";

    for (const auto& value : result.values)
        std::cout << value << " ";

    std::cout << "\n\n";

    assert(result.values.size() == 3);

    assert(result.values[1] == "20");

    scan.close();

    std::cout << "--- explain() ---\n";

    scan.explain(std::cout);

    std::cout << "\n[test_index_scan] OK\n";

    return 0;
}