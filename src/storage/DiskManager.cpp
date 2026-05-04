#include "DiskManager.hpp"
#include <fcntl.h>       // open(), O_RDWR, O_CREAT
#include <unistd.h>      // read(), write(), lseek(), fsync(), close()
#include <sys/stat.h>    // S_IRUSR, S_IWUSR
#include <cstring>
#include <iostream>
using namespace std;

DiskManager::DiskManager(const string& filename) {
    // O_RDWR | O_CREAT: abre o crea el archivo en modo lectura/escritura
    fd_ = open(filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd_ == -1)
        throw DiskException("No se pudo abrir el archivo: " + filename);

    // Calcular nro de páginas existentes en el archivo
    off_t size = lseek(fd_, 0, SEEK_END);
    if (size == -1)
        throw DiskException("Error al calcular tamaño del archivo");

    num_pages_ = static_cast<uint32_t>(size / PAGE_SIZE);
}

DiskManager::~DiskManager() {
    sync();
    close(fd_);
}

void DiskManager::writePage(uint32_t page_id, const char* data) {
    if (page_id >= MAX_PAGES)
        throw DiskException("writePage: page_id " + to_string(page_id) + " fuera de rango");

    off_t offset = static_cast<off_t>(page_id) * PAGE_SIZE;
    if (lseek(fd_, offset, SEEK_SET) == -1)
        throw DiskException("writePage: lseek falló en página " + to_string(page_id));

    ssize_t written = write(fd_, data, PAGE_SIZE);
    if (written != static_cast<ssize_t>(PAGE_SIZE))
        throw DiskException("writePage: escritura incompleta en página " + to_string(page_id));

    // Actualizar contador si se escribió más allá del final actual
    if (page_id >= num_pages_)
        num_pages_ = page_id + 1;
}

void DiskManager::readPage(uint32_t page_id, char* dest) {
    if (page_id >= MAX_PAGES)
        throw DiskException("readPage: page_id " + to_string(page_id) + " fuera de rango");

    off_t offset = static_cast<off_t>(page_id) * PAGE_SIZE;
    if (lseek(fd_, offset, SEEK_SET) == -1)
        throw DiskException("readPage: lseek falló en página " + to_string(page_id));

    ssize_t bytes = read(fd_, dest, PAGE_SIZE);
    if (bytes < 0)
        throw DiskException("readPage: error al leer página " + to_string(page_id));

    // Si la página aún no existe en disco (archivo nuevo o truncado), rellena con ceros para garantizar un estado inicial limpio
    if (bytes < static_cast<ssize_t>(PAGE_SIZE))
        memset(dest + bytes, 0, PAGE_SIZE - bytes);
}

void DiskManager::sync() {
    // fsync() garantiza que los datos lleguen al hardware físicamente
    if (fsync(fd_) == -1)
        cerr << "[DiskManager] Advertencia: fsync fallo\n";
}