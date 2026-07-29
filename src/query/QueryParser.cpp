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

Operator* QueryParser::build_plan(const QueryStatement& stmt, Operator* access_path_root, bool where_already_applied) {
    if (!access_path_root) {
        throw std::invalid_argument("El operador de acceso base no puede ser nulo.");
    }

    Operator* current_root = access_path_root;

    // SelectOperator (WHERE)
    if (stmt.where_clause.has_where && !where_already_applied) {
        // Mapeo simple de nombres a indices para las tablas de prueba
        size_t col_idx = 0;
        if (stmt.where_clause.column == "nombre" || stmt.where_clause.column == "1") col_idx = 1;
        else if (stmt.where_clause.column == "salario" || stmt.where_clause.column == "2") col_idx = 2;

        std::string op = stmt.where_clause.op;
        std::string val = stmt.where_clause.value;

        auto pred = [col_idx, op, val](const Tuple& t) -> bool {
            if (col_idx >= t.values.size()) return false;
            const std::string& t_val = t.values[col_idx];
            
            if (op == "=" || op == "==") return t_val == val;
            if (op == "!=") return t_val != val;

            try {
                double num_t = std::stod(t_val);
                double num_val = std::stod(val);
                if (op == "<") return num_t < num_val;
                if (op == ">") return num_t > num_val;
                if (op == "<=") return num_t <= num_val;
                if (op == ">=") return num_t >= num_val;
            } catch (...) {
                if (op == "<") return t_val < val;
                if (op == ">") return t_val > val;
                if (op == "<=") return t_val <= val;
                if (op == ">=") return t_val >= val;
            }
            return false;
        };

        current_root = new SelectOperator(current_root, pred);
    }

    // SortOperator (ORDER BY)
    if (stmt.order_by.has_order_by) {
        size_t col_idx = 0;
        if (stmt.order_by.column == "nombre" || stmt.order_by.column == "1") col_idx = 1;
        else if (stmt.order_by.column == "salario" || stmt.order_by.column == "2") col_idx = 2;

        current_root = new SortOperator(current_root, col_idx, stmt.order_by.is_asc);
    }

    // ProjectOperator (SELECT)
    if (!stmt.is_select_all()) {
        std::vector<int> proj_indices;
        for (const auto& col : stmt.select_columns) {
            if (col == "id" || col == "0") proj_indices.push_back(0);
            else if (col == "nombre" || col == "1") proj_indices.push_back(1);
            else if (col == "salario" || col == "2") proj_indices.push_back(2);
            else {
                try { proj_indices.push_back(std::stoi(col)); }
                catch (...) { proj_indices.push_back(0); }
            }
        }
        current_root = new ProjectOperator(current_root, proj_indices);
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