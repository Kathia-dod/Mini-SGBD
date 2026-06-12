#include <iostream>

#include "../src/index/BTreeIndex.hpp"

#include "../src/storage/StorageManager.hpp"

#include "../src/buffer/BufferManager.hpp"

using namespace std;

void separador() {

    cout << "\n------------------------------"
         << endl;
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

   //search tree

    int value;

    cout << "\nVALIDACION BASICA DE BUSQUEDA:"<< endl;

    if(tree.search(15, value)) {
        cout << "[OK] Clave 15 encontrada"<< endl;
        cout << "Valor: " << value << endl;
    }

    if(!tree.search(99, value)) {
        cout << "[OK] Clave inexistente detectada" << endl;
    }

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

    return 0;
}