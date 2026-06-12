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

// Insercion basica en nodo hoja raiz
void BTreeIndex::insert(int key, int value) {

    // Obtener nodo hoja raiz
    BLeafNode* leaf =
        dynamic_cast<BLeafNode*>(root);

    // Simulacion de acceso mediante  Buffer Manager
    cout << "\n[BUFFER FETCH] page="
         << leaf->pageId
         << endl;

    //  Insercion ordenada de claves
    int pos = 0;

    while(pos < leaf->keys.size()
          && leaf->keys[pos] < key) {

        pos++;
    }

    leaf->keys.insert(
        leaf->keys.begin() + pos,
        key
    );

    leaf->values.insert(
        leaf->values.begin() + pos,
        value
    );

    leaf->numKeys++;

    cout << "[INSERT] key="
         << key
         << " value="
         << value
         << endl;

    //  Simulacion de liberacion de pagina
    cout << "[UNPIN PAGE] dirty=true"
         << endl;
}

// Busqueda basica
bool BTreeIndex::search(int key, int& value) {

    return false;
}

// Mostrar estructura del arbol
void BTreeIndex::printTree() {

    BLeafNode* leaf =
        dynamic_cast<BLeafNode*>(root);

    cout << "\nROOT LEAF NODE"
         << endl;

    for(size_t i = 0;
        i < leaf->keys.size();
        i++) {

        cout << "("
             << leaf->keys[i]
             << " -> "
             << leaf->values[i]
             << ") ";
    }

    cout << endl;
}