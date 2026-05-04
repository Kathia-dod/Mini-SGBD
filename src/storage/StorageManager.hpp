#pragma once
#include "DiskManager.hpp"
#include "Page.hpp"
#include <string>
using namespace std;

// Página 0: reservada para metadatos del sistema (next_page_id, etc.), nunca para almacenar registros de usuario
static const uint32_t META_PAGE_ID = 0;

// StorageManager: capa sobre DiskManager que trabaja con objetos Page
class StorageManager {
public:
    explicit StorageManager(const string& filename);
    ~StorageManager();

    // Devuelve puntero a la página (la carga del disco si no está en memoria)
    Page* fetchPage(uint32_t page_id);

    // Escribe la página al disco y marca dirty = false, solo la escribe si dirty == true 
    void flushPage(uint32_t page_id);

    // Reserva una nueva página vacía y devuelve su page_id, persiste el contador next_page_id en la metapágina (página 0)
    uint32_t allocatePage();

    uint32_t getNumPages() const { return next_page_id_; }

private:
    DiskManager disk_;
    Page        pages_[MAX_PAGES];
    bool        loaded_[MAX_PAGES];
    uint32_t    next_page_id_;

    // Lee/escribe next_page_id desde/hacia la metapágina (página 0 en disco), garantiza que tras reiniciar el programa, allocatePage() no sobreescriba páginas que ya tienen datos
    void loadMeta();
    void saveMeta();
};