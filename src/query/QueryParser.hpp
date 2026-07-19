#ifndef QUERY_PARSER_HPP
#define QUERY_PARSER_HPP

#include "QueryStatement.hpp"
#include <string>
#include <vector>
#include <stdexcept>

class QueryParser {
public:
    QueryParser() = default;

    /**
     * @brief Analiza una consulta SQL minimalista y genera su representacion estructural.
     * @param sql Cadena de texto con el query (ej: "SELECT id, nombre FROM datos WHERE id > 10 ORDER BY id DESC")
     * @return QueryStatement Estructura con la metadata de la consulta
     */
    QueryStatement parse(const std::string& sql);

private:
    std::vector<std::string> tokenize(const std::string& sql);
    std::string to_upper(const std::string& str);
    void parse_select(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt);
    void parse_from(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt);
    void parse_where(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt);
    void parse_order_by(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt);
};

#endif