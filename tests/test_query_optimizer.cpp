#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/query/CsvLoader.hpp"
#include "../src/query/QueryOptimizer.hpp"
#include "../src/query/SelectOperator.hpp"
#include "../src/query/QueryStatement.hpp"
#include "../src/index/BTreeIndex.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <iomanip>

// nombre | edad | ciudad | departamento | salario | fecha_ingreso | activo
static void imprimirTupla(const Tuple& t) {
    static const char* etiquetas[] = {
        "nombre", "edad", "ciudad", "departamento", "salario", "fecha_ingreso", "activo"
    };
    for (size_t i = 0; i < t.values.size(); i++) {
        std::cout << "      " << std::left << std::setw(15) << etiquetas[i]
                   << ": " << t.values[i] << "\n";
    }
}

int main() {
    std::remove("test_query_optimizer.bin");

    StorageManager sm("test_query_optimizer.bin");
    BufferManager  bm(10, sm);
    BTreeIndex     index(&bm);

    int inserted = CsvLoader::load("data/datos1.csv", bm, &index); 
    assert(inserted > 0);
    uint32_t maxPageId = sm.getNumPages();

    std::cout << "Dataset cargado: " << inserted << " registros, "
               << maxPageId << " paginas de datos\n\n";

    // Caso 1: columna con indice + operador =, se debe elegir IndexScan
    {
        QueryOptimizer opt;
        opt.registerIndex("edad", &index);

        QueryStatement stmt;
        stmt.select_columns = {"*"};
        stmt.from_table = "datos1";
        stmt.where_clause = {true, "edad", "=", "20"};

        Operator* plan = opt.chooseAccessPath(stmt, bm, maxPageId);
        assert(plan->name() == "IndexScan");
        assert(opt.lastPathUsedIndex());

        plan->open();
        Tuple t;
        bool found = plan->next(t);
        plan->close();
        assert(found);

        std::cout << "[Caso 1] columna indexada 'edad' + '=' -> " << plan->name() << " (paginas leidas: " << plan->pagesRead()
                  << ", tiempo: " << plan->elapsedMs() << " ms)\n";
        imprimirTupla(t);
        std::cout << "\n";
        delete plan;
    }

    // Caso 2: columna SIN indice registrado, se debe elegir Scan
    {
        QueryOptimizer opt; 

        QueryStatement stmt;
        stmt.select_columns = {"*"};
        stmt.from_table = "datos1";
        stmt.where_clause = {true, "edad", "=", "20"};

        Operator* plan = opt.chooseAccessPath(stmt, bm, maxPageId);
        assert(plan->name() == "Scan");
        assert(!opt.lastPathUsedIndex());

        std::cout << "[Caso 2] columna 'edad' sin indice registrado : " << plan->name() << "\n\n";
        delete plan;
    }

    // Caso 3: columna CON indice pero operador NO es "=" -> debe elegir Scan
    {
        QueryOptimizer opt;
        opt.registerIndex("edad", &index);

        QueryStatement stmt;
        stmt.select_columns = {"*"};
        stmt.from_table = "datos1";
        stmt.where_clause = {true, "edad", ">", "20"};

        Operator* plan = opt.chooseAccessPath(stmt, bm, maxPageId);
        assert(plan->name() == "Scan");
        assert(!opt.lastPathUsedIndex());

        std::cout << "[Caso 3] indice existe pero operador es '>' : " << plan->name() << "\n\n";
        delete plan;
    }

    // Caso 4: correctud cruzada: ambas rutas devuelven el MISMO resultado
    {
        int edadBuscada = 32;

        QueryStatement stmt;
        stmt.select_columns = {"*"};
        stmt.from_table = "datos1";
        stmt.where_clause = {true, "edad", "=", std::to_string(edadBuscada)};

        QueryOptimizer optConIndice;
        optConIndice.registerIndex("edad", &index);
        Operator* planConIndice = optConIndice.chooseAccessPath(stmt, bm, maxPageId);
        planConIndice->open();
        Tuple resultadoIdx;
        bool encontradoIdx = planConIndice->next(resultadoIdx);
        planConIndice->close();

        QueryOptimizer optSinIndice; 
        Operator* accessSinIndice = optSinIndice.chooseAccessPath(stmt, bm, maxPageId);
        Operator* planSinIndice = new SelectOperator(accessSinIndice, [edadBuscada](const Tuple& t) { 
            return std::stoi(t.values[1]) == edadBuscada; 
        }
        );
        planSinIndice->open();
        Tuple resultadoScan;
        bool encontradoScan = false;
        while (planSinIndice->next(resultadoScan)) encontradoScan = true;
        planSinIndice->close();

        assert(encontradoIdx == encontradoScan);
        assert(resultadoIdx.values == resultadoScan.values);

        std::cout << "[Caso 4] IndexScan y Scan+Select devuelven el mismo resultado (edad = " << edadBuscada << ")\n";
        imprimirTupla(resultadoIdx);
        std::cout << "\n";

        delete planConIndice;
        delete planSinIndice;
    }

    // Caso 5: columna 'departamento' (sin indice, texto): Scan recorre varios resultados
    {
        QueryOptimizer opt; 

        QueryStatement stmt;
        stmt.select_columns = {"*"};
        stmt.from_table = "datos1";
        stmt.where_clause = {true, "departamento", "=", "tecnologia"};

        Operator* access = opt.chooseAccessPath(stmt, bm, maxPageId);
        assert(access->name() == "Scan");

        Operator* plan = new SelectOperator(access, [](const Tuple& t) { return t.values[3] == "tecnologia"; });

        plan->open();
        Tuple t;
        int encontrados = 0;
        while (plan->next(t)) {
            encontrados++;
            if (encontrados <= 4) { 
                std::cout << "  Resultado " << encontrados << ":\n";
                imprimirTupla(t);
            }
        }
        plan->close();

        std::cout << "[Caso 5] departamento = 'tecnologia' (sin indice) -> " << encontrados
                   << " registros encontrados, paginas leidas: " << plan->pagesRead()
                   << ", tiempo: " << plan->elapsedMs() << " ms\n\n";
        delete plan;
    }

    std::cout << "[test_query_optimizer] OK\n";
    return 0;
}