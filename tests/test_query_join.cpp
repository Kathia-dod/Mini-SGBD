#include "../src/query/JoinOperator.hpp"
#include <cassert>
#include <iostream>

class MockOperator : public Operator {
private:
    std::vector<Tuple> tuples_;
    size_t index_ = 0;

public:
    MockOperator(std::vector<Tuple> tuples)
        : tuples_(std::move(tuples)) {}

    void open() override {
        Operator::open();
        index_ = 0;
    }

    bool next(Tuple& out) override {
        if (index_ >= tuples_.size())
            return false;

        out = tuples_[index_++];
        tuplesProduced_++;
        return true;
    }

    void close() override {
        Operator::close();
    }

    std::string name() const override {
        return "Mock";
    }

    void explain(std::ostream& out, int depth = 0) const override {
        out << std::string(depth * 2, ' ')
            << "Mock\n";
    }
};

int main() {

    Tuple a1;
    a1.values = {"1", "Juan"};

    Tuple a2;
    a2.values = {"2", "Maria"};

    Tuple b1;
    b1.values = {"1", "Lima"};

    Tuple b2;
    b2.values = {"2", "Arequipa"};

    Operator* join = new JoinOperator(
        new MockOperator({a1, a2}),
        new MockOperator({b1, b2}),
        [](const Tuple& left, const Tuple& right) {
            return left.values[0] == right.values[0];
        }
    );

    join->open();

    Tuple result;
    int rows = 0;

    while (join->next(result)) {
        for (const auto& value : result.values)
            std::cout << value << " ";

        std::cout << "\n";
        rows++;
    }

    join->close();

    assert(rows == 2);

    delete join;

    std::cout << "\n[test_query_join] OK\n";

    return 0;
}