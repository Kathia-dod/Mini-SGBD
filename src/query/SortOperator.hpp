#ifndef SORT_OPERATOR_HPP
#define SORT_OPERATOR_HPP

#include "Operator.hpp"
#include "Tuple.hpp"
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <stdexcept>

class SortOperator : public Operator {
private:
    std::unique_ptr<Operator> child_;
    std::string sort_column_;
    bool is_asc_;
    
    std::vector<Tuple> sorted_tuples_;
    size_t current_index_;
    int sort_col_idx_;

    bool is_numeric(const std::string& str) const {
        if (str.empty()) return false;
        char* end = nullptr;
        std::strtod(str.c_str(), &end);
        return end != str.c_str() && *end == '\0';
    }

public:
    SortOperator(std::unique_ptr<Operator> child, const std::string& sort_column, bool is_asc = true)
        : child_(std::move(child)), sort_column_(sort_column), is_asc_(is_asc), current_index_(0), sort_col_idx_(-1) {}

    void open() override {
        child_->open();
        sorted_tuples_.clear();
        current_index_ = 0;
        sort_col_idx_ = -1;

        Tuple tuple;
        while (child_->next(tuple)) {
            sorted_tuples_.push_back(tuple);
        }

        if (sorted_tuples_.empty()) {
            return;
        }

        const auto& schema = sorted_tuples_[0].get_schema();
        for (size_t i = 0; i < schema.size(); ++i) {
            if (schema[i] == sort_column_) {
                sort_col_idx_ = static_cast<int>(i);
                break;
            }
        }

        if (sort_col_idx_ == -1) {
            throw std::runtime_error("SortOperator Error: Columna de ordenamiento '" + sort_column_ + "' no encontrada en el esquema.");
        }

        std::stable_sort(sorted_tuples_.begin(), sorted_tuples_.end(), [this](const Tuple& a, const Tuple& b) {
            const std::string& val_a = a.get_value(sort_col_idx_);
            const std::string& val_b = b.get_value(sort_col_idx_);

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
};

#endif