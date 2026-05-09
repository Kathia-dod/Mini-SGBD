#pragma once
#include "common.hpp"
#include <cstdint>
#include <cstring>
using namespace std;

// Tamaño del header al inicio de cada página en disco
static const uint32_t PAGE_HEADER_SIZE = 12;

// Header que se serializa al inicio de cada página en disco.
// Ocupa exactamente 12 bytes (4+2+2+4 con padding implícito).
/* Layout en disco (PAGE_SIZE = 4096 bytes):
 [0..3]   page_id           (4 bytes)
 [4..5]   num_slots         (2 bytes)
 [6..7]   free_space_offset (2 bytes) -> dónde empieza el espacio libre
 [8..11]  reserved          (4 bytes) -> reservado para uso futuro
 [12..]   área de datos (registros + slot directory)

 Crecimiento del área de datos:
 [HEADER][rec0][rec1]...[libre]...[slot1][slot0]
                        ↑                ↑
              free_space_offset    slots desde el final */
#pragma pack(push, 1)
struct PageHeader {
    uint32_t page_id;
    uint16_t num_slots;
    uint16_t free_space_offset;
    uint32_t reserved;
};
#pragma pack(pop)

/* Entrada del Slot Directory. Vive al FINAL de la página y crece hacia el inicio.
Slot 0 ocupa data[PAGE_SIZE-4 .. PAGE_SIZE-1]
Slot 1 ocupa data[PAGE_SIZE-8 .. PAGE_SIZE-5]
Slot N ocupa data[PAGE_SIZE - (N+1)*4 .. ...]
Si offset == SLOT_DELETED (0xFFFF): el registro fue eliminado lógicamente.
El espacio físico no se recupera...) */
#pragma pack(push, 1)
struct Slot {
    uint16_t offset;   // posición del registro dentro de data[], desde data[0]
    uint16_t length;   // tamaño del registro en bytes (0 si es eliminado)
};
#pragma pack(pop)

class Page {
public:
    char data[PAGE_SIZE];   // buffer raw de 4KB (lo que se escribe/lee de disco)

    // dirty: true si esta página fue modificada desde la última vez que se leyó del disco. El Buffer Manager lo usa para decidir si debe escribirla al disco antes de desalojarla del pool (política LRU).
    bool dirty = false;

    // Header 
    PageHeader* header() {
        return reinterpret_cast<PageHeader*>(data);
    }
    const PageHeader* header() const {
        return reinterpret_cast<const PageHeader*>(data);
    }

    // Inicializa la página con ceros y establece los valores del header, llamar siempre en páginas recién asignadas con allocatePage().
    void init(uint32_t page_id) {
        memset(data, 0, PAGE_SIZE);
        header()->page_id           = page_id;
        header()->num_slots         = 0;
        header()->free_space_offset = PAGE_HEADER_SIZE;
        header()->reserved          = 0;
        dirty = true;
    }

    // Espacio disponible entre el final de los datos y el inicio de los slots. Descuenta tanto los bytes usados por registros como por entradas de slot.
    uint32_t freeSpace() const {
        uint32_t slots_area = header()->num_slots * sizeof(Slot);
        return PAGE_SIZE - header()->free_space_offset - slots_area;
    }



private:
    // Retorna puntero al Slot N dentro del buffer, no valida si slot_id existe
    Slot* getSlot(uint16_t slot_id) {
        return reinterpret_cast<Slot*>(data + PAGE_SIZE) - (slot_id + 1);
    }
};