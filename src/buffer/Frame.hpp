#pragma once
#include "../storage/Page.hpp"

struct Frame {
    Page page;
    uint32_t page_id;
    bool dirty;
    int pin_count;
    bool occupied;
    Frame()
        : page_id(0),
          dirty(false),
          pin_count(0),
          occupied(false) {}
};