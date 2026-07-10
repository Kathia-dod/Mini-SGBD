#pragma once
#include "Operator.hpp"
#include <functional>

class SelectOperator : public Operator {
    Operator* child_;
    std::function<bool(const Tuple&)> predicate_;

public:
    SelectOperator(Operator* child, std::function<bool(const Tuple&)> pred)
        : child_(child), predicate_(std::move(pred)) {}
    ~SelectOperator() override { delete child_; }

    void open() override {
        Operator::open();
        child_->open();
    }

    bool next(Tuple& out) override {
        Tuple t;
        while (child_->next(t)) {
            if (predicate_(t)) {
                out = t;
                tuplesProduced_++;
                return true;
            }
        }
        return false;
    }

    void close() override {
        child_->close();
        Operator::close();
    }

    std::string name() const override { return "Select"; }

    void explain(std::ostream& out, int depth) const override {
        out << std::string(depth * 2, ' ') << "Select -> tuplas: " << tuplesProduced_
            << ", tiempo: " << elapsedMs() << "ms\n";
        child_->explain(out, depth + 1);
    }
};