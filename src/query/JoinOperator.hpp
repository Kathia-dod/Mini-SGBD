#ifndef JOIN_OPERATOR_HPP
#define JOIN_OPERATOR_HPP

#include "Operator.hpp"
#include <functional>

class JoinOperator : public Operator {
private:
    Operator* left_;
    Operator* right_;

    std::function<bool(const Tuple&, const Tuple&)> condition_;

    Tuple leftTuple_;
    Tuple rightTuple_;

    bool leftValid_;
    bool rightValid_;

public:
    JoinOperator(
        Operator* left,
        Operator* right,
        std::function<bool(const Tuple&, const Tuple&)> condition
    );

    ~JoinOperator();

    void open() override;

    bool next(Tuple& tuple) override;

    void close() override;

    std::string name() const override {
    return "Join";
    }

    void explain(std::ostream& os, int depth = 0) const override;
};

#endif