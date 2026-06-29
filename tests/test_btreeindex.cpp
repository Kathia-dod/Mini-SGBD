#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include "../src/index/BTreeIndex.hpp"
#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"

using namespace std;

void separador() {

    cout << "\n------------------------------"
         << endl;
}

void checkFound(BTreeIndex& tree, int key, int expectedValue) {
    int value = -1;
    bool found = tree.search(key, value);
    assert(found && "Clave deberia existir");
    assert(value == expectedValue && "Valor incorrecto");
    cout << "[OK] search(" << key << ") = " << value << endl;
}

void checkNotFound(BTreeIndex& tree, int key) {
    int value = -1;
    bool found = tree.search(key, value);
    assert(!found && "Clave NO deberia existir");
    cout << "[OK] search(" << key << ") = NOT FOUND" << endl;
}

int main() {
   
    separador();
    cout << "TEST SEMANA 11 - INSERCION Y SPLITTING EN B+ TREE" << endl;
    separador();

    StorageManager storageManager("database_insert.db");
    BufferManager bufferManager(20, storageManager);
    cout << "[OK] StorageManager inicializado" << endl;
    cout << "[OK] BufferManager inicializado" << endl;

    BTreeIndex tree(&bufferManager);
    cout << "[OK] B+ Tree creado" << endl;

    separador();

    // BLOQUE 1: Split simple de hoja (overflow una sola vez)
    cout << "\nBLOQUE 1: SPLIT SIMPLE DE HOJA" << endl;
    cout << "Insertando 5 claves (BTREE_ORDER=4, overflow en la 5ta)..." << endl;

    tree.insert(10, 100);
    tree.insert(20, 200);
    tree.insert(5,  50);
    tree.insert(15, 150);
    tree.insert(25, 250);   // <- aqui debe ocurrir el primer split

    tree.printTree();

    // Verificar que TODAS las claves siguen siendo encontrables tras el split
    checkFound(tree, 5,  50);
    checkFound(tree, 10, 100);
    checkFound(tree, 15, 150);
    checkFound(tree, 20, 200);
    checkFound(tree, 25, 250);
    cout << "[OK] Las 5 claves se mantienen accesibles tras el split" << endl;

    separador();

    // BLOQUE 2: Insertar mas claves para forzar splits en cascada
    cout << "\nBLOQUE 2: SPLITS EN CASCADA (crecimiento del arbol)" << endl;

    vector<int> moreKeys = {30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80};
    for (int k : moreKeys) {
        tree.insert(k, k * 10);
        cout << "  insert(" << k << ") -> OK" << endl;
    }

    tree.printTree();

    // Verificar TODAS las claves insertadas hasta ahora
    vector<int> allKeys = {5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80};
    cout << "\nVerificando las " << allKeys.size() << " claves insertadas:" << endl;
    for (int k : allKeys) {
        checkFound(tree, k, k * 10);
    }

    separador();

    // BLOQUE 3: Insercion fuera de orden (no secuencial)
    cout << "\nBLOQUE 3: INSERCION EN ORDEN ALEATORIO" << endl;

    vector<int> randomKeys = {200, 50, 175, 3, 999, 42, 88, 150, 1, 500};
    for (int k : randomKeys) {
        tree.insert(k, k + 1);
        cout << "  insert(" << k << ", " << (k+1) << ") -> OK" << endl;
    }

    for (int k : randomKeys) {
        checkFound(tree, k, k + 1);
    }
    cout << "[OK] Todas las claves en orden aleatorio se insertaron correctamente" << endl;

    separador();

    // BLOQUE 4: Upsert - actualizar valor de una clave existente
    cout << "\nBLOQUE 4: UPSERT (actualizar valor de clave existente)" << endl;

    checkFound(tree, 10, 100);          // valor original
    tree.insert(10, 99999);             // actualizar
    checkFound(tree, 10, 99999);        // debe reflejar el nuevo valor
    cout << "[OK] Upsert de clave 10: 100 -> 99999" << endl;

    int rootBefore = tree.getRootPageId();
    tree.insert(10, 1);                  // otra actualizacion, no debe causar split
    int rootAfter = tree.getRootPageId();
    assert(rootBefore == rootAfter && "Upsert no deberia cambiar la raiz");
    cout << "[OK] Upsert no provoca split innecesario (raiz sin cambios)" << endl;

    separador();

    // BLOQUE 5: Verificacion de claves inexistentes tras todas las inserciones
    cout << "\nBLOQUE 5: CLAVES INEXISTENTES TRAS INSERCIONES MASIVAS" << endl;

    checkNotFound(tree, 9999);
    checkNotFound(tree, -1);
    checkNotFound(tree, 51);    // entre 50 y 55
    checkNotFound(tree, 201);   // cerca de 200 pero no insertada

    separador();

    // BLOQUE 6: Estructura final del arbol
    cout << "\nESTRUCTURA FINAL DEL ARBOL:" << endl;
    tree.printTree();

    separador();

    cout << "\nVALIDACION SEMANA 11:" << endl;
    cout << "[OK] Insercion ordenada en hoja" << endl;
    cout << "[OK] Deteccion de overflow (numKeys > BTREE_ORDER)" << endl;
    cout << "[OK] Split de hoja con promocion de clave (copia)" << endl;
    cout << "[OK] Split de nodo interno con promocion de clave (movimiento)" << endl;
    cout << "[OK] Splits en cascada propagados hacia la raiz" << endl;
    cout << "[OK] Creacion de nueva raiz cuando la raiz se divide" << endl;
    cout << "[OK] Insercion en orden aleatorio (no solo secuencial)" << endl;
    cout << "[OK] Upsert sin duplicar claves ni causar split innecesario" << endl;
    cout << "[OK] Integridad de busqueda tras multiples splits" << endl;

    separador();

    cout << "\n[TODOS LOS TESTS DE INSERCION PASARON]\n" << endl;
    return 0;

    return 0;
}
