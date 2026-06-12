#pragma once
//Clase principal del indice B+ Tree.
#include "BLeafNode.hpp"

#include "../buffer/BufferManager.hpp"

class BTreeIndex {

private:

    // Referencia al Buffer Manager
    BufferManager* bufferManager;

    // Nodo raiz actual
    BNode* root;

    // Identificador de pagina raiz
    int rootPageId;

public:

    BTreeIndex(BufferManager* bm);
    
    ~BTreeIndex();

    int getRootPageId() const;

    
    // Inserta una clave y valor en el arbol
    void insert(int key, int value);

    // Busca una clave dentro del arbol
    bool search(int key, int& value);
   
    // Muestra la estructura actual del arbol
    void printTree();
};