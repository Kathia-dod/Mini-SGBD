#pragma once
#include "Tuple.hpp"
#include <chrono>
#include <string>
#include <ostream>

class Operator {
protected:
    long tuplesProduced_ = 0;
    long pagesRead_ = 0; // solo ScanOperator lo incrementa realmente
    std::chrono::steady_clock::time_point startTime_, endTime_;

public:
    virtual void open() { startTime_ = std::chrono::steady_clock::now(); }
    virtual bool next(Tuple& out) = 0;
    virtual void close() { endTime_ = std::chrono::steady_clock::now(); }
    virtual ~Operator() = default;

    virtual std::string name() const = 0;
    virtual void explain(std::ostream& out, int depth = 0) const = 0;

    long tuplesProduced() const { return tuplesProduced_; }
    long pagesRead() const { return pagesRead_; }
    double elapsedMs() const {
        return std::chrono::duration<double, std::milli>(endTime_ - startTime_).count();
    }
};