// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: testobject/NodeNormal.cpp
// ----------------------------------

#include "NodeNormal.h"

#include <iostream>
#include <ostream>

using namespace std;

NodeNormal::NodeNormal(int id) : id(id), left(nullptr), right(nullptr) {
    cout << "Node " << id <<" created" << endl;
}

NodeNormal::~NodeNormal() {
    cout << "Node " << id <<" destroyed" << endl;
}