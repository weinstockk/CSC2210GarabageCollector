// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GC.h
// ----------------------------------

#ifndef TERMPROJECT_GC_H
#define TERMPROJECT_GC_H

#include <unordered_set>
#include <vector>

class GCObject;
class GCRefBase;

/**
 * GC - static-only incremental tri-color mark & sweep collector with
 * simple generational support.
 *
 * Typical usage:
 *   - GC::init(); // optional
 *   - create objects (derive from GCObject)
 *   - use GCRef<T> for member/root pointers
 *   - GC::startIncrementalCollect(); // begin incremental cycle
 *   - call GC::incrementalCollectStep() periodically until it returns true
 *
 * There is also a blocking fallback: GC::collectNow(major).
 */
class GC {
public:
    // init / tuning
    static void init(int markBudget = 20, int sweepBudget = 10, int allocThreshold = 100, int youngThresh = 50);

    // lifecycle
    static void registerObject(GCObject* obj);
    static void registerRoot(GCRefBase* r);
    static void unregisterRoot(GCRefBase* r);

    // Blocking collection (fallback)
    static void collectNow(bool major = false);

    // Incremental collection control:
    //   startIncrementalCollect() -> call incrementalCollectStep() repeatedly until it returns true.
    static void startIncrementalCollect();
    static bool incrementalCollectStep(); // returns true when fully finished (Idle)

    // Write barrier invoked on member pointer stores (owner != nullptr).
    static void writeBarrier(GCObject* owner, GCObject* child);

    // Tuning helpers (optional)
    static void setMarkBudget(int b);
    static void setSweepBudget(int b);

    // Debug
    static bool debug;
};

#endif // TERMPROJECT_GC_H
