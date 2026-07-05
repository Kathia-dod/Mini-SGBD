#include <iostream>
#include <cstdio>

#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"
#include "../src/index/BTreeIndex.hpp"

using namespace std;

const char* DB = "persistencia.db";

void crear() {

    cout << "PRIMERA EJECUCION - CREANDO ARBOL\n";

    remove(DB);

    StorageManager storage(DB);
    BufferManager buffer(3, storage);
    BTreeIndex tree(&buffer);

    for (int i = 10; i <= 200; i += 10) {
        tree.insert(i, i * 10);
    }

    cout << "\nARBOL GENERADO\n";
    tree.printTree();

    buffer.flushAll();

    cout << "\nGUARDADO EN DISCO\n";
}

void reabrir() {

    cout << "\nSEGUNDA EJECUCION - RECUPERANDO ARBOL\n";

    StorageManager storage(DB);
    BufferManager buffer(3, storage);
    BTreeIndex tree(&buffer);

    cout << "\nVERIFICANDO DATOS\n";

    int value;
    for (int i = 10; i <= 200; i += 10) {

        if (tree.search(i, value))
            cout << i << " -> " << value << endl;
        else
            cout << i << " NO ENCONTRADO\n";
    }

    cout << "\nMODIFICACIONES (DELETE)\n";

    tree.remove(20);
    tree.remove(70);
    tree.remove(120);

    buffer.flushAll();

    cout << "\nARBOL DESPUES DE DELETE\n";
    tree.printTree();

    cout << "\nGUARDADO CAMBIOS\n";
}

void verificacionFinal() {

    cout << "\nTERCERA EJECUCION - VERIFICACION FINAL\n";

    StorageManager storage(DB);
    BufferManager buffer(3, storage);
    BTreeIndex tree(&buffer);

    cout << "\nESTADO FINAL DEL ARBOL\n";

    int value;
    for (int i = 10; i <= 200; i += 10) {

        if (tree.search(i, value))
            cout << i << " -> " << value << endl;
        else
            cout << i << " ELIMINADA\n";
    }

    tree.printTree();
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "uso: crear | reabrir | final\n";
        return 0;
    }

    string op = argv[1];

    if (op == "crear") crear();
    else if (op == "reabrir") reabrir();
    else if (op == "final") verificacionFinal();
}