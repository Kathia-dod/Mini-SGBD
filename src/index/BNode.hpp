#pragma once
// Nodo del arbol B-tree
class BNode {
public:
    //nodo hoja
    bool isLeaf;
    //claves almacenadas
    int numKeys;

    int pageId;

    BNode(bool leaf = false)
        : isLeaf(leaf),
          numKeys(0),
          pageId(-1) {}

    virtual ~BNode() = default;
};