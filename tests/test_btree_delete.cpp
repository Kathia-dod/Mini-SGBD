#include <iostream>
#include <cassert>
#include <vector>
#include <algorithm>
#include "../src/index/BTreeIndex.hpp"
#include "../src/storage/StorageManager.hpp"
#include "../src/buffer/BufferManager.hpp"

using namespace std;

void separador() {
    cout << "\n------------------------------" << endl;
}

void checkFound(BTreeIndex& tree, int key, int expectedValue) {
    int value = -1;
    bool found = tree.search(key, value);
    assert(found  && "Clave deberia existir");
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
    cout << "TEST SEMANA 12 - ELIMINACION EN B+ TREE" << endl;
    separador();

    StorageManager storageManager("database_delete.db");
    BufferManager  bufferManager(20, storageManager);

    cout << "[OK] StorageManager inicializado" << endl;
    cout << "[OK] BufferManager inicializado" << endl;

    BTreeIndex tree(&bufferManager);
    cout << "[OK] B+ Tree creado" << endl;

    //Insertar el mismo conjunto que semana 11 para tener un árbol con varios niveles y hojas encadenadas ─
    vector<int> keys = {10, 20, 5, 15, 25, 30, 35, 40, 45, 50, 55, 60};
    for (int k : keys)
        tree.insert(k, k * 10);

    separador();
    cout << "ARBOL INICIAL (tras " << keys.size() << " inserciones):" << endl;
    tree.printTree();

    // Verificar que todas las claves insertadas existen
    for (int k : keys)
        checkFound(tree, k, k * 10);
    cout << "[OK] Todas las claves existen antes de eliminar\n";

    // BLOQUE 1: Eliminación simple (sin underflow)
    separador();
    cout << "\nBLOQUE 1: ELIMINACION SIMPLE (sin underflow)" << endl;

    int rootAntes = tree.getRootPageId();
    tree.remove(45);
    int rootDespues = tree.getRootPageId();

    checkNotFound(tree, 45);
    checkFound(tree, 40, 400);
    checkFound(tree, 50, 500);

    assert(rootAntes == rootDespues && "Raiz no deberia cambiar en borrado simple");
    cout << "[OK] Raiz sin cambios tras eliminacion simple\n";

    // BLOQUE 2: Eliminar una clave inexistente (no debe romper nada)
    separador();
    cout << "\nBLOQUE 2: ELIMINACION DE CLAVE INEXISTENTE" << endl;

    tree.remove(999);
    tree.remove(-1);
    cout << "[OK] remove(999) no lanzo excepcion\n";
    cout << "[OK] remove(-1)  no lanzo excepcion\n";

    for (int k : keys) {
        if (k == 45) continue;
        checkFound(tree, k, k * 10);
    }
    cout << "[OK] Claves restantes intactas tras intentar borrar clave inexistente\n";

    // BLOQUE 3: Eliminaciones hasta forzar underflow y redistribución
    separador();
    cout << "\nBLOQUE 3: UNDERFLOW CON REDISTRIBUCION (borrow)" << endl;

    tree.remove(55);
    tree.remove(60);

    checkNotFound(tree, 55);
    checkNotFound(tree, 60);

    checkFound(tree, 50, 500);
    checkFound(tree, 40, 400);
    checkFound(tree, 30, 300);

    cout << "\nArbol tras redistribucion:\n";
    tree.printTree();
    cout << "[OK] Arbol consistente tras borrow\n";

    // BLOQUE 4: Eliminaciones hasta forzar merge
    separador();
    cout << "\nBLOQUE 4: UNDERFLOW CON MERGE" << endl;

    tree.remove(50);
    tree.remove(40);
    tree.remove(35);

    checkNotFound(tree, 50);
    checkNotFound(tree, 40);
    checkNotFound(tree, 35);

    checkFound(tree, 5,  50);
    checkFound(tree, 10, 100);
    checkFound(tree, 15, 150);
    checkFound(tree, 20, 200);
    checkFound(tree, 25, 250);
    checkFound(tree, 30, 300);

    cout << "\nArbol tras merge:\n";
    tree.printTree();
    cout << "[OK] Arbol consistente tras merge\n";

    // BLOQUE 5: Vaciado total del árbol
    separador();
    cout << "\nBLOQUE 5: VACIADO TOTAL DEL ARBOL" << endl;

    vector<int> remaining = {5, 10, 15, 20, 25, 30};
    for (int k : remaining) {
        tree.remove(k);
        cout << "  remove(" << k << ") -> OK" << endl;
    }

    for (int k : remaining)
        checkNotFound(tree, k);

    cout << "\nArbol vacio:\n";
    tree.printTree();
    cout << "[OK] Arbol vacio: ninguna clave encontrada\n";

    // BLOQUE 6: Reinsertar tras vaciado 
    separador();
    cout << "\nBLOQUE 6: REINSERCION TRAS VACIADO" << endl;

    tree.insert(100, 1000);
    tree.insert(200, 2000);
    tree.insert(150, 1500);

    checkFound(tree, 100, 1000);
    checkFound(tree, 200, 2000);
    checkFound(tree, 150, 1500);
    cout << "[OK] Arbol funcional tras reinsercion despues del vaciado\n";

    separador();
    cout << "\nVALIDACION SEMANA 12:" << endl;
    cout << "[OK] Eliminacion simple sin underflow" << endl;
    cout << "[OK] Eliminacion de clave inexistente (no-op seguro)" << endl;
    cout << "[OK] Underflow con redistribucion (borrowFromLeft/Right)" << endl;
    cout << "[OK] Underflow con fusion de nodos (merge)" << endl;
    cout << "[OK] Vaciado total del arbol" << endl;
    cout << "[OK] Reinsercion tras vaciado" << endl;
    separador();

    cout << "\n[TODOS LOS TESTS DE ELIMINACION PASARON]\n" << endl;
    return 0;
}
