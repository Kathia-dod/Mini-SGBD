#pragma once
#include "Operator.hpp"
#include "../buffer/BufferManager.hpp"

class ScanOperator : public Operator {
    BufferManager& bm_;
    uint32_t maxPageId_;          // exclusivo: hasta donde escanear
    uint32_t currentPageId_ = 1;  // pagina 0 es metapagina, se salta
    uint16_t currentSlot_ = 0;
    Page* currentPage_ = nullptr;

public:
    ScanOperator(BufferManager& bm, uint32_t maxPageIdExclusive)
        : bm_(bm), maxPageId_(maxPageIdExclusive) {}

    void open() override {
        Operator::open();
        currentPageId_ = 1;
        currentSlot_ = 0;
        currentPage_ = nullptr;
    }

    bool next(Tuple& out) override {
        while (currentPageId_ < maxPageId_) {
            if (!currentPage_) {
                currentPage_ = bm_.fetchPage(currentPageId_);
                pagesRead_++;
            }
            const PageHeader* h = currentPage_->header();
            while (currentSlot_ < h->num_slots) {
                char buf[PAGE_SIZE];
                uint16_t len;
                bool ok = currentPage_->getRecord(currentSlot_, buf, len);
                currentSlot_++;
                if (ok) {
                    out = Tuple::deserialize(buf, len);
                    out.rid = static_cast<int>(currentPageId_ * 1000 + currentSlot_ - 1);
                    tuplesProduced_++;
                    return true;
                }
            }
            bm_.unpinPage(currentPageId_, false);
            currentPage_ = nullptr;
            currentPageId_++;
            currentSlot_ = 0;
        }
        return false;
    }

    void close() override {
        if (currentPage_) bm_.unpinPage(currentPageId_, false);
        Operator::close();
    }

    std::string name() const override { return "Scan"; }

    void explain(std::ostream& out, int depth) const override {
        out << std::string(depth * 2, ' ')
            << "Scan [paginas 1-" << (maxPageId_ > 0 ? maxPageId_ - 1 : 0) << "]"
            << " -> tuplas: " << tuplesProduced_
            << ", paginas leidas: " << pagesRead_
            << ", tiempo: " << elapsedMs() << "ms\n";
    }
};
