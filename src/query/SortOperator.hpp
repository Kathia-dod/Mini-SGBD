#pragma once

#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <memory>

#include "Operator.hpp"
#include "Tuple.hpp"

// SortOperator: implementa un External Merge Sort de dos fases.
//  Fase 1 (open): consume al hijo por bloques de tamano acotado (run_capacity_),
//                 ordena cada bloque en memoria y lo escribe como un "run" en disco.
//  Fase 2 (next): fusiona los runs mediante un merge k-way, manteniendo en memoria
//                 solo UNA tupla por run (no la relacion completa).
class SortOperator : public Operator {
private:
    Operator* child_;
    size_t sort_col_idx_;
    bool is_asc_;
    size_t run_capacity_; // maximo de tuplas por run (cota de memoria de la Fase 1)

    std::vector<std::string> run_files_;
    std::vector<std::unique_ptr<std::ifstream>> run_streams_;
    std::vector<Tuple> current_head_;   // tupla actualmente cargada de cada run
    std::vector<bool> run_active_;      // si ese run todavia tiene datos

    bool is_numeric(const std::string& str) const {
        if (str.empty()) return false;
        char* end = nullptr;
        std::strtod(str.c_str(), &end);
        return end != str.c_str() && *end == '\0';
    }

    bool lessThan(const Tuple& a, const Tuple& b) const {
        const std::string& va = a.values[sort_col_idx_];
        const std::string& vb = b.values[sort_col_idx_];
        if (is_numeric(va) && is_numeric(vb))
            return std::stod(va) < std::stod(vb);
        return va < vb;
    }

    static void writeTuple(std::ostream& out, const Tuple& t) {
        std::string s = t.serialize();
        uint32_t len = static_cast<uint32_t>(s.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(s.data(), len);
    }

    static bool readTuple(std::istream& in, Tuple& t) {
        uint32_t len = 0;
        if (!in.read(reinterpret_cast<char*>(&len), sizeof(len))) return false;
        std::string buf(len, '\0');
        if (len > 0 && !in.read(&buf[0], len)) return false;
        t = Tuple::deserialize(buf.data(), static_cast<uint16_t>(len));
        return true;
    }

    // Fase 1: ordena un bloque en memoria y lo persiste como un nuevo run en disco.
    void flushRun(std::vector<Tuple>& buffer) {
        std::stable_sort(buffer.begin(), buffer.end(),
            [this](const Tuple& a, const Tuple& b) {
                return is_asc_ ? lessThan(a, b) : lessThan(b, a);   // <-- antes solo lessThan(a,b)
            });

        std::string filename = "sortop_run_" +
            std::to_string(reinterpret_cast<uintptr_t>(this)) + "_" +
            std::to_string(run_files_.size()) + ".tmp";

        std::ofstream out(filename, std::ios::binary);
        for (auto& tup : buffer) writeTuple(out, tup);
        run_files_.push_back(filename);
    }

    void cleanupRuns() {
        run_streams_.clear();
        for (auto& f : run_files_) std::remove(f.c_str());
        run_files_.clear();
        current_head_.clear();
        run_active_.clear();
    }

public:
    SortOperator(Operator* child, size_t sort_col_idx, bool is_asc = true,
                 size_t run_capacity = 1024)
        : child_(child), sort_col_idx_(sort_col_idx), is_asc_(is_asc),
          run_capacity_(run_capacity) {}

    ~SortOperator() override { cleanupRuns(); }

    void open() override {
        Operator::open();
        if (!child_) return;
        cleanupRuns();
        child_->open();

        std::vector<Tuple> buffer;
        buffer.reserve(run_capacity_);
        bool first = true;
        Tuple t;

        // --- Fase 1: generacion de runs ordenados en disco ---
        while (child_->next(t)) {
            if (first) {
                if (sort_col_idx_ >= t.values.size()) {
                    child_->close();
                    throw std::runtime_error(
                        "SortOperator Error: indice de columna fuera de limites.");
                }
                first = false;
            }
            buffer.push_back(t);
            if (buffer.size() >= run_capacity_) {
                flushRun(buffer);
                buffer.clear();
            }
        }
        if (!buffer.empty()) flushRun(buffer);
        child_->close();

        // --- Prepara la Fase 2: abre un stream por run y carga su primera tupla ---
        run_streams_.resize(run_files_.size());
        current_head_.resize(run_files_.size());
        run_active_.resize(run_files_.size());
        for (size_t i = 0; i < run_files_.size(); i++) {
            run_streams_[i] = std::make_unique<std::ifstream>(run_files_[i], std::ios::binary);
            run_active_[i] = readTuple(*run_streams_[i], current_head_[i]);
        }
    }

    // Fase 2: merge k-way. En memoria solo vive una tupla por run.
    bool next(Tuple& tuple) override {
        int best = -1;
        for (size_t i = 0; i < run_active_.size(); i++) {
            if (!run_active_[i]) continue;
            if (best == -1) { best = static_cast<int>(i); continue; }
            bool challengerWins = is_asc_
                ? lessThan(current_head_[i], current_head_[best])
                : lessThan(current_head_[best], current_head_[i]);
            if (challengerWins) best = static_cast<int>(i);
        }
        if (best == -1) return false;

        tuple = current_head_[best];
        run_active_[best] = readTuple(*run_streams_[best], current_head_[best]);
        tuplesProduced_++;
        return true;
    }

    void close() override {
        Operator::close();
        cleanupRuns();
    }

    std::string name() const override { return "SortOperator"; }

    void explain(std::ostream& out, int depth = 0) const override {
        std::string indent(depth * 2, ' ');
        out << indent << "SortOperator(col_idx=" << sort_col_idx_
            << ", order=" << (is_asc_ ? "ASC" : "DESC")
            << ", runs=" << run_files_.size() << ")\n";
        if (child_) child_->explain(out, depth + 1);
    }
};
