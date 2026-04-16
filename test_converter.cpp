// test_converter.cpp
#include "Node.h"
#include <iostream>

// Declare functions from other files
Node* buildTree(const std::string& filename);
void printTree(Node* node, int depth = 0);
Node* convertNandNot(Node* original);

int main() {
    Node* originalRoot = buildTree("netlist.txt");
    if (!originalRoot) return 1;

    std::cout << "\n=== Original Tree ===\n";
    printTree(originalRoot);

    Node* nandRoot = convertNandNot(originalRoot);
    if (nandRoot) {
        std::cout << "\n=== NAND-NOT Tree (after conversion) ===\n";
        printTree(nandRoot);
    } else {
        std::cout << "Conversion failed!\n";
    }

    return 0;
}