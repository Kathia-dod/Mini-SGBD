#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/query/CsvLoader.hpp"
#include "../src/query/ScanOperator.hpp"
#include "../src/query/JoinOperator.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>

int main() {

    std::remove("join_personas.bin");
    std::remove("join_ciudades.bin");

    // RELACION PERSONAS

    StorageManager smPersonas("join_personas.bin");
    BufferManager bmPersonas(5, smPersonas);

    int personas = CsvLoader::load(
        "data/datos1.csv",
        bmPersonas
    );

    assert(personas > 0);

    // RELACION CIUDADES

    StorageManager smCiudades("join_ciudades.bin");
    BufferManager bmCiudades(5, smCiudades);

    int ciudades = CsvLoader::load(
        "data/ciudades.csv",
        bmCiudades
    );

    assert(ciudades > 0);


    // NESTED LOOP JOIN

    Operator* join = new JoinOperator(

        new ScanOperator(
            bmPersonas,
            smPersonas.getNumPages()
        ),

        new ScanOperator(
            bmCiudades,
            smCiudades.getNumPages()
        ),

        [](const Tuple& persona, const Tuple& ciudad) {

            // personas:
            // nombre | edad | ciudad
            // ciudades:
            // ciudad | pais

            return persona.values[2] == ciudad.values[0];
        }
    );

    join->open();

    Tuple result;
    int rows = 0;

    while (join->next(result)) {

        for (const auto& value : result.values)
            std::cout << value << " ";

        std::cout << "\n";

        rows++;
    }

    join->close();

    std::cout << "\n--- explain() ---\n";
    join->explain(std::cout);

    std::cout << "\nTotal joins encontrados: "
              << rows << "\n";

    assert(rows > 0);

    delete join;

    std::cout << "\n[test_query_join] OK\n";

    return 0;
}