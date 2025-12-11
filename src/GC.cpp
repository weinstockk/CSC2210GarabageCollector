// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: src/GC.cpp
// ----------------------------------

#include "../include/GC.h"
#include "../include/GCObject.h"
#include "../include/GCRefBase.h"

#include <algorithm>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

using namespace std;

bool GC::debug = false;

#define LOG(x) \
    do { if (GC::debug) { auto now = chrono::system_clock::now(); auto t = chrono::system_clock::to_time_t(now); cout << "[" << put_time(localtime(&t), "%H:%M:%S") << "] " << x << endl; } } while(0)

namespace {
    // Internal state
    enum class Phase { Idle, MarkRoots, Marking, Sweep };

    Phase phase = Phase::Idle;

    unordered_set<GCObject*> youngObjects;
    unordered_set<GCObject*> oldObjects;
    unordered_set<GCRefBase*> roots;

    // incremental state
    vector<GCObject*> markStack; // gray stack
    unordered_set<GCObject*>::iterator sweepIt;
    unordered_set<GCObject*>* sweepPool = nullptr; // pointer to current pool being swept
    bool sweepingOld = false;

    // Budgets / thresholds
    int markBudget = 20;
    int sweepBudget = 10;
    int allocationCounter = 0;
    int allocationThreshold = 100;
    int youngThreshold = 50;
    int promotedSurvivals = 2;

    int lastMinorCollected = 0;
    int lastMajorCollected = 0;
}

// Forward helpers
static void seedRoots();
static bool doMarkStep();
static bool doSweepStep();
static int blockingMark();
static int blockingSweep(unordered_set<GCObject*>& pool);
static void promoteObject(GCObject* obj);
static void adaptThresholds();

void GC::init(int markB, int sweepB, int allocThreshold, int youngThresh) {
    markBudget = markB;
    sweepBudget = sweepB;
    allocationThreshold = allocThreshold;
    youngThreshold = youngThresh;
    LOG("GC initialized: markBudget=" << markBudget << " sweepBudget=" << sweepBudget
        << " allocThreshold=" << allocationThreshold << " youngThreshold=" << youngThreshold);
}

void GC::registerObject(GCObject* obj) {
    if (!obj) return;
    youngObjects.insert(obj);

    // **drive collections from allocations**
    allocationCounter++;
    if (allocationCounter >= allocationThreshold) {
        allocationCounter = 0;
        startIncrementalCollect();
    }
}

void GC::registerRoot(GCRefBase* r) {
    if (!r) return;
    roots.insert(r);
}

void GC::unregisterRoot(GCRefBase* r) {
    roots.erase(r);
}

void GC::collectNow(bool major) {
    LOG("collectNow called (major=" << major << ")");
    if (major) {
        // Mark from roots (blocking)
        blockingMark();
        lastMajorCollected = blockingSweep(youngObjects) + blockingSweep(oldObjects);
        adaptThresholds();
    } else {
        blockingMark();
        lastMinorCollected = blockingSweep(youngObjects);
        // Handle promotions and clear old marks
        vector<GCObject*> survivors;
        survivors.reserve(youngObjects.size());
        for (auto* o : youngObjects) {
            if (o->marked) {
                o->survivalCount++;
                if (o->survivalCount >= promotedSurvivals) {
                    survivors.push_back(o);
                } else {
                    // clear mark/black for next cycle
                    o->marked = false;
                    o->black = false;
                }
            } else {
                // not marked => already freed by blockingSweep
            }
        }
        // Promote survivors
        for (GCObject* p : survivors) {
            if (youngObjects.erase(p) > 0) {
                oldObjects.insert(p);
                p->generation = Generation::Old;
                p->survivalCount = 0;
                p->marked = false;
                p->black = false;
            }
        }

        for (auto* o : oldObjects) { o->marked = false; o->black = false; }
        adaptThresholds();
    }
}

void GC::startIncrementalCollect() {
    if (phase != Phase::Idle) return;
    LOG("Starting incremental collect");
    phase = Phase::MarkRoots;
    markStack.clear();
    sweepingOld = false;
    sweepPool = &youngObjects;
    sweepIt = sweepPool->begin();
}

