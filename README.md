# Mini SGBD — Base de Datos II (2026-A)

## Integrantes

| Nombre |
| ------ | 
| Cuevas Apaza Kathia   | 
| Jara Arisaca Daysi    |
| Tito Vilca Lizeth     | 

---

## Compilación

```bash
mkdir -p build
cd build
cmake ..
make
```

---

## Ejecutar tests

```bash
cd build
./tests
```

---

## Estructura del proyecto

```
src/
 ├── storage/   # DiskManager, Page, StorageManager
 ├── buffer/    # BufferPool, LRU
 ├── index/     # B+ Tree
 └── query/     # Volcano Model, Join, Sort

tests/          # Pruebas unitarias por módulo

```

---

## Descripción

Este proyecto implementa un **Mini Sistema Gestor de Base de Datos (SGBD)** simplificado, incluyendo:

* Gestión de almacenamiento en disco
* Buffer Pool con política LRU
* Índices (B+ Tree)
* Motor de consultas (modelo Volcano)

---

## Notas

* Compilar siempre dentro de `build/`
* No subir la carpeta `build/` al repositorio

