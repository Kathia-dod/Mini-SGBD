#pragma once
//Clase principal del indice B+ Tree.
#include "BLeafNode.hpp"
#include "BInternalNode.hpp"
#include "../buffer/BufferManager.hpp"

#include <vector>
#include <utility>

class BTreeIndex {
private:

    // Referencia al Buffer Manager
    BufferManager* bufferManager;

    // Nodo raiz actual
    BNode* root;

    // Identificador de pagina raiz
    int rootPageId;
    // Helpers de I/O (Solo Lectura) 
    // Carga un nodo desde disco (lo deserializa). El llamador hace delete.
    BNode* loadNode(int pageId) const;
    BLeafNode* loadLeaf(int pageId) const;
    BInternalNode* loadInternal(int pageId) const;

    //Busqueda
    int findLeafPage(int pageId, int key) const;

public:

    explicit BTreeIndex(BufferManager* bm);
    
    ~BTreeIndex();

    int getRootPageId() const { return rootPageID; };

    
    // Inserta una clave y valor en el arbol
    void insert(int key, int value);

    // Busca una clave dentro del arbol
    bool search(int key, int& value) const;
   
    // Muestra la estructura actual del arbol
    void printTree() const;
};
