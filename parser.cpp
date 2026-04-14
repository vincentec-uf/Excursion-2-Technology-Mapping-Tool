// parser.cpp
#include "Node.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>

Node* buildTree(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open netlist file: " << filename << std::endl;
        return nullptr;
    }

    std::map<std::string, Node*> nodeMap;
    Node* outputRoot = nullptr;
    std::string outputName;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos) continue;

        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        if (tokens.empty()) continue;

        // INPUT or OUTPUT line
        if (tokens.size() == 2) {
            std::string var = tokens[0];
            std::string type = tokens[1];

            if (type == "INPUT") {
                if (nodeMap.find(var) == nodeMap.end()) {
                    Node* n = new Node{var, "INPUT", {}};
                    nodeMap[var] = n;
                }
            } 
            else if (type == "OUTPUT") {
                outputName = var;
                if (nodeMap.find(var) == nodeMap.end()) {
                    Node* n = new Node{var, "OUTPUT", {}};
                    nodeMap[var] = n;
                }
                outputRoot = nodeMap[var];
            }
        }
        // Gate definition: e.g. t1 = AND b c    or    F = OR t9 t10
        else if (tokens.size() >= 4 && tokens[1] == "=") {
            std::string nodeName = tokens[0];
            std::string gateType = tokens[2];

            Node* currentNode;
            if (nodeMap.find(nodeName) == nodeMap.end()) {
                currentNode = new Node{nodeName, gateType, {}};
                nodeMap[nodeName] = currentNode;
            } else {
                currentNode = nodeMap[nodeName];
                currentNode->gate = gateType;   // update if it was previously just OUTPUT
            }

            // Add children (1 for NOT, 2 for AND/OR)
            for (size_t i = 3; i < tokens.size(); ++i) {
                std::string childName = tokens[i];
                Node* child;
                if (nodeMap.find(childName) == nodeMap.end()) {
                    // forward reference safety (should not happen on legal input)
                    child = new Node{childName, "UNKNOWN", {}};
                    nodeMap[childName] = child;
                } else {
                    child = nodeMap[childName];
                }
                currentNode->children.push_back(child);
            }
        }
    }
    file.close();

    if (outputRoot == nullptr) {
        std::cerr << "Error: No OUTPUT node found in netlist." << std::endl;
        return nullptr;
    }

    std::cout << "Netlist parsed successfully. Output node: " << outputName << std::endl;
    return outputRoot;
}

// Simple tree printer
void printTree(Node* node, int depth = 0) {
    if (!node) return;
    std::string indent(depth * 2, ' ');
    std::cout << indent << node->name << " : " << node->gate;
    if (!node->children.empty()) {
        std::cout << " (";
        for (size_t i = 0; i < node->children.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << node->children[i]->name;
        }
        std::cout << ")";
    }
    std::cout << std::endl;

    for (Node* child : node->children) {
        printTree(child, depth + 1);
    }
}