#pragma once
#include "QueryStatement.hpp"
#include "Operator.hpp"
#include "ScanOperator.hpp"
#include "IndexScanOperator.hpp"
#include "../buffer/BufferManager.hpp"
#include "../index/BTreeIndex.hpp"
#include <unordered_map>
#include <string>

// Optimizador basado en reglas: para cada WHERE columna = valor, decide entre IndexScan o Scan.
class QueryOptimizer {
public:
    void registerIndex(const std::string& column, BTreeIndex* index) {
        indexes_[column] = index;
    }

    Operator* chooseAccessPath(const QueryStatement& stmt, BufferManager& bm, uint32_t maxPageIdExclusive) {
        usedIndex_ = false;

        if (stmt.where_clause.has_where && stmt.where_clause.op == "=" && indexes_.count(stmt.where_clause.column)) {
            try {
                int key = std::stoi(stmt.where_clause.value);
                usedIndex_ = true;
                return new IndexScanOperator(
                    bm, *indexes_[stmt.where_clause.column], key);
            } catch (...) {
                // valor no numérico: a Scan normal
            }
        }
        return new ScanOperator(bm, maxPageIdExclusive);
    }

    bool lastPathUsedIndex() const { return usedIndex_; }

private:
    std::unordered_map<std::string, BTreeIndex*> indexes_;
    bool usedIndex_ = false;
};