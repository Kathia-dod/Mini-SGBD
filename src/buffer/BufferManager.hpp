#pragma once

#include <vector>
#include <unordered_map>

#include "Frame.hpp"
#include "../storage/StorageManager.hpp"

class BufferManager {

private:

    size_t pool_size;

    std::vector<Frame> buffer_pool;
    std::unordered_map<uint32_t, uint32_t> page_table;

    StorageManager& storage_manager;

public:

    BufferManager(size_t pool_size,
                  StorageManager& storage_manager);

    Page* fetchPage(uint32_t page_id);

    bool unpinPage(uint32_t page_id, bool dirty);
};