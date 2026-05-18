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

add_executable(mini_sgbd
    src/main.cpp
    ${STORAGE_SOURCES}
)

add_executable(tests
    tests/test_main.cpp
    ${STORAGE_SOURCES}
)

add_executable(test_disk
    tests/test_disk.cpp
    ${STORAGE_SOURCES}
)

#------------------TESTS-----------------------
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
