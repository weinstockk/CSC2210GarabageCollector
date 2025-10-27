// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GC.h
// ----------------------------------

#ifndef TERMPROJECT_GC_H
#define TERMPROJECT_GC_H

#pragma once

#include <unordered_set>

#include "GCObject.h"
#include "GCRefBase.h"

class GC {
public:
    /**
     * @brief Registers a newly allocated GC-managed object.
     * @param obj Pointer to the @ref GCObject to track.
     */
    static void registerObject(GCObject* obj);

    /**
     * @brief Registers a root reference (global or stack-level @ref GCRefBase).
     * @param ref Pointer to the root reference.
     * @note Roots are starting points for reachability during GC.
     */
    static void registerRef(GCRefBase* ref);

    /**
     * @brief Unregisters a root reference when it goes out of scope.
     * @param ref Pointer to the reference being unregistered.
     */
    static void unregisterRef(GCRefBase* ref);

    /**
     * @brief Performs a full garbage collection (mark + sweep).
     * @details
     * - **Mark phase:** Traverses all roots and marks reachable objects.
     * - **Sweep phase:** Frees all unmarked (unreachable) objects.
     */
    static void collect(bool major = false);

    static void collectMinor();

    static void collectMajor();

private:

    /** @brief Recursively marks reachable objects starting from roots. */
    static void mark(const std::unordered_set<GCRefBase*>& refs);

    /**
     * @brief Marks a specific object and its transitive references.
     * @param obj Pointer to the object to mark.
     * @return The number of objects marked from this call.
     */
    static int markObject(GCObject *obj);

    /** @brief Deletes unmarked objects and resets the mark flags. */
    static int sweep(std::unordered_set<GCObject*>& pool);

    static void promote(GCObject *obj);

    static void adaptThresholds();

    static std::unordered_set<GCObject*> youngObjects;
    static std::unordered_set<GCObject*> oldObjects;

    /** @brief List of all root references (GCRefBase). */
    static std::unordered_set<GCRefBase*> refs;

    /** @brief Counter for allocations since last collection. */
    static int allocatedCount;

    /** @brief Number of allocations required to trigger collection. */
    static int allocationThreshold;

    static int youngThreshold;
    static int promotedSurvivals;
};

#endif // TERMPROJECT_GC_H
