#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/query/CsvLoader.hpp"
#include "../src/index/BTreeIndex.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>

int main() {
    std::remove("test_index.bin");

    StorageManager sm("test_index.bin");
    BufferManager bm(5, sm);

    // Crear el índice B+ Tree
    BTreeIndex index(&bm);

    // Cargar los registros e indexar por edad
    int inserted = CsvLoader::load(
        "data/datos1.csv",
        bm,
        &index
    );

    assert(inserted > 0);

    // Buscar la edad 20 en el índice
    int encodedRID;

    bool found = index.search(20, encodedRID);

    assert(found);

    // Decodificar el RID
    uint32_t pageId = encodedRID / 1000;
    uint16_t slotId = encodedRID % 1000;

    // Obtener directamente la página
    Page* page = bm.fetchPage(pageId);
    assert(page != nullptr);

    // Obtener directamente el registro
    char buffer[PAGE_SIZE];
    uint16_t length;

    bool ok = page->getRecord(slotId, buffer, length);
    assert(ok);

    Tuple tuple = Tuple::deserialize(buffer, length);

    std::cout << "Registro encontrado usando B+ Tree:\n";

    for (const auto& value : tuple.values) {
        std::cout << value << " ";
    }

    std::cout << "\n";

    bm.unpinPage(pageId, false);

    std::cout << "\n[test_query_index] OK\n";

    return 0;
}