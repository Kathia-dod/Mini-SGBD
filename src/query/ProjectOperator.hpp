#pragma once

#include "Operator.hpp"
#include <vector>

class ProjectOperator : public Operator {
    Operator* child_;
    std::vector<int> columns_;

public:
    ProjectOperator(
        Operator* child,
        std::vector<int> columns
    )
        : child_(child),
          columns_(std::move(columns)) {}

    ~ProjectOperator() override {
        delete child_;
    }

    void open() override {
        Operator::open();
        child_->open();
    }

    bool next(Tuple& out) override {
        Tuple input;

        if (!child_->next(input)) {
            return false;
        }

        out.values.clear();

        for (int index : columns_) {
            if (index >= 0 &&
                index < static_cast<int>(input.values.size())) {
                out.values.push_back(input.values[index]);
            }
        }

        tuplesProduced_++;

        return true;
    }

    void close() override {
        child_->close();
        Operator::close();
    }

    std::string name() const override {
        return "Project";
    }

    void explain(std::ostream& out, int depth) const override {
        out << std::string(depth * 2, ' ')
            << "Project -> tuplas: "
            << tuplesProduced_
            << ", tiempo: "
            << elapsedMs()
            << "ms\n";

        child_->explain(out, depth + 1);
    }
};