// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: src/GC.cpp
// ----------------------------------

#include "../include/GC.h"
#include <unordered_set>

using namespace std;

unordered_set<GCObject*> GC::youngObjects;
unordered_set<GCObject*> GC::oldObjects;
unordered_set<GCRefBase*> GC::refs;

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
    GC_LOG("Minor GC Collecting...");
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

    GC_LOG("Minor GC collected " << lastMinorCollected << " objects.");
    adaptThresholds();
}

void GC::collectMajor() {
    GC_LOG("Major GC Collecting...");
    mark(refs);
    lastMajorCollected = sweep(youngObjects) + sweep(oldObjects);
    GC_LOG("Major GC collected " << lastMajorCollected << " objects.")
    adaptThresholds();
}

void GC::adaptThresholds() {
    // How aggressive the GC is
    double growthFactor = 1.5;
    double shrinkFactor = 0.75;

    if (lastMinorCollected < youngThreshold * 0.1 && youngThreshold < 2000) {
        youngThreshold = static_cast<int>(youngThreshold * growthFactor);
        GC_LOG("Increasing young threshold to " << youngThreshold);
    }
    else if (lastMinorCollected > youngThreshold * 0.5 && youngThreshold > 50) {
        youngThreshold = static_cast<int>(youngThreshold * shrinkFactor);
        GC_LOG("Decreasing young threshold to " << youngThreshold);
    }

    if (lastMajorCollected > 0) {
        int totalObjects = youngObjects.size() + oldObjects.size() + refs.size();
        if (youngObjects.empty()) return;
        double ratio = static_cast<double>(totalObjects) / static_cast<double>(youngObjects.size());
        if (ratio > 0.5 && allocationThreshold > 200) {
            allocationThreshold = static_cast<int>(allocationThreshold * shrinkFactor);
        }
        else if (ratio < 1.0 && allocationThreshold < 5000) {
            allocationThreshold = static_cast<int>(allocationThreshold * growthFactor);
        }

        GC_LOG("Changed Allocation threshold to " << allocationThreshold);
    }
}

/**
 * @brief Executes the mark phase of garbage collection.
 *
 * Marks all reachable objects by traversing from root references.
 */
void GC::mark(const unordered_set<GCRefBase*>& roots) {
    int objectsMarked = 0;
    GC_LOG("Mark phase: root count = " << refs.size() << ", tracked objects = " << youngObjects.size() + oldObjects.size());

    for (GCRefBase* ref : roots) {
        GCObject* obj = ref->getObject();
        if (obj && !obj->marked) {
            objectsMarked += markObject(obj);
        }
    }

    GC_LOG("Total objects marked: " << objectsMarked);
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
    GC_LOG("Sweeping unreachable objects");
    vector<GCObject*> deadObjects;
    for (auto it = pool.begin(); it != pool.end(); ) {
        if (!(*it)->marked) {
            deadObjects.push_back(*it);
            delete *it;
            it = pool.erase(it);
            freed++;
        } else {
            (*it)->marked = false;
            ++it;
        }
    }

    for (GCObject* dead : deadObjects) {
        for (GCRefBase* ref : refs) { ref->nullIfPointsTo(dead); }
    }

    GC_LOG("Sweep complete. Remaining objects = " << pool.size());
    return freed;
}


void GC::promote(GCObject* obj) {
    if (!obj) return;
    youngObjects.erase(obj);
    oldObjects.insert(obj);
    obj->generation = Generation::Old;
    obj->survivalCount = 0;
    GC_LOG("Promoted object to old generation.");
}
