// excursion2_tech_mapping.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <unordered_map>
#include <climits>
#include <algorithm>

static const int COST_NOT = 2;
static const int COST_NAND2 = 3;
static const int COST_AND2 = 4;
static const int COST_NOR2 = 6;
static const int COST_OR2 = 4;
static const int COST_AOI21 = 7;
static const int COST_AOI22 = 7;

struct Node {
    std::string name;               // e.g. "t1", "F", "a"
    std::string gate;               // "INPUT", "OUTPUT", "AND", "OR", "NOT"
    std::vector<Node*> children;    // 0 children = INPUT, 1 = NOT, 2 = AND/OR
};

Node* buildTree(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open netlist file: " << filename << std::endl;
        return nullptr;
    }

    std::map<std::string, Node*> nodeMap;
    Node* outputRoot = nullptr;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) continue;

        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) tokens.push_back(token);
        if (tokens.empty()) continue;

        // INPUT or OUTPUT line
        if (tokens.size() == 2) {
            std::string var = tokens[0];
            std::string type = tokens[1];

            if (type == "INPUT") {
                if (nodeMap.find(var) == nodeMap.end()) {
                    nodeMap[var] = new Node{var, "INPUT", {}};
                }
            } else if (type == "OUTPUT") {
                if (nodeMap.find(var) == nodeMap.end()) {
                    nodeMap[var] = new Node{var, "UNKNOWN", {}};
                }
                outputRoot = nodeMap[var];
            }
        }
        // Gate definition: e.g. t1 = AND b c    or    F = OR t9 t10
        else if (tokens.size() >= 3 && tokens[1] == "=") {
            std::string nodeName = tokens[0];

            Node* currentNode;
            if (nodeMap.find(nodeName) == nodeMap.end()) {
                currentNode = new Node{nodeName, "UNKNOWN", {}};
                nodeMap[nodeName] = currentNode;
            } else {
                currentNode = nodeMap[nodeName];
            }

            // Case 1: alias (e.g. F = t5) — no gate, just copy child
            if (tokens.size() == 3) {
                std::string childName = tokens[2];
                if (nodeMap.find(childName) == nodeMap.end()) {
                    nodeMap[childName] = new Node{childName, "UNKNOWN", {}};
                }
                currentNode->children.push_back(nodeMap[childName]);
                if (currentNode->gate == "UNKNOWN") currentNode->gate = "WIRE"; // internal flag
            }
            // Case 2: normal gate (e.g. t1 = AND b c)
            else {
                std::string gateType = tokens[2];
                currentNode->gate = gateType;

                for (size_t i = 3; i < tokens.size(); ++i) {
                    std::string childName = tokens[i];
                    if (nodeMap.find(childName) == nodeMap.end()) {
                        nodeMap[childName] = new Node{childName, "UNKNOWN", {}};
                    }
                    currentNode->children.push_back(nodeMap[childName]);
                }
            }
        }
    }
    file.close();

    if (outputRoot == nullptr) {
        std::cerr << "Error: No OUTPUT node found" << std::endl;
        return nullptr;
    }

    std::cout << "Netlist parsed successfully. Output node: " << outputRoot->name << std::endl;
    return outputRoot;
}

Node* convertNandNot(Node* original) 
{
    // Base cases
    if (!original) return nullptr;
    if (original->gate == "INPUT") {
        return new Node{original->name, "INPUT", {}};
    }
    
    // Recursive Step: Convert children first
    Node* left = nullptr;
    Node* right = nullptr;
    if (!original->children.empty()) left = convertNandNot(original->children[0]);
    if (original->children.size() > 1) right = convertNandNot(original->children[1]);
    
    // Build new nodes based on the original gate type
    if (original->gate == "NOT") {
        return new Node{original->name + "_NOT", "NOT", {left}};
    } else if (original->gate == "AND") {
        Node* nand = new Node{original->name + "_NAND", "NAND", {left, right}};
        return new Node{original->name + "_NOT", "NOT", {nand}};
    } else if (original->gate == "OR") {
        Node* notL = new Node{original->name + "_NOT_LEFT", "NOT", {left}};
        Node* notR = new Node{original->name + "_NOT_RIGHT", "NOT", {right}};
        return new Node{original->name + "_NAND", "NAND", {notL, notR}};
    } else if (original->gate == "WIRE" || original->gate == "UNKNOWN") {
        // alias case
        return left ? left : nullptr;
    }

    std::cerr << "Error: Unsupported gate type: " << original->gate << std::endl;
    return nullptr;
}

