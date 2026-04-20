// calculateMinCost.cpp
#include "Node.h"
#include <iostream>
#include <unordered_map>
#include <climits>
#include <algorithm>

static bool isGate(Node* node, const std::string& gateType) {
    return node != nullptr && node->gate == gateType;
}

// If conversion produced NOT(NOT(X)), treat it as X for pattern matching
static Node* stripDoubleNot(Node* node) {
    if (node != nullptr &&
        node->gate == "NOT" &&
        node->children.size() == 1 &&
        node->children[0] != nullptr &&
        node->children[0]->gate == "NOT" &&
        node->children[0]->children.size() == 1) {
        return node->children[0]->children[0];
    }
    return node;
}

int calculateMinCost(Node* currentNode, std::unordered_map<Node*, int>& memo) {
    // Base cases
    if (currentNode == nullptr) return 0;
    if (currentNode->gate == "INPUT") return 0;

    // Memoization
    if (memo.find(currentNode) != memo.end()) {
        return memo[currentNode];
    }

    int minCost = INT_MAX;

    // --------------------------------------------------
    // Root = NOT
    // Possible library matches:
    //   NOT   : 2
    //   AND2  : 4   = NOT(NAND(x,y))
    //   NOR2  : 6   = NOT(NAND(NOT(x), NOT(y)))
    //   AOI21 : 7   = NOT(NAND(NAND(x,y), NOT(z))) or flipped
    //   AOI22 : 7   = NOT(NAND(NAND(x,y), NAND(z,w)))
    // --------------------------------------------------
    if (isGate(currentNode, "NOT") && currentNode->children.size() == 1) {
        Node* child = currentNode->children[0];

        // 1) Plain NOT
        minCost = std::min(minCost, 2 + calculateMinCost(child, memo));

        // Remaining patterns require child = NAND(...)
        if (isGate(child, "NAND") && child->children.size() == 2) {
            Node* left = child->children[0];
            Node* right = child->children[1];

            // 2) AND2 = NOT(NAND(x,y))
            minCost = std::min(
                minCost,
                4 + calculateMinCost(left, memo)
                  + calculateMinCost(right, memo)
            );

            // 3) NOR2 = NOT(NAND(NOT(x), NOT(y)))
            if (isGate(left, "NOT") && left->children.size() == 1 &&
                isGate(right, "NOT") && right->children.size() == 1) {
                minCost = std::min(
                    minCost,
                    6 + calculateMinCost(left->children[0], memo)
                      + calculateMinCost(right->children[0], memo)
                );
            }

            // Normalize possible NOT(NOT(X)) wrappers caused by conversion
            Node* leftNorm = stripDoubleNot(left);
            Node* rightNorm = stripDoubleNot(right);

            // 4) AOI21 = NOT(NAND(NAND(x,y), NOT(z))) or flipped
            if (isGate(leftNorm, "NAND") && leftNorm->children.size() == 2 &&
                isGate(rightNorm, "NOT") && rightNorm->children.size() == 1) {
                minCost = std::min(
                    minCost,
                    7 + calculateMinCost(leftNorm->children[0], memo)
                      + calculateMinCost(leftNorm->children[1], memo)
                      + calculateMinCost(rightNorm->children[0], memo)
                );
            }

            if (isGate(rightNorm, "NAND") && rightNorm->children.size() == 2 &&
                isGate(leftNorm, "NOT") && leftNorm->children.size() == 1) {
                minCost = std::min(
                    minCost,
                    7 + calculateMinCost(rightNorm->children[0], memo)
                      + calculateMinCost(rightNorm->children[1], memo)
                      + calculateMinCost(leftNorm->children[0], memo)
                );
            }

            // 5) AOI22 = NOT(NAND(NAND(x,y), NAND(z,w)))
            if (isGate(leftNorm, "NAND") && leftNorm->children.size() == 2 &&
                isGate(rightNorm, "NAND") && rightNorm->children.size() == 2) {
                minCost = std::min(
                    minCost,
                    7 + calculateMinCost(leftNorm->children[0], memo)
                      + calculateMinCost(leftNorm->children[1], memo)
                      + calculateMinCost(rightNorm->children[0], memo)
                      + calculateMinCost(rightNorm->children[1], memo)
                );
            }
        }
    }

    // --------------------------------------------------
    // Root = NAND
    // Possible library matches:
    //   NAND2 : 3   = NAND(x,y)
    //   OR2   : 4   = NAND(NOT(x), NOT(y))
    // --------------------------------------------------
    else if (isGate(currentNode, "NAND") && currentNode->children.size() == 2) {
        Node* left = currentNode->children[0];
        Node* right = currentNode->children[1];

        // 6) Plain NAND2
        minCost = std::min(
            minCost,
            3 + calculateMinCost(left, memo)
              + calculateMinCost(right, memo)
        );

        // 7) OR2 = NAND(NOT(x), NOT(y))
        if (isGate(left, "NOT") && left->children.size() == 1 &&
            isGate(right, "NOT") && right->children.size() == 1) {
            minCost = std::min(
                minCost,
                4 + calculateMinCost(left->children[0], memo)
                  + calculateMinCost(right->children[0], memo)
            );
        }
    }

    // Fallback for malformed/unexpected nodes
    if (minCost == INT_MAX) {
        std::cerr << "Warning: no valid library match for node "
                  << currentNode->name << " (" << currentNode->gate << ")\n";
        minCost = 0;
    }

    memo[currentNode] = minCost;
    return minCost;
}