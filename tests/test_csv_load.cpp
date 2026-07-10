#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/query/CsvLoader.hpp"
#include "../src/query/ScanOperator.hpp"
#include <cassert>
#include <cstdio>
#include <iostream>

int main() {
    std::remove("test_csv.bin");

    StorageManager sm("test_csv.bin");
    BufferManager bm(5, sm);

    int inserted = CsvLoader::load("data/datos1.csv", bm);
    assert(inserted > 0);

    Operator* scan = new ScanOperator(bm, sm.getNumPages());
    scan->open();
    Tuple t;
    int count = 0;
    while (scan->next(t)) count++;
    scan->close();

    std::cout << "\n--- explain() ---\n";
    scan->explain(std::cout);
    std::cout << "Total leido: " << count << "\n";

    assert(count == inserted);

    delete scan;
    std::cout << "\n[test_csv_load] OK\n";
    return 0;
}