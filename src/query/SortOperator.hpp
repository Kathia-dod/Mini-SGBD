#ifndef SORT_OPERATOR_HPP
#define SORT_OPERATOR_HPP

#include "Operator.hpp"
#include "Tuple.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <cstdlib>

class SortOperator : public Operator {
private:
    std::unique_ptr<Operator> child_;
    size_t sort_col_idx_;
    bool is_asc_;
    
    std::vector<Tuple> sorted_tuples_;
    size_t current_index_;

    bool is_numeric(const std::string& str) const {
        if (str.empty()) return false;
        char* end = nullptr;
        std::strtod(str.c_str(), &end);
        return end != str.c_str() && *end == '\0';
    }

public:
    /**
     * @brief Constructor del operador de ordenamiento en memoria.
     * @param child Operador hijo en el árbol algebraico.
     * @param sort_col_idx Índice numérico de la columna por la cual ordenar (0-indexed).
     * @param is_asc true para ASC, false para DESC.
     */
    SortOperator(std::unique_ptr<Operator> child, size_t sort_col_idx, bool is_asc = true)
        : child_(std::move(child)), sort_col_idx_(sort_col_idx), is_asc_(is_asc), current_index_(0) {}

    void open() override {
        child_->open();
        sorted_tuples_.clear();
        current_index_ = 0;

        Tuple tuple;
        while (child_->next(tuple)) {
            sorted_tuples_.push_back(tuple);
        }

        if (sorted_tuples_.empty()) {
            return;
        }

        if (sort_col_idx_ >= sorted_tuples_[0].values.size()) {
            throw std::runtime_error("SortOperator Error: Indice de columna fuera de los limites de la tupla.");
        }

        std::stable_sort(sorted_tuples_.begin(), sorted_tuples_.end(), [this](const Tuple& a, const Tuple& b) {
            const std::string& val_a = a.values[sort_col_idx_];
            const std::string& val_b = b.values[sort_col_idx_];

            bool less_than = false;
            if (is_numeric(val_a) && is_numeric(val_b)) {
                less_than = std::stod(val_a) < std::stod(val_b);
            } else {
                less_than = val_a < val_b;
            }

            return is_asc_ ? less_than : !less_than && (val_a != val_b);
        });
    }

    bool next(Tuple& tuple) override {
        if (current_index_ < sorted_tuples_.size()) {
            tuple = sorted_tuples_[current_index_++];
            return true;
        }
        return false;
    }

    void close() override {
        sorted_tuples_.clear();
        current_index_ = 0;
        child_->close();
    }

    std::string name() const override {
        return "SortOperator";
    }

    void explain(std::ostream& out, int depth = 0) const override {
        std::string indent(depth * 2, ' ');
        out << indent << "SortOperator(col_idx=" << sort_col_idx_ 
            << ", order=" << (is_asc_ ? "ASC" : "DESC") << ")\n";
        if (child_) {
            child_->explain(out, depth + 1);
        }
    }
};

#endif