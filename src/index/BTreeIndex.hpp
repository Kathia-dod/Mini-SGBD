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

    // Identificador de pagina raiz
    int rootPageId_;
    
    // Helpers de I/O (Solo Lectura) 
    // Carga un nodo desde disco (lo deserializa). El llamador hace delete.
    BNode* loadNode(int pageId) const;
    BLeafNode* loadLeaf(int pageId) const;
    BInternalNode* loadInternal(int pageId) const;
    void saveNode(BNode* node) const;

    int allocPage();

    //Busqueda
    int findLeafPage(int pageId, int key) const;

    struct InsertResult {
        int promoted = -1;
        int newChildPageId = -1;
    };

    InsertResult insertRec(int pageId, int key, int value);
    InsertResult splitLeaf(BLeafNode* leaf, int key, int value);    InsertResult splitInternal(BInternalNode* node, int promoted, int newChildPageId);

    // Mínimo de claves permitido en un nodo no-raíz: ceil(ORDER / 2)
    static const int BTREE_MIN = (BTREE_ORDER + 1) / 2;   

    struct RemoveResult {
        bool underflow = false; 
    };

    RemoveResult removeRec(int pageId, int key);
    bool fixUnderflow(BInternalNode* parent, int childIdx);
    void mergeChildren(BInternalNode* parent, int childIdx);
    void borrowFromLeft(BInternalNode* parent, int childIdx);
    void borrowFromRight(BInternalNode* parent, int childIdx);

public:

    explicit BTreeIndex(BufferManager* bm);
    
    ~BTreeIndex();
    
    // Inserta una clave y valor en el arbol
    void insert(int key, int value);

    // Busca una clave dentro del arbol
    bool search(int key, int& value) const;
   
    // Muestra la estructura actual del arbol
    void printTree() const;

    int getRootPageId() const { return rootPageId_; };

    void remove(int key);
};