bool GC::incrementalCollectStep() {
    switch (phase) {
        case Phase::Idle:
            return true;
        case Phase::MarkRoots: {
            // Seed roots, then perform one marking unit immediately so
            // a single incrementalCollectStep() after startIncrementalCollect()
            // actually begins marking (useful for tests and simple driver loops).
            seedRoots();
            phase = Phase::Marking;

            // Do one mark step immediately (so roots can be blackened).
            {
                bool more = doMarkStep();
                if (!more) {
                    // If nothing left to mark, transition to sweep immediately.
                    sweepPool = &youngObjects;
                    sweepIt = sweepPool->begin();
                    sweepingOld = false;
                    phase = Phase::Sweep;
                }
            }
            return false;
        }
        case Phase::Marking: {
            bool more = doMarkStep();
            if (!more) {
                // transition to sweep
                sweepPool = &youngObjects;
                sweepIt = sweepPool->begin();
                sweepingOld = false;
                phase = Phase::Sweep;
            }
            return false;
        }
        case Phase::Sweep: {
            bool more = doSweepStep();
            if (!more) {
                if (!sweepingOld) {
                    // move to old sweep
                    sweepingOld = true;
                    sweepPool = &oldObjects;
                    sweepIt = sweepPool->begin();
                    // try to sweep old in same cycle (optional)
                    more = doSweepStep();
                }
                if (!more) {
                    phase = Phase::Idle;
                    LOG("Incremental collection finished");
                    adaptThresholds();
                    return true;
                }
            }
            return false;
        }
    }
    return true;
}


void GC::writeBarrier(GCObject* owner, GCObject* child) {
    if (!owner || !child) return;

    // Apply barrier if owner is marked (gray or black) and child is white
    if (owner->marked && !child->marked) {
        child->marked = true;       // gray
        markStack.push_back(child);
        LOG("writeBarrier: pushed child to markStack");
    }
}


void GC::setMarkBudget(int b) { markBudget = b; }
void GC::setSweepBudget(int b) { sweepBudget = b; }

// ----------------------
// Internal helpers
// ----------------------

static void seedRoots() {
    LOG("seedRoots: scanning roots (" << roots.size() << ")");
    for (GCRefBase* r : roots) {
        if (!r) continue;
        GCObject* obj = r->getObject();
        if (obj && !obj->marked) {
            obj->marked = true;   // gray
            markStack.push_back(obj);
        }
    }
    LOG("seedRoots pushed " << markStack.size() << " objects");
}

static bool doMarkStep() {
    int work = 0;
    while (!markStack.empty() && work < markBudget) {
        GCObject* obj = markStack.back();
        markStack.pop_back();

        // Gray -> Black: this object is now scanned
        obj->black = true;

        // trace children either via traceChildren() or via member refs by default
        vector<GCObject*> children;
        obj->traceChildren(children);
        for (GCObject* c : children) {
            if (c && !c->marked) {
                c->marked = true;   // gray
                markStack.push_back(c);
            }
        }
        ++work;
    }
    bool more = !markStack.empty();
    LOG("doMarkStep did " << work << " units; more=" << more);
    return more;
}

