// Node.h
#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>

struct Node {
    std::string name;           // e.g. "t1", "F", "a"
    std::string gate;           // "INPUT", "OUTPUT", "AND", "OR", "NOT"
    std::vector<Node*> children; // 0 children = INPUT, 1 = NOT, 2 = AND/OR
};

#endif