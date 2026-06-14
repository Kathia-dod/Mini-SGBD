#include "BTreeIndex.hpp"

#include <iostream>
#include <queue>
#include <algorithm>
#include <stdexcept>

using namespace std;

//constructor principal
BTreeIndex::BTreeIndex(BufferManager* bm) {
    bufferManager = bm;

    rootPageId = allocPage();

    cout << "B+ TREE INICIALIZADO"
         << endl;

    cout << "ROOT PAGE ID: "
         << rootPageId
         << endl;

}

//destructor
BTreeIndex::~BTreeIndex() {}

//Helpers de I/O

BNode* BTreeIndex::loadNode(int pageId) const {
    Page* page = bufferManager->fetchPage(static_cast<uint32_t>(pageId));
    if (!page) throw runtime_error("[BTree] fetchPage retornó nullptr para page " + to_string(pageId));

    if (BNode::peekIsLeaf(page))
        return BLeafNode::deserialize(page, pageId);
    else
        return BInternalNode::deserialize(page, pageId);
}

BLeafNode* BTreeIndex::loadLeaf(int pageId) const {
    Page* page = bufferManager->fetchPage(static_cast<uint32_t>(pageId));
    if (!page) throw runtime_error("[BTree] fetchPage nullptr (hoja) page " + to_string(pageId));

    auto* node = BLeafNode::deserialize(page, pageId);
    bufferManager->unpinPage(static_cast<uint32_t>(pageId), false);
    return node;
}

BInternalNode* BTreeIndex::loadInternal(int pageId) const {
    Page* page = bufferManager->fetchPage(static_cast<uint32_t>(pageId));
    if (!page) throw runtime_error("[BTree] fetchPage nullptr (interno) page " + to_string(pageId));

    auto* node = BInternalNode::deserialize(page, pageId);
    bufferManager->unpinPage(static_cast<uint32_t>(pageId), false);
    return node;
}

int BTreeIndex::allocPage() {
    uint32_t pid;
    Page* page = bufferManager->newPage(pid);
    if (!page) throw runtime_error("[BTree] newPage falló");

    // Inicializar la página como hoja vacía (isLeaf=1, numKeys=0)
    BLeafNode tmp;
    tmp.pageId = static_cast<int>(pid);
    tmp.serialize(page);

    bufferManager->unpinPage(pid, true); // true porque escribimos la estructura inicial
    return static_cast<int>(pid);
}

// Busqueda y Navegacion 
int BTreeIndex::findLeafPage(int pageId, int key) const {
    Page* page = bufferManager->fetchPage(static_cast<uint32_t>(pageId));
    if (!page) throw runtime_error("[BTree] findLeafPage: fetchPage nullptr");

    if (BNode::peekIsLeaf(page)) {
        bufferManager->unpinPage(static_cast<uint32_t>(pageId), false);
        return pageId; // Llegamos a la hoja
    }

    // Nodo interno: leer para navegar
    BInternalNode* node = BInternalNode::deserialize(page, pageId);
    bufferManager->unpinPage(static_cast<uint32_t>(pageId), false);

    // Buscar el hijo correcto con upper_bound (primer elemento > key)
    auto it = upper_bound(node->keys.begin(), node->keys.end(), key);
    int idx = static_cast<int>(it - node->keys.begin());
    int child = node->children[idx];

    delete node; // Liberar memoria RAM del objeto deserializado
    return findLeafPage(child, key); // Descenso recursivo
}

bool BTreeIndex::search(int key, int& value) const {
    int leafPageId = findLeafPage(rootPageId, key);
    BLeafNode* leaf = loadLeaf(leafPageId);

    // Búsqueda binaria en la hoja
    auto it = lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
    bool found = (it != leaf->keys.end() && *it == key);

    if (found) {
        int idx = static_cast<int>(it - leaf->keys.begin());
        value = leaf->values[idx]; // Escribir en la variable de salida pasada por referencia
    }

    delete leaf; // Liberar memoria del nodo cargado
    return found;
}

void BTreeIndex::saveNode(BNode* node) const {
    uint32_t pid = static_cast<uint32_t>(node->pageId);
    Page* page   = bufferManager->fetchPage(pid);
    if (!page) throw runtime_error("[BTree] saveNode: fetchPage nullptr page " + to_string(pid));
    node->serialize(page);
    bufferManager->unpinPage(pid, true);
}

// Insercion basica en nodo hoja raiz
void BTreeIndex::insert(int key, int value) {
    
    int leafPageId = findLeafPage(rootPageId, key); 
    // Obtener nodo hoja raiz
    BLeafNode* leaf = loadLeaf(leafPageId);

    // Simulacion de acceso mediante  Buffer Manager
    cout << "\n[BUFFER FETCH] Cargando página hoja para inserción: id="
         << leaf->pageId
         << endl;

    //  Insercion ordenada de claves
    size_t pos = 0;

    while(pos < leaf->keys.size() && leaf->keys[pos] < key) {
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

    saveNode(leaf);
    cout << "[BUFFER UNPIN] Guardado nodo hoja persistido con dirty=true" << endl;
    // Validacion de overflow del nodo hoja
    const int MAX_KEYS = 4;

    if(leaf->numKeys > MAX_KEYS) {
        cout << "\n[SPLIT NECESARIO] "
            << "Nodo hoja lleno"
            << endl;
    }
    delete leaf;    
}

// Mostrar estructura del arbol (Implementación BFS Real sobre Disco)
void BTreeIndex::printTree() const {
    cout << "\nB+ TREE DUMP\n";
    queue<int> q;
    q.push(rootPageId);

    while (!q.empty()) {
        int currentId = q.front();
        q.pop();

        Page* page = bufferManager->fetchPage(static_cast<uint32_t>(currentId));
        if (!page) continue;
        bool isLeaf = BNode::peekIsLeaf(page);
        bufferManager->unpinPage(static_cast<uint32_t>(currentId), false);

        if (isLeaf) {
            BLeafNode* leaf = loadLeaf(currentId);
            cout << "  [Hoja Page " << currentId << "]: ";
            for (size_t i = 0; i < leaf->keys.size(); ++i) {
                cout << "(" << leaf->keys[i] << " -> " << leaf->values[i] << ") ";
            }
            cout << "\n";
            delete leaf;
        } else {
            BInternalNode* internal = loadInternal(currentId);
    
            cout << "  [Interno Page " << currentId << "]: Claves: [ ";
            for (int k : internal->keys) cout << k << " ";
            cout << "] | Hijos Page: [ ";
            
            for (int childId : internal->children) {
                cout << childId << " ";
                q.push(childId);
            }
            cout << "]\n";
            delete internal;
        }
    }
}
