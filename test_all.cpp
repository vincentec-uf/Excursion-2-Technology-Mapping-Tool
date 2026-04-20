// test_all.cpp
#include "Node.h"
#include <iostream>
#include <fstream>
#include <unordered_map>

// Declare functions from the other files
Node* buildTree(const std::string& filename);
Node* convertNandNot(Node* original);
int calculateMinCost(Node* currentNode, std::unordered_map<Node*, int>& memo);

int main() {
    Node* original = buildTree("netlist.txt");
    if (!original) return 1;

    Node* nandRoot = convertNandNot(original);
    if (!nandRoot) return 1;

    std::unordered_map<Node*, int> memo;
    int minCost = calculateMinCost(nandRoot, memo);

    std::cout << "\n=== FINAL MIN COST ===\n";
    std::cout << minCost << std::endl;

    std::ofstream out("output.txt");
    if (out) {
        out << minCost << std::endl;
        std::cout << "output.txt created with cost: " << minCost << std::endl;
    }

    return 0;
}