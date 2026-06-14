#pragma once
#include "../storage/Page.hpp"
#include <cstdint>
#include <cstring>

static const int BTREE_ORDER = 4;   // máximo de claves por nodo

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
    
    //Serializacion
    virtual void serialize(Page* page) const = 0;
 
    // Escribe la cabecera común (isLeaf + numKeys) y devuelve offset tras ella
    static int writeHeader(Page* page, bool isLeaf, int numKeys) {
        char* d = page->data;
        d[0] = static_cast<uint8_t>(isLeaf ? 1 : 0);
        memcpy(d + 1, &numKeys, sizeof(int));
        return 1 + sizeof(int);   // = 5
    }
 
    // Escribe un arreglo de `count` ints a partir de `offset`
    static int writeIntArray(Page* page, int offset, const int* arr, int count) {
        memcpy(page->data + offset, arr, count * sizeof(int));
        return offset + count * sizeof(int);
    }
 
    // Lee un arreglo de `count` ints desde `offset`
    static int readIntArray(const Page* page, int offset, int* arr, int count) {
        memcpy(arr, page->data + offset, count * sizeof(int));
        return offset + count * sizeof(int);
    }
 
    // Lee isLeaf desde una página ya cargada
    static bool peekIsLeaf(const Page* page) {
        return static_cast<uint8_t>(page->data[0]) == 1;
    }
 
    // Lee numKeys desde una página ya cargada
    static int peekNumKeys(const Page* page) {
        int n;
        memcpy(&n, page->data + 1, sizeof(int));
        return n;
    }
};


