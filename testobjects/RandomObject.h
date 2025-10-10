//
// Created by weinstockk on 9/30/2025.
//

#ifndef TERMPROJECT_RANDOMOBJECT_H
#define TERMPROJECT_RANDOMOBJECT_H
#include "NodeGC.h"
#include "../gc/GCObject.h"


class RandomObject : public GCObject {
public:
    int id;
    GCRef<NodeGC> node;

    explicit RandomObject(int id, NodeGC* nodePtr);
    ~RandomObject() override;

    void printId();

    GCRef<NodeGC> getNode();
    void setNodeLeft(GCRef<NodeGC> leftNode);
    void clearNode() {node = nullptr;}
};


#endif //TERMPROJECT_RANDOMOBJECT_H