#include "QueryParser.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

std::string QueryParser::to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::vector<std::string> QueryParser::tokenize(const std::string& sql) {
    std::vector<std::string> tokens;
    std::string current_token;
    
    for (char c : sql) {
        if (std::isspace(c)) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        } else if (c == ',' || c == ';' || c == '(' || c == ')') {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        } else {
            current_token += c;
        }
    }
    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }
    return tokens;
}

std::unique_ptr<Operator> QueryParser::build_plan(const QueryStatement& stmt, std::unique_ptr<Operator> scan_op) {
    if (!scan_op) {
        throw std::invalid_argument("El operador Scan base no puede ser nulo.");
    }

    std::unique_ptr<Operator> current_root = std::move(scan_op);

    // SelectOperator (WHERE)
    if (stmt.where_clause.has_where) {
        current_root = std::make_unique<SelectOperator>(
            std::move(current_root),
            stmt.where_clause.column,
            stmt.where_clause.op,
            stmt.where_clause.value
        );
    }

    // SortOperator (ORDER BY)
    if (stmt.order_by.has_order_by) {
        size_t col_idx = 0; 
        if (stmt.order_by.column == "nombre" || stmt.order_by.column == "1") col_idx = 1;
        else if (stmt.order_by.column == "salario" || stmt.order_by.column == "2") col_idx = 2;

        current_root = std::make_unique<SortOperator>(
            std::move(current_root),
            col_idx,
            stmt.order_by.is_asc
        );
    }

    // ProjectOperator (SELECT) 
    if (!stmt.is_select_all()) {
        current_root = std::make_unique<ProjectOperator>(
            std::move(current_root),
            stmt.select_columns
        );
    }

    return current_root;
}

QueryStatement QueryParser::parse(const std::string& sql) {
    std::vector<std::string> tokens = tokenize(sql);
    if (tokens.empty()) {
        throw std::invalid_argument("Error de sintaxis: Consulta vacia.");
    }

    QueryStatement stmt;
    size_t index = 0;

    parse_select(tokens, index, stmt);
    parse_from(tokens, index, stmt);

    if (index < tokens.size() && to_upper(tokens[index]) == "WHERE") {
        parse_where(tokens, index, stmt);
    }

    if (index < tokens.size() && to_upper(tokens[index]) == "ORDER") {
        parse_order_by(tokens, index, stmt);
    }

    if (index < tokens.size()) {
        throw std::runtime_error("Error de sintaxis: Tokens inesperados al final de la consulta -> " + tokens[index]);
    }

    return stmt;
}

void QueryParser::parse_select(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt) {
    if (index >= tokens.size() || to_upper(tokens[index]) != "SELECT") {
        throw std::runtime_error("Error de sintaxis: La consulta debe iniciar con SELECT.");
    }
    index++;

    while (index < tokens.size() && to_upper(tokens[index]) != "FROM") {
        stmt.select_columns.push_back(tokens[index]);
        index++;
    }

    if (stmt.select_columns.empty()) {
        throw std::runtime_error("Error de sintaxis: Se debe especificar al menos una columna en SELECT.");
    }
}

void QueryParser::parse_from(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt) {
    if (index >= tokens.size() || to_upper(tokens[index]) != "FROM") {
        throw std::runtime_error("Error de sintaxis: Se esperaba la clausula FROM.");
    }
    index++;

    if (index >= tokens.size()) {
        throw std::runtime_error("Error de sintaxis: Se esperaba el nombre de una tabla luego de FROM.");
    }
    
    stmt.from_table = tokens[index];
    index++; 
}

void QueryParser::parse_where(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt) {
    index++;
    stmt.where_clause.has_where = true;

    if (index + 2 >= tokens.size()) {
        throw std::runtime_error("Error de sintaxis: Clausula WHERE incompleta. Formato esperado: <columna> <op> <valor>");
    }

    stmt.where_clause.column = tokens[index++];
    stmt.where_clause.op = tokens[index++];
    stmt.where_clause.value = tokens[index++];
}

void QueryParser::parse_order_by(const std::vector<std::string>& tokens, size_t& index, QueryStatement& stmt) {
    index++;
    
    if (index >= tokens.size() || to_upper(tokens[index]) != "BY") {
        throw std::runtime_error("Error de sintaxis: Se esperaba BY luego de ORDER.");
    }
    index++;

    if (index >= tokens.size()) {
        throw std::runtime_error("Error de sintaxis: Se esperaba el nombre de una columna para ORDER BY.");
    }

    stmt.order_by.has_order_by = true;
    stmt.order_by.column = tokens[index++];
    stmt.order_by.is_asc = true;

    if (index < tokens.size()) {
        std::string mod = to_upper(tokens[index]);
        if (mod == "ASC") {
            stmt.order_by.is_asc = true;
            index++;
        } else if (mod == "DESC") {
            stmt.order_by.is_asc = false;
            index++;
        }
    }
}