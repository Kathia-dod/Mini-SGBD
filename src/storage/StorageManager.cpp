#include "StorageManager.hpp"
#include <stdexcept>
#include <cstring>
#include <iostream>
using namespace std;

StorageManager::StorageManager(const string& filename) : disk_(filename), next_page_id_(1) {
    memset(loaded_, false, sizeof(loaded_));

    if (disk_.numPages() > 0) {
        loadMeta();
    } else {
        pages_[META_PAGE_ID].init(META_PAGE_ID);
        loaded_[META_PAGE_ID] = true;
        saveMeta();
        disk_.writePage(META_PAGE_ID, pages_[META_PAGE_ID].data);
    }
}

StorageManager::~StorageManager() {
    for (uint32_t i = 0; i < next_page_id_; i++) {
        if (loaded_[i] && pages_[i].dirty) {
            disk_.writePage(i, pages_[i].data);
        }
    }
    disk_.sync();
}

Page* StorageManager::fetchPage(uint32_t page_id) {
    if (page_id >= MAX_PAGES)
        throw DiskException("fetchPage: page_id " + to_string(page_id) + " fuera de rango");

    if (!loaded_[page_id]) {
        disk_.readPage(page_id, pages_[page_id].data);
        pages_[page_id].dirty = false;
        loaded_[page_id] = true;
    }
    return &pages_[page_id];
}

void StorageManager::flushPage(uint32_t page_id) {
    if (page_id >= MAX_PAGES || !loaded_[page_id])
        throw DiskException("flushPage: página " + to_string(page_id) + " no está cargada");

    if (pages_[page_id].dirty) {
        disk_.writePage(page_id, pages_[page_id].data);
        pages_[page_id].dirty = false;
    }
}

// Escribe el buffer externo (del BufferManager) directamente a disco.
// También actualiza el cache interno para mantener coherencia.
void StorageManager::writePage(uint32_t page_id, const char* data) {
    if (page_id >= MAX_PAGES)
        throw DiskException("writePage: page_id " + to_string(page_id) + " fuera de rango");

    disk_.writePage(page_id, data);

    // Sincronizar cache interno si la página estaba cargada
    if (loaded_[page_id]) {
        memcpy(pages_[page_id].data, data, PAGE_SIZE);
        pages_[page_id].dirty = false;
    }
}

uint32_t StorageManager::allocatePage() {
    uint32_t pid = next_page_id_++;
    if (pid >= MAX_PAGES)
        throw DiskException("allocatePage: límite de " + to_string(MAX_PAGES) + " páginas alcanzado");

    pages_[pid].init(pid);
    loaded_[pid] = true;

    saveMeta();
    return pid;
}

void StorageManager::saveMeta() {
    Page* meta = &pages_[META_PAGE_ID];
    memcpy(meta->data + PAGE_HEADER_SIZE, &next_page_id_, sizeof(uint32_t));
    meta->dirty = true;
    disk_.writePage(META_PAGE_ID, meta->data);
    meta->dirty = false;
}

void StorageManager::loadMeta() {
    disk_.readPage(META_PAGE_ID, pages_[META_PAGE_ID].data);
    loaded_[META_PAGE_ID] = true;
    pages_[META_PAGE_ID].dirty = false;

    memcpy(&next_page_id_, pages_[META_PAGE_ID].data + PAGE_HEADER_SIZE, sizeof(uint32_t));

    if (next_page_id_ == 0 || next_page_id_ > MAX_PAGES)
        next_page_id_ = 1;
}
/// métodos para persistir y recuperar el rootPageId del B+ Tree
void StorageManager::setRootPageId(uint32_t rootPageId) {
    Page* meta = &pages_[META_PAGE_ID];

    memcpy(meta->data + PAGE_HEADER_SIZE + sizeof(uint32_t),
           &rootPageId,
           sizeof(uint32_t));

    meta->dirty = true;
    disk_.writePage(META_PAGE_ID, meta->data);
    meta->dirty = false;
}

uint32_t StorageManager::getRootPageId() const {
    uint32_t rootPageId = 0;

    memcpy(&rootPageId,
           pages_[META_PAGE_ID].data + PAGE_HEADER_SIZE + sizeof(uint32_t),
           sizeof(uint32_t));

    return rootPageId;
}