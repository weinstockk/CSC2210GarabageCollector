// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: testobject/NodeGC.cpp
// ----------------------------------

#include "NodeGC.h"

#include <iostream>
#include <ostream>

using namespace std;

NodeGC::NodeGC(int id) : id(id), left(this, nullptr), right(this, nullptr) {
    cout << "Node " << id <<" created" << endl;
}

NodeGC::~NodeGC() {
    cout << "Node " << id <<" destroyed" << endl;
}

void NodeGC::addChild(NodeGC* child) {
    children.emplace_back(this, child);
}