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
            seedRoots();
            phase = Phase::Marking;

            {
                bool more = doMarkStep();
                if (!more) {
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
                    sweepingOld = true;
                    sweepPool = &oldObjects;
                    sweepIt = sweepPool->begin();
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

    if (owner->marked && !child->marked) {
        child->marked = true;
        markStack.push_back(child);
        LOG("writeBarrier: pushed child to markStack");
    }
}


void GC::setMarkBudget(int b) { markBudget = b; }
void GC::setSweepBudget(int b) { sweepBudget = b; }

static void seedRoots() {
    LOG("seedRoots: scanning roots (" << roots.size() << ")");
    for (GCRefBase* r : roots) {
        if (!r) continue;
        GCObject* obj = r->getObject();
        if (obj && !obj->marked) {
            obj->marked = true;
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
        obj->black = true;

        vector<GCObject*> children;
        obj->traceChildren(children);
        for (GCObject* c : children) {
            if (c && !c->marked) {
                c->marked = true;
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
    if (sweepIt == sweepPool->end()) sweepIt = sweepPool->begin();

    while (sweepIt != sweepPool->end() && work < sweepBudget) {
        GCObject* obj = *sweepIt;
        if (!obj->marked) {
            for (GCRefBase* r : roots) {
                if (r) r->nullIfPointsTo(obj);
            }
            for (GCObject* o : youngObjects) {
                for (GCRefBase* mr : o->getMemberRefs()) {
                    if (mr) mr->nullIfPointsTo(obj);
                }
            }
            for (GCObject* o : oldObjects) {
                for (GCRefBase* mr : o->getMemberRefs()) {
                    if (mr) mr->nullIfPointsTo(obj);
                }
            }

            auto itToErase = sweepIt++;
            sweepPool->erase(itToErase);
            delete obj;
            ++work;
            continue;
        } else {

            if (!sweepingOld) {
                obj->survivalCount++;
                obj->marked = false;
                obj->black = false;
                if (obj->survivalCount >= promotedSurvivals) {
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
    for (GCRefBase* r : roots) {
        if (!r) continue;
        GCObject* o = r->getObject();
        if (o && !o->marked) {
            o->marked = true;
            markStack.push_back(o);
        }
    }
    while (!markStack.empty()) {
        GCObject* o = markStack.back();
        markStack.pop_back();
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
        for (GCRefBase* r : roots) if (r) r->nullIfPointsTo(d);
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
    if (lastMinorCollected < youngThreshold / 10 && youngThreshold < 2000) {
        youngThreshold = static_cast<int>(youngThreshold * 1.5);
    } else if (lastMinorCollected > youngThreshold / 2 && youngThreshold > 20) {
        youngThreshold = static_cast<int>(youngThreshold * 0.8);
    }
    int total = static_cast<int>(youngObjects.size() + oldObjects.size() + roots.size());
    if (total > 1000 && allocationThreshold < 100000) allocationThreshold *= 2;
    LOG("adaptThresholds: youngThreshold=" << youngThreshold << " allocationThreshold=" << allocationThreshold);
}
