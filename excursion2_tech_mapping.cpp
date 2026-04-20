// parser.cpp

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <unordered_map>
#include <climits>   // for INT_MAX
#include <algorithm> // for std::min

static const int COST_NOT = 2;
static const int COST_NAND2 = 3;
static const int COST_AND2 = 4;
static const int COST_NOR2 = 6;
static const int COST_OR2 = 4;
static const int COST_AOI21 = 7;
static const int COST_AOI22 = 7;

struct Node {
    std::string name;           // e.g. "t1", "F", "a"
    std::string gate;           // "INPUT", "OUTPUT", "AND", "OR", "NOT"
    std::vector<Node*> children; // 0 children = INPUT, 1 = NOT, 2 = AND/OR
};

Node *buildTree(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Cannot open netlist file: " << filename << std::endl;
        return nullptr;
    }

    std::map<std::string, Node *> nodeMap;
    Node *outputRoot = nullptr;
    std::string outputName;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.empty() || line.find_first_not_of(" \t") == std::string::npos)
            continue;

        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token)
        {
            tokens.push_back(token);
        }
        if (tokens.empty())
            continue;

        // INPUT or OUTPUT line
        if (tokens.size() == 2)
        {
            std::string var = tokens[0];
            std::string type = tokens[1];

            if (type == "INPUT")
            {
                if (nodeMap.find(var) == nodeMap.end())
                {
                    Node *n = new Node{var, "INPUT", {}};
                    nodeMap[var] = n;
                }
            }
            else if (type == "OUTPUT")
            {
                outputName = var;
                if (nodeMap.find(var) == nodeMap.end())
                {
                    Node *n = new Node{var, "OUTPUT", {}};
                    nodeMap[var] = n;
                }
                outputRoot = nodeMap[var];
            }
        }
        // Gate definition: e.g. t1 = AND b c    or    F = OR t9 t10
        else if (tokens.size() >= 4 && tokens[1] == "=")
        {
            std::string nodeName = tokens[0];
            std::string gateType = tokens[2];

            Node *currentNode;
            if (nodeMap.find(nodeName) == nodeMap.end())
            {
                currentNode = new Node{nodeName, gateType, {}};
                nodeMap[nodeName] = currentNode;
            }
            else
            {
                currentNode = nodeMap[nodeName];
                currentNode->gate = gateType; // update if it was previously just OUTPUT
            }

            // Add children (1 for NOT, 2 for AND/OR)
            for (size_t i = 3; i < tokens.size(); ++i)
            {
                std::string childName = tokens[i];
                Node *child;
                if (nodeMap.find(childName) == nodeMap.end())
                {
                    // forward reference safety (should not happen on legal input)
                    child = new Node{childName, "UNKNOWN", {}};
                    nodeMap[childName] = child;
                }
                else
                {
                    child = nodeMap[childName];
                }
                currentNode->children.push_back(child);
            }
        }
    }
    file.close();

    if (outputRoot == nullptr)
    {
        std::cerr << "Error: No OUTPUT node found in netlist." << std::endl;
        return nullptr;
    }

    std::cout << "Netlist parsed successfully. Output node: " << outputName << std::endl;
    return outputRoot;
}

// Simple tree printer
void printTree(Node *node, int depth = 0)
{
    if (!node)
        return;
    std::string indent(depth * 2, ' ');
    std::cout << indent << node->name << " : " << node->gate;
    if (!node->children.empty())
    {
        std::cout << " (";
        for (size_t i = 0; i < node->children.size(); ++i)
        {
            if (i > 0)
                std::cout << ", ";
            std::cout << node->children[i]->name;
        }
        std::cout << ")";
    }
    std::cout << std::endl;

    for (Node *child : node->children)
    {
        printTree(child, depth + 1);
    }
}
// helper function to prevent memory leaks
void deleteTree(Node *node)
{
    if (node == nullptr)
        return;
    for (Node *child : node->children)
    {
        deleteTree(child);
    }
    delete node;
}
Node *convertNandNot(Node *original)
{
    // Base cases
    if (original == nullptr)
        return nullptr;
    if (original->gate == "INPUT")
    {
        return new Node{original->name, "INPUT", {}};
    }
    // Recursive Step: Convert children first
    Node *leftChild = nullptr;
    Node *rightChild = nullptr;
    if (original->children.size() > 0)
    {
        leftChild = convertNandNot(original->children[0]);
    }
    if (original->children.size() > 1)
    {
        rightChild = convertNandNot(original->children[1]);
    }
    // Build new nodes based on the original gate type
    if (original->gate == "NOT")
    {
        return new Node{original->name + "_NOT", "NOT", {leftChild}};
    }
    else if (original->gate == "AND")
    {
        Node *nandNode = new Node{original->name + "_NAND", "NAND", {leftChild, rightChild}};
        return new Node{original->name + "_NOT", "NOT", {nandNode}};
    }
    else if (original->gate == "OR")
    {
        Node *notLeft = new Node{original->name + "_NOT_LEFT", "NOT", {leftChild}};
        Node *notRight = new Node{original->name + "_NOT_RIGHT", "NOT", {rightChild}};
        return new Node{original->name + "_NAND", "NAND", {notLeft, notRight}};
    }
    else
    {
        std::cerr << "Error: Unsupported gate type: " << original->gate << std::endl;
        return nullptr;
    }
}

