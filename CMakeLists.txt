cmake_minimum_required(VERSION 3.15)
project(MiniSGBD CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Flags útiles: warnings estrictos + símbolos de debug para gdb/valgrind
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

set(STORAGE_SOURCES
    src/storage/DiskManager.cpp
    src/storage/StorageManager.cpp
)

set(BUFFER_SOURCES
    src/buffer/BufferManager.cpp
)

set(INDEX_SOURCES
    src/index/BTreeIndex.cpp
)

set(QUERY_SOURCES
    src/query/JoinOperator.cpp
)

set(QUERY_PARSER_SOURCES
    src/query/QueryParser.cpp
)

add_executable(mini_sgbd
    src/main.cpp
    ${STORAGE_SOURCES}
    ${BUFFER_SOURCES}
)

add_executable(tests
    tests/test_main.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_disk
    tests/test_disk.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_1
    tests/test_disk_multipage.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_2
    tests/test_storage_persist.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_3
    tests/test_slot_directory.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_4
    tests/test_integrity.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_5
    tests/test_end2end.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_buffer_lru
    tests/test_buffer_lru.cpp
    ${STORAGE_SOURCES}
    ${BUFFER_SOURCES}
)

add_executable(test_btreeindex
    tests/test_btreeindex.cpp
    ${STORAGE_SOURCES}
    ${BUFFER_SOURCES}
    ${INDEX_SOURCES}
)

add_executable(test_btree_delete
    tests/test_btree_delete.cpp
    ${STORAGE_SOURCES}
    ${BUFFER_SOURCES}
    ${INDEX_SOURCES}
)

add_executable(test_persistencia
    tests/test_persistencia.cpp
    ${STORAGE_SOURCES}
    ${BUFFER_SOURCES}
    ${INDEX_SOURCES}
)

add_executable(test_query_scan
    tests/test_query_scan.cpp
    ${STORAGE_SOURCES}
    ${BUFFER_SOURCES}
    ${QUERY_SOURCES}
)

add_executable(test_csv_load
    tests/test_csv_load.cpp
    ${STORAGE_SOURCES}
    ${BUFFER_SOURCES}
    ${INDEX_SOURCES}
)

add_executable(test_query_index
    tests/test_query_index.cpp
    ${STORAGE_SOURCES}
    ${BUFFER_SOURCES}
    ${INDEX_SOURCES}
)

add_executable(test_index_scan
    tests/test_index_scan.cpp
    ${STORAGE_SOURCES}
    ${BUFFER_SOURCES}
    ${INDEX_SOURCES}
)

add_executable(test_query_join
    tests/test_query_join.cpp
    ${STORAGE_SOURCES}
    ${BUFFER_SOURCES}
    ${INDEX_SOURCES}
    ${QUERY_SOURCES}
)

add_executable(test_query_parser
    tests/test_query_parser.cpp
    ${QUERY_PARSER_SOURCES}
)