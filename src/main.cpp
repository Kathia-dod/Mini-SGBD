#include <iostream>

#include "storage/StorageManager.hpp"
#include "buffer/BufferManager.hpp"

int main() {

    StorageManager storage_manager("database.bin");

    BufferManager buffer_manager(3, storage_manager);

    buffer_manager.fetchPage(0);
    std::cout << "Pagina 0 cargada\n";

    buffer_manager.unpinPage(0, false);

    buffer_manager.fetchPage(1);
    std::cout << "Pagina 1 cargada\n";

    buffer_manager.unpinPage(1, false);

    buffer_manager.fetchPage(2);
    std::cout << "Pagina 2 cargada\n";

    buffer_manager.unpinPage(2, false);

    buffer_manager.fetchPage(3);
    std::cout << "Pagina 3 cargada reutilizando frame\n";

    return 0;
}