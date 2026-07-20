#ifndef QUERY_PARSER_HPP
#define QUERY_PARSER_HPP

#include "QueryStatement.hpp"
#include "Operator.hpp"
#include "ScanOperator.hpp"
#include "SelectOperator.hpp"
#include "SortOperator.hpp"
#include "ProjectOperator.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

class QueryParser {
public:
    QueryParser() = default;

    /**
     * @brief Construye el árbol físico de operadores (Execution Plan) a partir de un QueryStatement.
     * @param stmt Estructura con la metadata de la consulta ya parseada.
     * @param scan_op Puntero único al ScanOperator raíz de la tabla (injected dependency).
     * @return std::unique_ptr<Operator> Raíz del árbol de operadores listos para ejecutar.
     */
    QueryStatement parse(const std::string& sql);
    Operator* build_plan(const QueryStatement& stmt, Operator* scan_op);

private:
    std::vector<std::string> tokenize(const std::string& sql);
    std::string to_upper(const std::string& str);
    void parse_select(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt);
    void parse_from(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt);
    void parse_where(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt);
    void parse_order_by(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt);
};

#endif