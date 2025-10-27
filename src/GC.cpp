// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: src/GC.cpp
// ----------------------------------

#include "../include/GC.h"
#include <iostream>
#include <unordered_set>

using namespace std;

int GC::allocatedCount = 0;
int GC::allocationThreshold = 10;
int GC::youngThreshold = 10;
int GC::promotedSurvivals = 2;

int lastMinorCollected = 0;
int lastMajorCollected = 0;

/**
 * @brief Registers a GC-managed object into the global tracking list.
 * @param obj Pointer to the @ref GCObject to track.
 */
void GC::registerObject(GCObject *obj) {
    youngObjects.insert(obj);
}

/**
 * @brief Registers a root reference and triggers collection if needed.
 * @param ref Pointer to a @ref GCRefBase instance.
 *
 * @details Each root increases the allocation counter. Once the threshold
 * is reached, a full collection is automatically performed.
 */
void GC::registerRef(GCRefBase* ref) {
    refs.insert(ref);
    allocatedCount++;
    if (allocatedCount >= allocationThreshold) {
        if (youngObjects.size() >= youngThreshold) {
            collect(false);
        } else {
            collect(true);
        }
        allocatedCount = 0;
    }
}

/**
 * @brief Unregisters a root reference.
 * @param ref Pointer to the reference being removed.
 */
void GC::unregisterRef(GCRefBase* ref) {
    refs.erase(ref);
}

/**
 * @brief Performs a full mark-and-sweep garbage collection.
 */
void GC::collect(bool major) {
    if (major) {
        collectMajor();
    } else {
        collectMinor();
    }
}

void GC::collectMinor() {
    cout << "Minor GC Collecting..." << endl;
    mark(refs);
    lastMinorCollected = sweep(youngObjects);

    for (auto* obj : youngObjects) {
        if (obj->marked) {
            obj->survivalCount++;
            if (obj->survivalCount >= promotedSurvivals) {
                promote(obj);
            }
        }
    }

    for (auto* obj : oldObjects)
        obj->marked = false;

    for (auto it = youngObjects.begin(); it != youngObjects.end(); ) {
        if ((*it)->generation == Generation::Old) {
            it = youngObjects.erase(it);
        } else {
            ++it;
        }
    }


    cout << "Minor GC collected " << lastMinorCollected << " objects." << endl;
    adaptThresholds();
}

void GC::collectMajor() {
    cout << "Major GC Collecting..." << endl;
    mark(refs);
    lastMajorCollected = sweep(youngObjects) + sweep(oldObjects);
    cout << "Major GC collected " << lastMajorCollected << " objects." << endl;
    adaptThresholds();
}

void GC::adaptThresholds() {
    // How aggressive the GC is
    double growthFactor = 1.5;
    double shrinkFactor = 0.75;

    if (lastMinorCollected < youngThreshold * 0.1 && youngThreshold < 2000) {
        youngThreshold = static_cast<int>(youngThreshold * growthFactor);
        cout << "Increasing young threshold to " << youngThreshold << endl;
    }
    else if (lastMinorCollected > youngThreshold * 0.5 && youngThreshold > 50) {
        youngThreshold = static_cast<int>(youngThreshold * shrinkFactor);
        cout << "Decreasing young threshold to " << youngThreshold << endl;
    }

    if (lastMajorCollected > 0) {
        int totalObjects = youngObjects.size() + oldObjects.size() + refs.size();
        double ratio = static_cast<double>(totalObjects) / static_cast<double>(youngObjects.size());
        if (ratio > 0.5 && allocationThreshold > 200) {
            allocationThreshold = static_cast<int>(allocationThreshold * shrinkFactor);
        }
        else if (ratio < 1.0 && allocationThreshold < 5000) {
            allocationThreshold = static_cast<int>(allocationThreshold * growthFactor);
        }

        cout << "Changed Allocation threshold to " << allocationThreshold << endl;
    }
}

/**
 * @brief Executes the mark phase of garbage collection.
 *
 * Marks all reachable objects by traversing from root references.
 */
void GC::mark(const unordered_set<GCRefBase*>& roots) {
    int objectsMarked = 0;
    cout << "Mark phase: root count = " << refs.size()
         << ", tracked objects = " << youngObjects.size() + oldObjects.size() << endl;

    for (GCRefBase* ref : roots) {
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
int GC::sweep(unordered_set<GCObject*>& pool) {
    int freed = 0;
    cout << "Sweeping unreachable objects\n";

    for (auto it = pool.begin(); it != pool.end(); ) {
        GCObject* object = *it;
        if (!object->marked) {
            for (GCRefBase* ref : refs) {
                ref->nullIfPointsTo(object);
            }
            for (GCObject* obj : pool) {
                for (GCRefBase* memberRef : obj->getRefs()) {
                    memberRef->nullIfPointsTo(object);
                }
            }

            delete object;
            it = pool.erase(it);
            freed++;
        } else {
            object->marked = false;
            ++it;
        }
    }

    cout << "Sweep complete. Remaining objects = " << pool.size() << endl;
    return freed;
}


void GC::promote(GCObject *obj) {
    youngObjects.erase(obj);
    obj->generation = Generation::Old;
    obj->survivalCount = 0;
    oldObjects.insert(obj);
}
