// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: src/GC.cpp
// ----------------------------------

#include "../include/GC.h"
#include <iostream>
#include <algorithm>

using namespace std;

// Static member definitions
vector<GCObject*> GC::objects;
vector<GCRefBase*> GC::refs;
int GC::allocatedCount = 0;
int GC::allocationThreshold = 10;

/**
 * @brief Registers a GC-managed object into the global tracking list.
 * @param obj Pointer to the @ref GCObject to track.
 */
void GC::registerObject(GCObject *obj) {
    objects.push_back(obj);
}

/**
 * @brief Registers a root reference and triggers collection if needed.
 * @param ref Pointer to a @ref GCRefBase instance.
 *
 * @details Each root increases the allocation counter. Once the threshold
 * is reached, a full collection is automatically performed.
 */
void GC::registerRef(GCRefBase* ref) {
    refs.push_back(ref);
    allocatedCount++;
    if (allocatedCount >= allocationThreshold) {
        collect();
        allocatedCount = 0;
    }
}

/**
 * @brief Unregisters a root reference.
 * @param ref Pointer to the reference being removed.
 */
void GC::unregisterRef(GCRefBase* ref) {
    refs.erase(remove(refs.begin(), refs.end(), ref), refs.end());
}

/**
 * @brief Performs a full mark-and-sweep garbage collection.
 */
void GC::collect() {
    cout << "GC Collecting..." << endl;
    mark();
    sweep();
}

/**
 * @brief Executes the mark phase of garbage collection.
 *
 * Marks all reachable objects by traversing from root references.
 */
void GC::mark() {
    int objectsMarked = 0;
    cout << "Mark phase: root count = " << refs.size()
         << ", tracked objects = " << objects.size() << endl;

    for (GCRefBase* ref : refs) {
        GCObject* obj = ref->getObject();
        if (obj && !obj->marked) {
            objectsMarked += markObject(obj);
        }
    }

    cout << "Total objects marked: " << objectsMarked << endl;
}

/**
 * @brief Recursively marks an object and its children.
 * @param obj Pointer to the object being marked.
 * @return Number of objects marked during traversal.
 */
int GC::markObject(GCObject *obj) {
    int markedObjects = 0;
    if (!obj || obj->marked)
        return 0;

    obj->marked = true;
    markedObjects++;

    // Recursively mark reachable objects
    for (GCRefBase* ref : obj->getRefs()) {
        if (!ref) continue;
        GCObject* child = ref->getObject();
        if (child && !child->marked) {
            markObject(child);
        }
    }
    return markedObjects;
}

/**
 * @brief Executes the sweep phase of garbage collection.
 *
 * Deletes all objects that were not marked during the mark phase,
 * then resets the mark flag for all remaining objects.
 */
void GC::sweep() {
    cout << "Sweeping unreachable objects\n";
    for (int i = 0; i < objects.size();) {
        GCObject* object = objects[i];
        if (!object->marked) {
            delete object;
            objects.erase(objects.begin() + i);
        } else {
            object->marked = false;
            i++;
        }
    }
    cout << "Sweep complete. Remaining objects = " << objects.size() << endl;
}
