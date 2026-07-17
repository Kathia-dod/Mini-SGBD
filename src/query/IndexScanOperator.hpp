#pragma once

#include "Operator.hpp"
#include "../buffer/BufferManager.hpp"
#include "../index/BTreeIndex.hpp"

class IndexScanOperator : public Operator {

private:

    BufferManager& bm_;
    BTreeIndex& index_;

    int searchKey_;

    bool executed_ = false;
    bool found_ = false;

    int encodedRID_ = -1;

    uint32_t pageId_ = 0;
    uint16_t slotId_ = 0;

public:

    IndexScanOperator(
        BufferManager& bm,
        BTreeIndex& index,
        int searchKey
    )
        : bm_(bm),
          index_(index),
          searchKey_(searchKey)
    {
    }

    void open() override {

        Operator::open();

        executed_ = false;
        found_ = false;
        encodedRID_ = -1;
        pageId_ = 0;
        slotId_ = 0;
    }

    bool next(Tuple& out) override {

        if (executed_)
            return false;

        executed_ = true;

        /*
         * Primero se busca la clave en el B+ Tree.
         */
        if (!index_.search(searchKey_, encodedRID_))
            return false;

        /*
         * Decodificamos el RID:
         *
         * RID = pageId * 1000 + slotId
         */
        pageId_ = encodedRID_ / 1000;
        slotId_ = encodedRID_ % 1000;

        Page* page = bm_.fetchPage(pageId_);

        if (page == nullptr)
            return false;

        pagesRead_++;

        char buffer[PAGE_SIZE];
        uint16_t length;

        bool ok = page->getRecord(
            slotId_,
            buffer,
            length
        );

        bm_.unpinPage(pageId_, false);

        if (!ok)
            return false;

        out = Tuple::deserialize(
            buffer,
            length
        );

        tuplesProduced_++;

        found_ = true;

        return true;
    }

    void close() override {

        Operator::close();
    }

    std::string name() const override {

        return "IndexScan";
    }

    void explain(
        std::ostream& out,
        int depth = 0
    ) const override {

        out << std::string(depth * 2, ' ')
            << "Index Scan [B+ Tree]"
            << " -> clave: " << searchKey_
            << ", tuplas: " << tuplesProduced_
            << ", paginas leidas: " << pagesRead_
            << ", tiempo: " << elapsedMs()
            << " ms\n";
    }
};