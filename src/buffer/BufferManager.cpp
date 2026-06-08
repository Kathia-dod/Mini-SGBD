#include "BufferManager.hpp"
#include <stdexcept>
#include <iostream>
#include <climits>

using namespace std;

BufferManager::BufferManager(size_t pool_size, StorageManager& storage_manager) : pool_size_(pool_size), storage_manager_(storage_manager)
{
    buffer_pool_.resize(pool_size_);
}
 
BufferManager::~BufferManager() {
    // Al destruir el BufferManager, persistir todas las páginas sucias.
    flushAll();
}


Page* BufferManager::fetchPage(uint32_t page_id) {
 
    // 1. Hit: la página ya está en el pool
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
        uint32_t frame_id = it->second;
        buffer_pool_[frame_id].pin_count++;
 
        // Al pinear, la página ya no es candidata para LRU
        lru_remove(page_id);
 
        hits_++;
        return &buffer_pool_[frame_id].page;
    }
 
    misses_++;
 
    // 2. Miss: buscar un frame libre (nunca usado)
    for (size_t i = 0; i < pool_size_; i++) {
        if (!buffer_pool_[i].occupied) {
            return loadPageIntoFrame(static_cast<uint32_t>(i), page_id);
        }
    }
 
    // 3. Pool lleno: evictar según LRU
    uint32_t victim_frame = lru_evict();
    if (victim_frame == UINT32_MAX) {
        // Todos los frames están pineados — error fatal
        std::cerr << "[BufferManager] ERROR: todos los frames están pineados. "
                     "No se puede cargar page_id=" << page_id << "\n";
        return nullptr;
    }
    return loadPageIntoFrame(victim_frame, page_id);
}


Page* BufferManager::newPage(uint32_t& out_page_id) {
    out_page_id = storage_manager_.allocatePage();
    return fetchPage(out_page_id);
}


bool BufferManager::unpinPage(uint32_t page_id, bool dirty) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end())
        return false;

    uint32_t frame_id = it->second;
    Frame& frame = buffer_pool_[frame_id];

    if (frame.pin_count == 0) {
        return true;
    }

    frame.pin_count--;

    if (dirty)
        frame.dirty = true;

    // Si pin_count llega a 0, añadir al frente de la lista LRU
    if (frame.pin_count == 0) {
        lru_touch(page_id);
    }

    return true;
}

void BufferManager::flushAll() {
    for (size_t i = 0; i < pool_size_; i++) {
        Frame& frame = buffer_pool_[i];
        if (frame.occupied && frame.dirty) {
            // Escribir el buffer del frame
            storage_manager_.writePage(frame.page_id, frame.page.data);
            frame.dirty = false;
        }
    }
}
 
bool BufferManager::flushPage(uint32_t page_id) {
    auto it = page_table_.find(page_id);
    if (it == page_table_.end())
        return false;
 
    uint32_t frame_id = it->second;
    Frame& frame = buffer_pool_[frame_id];
    if (frame.dirty) {
        storage_manager_.writePage(page_id, frame.page.data);
        frame.dirty = false;
    }
    return true;
}


// Mueve (o inserta) page_id al FRENTE de la lista (= MRU)
void BufferManager::lru_touch(uint32_t page_id) {
    auto map_it = lru_map_.find(page_id);
    if (map_it != lru_map_.end()) {
        lru_list_.erase(map_it->second);
        lru_map_.erase(map_it);
    }
    lru_list_.push_front(page_id);
    lru_map_[page_id] = lru_list_.begin();
}

// Elimina page_id de la lista 
void BufferManager::lru_remove(uint32_t page_id) {
    auto map_it = lru_map_.find(page_id);
    if (map_it != lru_map_.end()) {
        lru_list_.erase(map_it->second);
        lru_map_.erase(map_it);
    }
}

// Elige la víctima LRU y libera el frame 
uint32_t BufferManager::lru_evict() {
    // Recorrer de fondo a frente buscando pin_count == 0
    for (auto rit = lru_list_.rbegin(); rit != lru_list_.rend(); ++rit) {
        uint32_t victim_page_id = *rit;
        uint32_t victim_frame   = page_table_[victim_page_id];
        Frame&   frame          = buffer_pool_[victim_frame];
 
        if (frame.pin_count != 0) continue;  // seguridad extra
 
        // Flush si está dirty antes de reutilizar el frame.
        if (frame.dirty) {
            storage_manager_.writePage(victim_page_id, frame.page.data);
            frame.dirty = false;
        }
 
        // Limpiar estructuras de la víctima
        page_table_.erase(victim_page_id);
        lru_list_.erase(std::next(rit).base());
        lru_map_.erase(victim_page_id);
 
        // Marcar frame como libre
        frame.occupied  = false;
        frame.pin_count = 0;
 
        return victim_frame;
    }
    return UINT32_MAX;  // no hay víctima disponible
}

// Carga page_id en el frame_id 
Page* BufferManager::loadPageIntoFrame(uint32_t frame_id, uint32_t page_id) {
    Frame& frame = buffer_pool_[frame_id];
 
    // Obtener la página del StorageManager (lee de disco si es necesario)
    Page* disk_page = storage_manager_.fetchPage(page_id);
    frame.page      = *disk_page;
    frame.page_id   = page_id;
    frame.occupied  = true;
    frame.pin_count = 1;
    frame.dirty     = false;
 
    page_table_[page_id] = frame_id;
 
    return &frame.page;
}