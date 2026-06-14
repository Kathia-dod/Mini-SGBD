#pragma once
//    Nodo hoja del arbol B+.
#include "BNode.hpp"
#include <vector>

class BLeafNode : public BNode {

public:

    // Claves almacenadas en la hoja
    std::vector<int> keys;

    // Valores asociados a cada clave
    std::vector<int> values;

    // Puntero a la siguiente hoja
    int nextLeaf;


    BLeafNode()
        : BNode(true),
          nextLeaf(-1) {}

    //Serializacion 
    void serialize(Page* page) const override {
        int off = writeHeader(page, true, numKeys);
        off = writeIntArray(page, off, keys.data(),   numKeys);
        off = writeIntArray(page, off, values.data(), numKeys);
        writeIntArray(page, off, &nextLeaf, 1);
        page->dirty = true;
    }

    // Deserializacion
    static BLeafNode* deserialize(const Page* page, int pageId) {
        auto* node = new BLeafNode();
        node->pageId  = pageId;
        node->numKeys = peekNumKeys(page);

        node->keys.resize(node->numKeys);
        node->values.resize(node->numKeys);

        int off = 1 + sizeof(int);   // tras isLeaf + numKeys
        off = readIntArray(page, off, node->keys.data(),   node->numKeys);
        off = readIntArray(page, off, node->values.data(), node->numKeys);
        readIntArray(page, off, &node->nextLeaf, 1);
        return node;
    }
};
