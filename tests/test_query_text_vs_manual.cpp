#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <memory>
#include "../src/query/Operator.hpp"
#include "../src/query/Tuple.hpp"
#include "../src/query/SortOperator.hpp"
#include "../src/query/QueryParser.hpp"

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

    void open() override { current_index_ = 0; }
    
    bool next(Tuple& tuple) override {
        if (current_index_ < tuples_.size()) {
            tuple = tuples_[current_index_++];
            return true;
        }
        return false;
    }
    
    void close() override { current_index_ = 0; }
    std::string name() const override { return "MockScanOperator"; }
    
    void explain(std::ostream& out, int depth = 0) const override {
        std::string indent(depth * 2, ' ');
        out << indent << "MockScanOperator(rows=" << tuples_.size() << ")\n";
    }
};

std::vector<Tuple> execute_plan(Operator* root) {
    std::vector<Tuple> result;
    if (!root) return result;

    root->open();
    Tuple t;
    while (root->next(t)) {
        result.push_back(t);
    }
    root->close();
    return result;
}

void print_results(const std::string& title, const std::vector<Tuple>& rows) {
    std::cout << "   --- " << title << " (" << rows.size() << " filas) ---" << std::endl;
    for (size_t i = 0; i < rows.size(); ++i) {
        std::cout << "   Row " << i << ": [ ";
        for (const auto& val : rows[i].values) {
            std::cout << val << " ";
        }
        std::cout << "]" << std::endl;
    }
}

void test_equivalence_order_by_asc() {
    std::cout << "\n[TEST 1] Comparando SQL Texto vs. Plan Manual (ORDER BY id ASC)..." << std::endl;

    std::vector<Tuple> tabla_empleados = {
        make_tuple({"10", "Zoe", "3500"}),
        make_tuple({"2", "Carlos", "2800"}),
        make_tuple({"50", "Ana", "4200"}),
        make_tuple({"1", "David", "1900"})
    };

    // RUTA A: Plan armado por el QueryParser desde texto SQL
    std::string sql = "SELECT * FROM empleados ORDER BY id ASC;";
    QueryParser parser;
    QueryStatement stmt = parser.parse(sql);

    auto scan_parser = std::make_unique<MockScanOperator>(tabla_empleados);
    Operator* plan_parser = parser.build_plan(stmt, scan_parser.get());

    std::vector<Tuple> resultados_parser = execute_plan(plan_parser);

    // RUTA B: Plan armado manualmente en C++
    auto scan_manual = std::make_unique<MockScanOperator>(tabla_empleados);
    SortOperator plan_manual(scan_manual.get(), 0, true); // 0 = id, true = ASC

    std::vector<Tuple> resultados_manual = execute_plan(&plan_manual);

    // VERIFICACION
    print_results("Resultados desde Parser SQL", resultados_parser);
    print_results("Resultados desde Plan Manual", resultados_manual);

    assert(resultados_parser.size() == resultados_manual.size());
    assert(resultados_parser.size() == 4);

    for (size_t i = 0; i < resultados_parser.size(); ++i) {
        assert(resultados_parser[i].values == resultados_manual[i].values);
    }

    assert(resultados_parser[0].values[0] == "1");
    assert(resultados_parser[1].values[0] == "2");
    assert(resultados_parser[2].values[0] == "10");
    assert(resultados_parser[3].values[0] == "50");

    std::cout << ">> [EXITO] Identidad verificada al 100% para ORDER BY ASC!" << std::endl;
    delete plan_parser;
}

void test_equivalence_order_by_desc() {
    std::cout << "\n[TEST 2] Comparando SQL Texto vs. Plan Manual (ORDER BY salario DESC)..." << std::endl;

    std::vector<Tuple> tabla_empleados = {
        make_tuple({"1", "Elena", "1500"}),
        make_tuple({"2", "Mateo", "4500"}),
        make_tuple({"3", "Lucia", "3200"})
    };

    std::string sql = "SELECT * FROM empleados ORDER BY salario DESC;";
    QueryParser parser;
    QueryStatement stmt = parser.parse(sql);
    
    auto scan_parser = std::make_unique<MockScanOperator>(tabla_empleados);
    Operator* plan_parser = parser.build_plan(stmt, scan_parser.get());

    std::vector<Tuple> res_parser = execute_plan(plan_parser);

    auto scan_manual = std::make_unique<MockScanOperator>(tabla_empleados);
    SortOperator plan_manual(scan_manual.get(), 2, false);

    std::vector<Tuple> res_manual = execute_plan(&plan_manual);

    assert(res_parser.size() == res_manual.size());
    for (size_t i = 0; i < res_parser.size(); ++i) {
        assert(res_parser[i].values == res_manual[i].values);
    }

    assert(res_parser[0].values[2] == "4500");
    assert(res_parser[1].values[2] == "3200");
    assert(res_parser[2].values[2] == "1500");

    std::cout << ">> [EXITO] Identidad verificada al 100% para ORDER BY DESC!" << std::endl;
    delete plan_parser;
}

int main() {
    std::cout << "  TEST DEDICADO: QUERIES EN TEXTO vs. PLAN ARMADO A MANO  " << std::endl;

    try {
        test_equivalence_order_by_asc();
        test_equivalence_order_by_desc();

        std::cout << "\n==========================================================" << std::endl;
        std::cout << " TODAS LAS PRUEBAS DE EQUIVALENCIA PASARON PERFECTAMENTE" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n[FALLO EN TEST]: Excepcion inesperada: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}