static bool doSweepStep() {
    if (!sweepPool) return false;
    int work = 0;
    // clamp iterator if invalid
    if (sweepIt == sweepPool->end()) sweepIt = sweepPool->begin();

    // We'll iterate up to sweepBudget objects in this pool.
    while (sweepIt != sweepPool->end() && work < sweepBudget) {
        GCObject* obj = *sweepIt;

        // If object is dead (not marked) -> reclaim
        if (!obj->marked) {
            // Null out refs pointing to obj across:
            //  - roots
            //  - member refs in other objects (both pools)
            for (GCRefBase* r : roots) {
                if (r) r->nullIfPointsTo(obj);
            }
            // member refs in young pool
            for (GCObject* o : youngObjects) {
                for (GCRefBase* mr : o->getMemberRefs()) {
                    if (mr) mr->nullIfPointsTo(obj);
                }
            }
            // member refs in old pool
            for (GCObject* o : oldObjects) {
                for (GCRefBase* mr : o->getMemberRefs()) {
                    if (mr) mr->nullIfPointsTo(obj);
                }
            }

            // erase and delete safely: advance iterator first
            auto itToErase = sweepIt++;
            sweepPool->erase(itToErase);
            delete obj;
            ++work;
            continue;
        } else {
            // Surviving object: clear mark/black for next cycle and handle promotion if in young pool
            if (!sweepingOld) {
                // We're sweeping youngObjects
                obj->survivalCount++;
                obj->marked = false; // clear gray
                obj->black = false;  // clear black (ready for next cycle)
                if (obj->survivalCount >= promotedSurvivals) {
                    // Promote:
                    // advance iterator then erase and move to oldObjects
                    GCObject* toPromote = obj;
                    auto itCur = sweepIt++;
                    sweepPool->erase(itCur);
                    oldObjects.insert(toPromote);
                    toPromote->generation = Generation::Old;
                    toPromote->survivalCount = 0;
                    toPromote->marked = false;
                    toPromote->black = false;
                    LOG("Promoted object during incremental sweep");
                    ++work;
                    continue;
                } else {
                    ++sweepIt;
                }
            } else {
                // Sweeping old generation: just clear mark/black
                obj->marked = false;
                obj->black = false;
                ++sweepIt;
            }
        }
        ++work;
    }

    bool more = (sweepIt != sweepPool->end());
    LOG("doSweepStep did " << work << " units; more=" << more << " (pool=" << (sweepingOld ? "old" : "young") << ")");
    return more;
}

static int blockingMark() {
    int markedCount = 0;
    // seed
    for (GCRefBase* r : roots) {
        if (!r) continue;
        GCObject* o = r->getObject();
        if (o && !o->marked) {
            o->marked = true;   // gray
            markStack.push_back(o);
        }
    }
    // drain stack
    while (!markStack.empty()) {
        GCObject* o = markStack.back();
        markStack.pop_back();

        // Gray -> Black
        o->black = true;

        vector<GCObject*> children;
        o->traceChildren(children);
        for (GCObject* c : children) {
            if (c && !c->marked) {
                c->marked = true;
                markStack.push_back(c);
            }
        }
        ++markedCount;
    }
    LOG("blockingMark marked " << markedCount << " objects");
    return markedCount;
}

static int blockingSweep(unordered_set<GCObject*>& pool) {
    int freed = 0;
    vector<GCObject*> dead;
    for (auto it = pool.begin(); it != pool.end();) {
        GCObject* o = *it;
        if (!o->marked) {
            dead.push_back(o);
            it = pool.erase(it);
            ++freed;
        } else {
            o->marked = false;
            o->black = false;
            ++it;
        }
    }
    for (GCObject* d : dead) {
        // null references in roots
        for (GCRefBase* r : roots) if (r) r->nullIfPointsTo(d);
        // null references in member refs across remaining objects
        for (GCObject* o : youngObjects) {
            for (GCRefBase* mr : o->getMemberRefs()) if (mr) mr->nullIfPointsTo(d);
        }
        for (GCObject* o : oldObjects) {
            for (GCRefBase* mr : o->getMemberRefs()) if (mr) mr->nullIfPointsTo(d);
        }
        delete d;
    }
    LOG("blockingSweep freed " << freed << " objects; remaining=" << pool.size());
    return freed;
}

static void promoteObject(GCObject* obj) {
    if (!obj) return;
    if (youngObjects.erase(obj) > 0) {
        oldObjects.insert(obj);
        obj->generation = Generation::Old;
        obj->survivalCount = 0;
        obj->marked = false;
        obj->black = false;
    }
}

static void adaptThresholds() {
    // crude adaptive logic: keep it simple
    if (lastMinorCollected < youngThreshold / 10 && youngThreshold < 2000) {
        youngThreshold = static_cast<int>(youngThreshold * 1.5);
    } else if (lastMinorCollected > youngThreshold / 2 && youngThreshold > 20) {
        youngThreshold = static_cast<int>(youngThreshold * 0.8);
    }
    // allocationThreshold adjust based on total size
    int total = static_cast<int>(youngObjects.size() + oldObjects.size() + roots.size());
    if (total > 1000 && allocationThreshold < 100000) allocationThreshold *= 2;
    LOG("adaptThresholds: youngThreshold=" << youngThreshold << " allocationThreshold=" << allocationThreshold);
}
