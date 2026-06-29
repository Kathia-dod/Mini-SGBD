#include "BTreeIndex.hpp"

#include <iostream>
#include <queue>
#include <algorithm>
#include <stdexcept>

using namespace std;

//constructor principal
BTreeIndex::BTreeIndex(BufferManager* bm) {
    bufferManager = bm;

    rootPageId_ = allocPage();

    cout << "B+ TREE INICIALIZADO"
         << endl;
    BLeafNode root;
    root.pageId = rootPageId_;
    saveNode(&root);
    cout << "ROOT PAGE ID: "
         << rootPageId_
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

void BTreeIndex::saveNode(BNode* node) const {
    uint32_t pid = static_cast<uint32_t>(node->pageId);
    Page* page   = bufferManager->fetchPage(pid);
    if (!page) throw runtime_error("[BTree] fetchPage nullptr al guardar page " + to_string(pid));
    node->serialize(page);
    bufferManager->unpinPage(pid, true);   // dirty = true → el BM lo escribirá a disco
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
    int leafPageId = findLeafPage(rootPageId_, key);
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

void BTreeIndex::insert(int key, int value) {
    InsertResult res = insertRec(rootPageId_, key, value);

    if (res.promoted != -1) {
        // La raíz se dividió → crear nueva raíz interna
        int newRootPageId = allocPage();

        BInternalNode newRoot;
        newRoot.pageId = newRootPageId;
        newRoot.keys.push_back(res.promoted);
        newRoot.children.push_back(rootPageId_);
        newRoot.children.push_back(res.newChildPageId);
        newRoot.numKeys = 1;

        saveNode(&newRoot);
        rootPageId_ = newRootPageId;

        cout << "[BTree] Nueva raíz en page " << rootPageId_
             << " (clave promovida: " << res.promoted << ")\n";
    }
}

BTreeIndex::InsertResult BTreeIndex::insertRec(int pageId, int key, int value) {
    Page* page = bufferManager->fetchPage(static_cast<uint32_t>(pageId));
    if (!page) throw runtime_error("[BTree] insertRec: fetchPage nullptr");
 
    bool isLeaf = BNode::peekIsLeaf(page);
    bufferManager->unpinPage(static_cast<uint32_t>(pageId), false);
 
    // Caso hoja 
    if (isLeaf) {
        BLeafNode* leaf = loadLeaf(pageId);
 
        // Buscar posición de inserción (mantener orden ascendente)
        auto it  = lower_bound(leaf->keys.begin(), leaf->keys.end(), key);
        int  idx = static_cast<int>(it - leaf->keys.begin());
 
        // Si la clave ya existe, actualizar valor (upsert)
        if (it != leaf->keys.end() && *it == key) {
            leaf->values[idx] = value;
            saveNode(leaf);
            delete leaf;
            return {};   // sin split
        }
 
        leaf->keys.insert(leaf->keys.begin() + idx, key);
        leaf->values.insert(leaf->values.begin() + idx, value);
        leaf->numKeys++;
 
        if (leaf->numKeys <= BTREE_ORDER) {
            // Sin overflow → solo guardar
            saveNode(leaf);
            delete leaf;
            return {};
        }
 
        // Overflow → split
        InsertResult res = splitLeaf(leaf, key, value);
        delete leaf;
        return res;
    }
 
    // Caso nodo interno 
    BInternalNode* node = loadInternal(pageId);
 
    // Elegir hijo con upper_bound
    auto it    = upper_bound(node->keys.begin(), node->keys.end(), key);
    int  idx   = static_cast<int>(it - node->keys.begin());
    int  child = node->children[idx];
 
    InsertResult childRes = insertRec(child, key, value);
 
    if (childRes.promoted == -1) {
        // El hijo no se dividió; nada que propagar
        delete node;
        return {};
    }
 
    // El hijo se dividió → insertar clave promovida en este nodo interno
    node->keys.insert(node->keys.begin() + idx, childRes.promoted);
    node->children.insert(node->children.begin() + idx + 1, childRes.newChildPageId);
    node->numKeys++;
 
    if (node->numKeys <= BTREE_ORDER) {
        saveNode(node);
        delete node;
        return {};
    }
 
    // Overflow en nodo interno → split
    InsertResult res = splitInternal(node, childRes.promoted, childRes.newChildPageId);
    delete node;
    return res;
}

BTreeIndex::InsertResult BTreeIndex::splitLeaf(BLeafNode* leaf, int, int) {
    // Nota: leaf ya tiene la nueva clave insertada (numKeys = ORDER+1)
    int total = leaf->numKeys;
    int mid   = total / 2;   // índice de inicio del nuevo nodo derecho

    // Crear nuevo nodo derecho
    int rightPageId = allocPage();
    BLeafNode rightLeaf;
    rightLeaf.pageId  = rightPageId;
    rightLeaf.keys    = vector<int>(leaf->keys.begin()   + mid, leaf->keys.end());
    rightLeaf.values  = vector<int>(leaf->values.begin() + mid, leaf->values.end());
    rightLeaf.numKeys = static_cast<int>(rightLeaf.keys.size());
    rightLeaf.nextLeaf = leaf->nextLeaf;

    // Truncar el nodo izquierdo (original)
    leaf->keys.resize(mid);
    leaf->values.resize(mid);
    leaf->numKeys  = mid;
    leaf->nextLeaf = rightPageId;   // enlace hoja siguiente

    // Persistir ambos nodos
    saveNode(leaf);
    saveNode(&rightLeaf);

    int promoted = rightLeaf.keys[0];   // primera clave del nodo derecho
    cout << "[BTree] Split hoja: clave promovida=" << promoted
         << " | left page=" << leaf->pageId
         << " right page=" << rightPageId << "\n";

    return { promoted, rightPageId };
}

BTreeIndex::InsertResult BTreeIndex::splitInternal(BInternalNode* node, int, int) {
    int total = node->numKeys;
    int mid   = total / 2;

    int promotedKey = node->keys[mid];

    // Crear nodo interno derecho
    int rightPageId = allocPage();
    BInternalNode rightNode;
    rightNode.pageId    = rightPageId;
    rightNode.keys      = vector<int>(node->keys.begin()     + mid + 1, node->keys.end());
    rightNode.children  = vector<int>(node->children.begin() + mid + 1, node->children.end());
    rightNode.numKeys   = static_cast<int>(rightNode.keys.size());

    // Truncar nodo izquierdo (original), la clave del medio se va al padre
    node->keys.resize(mid);
    node->children.resize(mid + 1);
    node->numKeys = mid;

    saveNode(node);
    saveNode(&rightNode);

    cout << "[BTree] Split interno: clave promovida=" << promotedKey
         << " | left page=" << node->pageId
         << " right page=" << rightPageId << "\n";

    return { promotedKey, rightPageId };
}

void BTreeIndex::printTree() const {
    cout << "\n B+ Tree (raíz page=" << rootPageId_ << ") \n";
 
    // BFS: cola de (pageId, nivel)
    queue<pair<int,int>> q;
    q.push({ rootPageId_, 0 });
    int currentLevel = -1;
 
    while (!q.empty()) {
        auto [pid, level] = q.front(); q.pop();
 
        if (level != currentLevel) {
            if (currentLevel != -1) cout << "\n";
            cout << "Nivel " << level << ": ";
            currentLevel = level;
        }
 
        Page* page = bufferManager->fetchPage(static_cast<uint32_t>(pid));
        if (!page) { cout << "[null] "; continue; }
 
        if (BNode::peekIsLeaf(page)) {
            BLeafNode* leaf = BLeafNode::deserialize(page, pid);
            bufferManager->unpinPage(static_cast<uint32_t>(pid), false);
 
            cout << "[Hoja p" << pid << ": ";
            for (int i = 0; i < leaf->numKeys; i++) {
                if (i) cout << ", ";
                cout << leaf->keys[i] << "→" << leaf->values[i];
            }
            cout << "] ";
            delete leaf;
        } else {
            BInternalNode* node = BInternalNode::deserialize(page, pid);
            bufferManager->unpinPage(static_cast<uint32_t>(pid), false);
 
            cout << "[Int p" << pid << ": ";
            for (int i = 0; i < node->numKeys; i++) {
                if (i) cout << "|";
                cout << node->keys[i];
            }
            cout << "] ";
 
            for (int child : node->children)
                q.push({ child, level + 1 });
            delete node;
        }
    }
    cout << "\n--------------------------------------------\n\n";
}


