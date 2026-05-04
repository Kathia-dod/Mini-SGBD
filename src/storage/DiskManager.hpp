#pragma once
#include "common.hpp"
#include "DiskException.hpp"
#include <string>
using namespace std;

class DiskManager {
public:
    // Abre (o crea) el archivo binario de la bd y lanza DiskException si no se puede abrir
    explicit DiskManager(const string& filename);

    ~DiskManager();

    // Escribe PAGE_SIZE bytes al offset page_id * PAGE_SIZE y lanza DiskException si la escritura es incompleta o falla
    void writePage(uint32_t page_id, const char* data);

    // Lee PAGE_SIZE bytes del offset page_id * PAGE_SIZE a dest
    // Si la página aún no existe en disco, rellena con ceros (página nueva)
    void readPage(uint32_t page_id, char* dest);

    // Llama a fsync() para garantizar durabilidad física en disco
    void sync();

    // Cuántas páginas completas existen actualmente en el archivo
    uint32_t numPages() const { return num_pages_; }

private:
    int      fd_;         // file descriptor POSIX
    uint32_t num_pages_;  // calculado al abrir el archivo
};