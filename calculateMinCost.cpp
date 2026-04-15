// calculateMinCost.cpp
#include "Node.h"
#include <iostream>


int calculateMinCost(Node* currentNode, std::unordered_map<Node*, int>& memo) {
    // 1. Base cases [1]
    if (currentNode == nullptr) return 0;
    if (currentNode->gate == "INPUT") return 0;

    // 2. Check memoization table (return early if already calculated) [1]
    if (memo.find(currentNode) != memo.end()) {
        return memo[currentNode];
    }

    // 3. Recursive calls for children (Bottom-up DP) [1]
    int costLeft = 0;
    int costRight = 0;
    if (currentNode->children.size() > 0) {
        costLeft = calculateMinCost(currentNode->children[0], memo);
    }
    if (currentNode->children.size() > 1) {
        costRight = calculateMinCost(currentNode->children[1], memo);
    }

    // 4. Calculate minimal cost using available library gates [1]
    int minCost = std::INT_MAX; // Start with a very high number

    // Example: Check standard NAND
    if (currentNode->gate == "NAND") {
        int cost = costLeft + costRight + 2; // Assuming C_NAND = 2
        minCost = std::min(minCost, cost);
    }
    // Example: Check standard NOT
    else if (currentNode->gate == "NOT") {
        int cost = costLeft + 1; // Assuming C_NOT = 1
        minCost = std::min(minCost, cost);
    }

    // TODO: Insert your complex gate matching logic here!

    // 5. Store in memo table and return [1]
    memo[currentNode] = minCost;
    return minCost;
}