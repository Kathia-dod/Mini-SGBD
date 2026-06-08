#pragma once
#include "DiskManager.hpp"
#include "Page.hpp"
#include <string>
using namespace std;

// Página 0: reservada para metadatos del sistema (next_page_id, etc.), nunca para almacenar registros de usuario
static const uint32_t META_PAGE_ID = 0;

// StorageManager
class StorageManager {
public:
    explicit StorageManager(const string& filename);
    ~StorageManager();
 
    // Devuelve puntero a la página (la carga del disco si no está en memoria)
    Page* fetchPage(uint32_t page_id);
 
    // Escribe la página al disco y marca dirty = false, solo si dirty == true
    void flushPage(uint32_t page_id);
 
    // Escribe 'data' (PAGE_SIZE bytes) directamente a disco para page_id.
    // Sincroniza también el cache interno si la página está cargada.
    // Usado por BufferManager al evictar/flush frames con datos modificados.
    void writePage(uint32_t page_id, const char* data);
 
    // Reserva una nueva página vacía y devuelve su page_id
    uint32_t allocatePage();
 
    uint32_t getNumPages() const { return next_page_id_; }

private:
    DiskManager disk_;
    Page pages_[MAX_PAGES];
    bool loaded_[MAX_PAGES];
    uint32_t next_page_id_;

    // Lee/escribe next_page_id desde/hacia la metapágina (página 0 en disco), garantiza que tras reiniciar el programa, allocatePage() no sobreescriba páginas que ya tienen datos
    void loadMeta();
    void saveMeta();
};