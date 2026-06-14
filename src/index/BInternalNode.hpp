#pragma once
//     Nodo interno del arbol B+.
#include "BNode.hpp"
#include <vector>


class BInternalNode : public BNode {

public:

    // Claves separadoras del nodo
    std::vector<int> keys;

    // Referencias a paginas hijas
    std::vector<int> children;
    
    BInternalNode()
        : BNode(false) {}

    // Serializacion
    void serialize(Page* page) const override {
        int off = writeHeader(page, false, numKeys);
        off = writeIntArray(page, off, keys.data(), numKeys);
        writeIntArray(page, off, children.data(), numKeys + 1);
        page->dirty = true;
    }
 
    // Deserializacion 
    static BInternalNode* deserialize(const Page* page, int pageId) {
        auto* node = new BInternalNode();
        node->pageId  = pageId;
        node->numKeys = peekNumKeys(page);
 
        node->keys.resize(node->numKeys);
        node->children.resize(node->numKeys + 1);
 
        int off = 1 + sizeof(int);
        off = readIntArray(page, off, node->keys.data(),     node->numKeys);
        readIntArray(page, off, node->children.data(), node->numKeys + 1);
        return node;
    }
};
