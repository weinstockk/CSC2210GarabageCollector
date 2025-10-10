// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: testobject/NodeGC.h
// ----------------------------------

#ifndef TERMPROJECT_NODE_H
#define TERMPROJECT_NODE_H

#include <vector>

#include "../gc/GCObject.h"
#include "../gc/GCRef.h"

class NodeGC final : public GCObject {
public:
    int id;
    GCRef<NodeGC> left;
    GCRef<NodeGC> right;
    std::vector<GCRef<NodeGC>> children;

    void addChild(NodeGC* child);

    explicit NodeGC(int id);
    ~NodeGC() override;
};


#endif //TERMPROJECT_NODE_H