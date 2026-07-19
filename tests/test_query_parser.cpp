#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include "../src/query/QueryParser.hpp"

void print_test_header(const std::string& test_name) {
    std::cout << "[TEST] " << test_name << " ... ";
}

void print_success() {
    std::cout << "OK!" << std::endl;
}

// Caso 1: Consulta simple con SELECT *
void test_select_all() {
    print_test_header("SELECT * simple");
    QueryParser parser;
    QueryStatement stmt = parser.parse("SELECT * FROM usuarios;");

    assert(stmt.is_select_all() == true);
    assert(stmt.select_columns.size() == 1);
    assert(stmt.select_columns[0] == "*");
    assert(stmt.from_table == "usuarios");
    assert(stmt.where_clause.has_where == false);
    assert(stmt.order_by.has_order_by == false);
    print_success();
}

// Caso 2: Proyeccion de multiples columnas con comas y espacios
void test_select_columns() {
    print_test_header("SELECT columnas especificas");
    QueryParser parser;
    QueryStatement stmt = parser.parse("SELECT id, nombre, edad FROM empleados");

    assert(stmt.is_select_all() == false);
    assert(stmt.select_columns.size() == 3);
    assert(stmt.select_columns[0] == "id");
    assert(stmt.select_columns[1] == "nombre");
    assert(stmt.select_columns[2] == "edad");
    assert(stmt.from_table == "empleados");
    print_success();
}

// Caso 3: Filtrado con clausula WHERE
void test_where_clause() {
    print_test_header("SELECT con WHERE");
    QueryParser parser;
    QueryStatement stmt = parser.parse("SELECT nombre FROM productos WHERE precio >= 100;");

    assert(stmt.from_table == "productos");
    assert(stmt.where_clause.has_where == true);
    assert(stmt.where_clause.column == "precio");
    assert(stmt.where_clause.op == ">=");
    assert(stmt.where_clause.value == "100");
    assert(stmt.order_by.has_order_by == false);
    print_success();
}

// Caso 4: Ordenamiento con ORDER BY (ASC y DESC)
void test_order_by_clause() {
    print_test_header("SELECT con ORDER BY DESC");
    QueryParser parser;
    QueryStatement stmt = parser.parse("SELECT id, nombre FROM clientes ORDER BY apellido DESC");

    assert(stmt.order_by.has_order_by == true);
    assert(stmt.order_by.column == "apellido");
    assert(stmt.order_by.is_asc == false);
    print_success();
}

// Caso 5: Consulta completa combinando todas las clausulas
void test_full_query() {
    print_test_header("Consulta completa (SELECT + FROM + WHERE + ORDER BY)");
    QueryParser parser;
    QueryStatement stmt = parser.parse("select id, salario from nomina where salario > 2000 order by id asc;");

    assert(stmt.select_columns.size() == 2);
    assert(stmt.from_table == "nomina");
    assert(stmt.where_clause.has_where == true);
    assert(stmt.where_clause.column == "salario");
    assert(stmt.where_clause.op == ">");
    assert(stmt.where_clause.value == "2000");
    assert(stmt.order_by.has_order_by == true);
    assert(stmt.order_by.column == "id");
    assert(stmt.order_by.is_asc == true);
    print_success();
}

// Caso 6: Manejo de errores de sintaxis
void test_syntax_errors() {
    print_test_header("Manejo de errores de sintaxis (Excepciones)");
    QueryParser parser;

    // Error 1: Consulta vacia
    try {
        parser.parse("");
        assert(false); // No deberia llegar aqui
    } catch (const std::invalid_argument&) {}

    // Error 2: Falta FROM
    try {
        parser.parse("SELECT id, nombre tabla");
        assert(false);
    } catch (const std::runtime_error&) {}

    // Error 3: WHERE incompleto
    try {
        parser.parse("SELECT * FROM tabla WHERE id >");
        assert(false);
    } catch (const std::runtime_error&) {}

    print_success();
}

int main() {
    std::cout << "   EJECUTANDO TESTS DE QUERY PARSER       " << std::endl;

    try {
        test_select_all();
        test_select_columns();
        test_where_clause();
        test_order_by_clause();
        test_full_query();
        test_syntax_errors();

        std::cout << "==========================================" << std::endl;
        std::cout << " TODOS LOS TESTS PASARON EXITOSAMENTE!    " << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "\n[FALLO UN TEST]: Excepcion inesperada: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}