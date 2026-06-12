#include "BTreeIndex.hpp"

#include <iostream>

using namespace std;

//constructor principal

BTreeIndex::BTreeIndex(BufferManager* bm) {

    bufferManager = bm;

    // Crear nodo hoja inicial
    root = new BLeafNode();

    // Solicitar nueva pagina al Buffer Manager
    rootPageId = bufferManager->newPage();

    // Asociar pageId al nodo raiz
    root->pageId = rootPageId;

    cout << "B+ TREE INICIALIZADO"
         << endl;

    cout << "ROOT PAGE ID: "
         << rootPageId
         << endl;

}

//destructor
BTreeIndex::~BTreeIndex() {

    delete root;
}

//  Retorna pageId raiz
int BTreeIndex::getRootPageId() const {

    return rootPageId;
}


//Insercion basica
void BTreeIndex::insert(int key, int value) {

    cout << "[INSERT] key="
         << key
         << " value="
         << value
         << endl;
}

// Busqueda basica
bool BTreeIndex::search(int key, int& value) {

    return false;
}

// Mostrar estructura del arbol
void BTreeIndex::printTree() {

    cout << "ROOT NODE"
         << endl;
}