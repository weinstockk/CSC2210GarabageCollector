// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: testobject/NodeNormal.h
// ----------------------------------

#ifndef TERMPROJECT_NODENORMAL_H
#define TERMPROJECT_NODENORMAL_H

#include <vector>

class NodeNormal final {
public:
    int id;
    NodeNormal* left;
    NodeNormal* right;
    std::vector<NodeNormal*> children;

    explicit NodeNormal(int id);
    ~NodeNormal();
};


#endif //TERMPROJECT_NODENORMAL_H