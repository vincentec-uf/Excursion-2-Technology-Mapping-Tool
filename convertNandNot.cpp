// convertNandNot.cpp
#include "Node.h"
#include <iostream>

Node* convertNandNot(Node* original) {
	// Base cases
	if (original == nullptr) return nullptr;
	if(original->gate == "INPUT") {
		return new Node{original->name, "INPUT", {}};
	}
	// Recursive Step: Convert children first
	Node* leftChild = nullptr;
	Node* rightChild = nullptr;
	if (original->children.size() > 0) {
		leftChild = convertNandNot(original->children[0]);
	}
	if (original->children.size() > 1)
	{
		rightChild = convertNandNot(original->children[1]);
	}
	// Build new nodes based on the original gate type
	if (original->gate == "NOT") {
		return new Node{ original->name + "_NOT", "NOT", {leftChild} };
	}
	else if (original->gate == "AND") {
		Node* nandNode = new Node{ original->name + "_NAND", "NAND", {leftChild, rightChild} };
		return new Node{ original->name + "_NOT", "NOT", {nandNode}};
	}
	else if (original->gate == "OR") {
		Node* notLeft = new Node{ original->name + "_NOT_LEFT", "NOT", {leftChild} };
		Node* notRight = new Node{ original->name + "_NOT_RIGHT", "NOT", {rightChild} };
		 return new Node{ original->name + "_NAND", "NAND", {notLeft, notRight} };

	}
	else {
		std::cerr << "Error: Unsupported gate type: " << original->gate << std::endl;
		return nullptr;
	}
}
// helper function to prevent memory leaks
void deleteTree(Node* node) {
    if (node == nullptr) return;
    for (Node* child : node->children) {
        deleteTree(child);
    }
    delete node;
}