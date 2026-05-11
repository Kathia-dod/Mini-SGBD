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

    // Inicializa la página con ceros y establece valores del header, llamar siempre en páginas recién asignadas con allocatePage().
    void init(uint32_t page_id) {
        memset(data, 0, PAGE_SIZE);
        header()->page_id = page_id;
        header()->num_slots = 0;
        header()->free_space_offset = PAGE_HEADER_SIZE;
        header()->reserved = 0;
        dirty = true;
    }

    // Espacio disponible entre el final de los datos y el inicio de los slots. Descuenta tanto los bytes usados por registros como por entradas de slot.
    uint32_t freeSpace() const {
        uint32_t slots_area = header()->num_slots * sizeof(Slot);
        return PAGE_SIZE - header()->free_space_offset - slots_area;
    }

    // Inserta un registro de 'length' bytes apuntado por 'record', retorna el slot_id asignado (desde 0), o -1 si no hay espacio
    // espacio necesario = length bytes de datos + sizeof(Slot), dirty = true si la inserción fue exitosa.
    int insertRecord(const char* record, uint16_t length) {
        if (freeSpace() < static_cast<uint32_t>(length + sizeof(Slot)))
            return -1;
        PageHeader* h = header();
        // Copiar el registro al área de datos en la posición libre actual
        memcpy(data + h->free_space_offset, record, length);
        // Registrar el slot: nuevo slot_id = índice actual de num_slots
        uint16_t sid = h->num_slots;
        Slot* slot = getSlot(sid);
        slot->offset = h->free_space_offset;
        slot->length = length;

        h->free_space_offset += length;
        h->num_slots++;

        dirty = true;
        return sid;
    }

    // Lee el registro del slot_id al buffer 'dest', escribe en 'out_length' la longitud real del registro leído, da false si slot_id no existe o el registro fue eliminado.
    bool getRecord(uint16_t slot_id, char* dest, uint16_t& out_length) {
        const PageHeader* h = header();
        // el slot existe dentro del rango
        if (slot_id >= h->num_slots)
            return false;

        Slot* slot = getSlot(slot_id);

        if (slot->offset == SLOT_DELETED)
            return false;

        out_length = slot->length;
        memcpy(dest, data + slot->offset, slot->length);
        return true;
    }

    // Actualiza el contenido del registro en slot_id con new_data, PERO length debe ser igual al tamaño original del registro, solo actualiza bytes en el área de datos sin mover nada en página
    // Retorna false si slot_id no existe, fue eliminado, o la longitud no coincide
    bool updateRecord(uint16_t slot_id, const char* new_data, uint16_t length) {
        const PageHeader* h = header();

        if (slot_id >= h->num_slots)
            return false;

        Slot* slot = getSlot(slot_id);

        if (slot->offset == SLOT_DELETED)
            return false;

        if (slot->length != length)
            return false;

        memcpy(data + slot->offset, new_data, length);
        dirty = true;
        return true;
    }

    // Elimina lógicamente el registro del slot_id
    // Retorna false si slot_id no existe o ya estaba eliminado
    bool deleteRecord(uint16_t slot_id) {
        const PageHeader* h = header();

        if (slot_id >= h->num_slots)
            return false;

        Slot* slot = getSlot(slot_id);

        if (slot->offset == SLOT_DELETED)
            return false;

        slot->offset = SLOT_DELETED;
        slot->length = 0;
        dirty = true;
        return true;
    }

private:
    // Retorna puntero al Slot N dentro del buffer, no valida si slot_id existe
    Slot* getSlot(uint16_t slot_id) {
        return reinterpret_cast<Slot*>(data + PAGE_SIZE) - (slot_id + 1);
    }
};