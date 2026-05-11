#pragma once
#include <stdexcept>
#include <string>
using namespace std;

// Excepción propia para errores de I/O en disco.
// Usar esta clase (no runtime_error genérico) permite que el Buffer Manager
class DiskException : public runtime_error {
public:
    explicit DiskException(const string& msg) : runtime_error("[DiskManager] " + msg) {}
};