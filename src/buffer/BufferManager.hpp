#pragma once

#include <vector>
#include <unordered_map>
#include <list>
#include <cstdint>

#include "Frame.hpp"
#include "../storage/StorageManager.hpp"

using namespace std;

//  BufferManager — Semana 7: LRU + gestión de dirty page

class BufferManager {
public:
    BufferManager(size_t pool_size, StorageManager& storage_manager);
    ~BufferManager();
 
    // Devuelve puntero a la página (desde pool o cargada de disco).
    Page* fetchPage(uint32_t page_id);
 
    // Asigna una página nueva en disco y la carga en el pool.
    Page* newPage(uint32_t& out_page_id);
 
    // Decrementa pin_count. Si dirty==true, marca el frame como sucio
    bool unpinPage(uint32_t page_id, bool dirty);
 
    // Escribe al disco todas las páginas sucias
    void flushAll();
 
    // Escribe al disco la página indicada si está dirty 
    bool flushPage(uint32_t page_id);
 
    uint64_t getHits()   const { return hits_; }
    uint64_t getMisses() const { return misses_; }
    size_t   getPoolSize() const { return pool_size_; }
    
    StorageManager& getStorageManager() { return storage_manager_; }
private:
    size_t  pool_size_;
    StorageManager& storage_manager_;
 
    vector<Frame>  buffer_pool_;
    unordered_map<uint32_t, uint32_t>  page_table_;   // page_id → frame_id
 
    // Lista LRU: frente = MRU, fondo = LRU,contiene page_ids cuyo pin_count == 0 
    list<uint32_t>  lru_list_;
    unordered_map<uint32_t, list<uint32_t>::iterator> lru_map_;
 
    uint64_t hits_ = 0;
    uint64_t misses_ = 0;
 
    // Mueve page_id al frente de la lista LRU 
    void lru_touch(uint32_t page_id);
 
    // Elimina page_id de la lista LRU 
    void lru_remove(uint32_t page_id);
 
    // Elige el frame víctima según LRU (pin_count == 0).
    uint32_t lru_evict();
 
    // Carga page_id en el frame_id dado (no valida disponibilidad).
    Page* loadPageIntoFrame(uint32_t frame_id, uint32_t page_id);
};
