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
};