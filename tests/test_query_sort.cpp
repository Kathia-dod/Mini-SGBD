#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <memory>
#include "../src/query/Operator.hpp"
#include "../src/query/Tuple.hpp"
#include "../src/query/SortOperator.hpp"

Tuple make_tuple(const std::vector<std::string>& vals) {
    Tuple t;
    t.values = vals;
    return t;
}

class MockScanOperator : public Operator {
private:
    std::vector<Tuple> tuples_;
    size_t current_index_;

public:
    MockScanOperator(const std::vector<Tuple>& tuples) 
        : tuples_(tuples), current_index_(0) {}

    void open() override {
        current_index_ = 0;
    }

    bool next(Tuple& tuple) override {
        if (current_index_ < tuples_.size()) {
            tuple = tuples_[current_index_++];
            return true;
        }
        return false;
    }

    void close() override {
        current_index_ = 0;
    }

    std::string name() const override {
        return "MockScanOperator";
    }

    void explain(std::ostream& out, int depth = 0) const override {
        std::string indent(depth * 2, ' ');
        out << indent << "MockScanOperator(rows=" << tuples_.size() << ")\n";
    }
};

void print_test_header(const std::string& test_name) {
    std::cout << "[TEST] " << test_name << " ... ";
}

void print_success() {
    std::cout << "OK!" << std::endl;
}

// Caso 1: Ordenamiento Numerico Ascendente (ASC) por la columna 0 (id)
void test_sort_numeric_asc() {
    print_test_header("Ordenamiento Numerico ASC (columna id=0)");

    std::vector<Tuple> datos_input = {
        make_tuple({"100", "Carlos", "2500"}),
        make_tuple({"5", "Ana", "3000"}),
        make_tuple({"20", "Beatriz", "1800"}),
        make_tuple({"1", "David", "4000"})
    };

    auto mock_scan = std::make_unique<MockScanOperator>(datos_input);
    SortOperator sort_op(std::move(mock_scan), 0, true); 

    sort_op.open();

    Tuple t;
    assert(sort_op.next(t) && t.values[0] == "1");
    assert(sort_op.next(t) && t.values[0] == "5");
    assert(sort_op.next(t) && t.values[0] == "20");
    assert(sort_op.next(t) && t.values[0] == "100");
    assert(!sort_op.next(t)); 

    sort_op.close();
    print_success();
}

// Caso 2: Ordenamiento Lexicografico Descendente (DESC) por la columna 1 (nombre)
void test_sort_string_desc() {
    print_test_header("Ordenamiento Lexicografico DESC (columna nombre=1)");

    std::vector<Tuple> datos_input = {
        make_tuple({"1", "Ana"}),
        make_tuple({"2", "Zebra"}),
        make_tuple({"3", "Carlos"}),
        make_tuple({"4", "Beto"})
    };

    auto mock_scan = std::make_unique<MockScanOperator>(datos_input);
    SortOperator sort_op(std::move(mock_scan), 1, false);

    sort_op.open();

    Tuple t;
    assert(sort_op.next(t) && t.values[1] == "Zebra");
    assert(sort_op.next(t) && t.values[1] == "Carlos");
    assert(sort_op.next(t) && t.values[1] == "Beto");
    assert(sort_op.next(t) && t.values[1] == "Ana");
    assert(!sort_op.next(t));

    sort_op.close();
    print_success();
}

// Caso 3: Manejo de tabla vacia (Zero Tuples)
void test_sort_empty_table() {
    print_test_header("Manejo de tabla vacia");

    std::vector<Tuple> datos_input = {};

    auto mock_scan = std::make_unique<MockScanOperator>(datos_input);
    SortOperator sort_op(std::move(mock_scan), 0, true);

    sort_op.open();
    Tuple t;
    assert(!sort_op.next(t)); // Debe retornar false inmediatamente sin arrojar errores
    sort_op.close();

    print_success();
}

// Caso 4: Manejo de Errores - Columna fuera de limites
void test_sort_invalid_column() {
    print_test_header("Excepcion al ordenar por indice fuera de limites");

    std::vector<Tuple> datos_input = {
        make_tuple({"1", "Alice"})
    };

    auto mock_scan = std::make_unique<MockScanOperator>(datos_input);
    SortOperator sort_op(std::move(mock_scan), 99, true); // Indice 99 no existe

    try {
        sort_op.open();
        assert(false); // No deberia llegar aqui
    } catch (const std::runtime_error& e) {
        // Excepcion capturada exitosamente
    }

    print_success();
}

int main() {
    std::cout << "   EJECUTANDO TESTS DE SORT OPERATOR      " << std::endl;

    try {
        test_sort_numeric_asc();
        test_sort_string_desc();
        test_sort_empty_table();
        test_sort_invalid_column();

        std::cout << "==========================================" << std::endl;
        std::cout << " TODOS LOS TESTS PASARON EXITOSAMENTE!    " << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n[FALLO UN TEST]: Excepcion inesperada: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}