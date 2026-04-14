// test_parser.cpp
#include "Node.h"
#include <iostream>

// Declare the functions from parser.cpp
Node* buildTree(const std::string& filename);
void printTree(Node* node, int depth = 0);

int main() {
    Node* root = buildTree("netlist.txt");
    if (root) {
        std::cout << "\n=== Parsed Tree Structure ===\n";
        printTree(root);
    }
    return 0;
}