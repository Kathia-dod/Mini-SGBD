#include "StorageManager.hpp"
#include <stdexcept>
#include <cstring>
#include <iostream>
using namespace std;

StorageManager::StorageManager(const string& filename)
    : disk_(filename), next_page_id_(1)  // 0 reservada para metapágina
{
    memset(loaded_, false, sizeof(loaded_));

    // Si el archivo ya tenía páginas, recuperar el next_page_id desde la metapágina para no sobreescribir datos existentes.
    if (disk_.numPages() > 0) {
        loadMeta();
    } else {
        // Archivo nuevo: inicializa la metapágina
        pages_[META_PAGE_ID].init(META_PAGE_ID);
        loaded_[META_PAGE_ID] = true;
        saveMeta();
        disk_.writePage(META_PAGE_ID, pages_[META_PAGE_ID].data);
    }
}

StorageManager::~StorageManager() {
    // Al cerrar, flush de todas las páginas sucias
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
        pages_[page_id].dirty = false;  // recién leída del disco = limpia
        loaded_[page_id] = true;
    }
    return &pages_[page_id];
}

void StorageManager::flushPage(uint32_t page_id) {
    if (page_id >= MAX_PAGES || !loaded_[page_id])
        throw DiskException("flushPage: página " + to_string(page_id) + " no está cargada");

    // Solo escribir si fue modificada (dirty bit)
    if (pages_[page_id].dirty) {
        disk_.writePage(page_id, pages_[page_id].data);
        pages_[page_id].dirty = false;
    }
}

uint32_t StorageManager::allocatePage() {
    uint32_t pid = next_page_id_++;
    if (pid >= MAX_PAGES)
        throw DiskException("allocatePage: límite de " + to_string(MAX_PAGES) + " páginas alcanzado");

    pages_[pid].init(pid);
    loaded_[pid] = true;

    // Persistir el nuevo next_page_id en la metapágina
    saveMeta();
    return pid;
}

// Guarda next_page_id en los primeros 4 bytes del área de datos de la metapágina
void StorageManager::saveMeta() {
    Page* meta = &pages_[META_PAGE_ID];
    memcpy(meta->data + PAGE_HEADER_SIZE, &next_page_id_, sizeof(uint32_t));
    meta->dirty = true;
    disk_.writePage(META_PAGE_ID, meta->data);
    meta->dirty = false;
}

// Carga next_page_id desde la metapágina en disco.
void StorageManager::loadMeta() {
    disk_.readPage(META_PAGE_ID, pages_[META_PAGE_ID].data);
    loaded_[META_PAGE_ID] = true;
    pages_[META_PAGE_ID].dirty = false;

    memcpy(&next_page_id_, pages_[META_PAGE_ID].data + PAGE_HEADER_SIZE, sizeof(uint32_t));

    // Sanidad: si next_page_id es 0 o corrupto, inicializar desde 1
    if (next_page_id_ == 0 || next_page_id_ > MAX_PAGES)
        next_page_id_ = 1;
}