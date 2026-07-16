#include "JoinOperator.hpp"
#include <iostream>

JoinOperator::JoinOperator(
    Operator* left,
    Operator* right,
    std::function<bool(const Tuple&, const Tuple&)> condition
)
    : left_(left),
      right_(right),
      condition_(condition),
      leftValid_(false),
      rightValid_(false)
{
}

JoinOperator::~JoinOperator() {
    delete left_;
    delete right_;
}

void JoinOperator::open() {
    left_->open();
    right_->open();

    leftValid_ = left_->next(leftTuple_);
}

bool JoinOperator::next(Tuple& tuple) {

    while (leftValid_) {

        if (!rightValid_) {
            rightValid_ = right_->next(rightTuple_);
        }

        while (rightValid_) {

            if (condition_(leftTuple_, rightTuple_)) {

                tuple.values.clear();

                for (const auto& value : leftTuple_.values) {
                    tuple.values.push_back(value);
                }

                for (const auto& value : rightTuple_.values) {
                    tuple.values.push_back(value);
                }

                rightValid_ = right_->next(rightTuple_);

                return true;
            }

            rightValid_ = right_->next(rightTuple_);
        }

        right_->close();
        right_->open();

        rightValid_ = false;
        leftValid_ = left_->next(leftTuple_);
    }

    return false;
}

void JoinOperator::close() {
    left_->close();
    right_->close();
    Operator::close();
}

void JoinOperator::explain(std::ostream& os, int depth) const {
    os << std::string(depth * 2, ' ')
       << "Nested Loop Join"
       << " -> tuplas: " << tuplesProduced_
       << ", tiempo: " << elapsedMs() << "ms\n";
}