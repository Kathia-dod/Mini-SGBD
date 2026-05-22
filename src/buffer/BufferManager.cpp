#include "BufferManager.hpp"

BufferManager::BufferManager(size_t pool_size,
                             StorageManager& storage_manager)
    : pool_size(pool_size),
      storage_manager(storage_manager) {

    buffer_pool.resize(pool_size);
}

Page* BufferManager::fetchPage(uint32_t page_id) {

    // pagina en memoria
    if (page_table.find(page_id) != page_table.end()) {

        uint32_t frame_id = page_table[page_id];

        return &buffer_pool[frame_id].page;
    }

    // busqueda de frame disponible
    for (size_t i = 0; i < pool_size; i++) {

        if (!buffer_pool[i].occupied) {

            Page* page = storage_manager.fetchPage(page_id);

            buffer_pool[i].page = *page;

            buffer_pool[i].page_id = page_id;

            buffer_pool[i].occupied = true;

            page_table[page_id] = i;

            return &buffer_pool[i].page;
        }
    }

    return nullptr;
}