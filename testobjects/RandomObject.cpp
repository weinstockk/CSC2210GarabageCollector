//
// Created by weinstockk on 9/30/2025.
//

#include "RandomObject.h"

#include <iostream>

using namespace std;

RandomObject::RandomObject(int id, NodeGC* nodePtr = nullptr) : id(id), node(this, nodePtr) {
    cout << "Object " << id <<" created" << endl;
}

RandomObject::~RandomObject() {
    cout << "Object " << id <<" destroyed" << endl;
}

void RandomObject::printId() {
    cout << "Object " << id <<" printed" << endl;
}

GCRef<NodeGC> RandomObject::getNode() {
    return node;
}

void RandomObject::setNodeLeft(GCRef<NodeGC> leftNode) {
    node->left = leftNode.get();
}
