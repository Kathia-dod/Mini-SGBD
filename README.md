# Mini SGBD — Base de Datos II (2026-A)

## Integrantes

| Nombre                     |
| --------------------------- |
| Cuevas Apaza Kathia         |
| Jara Arisaca Daysi          |
| Tito Vilca Lizeth           |
| Cornejo Alvarez Mauricio    |

---

## Compilación

```bash
mkdir -p build
cd build
cmake ..
make
```

---

## Ejecutar todos los tests

Desde `build/`, en el orden recomendado (de menor a mayor dependencia entre módulos):

```bash
cd build

# --- Storage ---
./tests
./test_disk
./test_1
./test_2
./test_3
./test_4
./test_5

# --- Buffer Pool ---
./test_buffer_lru

# --- Indices (B+ Tree) ---
./test_btreeindex
./test_btree_delete
./test_persistencia

# --- Carga de datos y consultas basicas (Volcano Model) ---
./test_csv_load
./test_query_scan
./test_query_index
./test_index_scan
./test_query_join
./test_query_parser
./test_query_sort
./test_query_text_vs_manual
./test_edge_cases

# --- Caracteristica distintiva: Seleccion de ruta de acceso ---
./test_query_optimizer

# --- Benchmarks (evidencia de rendimiento con y sin indice) ---
./test_benchmark_index_scan
./test_benchmark_hitrate

# --- Demo en vivo (sustentacion) ---
./test_live_demo
```
```

También se puede ejecutar el binario principal del sistema:

```bash
./mini_sgbd
```

---

## Estructura del proyecto

```
src/
 ├── storage/   # DiskManager, Page, StorageManager
 ├── buffer/    # BufferPool, LRU (BufferManager, Frame)
 ├── index/     # B+ Tree (BTreeIndex, BNode, BLeafNode, BInternalNode)
 └── query/     # Volcano Model, Join, Sort, Parser y Optimizador
      ├── Operator.hpp            # Interfaz base Volcano (open/next/close)
      ├── ScanOperator.hpp        # Escaneo secuencial de paginas
      ├── SelectOperator.hpp      # Filtro WHERE
      ├── SortOperator.hpp        # ORDER BY
      ├── ProjectOperator.hpp     # Proyeccion de columnas (SELECT)
      ├── JoinOperator.hpp/.cpp   # JOIN
      ├── IndexScanOperator.hpp   # Acceso via B+ Tree
      ├── QueryParser.hpp/.cpp    # Parser SQL basico + build_plan()
      ├── QueryStatement.hpp      # AST de la consulta parseada
      ├── QueryOptimizer.hpp      # Caracteristica distintiva: seleccion de ruta de acceso
      ├── CsvLoader.hpp           # Carga masiva desde CSV
      └── Tuple.hpp               # Registro en memoria (serializacion)

tests/          # Pruebas unitarias y de integracion por modulo
data/           # Datasets de prueba (datos1.csv, ciudades.csv)
```

---

## Descripción

Este proyecto implementa un **Mini Sistema Gestor de Base de Datos (SGBD)** simplificado, incluyendo:

* Gestión de almacenamiento en disco (DiskManager, paginación, slots)
* Buffer Pool con política de reemplazo LRU
* Índices B+ Tree (búsqueda, inserción, borrado, persistencia)
* Motor de consultas basado en el modelo Volcano (Scan, Select, Sort, Project, Join)
* Parser SQL básico (SELECT / WHERE / ORDER BY)

---

## Característica distintiva: Selección de ruta de acceso

El componente `QueryOptimizer` (`src/query/QueryOptimizer.hpp`) decide automáticamente, para cada `WHERE columna = valor`, si construir un `IndexScanOperator` (cuando existe un B+ Tree registrado sobre esa columna) o un `ScanOperator + SelectOperator` (cuando no hay índice o el operador no es de igualdad), sin que el usuario tenga que elegir la ruta a mano.

---

## Dataset de prueba

`data/datos1.csv` contiene 1000 registros sintéticos con las columnas:

```
nombre, edad, ciudad, departamento, salario, fecha_ingreso, activo
```

La columna `edad` (posición 1) es la que se indexa por defecto en `CsvLoader::load()` cuando se le pasa un `BTreeIndex`.

---

## Notas

* Compilar siempre dentro de `build/`
* No subir la carpeta `build/` al repositorio