static bool isGate(Node* node, const std::string& g) {
    return node && node->gate == g;
}

int calculateMinCost(Node* node, std::unordered_map<Node*, int>& memo) {
    if (!node || node->gate == "INPUT") return 0;
    if (memo.count(node)) return memo[node];

    int best = INT_MAX;
    int c0 = 0, c1 = 0;
    if (!node->children.empty()) c0 = calculateMinCost(node->children[0], memo);
    if (node->children.size() > 1) c1 = calculateMinCost(node->children[1], memo);

    // --------------------------------------------------
    // Root = NOT
    // Possible library matches:
    //   NOT   : 2
    //   AND2  : 4   = NOT(NAND(x,y))
    //   NOR2  : 6   = NOT(NAND(NOT(x), NOT(y)))
    //   AOI21 : 7   = NOT(NAND(NAND(x,y), NOT(z))) or flipped
    //   AOI22 : 7   = NOT(NAND(NAND(x,y), NAND(z,w)))
    // --------------------------------------------------
    if (isGate(node, "NOT") && node->children.size() == 1) {
        best = std::min(best, COST_NOT + c0);

        Node* child = node->children[0];
        if (isGate(child, "NAND") && child->children.size() == 2) {
            Node* l = child->children[0];
            Node* r = child->children[1];

            best = std::min(best, COST_AND2 + calculateMinCost(l, memo) + calculateMinCost(r, memo));

            if (isGate(l, "NOT") && isGate(r, "NOT")) {
                best = std::min(best, COST_NOR2 + calculateMinCost(l->children[0], memo) + calculateMinCost(r->children[0], memo));
            }
            if (isGate(l, "NAND") && isGate(r, "NOT")) {
                best = std::min(best, COST_AOI21 + calculateMinCost(l->children[0], memo) + calculateMinCost(l->children[1], memo) + calculateMinCost(r->children[0], memo));
            }
            if (isGate(r, "NAND") && isGate(l, "NOT")) {
                best = std::min(best, COST_AOI21 + calculateMinCost(r->children[0], memo) + calculateMinCost(r->children[1], memo) + calculateMinCost(l->children[0], memo));
            }
            if (isGate(l, "NAND") && isGate(r, "NAND")) {
                best = std::min(best, COST_AOI22 + calculateMinCost(l->children[0], memo) + calculateMinCost(l->children[1], memo) + calculateMinCost(r->children[0], memo) + calculateMinCost(r->children[1], memo));
            }
        }
    } 
    
    // --------------------------------------------------
    // Root = NAND
    // Possible library matches:
    //   NAND2 : 3   = NAND(x,y)
    //   OR2   : 4   = NAND(NOT(x), NOT(y))
    // --------------------------------------------------
    else if (isGate(node, "NAND") && node->children.size() == 2) {
        best = std::min(best, COST_NAND2 + c0 + c1);

        Node* l = node->children[0];
        Node* r = node->children[1];
        if (isGate(l, "NOT") && isGate(r, "NOT")) {
            best = std::min(best, COST_OR2 + calculateMinCost(l->children[0], memo) + calculateMinCost(r->children[0], memo));
        }
    }

    // Fallback for malformed/unexpected nodes
    if (best == INT_MAX) best = 0;

    memo[node] = best;
    return best;
}

int main() {
    Node* original = buildTree("input.txt");
    if (!original) return 1;

    Node* nandRoot = convertNandNot(original);
    if (!nandRoot) return 1;

    std::unordered_map<Node*, int> memo;
    int minCost = calculateMinCost(nandRoot, memo);

    std::cout << "=== FINAL MIN COST ===\n" << minCost << std::endl;

    std::ofstream out("output.txt");
    if (out) {
        out << minCost << std::endl;
        std::cout << "output.txt created with cost: " << minCost << std::endl;
    }

    return 0;
}