#pragma once
#include <cstdint>

// Tamaño fijo de página: 4KB
// Cambiar este valor afecta todo el sistema automáticamente
static const uint32_t PAGE_SIZE  = 4096;
static const uint32_t MAX_PAGES  = 20000;

// Valor centinela para slots eliminados, nunca puede ser un offset real porque ningún registro cabe en 0xFFFF solo
static const uint16_t SLOT_DELETED = 0xFFFF;
