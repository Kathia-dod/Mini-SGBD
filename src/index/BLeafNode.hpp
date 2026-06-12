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
};