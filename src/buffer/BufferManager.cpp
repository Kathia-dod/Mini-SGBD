#include "BufferManager.hpp"

BufferManager::BufferManager(size_t pool_size,
                             StorageManager& storage_manager)
    : pool_size(pool_size),
      storage_manager(storage_manager) {

    buffer_pool.resize(pool_size);
}