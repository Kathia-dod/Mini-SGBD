#pragma once
#include "common.hpp"
#include <cstdint>
#include <cstring>
using namespace std;

// Tamaño del header al inicio de cada página en disco
static const uint32_t PAGE_HEADER_SIZE = 12;

// Header que se serializa al inicio de cada página en disco.
// Ocupa exactamente 12 bytes (4+2+2+4 con padding implícito -> mejor declarar explícito).
// IMPORTANTE: debe mantenerse consistente entre escrituras y lecturas.
/* Layout en disco (PAGE_SIZE = 4096 bytes):
 [0..3]   page_id           (4 bytes)
 [4..5]   num_slots         (2 bytes)
 [6..7]   free_space_offset (2 bytes) -> dónde empieza el espacio libre
 [8..11]  meta_page_id      (4 bytes) -> reservado para la metapágina
 [12..]   área de datos (registros + slot directory) */
// Crecimiento del área de datos: [HEADER][rec0][rec1]...[libre]...[slot1][slot0]
#pragma pack(push, 1)
struct PageHeader {
    uint32_t page_id;
    uint16_t num_slots;
    uint16_t free_space_offset;
    uint32_t reserved;  // para uso futuro (en listas enlazadas de B+Tree)
};
#pragma pack(pop)

class Page {
public:
    char data[PAGE_SIZE];  // buffer raw de 4KB (lo que se escribe/lee de disco)

    // dirty: true si esta página fue modificada desde la última vez que leyó del disco. si debe escribirla al disco antes de desalojarla del pool (política LRU).
    bool dirty = false;

    // Puntero al header al inicio del buffer
    PageHeader* header() {
        return reinterpret_cast<PageHeader*>(data);
    }

    const PageHeader* header() const {
        return reinterpret_cast<const PageHeader*>(data);
    }

    // Inicializa la página con ceros y establece los valores del header.
    void init(uint32_t page_id) {
        memset(data, 0, PAGE_SIZE);
        header()->page_id           = page_id;
        header()->num_slots         = 0;
        header()->free_space_offset = PAGE_HEADER_SIZE;
        header()->reserved          = 0;
        dirty = true;  // recién inicializada, debe escribirse al disco
    }

    // Espacio libre disponible en bytes (entre los datos y el slot directory)
    uint32_t freeSpace() const {
        uint32_t slots_used = header()->num_slots * 4; // cada Slot ocupa 4 bytes 
        return PAGE_SIZE - header()->free_space_offset - slots_used;
    }
};