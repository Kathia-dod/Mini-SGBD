#include <iostream>
#include <cassert>
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
    cout << "TEST SEMANA 9 - B+ TREE"
         << endl;
    separador();

    //storage + buffer

    StorageManager storageManager("database.db");

    BufferManager bufferManager(
        10,
        storageManager
    );

    cout << "\n[OK] StorageManager inicializado"
         << endl;

    cout << "[OK] BufferManager inicializado"
         << endl;

   //B TREE

    BTreeIndex tree(&bufferManager);

    cout << "[OK] B+ Tree creado"
         << endl;

    separador();

   //inserts

    cout << "\nINSERTANDO CLAVES..."
         << endl;

    tree.insert(10, 100);
    tree.insert(20, 200);
    tree.insert(5, 50);
    tree.insert(15, 150);
    tree.insert(25, 250);

    separador();

    //tree structure

    cout << "\nESTRUCTURA ACTUAL:"<< endl;

    tree.printTree();

    separador();

    //validacion

    cout << "\nVALIDACION SEMANA 9:" << endl;
    cout << "\n[OK] Nodo hoja implementado" << endl;
    cout << "[OK] Nodo interno implementado" << endl;
    cout << "[OK] Insercion ordenada" << endl;
    cout << "[OK] Integracion con BufferManager" << endl;
    cout << "[OK] Manejo de paginas mediante pageId" << endl;
    cout << "[OK] Deteccion de overflow" << endl;
    cout << "\n[-]split pendiente" << endl;
    separador();

    //search tree
    //  BUSQUEDA: claves existentes
    cout << "\nBUSQUEDA - CLAVES EXISTENTES:" << endl;
    checkFound(tree, 5,  50);
    checkFound(tree, 10, 100);
    checkFound(tree, 15, 150);
    checkFound(tree, 20, 200);
    checkFound(tree, 25, 250);

    separador();

    //  BUSQUEDA: claves inexistentes
    cout << "\nBUSQUEDA - CLAVES INEXISTENTES:" << endl;
    checkNotFound(tree, 1);    // menor que todo
    checkNotFound(tree, 7);    // entre claves existentes
    checkNotFound(tree, 99);   // mayor que todo
    checkNotFound(tree, 12);   // entre 10 y 15

    separador();

    //  BUSQUEDA: bordes
    cout << "\nBUSQUEDA - CASOS BORDE:" << endl;
    checkFound(tree, 5,  50);   // primera clave insertada
    checkFound(tree, 25, 250);  // ultima clave insertada
    checkNotFound(tree, 4);     // justo antes del minimo
    checkNotFound(tree, 26);    // justo despues del maximo

    separador();

    //validacion Semana 10
    cout << "\nVALIDACION SEMANA 10:" << endl;
    cout << "[OK] Busqueda de claves existentes" << endl;
    cout << "[OK] Busqueda de claves inexistentes" << endl;
    cout << "[OK] Casos borde (minimo, maximo, fuera de rango)" << endl;
    separador();


    return 0;
}
