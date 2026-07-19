#ifndef QUERY_STATEMENT_HPP
#define QUERY_STATEMENT_HPP

#include <string>
#include <vector>

struct WhereClause {
    bool has_where = false;
    std::string column;
    std::string op; // "=", "<", ">", "<=", ">=", "!="
    std::string value;
};

struct OrderByClause {
    bool has_order_by = false;
    std::string column;
    bool is_asc = true; // true para ASC (por defecto), false para DESC
};

struct QueryStatement {
    std::vector<std::string> select_columns; // Si contiene "*", proyecta todas
    std::string from_table;
    WhereClause where_clause;
    OrderByClause order_by;

    bool is_select_all() const {
        return select_columns.size() == 1 && select_columns[0] == "*";
    }
};

#endif