// calculateMinCost.cpp

static bool isGate(Node *node, const std::string &gateType)
{
    return node != nullptr && node->gate == gateType;
}

// If conversion produced NOT(NOT(X)), treat it as X for pattern matching
static Node *stripDoubleNot(Node *node)
{
    if (node != nullptr &&
        node->gate == "NOT" &&
        node->children.size() == 1 &&
        node->children[0] != nullptr &&
        node->children[0]->gate == "NOT" &&
        node->children[0]->children.size() == 1)
    {
        return node->children[0]->children[0];
    }
    return node;
}

int calculateMinCost(Node *currentNode, std::unordered_map<Node *, int> &memo)
{
    // Base cases
    if (currentNode == nullptr)
        return 0;
    if (currentNode->gate == "INPUT")
        return 0;

    // Memoization
    if (memo.find(currentNode) != memo.end())
    {
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
    if (isGate(currentNode, "NOT") && currentNode->children.size() == 1)
    {
        Node *child = currentNode->children[0];

        // 1) Plain NOT
        minCost = std::min(minCost, 2 + calculateMinCost(child, memo));

        // Remaining patterns require child = NAND(...)
        if (isGate(child, "NAND") && child->children.size() == 2)
        {
            Node *left = child->children[0];
            Node *right = child->children[1];

            // 2) AND2 = NOT(NAND(x,y))
            minCost = std::min(
                minCost,
                4 + calculateMinCost(left, memo) + calculateMinCost(right, memo));

            // 3) NOR2 = NOT(NAND(NOT(x), NOT(y)))
            if (isGate(left, "NOT") && left->children.size() == 1 &&
                isGate(right, "NOT") && right->children.size() == 1)
            {
                minCost = std::min(
                    minCost,
                    6 + calculateMinCost(left->children[0], memo) + calculateMinCost(right->children[0], memo));
            }

            // Normalize possible NOT(NOT(X)) wrappers caused by conversion
            Node *leftNorm = stripDoubleNot(left);
            Node *rightNorm = stripDoubleNot(right);

            // 4) AOI21 = NOT(NAND(NAND(x,y), NOT(z))) or flipped
            if (isGate(leftNorm, "NAND") && leftNorm->children.size() == 2 &&
                isGate(rightNorm, "NOT") && rightNorm->children.size() == 1)
            {
                minCost = std::min(
                    minCost,
                    7 + calculateMinCost(leftNorm->children[0], memo) + calculateMinCost(leftNorm->children[1], memo) + calculateMinCost(rightNorm->children[0], memo));
            }

            if (isGate(rightNorm, "NAND") && rightNorm->children.size() == 2 &&
                isGate(leftNorm, "NOT") && leftNorm->children.size() == 1)
            {
                minCost = std::min(
                    minCost,
                    7 + calculateMinCost(rightNorm->children[0], memo) + calculateMinCost(rightNorm->children[1], memo) + calculateMinCost(leftNorm->children[0], memo));
            }

            // 5) AOI22 = NOT(NAND(NAND(x,y), NAND(z,w)))
            if (isGate(leftNorm, "NAND") && leftNorm->children.size() == 2 &&
                isGate(rightNorm, "NAND") && rightNorm->children.size() == 2)
            {
                minCost = std::min(
                    minCost,
                    7 + calculateMinCost(leftNorm->children[0], memo) + calculateMinCost(leftNorm->children[1], memo) + calculateMinCost(rightNorm->children[0], memo) + calculateMinCost(rightNorm->children[1], memo));
            }
        }
    }

    // --------------------------------------------------
    // Root = NAND
    // Possible library matches:
    //   NAND2 : 3   = NAND(x,y)
    //   OR2   : 4   = NAND(NOT(x), NOT(y))
    // --------------------------------------------------
    else if (isGate(currentNode, "NAND") && currentNode->children.size() == 2)
    {
        Node *left = currentNode->children[0];
        Node *right = currentNode->children[1];

        // 6) Plain NAND2
        minCost = std::min(
            minCost,
            3 + calculateMinCost(left, memo) + calculateMinCost(right, memo));

        // 7) OR2 = NAND(NOT(x), NOT(y))
        if (isGate(left, "NOT") && left->children.size() == 1 &&
            isGate(right, "NOT") && right->children.size() == 1)
        {
            minCost = std::min(
                minCost,
                4 + calculateMinCost(left->children[0], memo) + calculateMinCost(right->children[0], memo));
        }
    }

    // Fallback for malformed/unexpected nodes
    if (minCost == INT_MAX)
    {
        std::cerr << "Warning: no valid library match for node "
                  << currentNode->name << " (" << currentNode->gate << ")\n";
        minCost = 0;
    }

    memo[currentNode] = minCost;
    return minCost;
}

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

    std::cout << "\n=== FINAL MIN COST (placeholder only) ===\n";
    std::cout << minCost << std::endl;

    std::ofstream out("output.txt");
    if (out) {
        out << minCost << std::endl;
        std::cout << "output.txt created with cost: " << minCost << std::endl;
    }

    return 0;
}