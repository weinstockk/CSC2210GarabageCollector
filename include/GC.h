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
 * @file GC.h
 * @brief Defines the static garbage collector interface.
 */

/**
 * @class GC
 * @brief Static incremental tri-color mark-and-sweep garbage collector.
 *
 * The collector supports incremental collection with optional generational
 * behavior. Objects must derive from GCObject, and references must be managed
 * through GCRef<T>.
 */
class GC {
public:
    /**
     * @brief Initializes the garbage collector.
     *
     * @param markBudget Maximum number of objects marked per step.
     * @param sweepBudget Maximum number of objects swept per step.
     * @param allocThreshold Allocation count before triggering GC.
     * @param youngThresh Survivals before promotion to old generation.
     */
    static void init(int markBudget = 20,
                     int sweepBudget = 10,
                     int allocThreshold = 100,
                     int youngThresh = 50);

    /**
     * @brief Registers a newly allocated object with the collector.
     * @param obj Pointer to the object.
     */
    static void registerObject(GCObject* obj);

    /**
     * @brief Registers a root reference.
     * @param r Pointer to the root reference.
     */
    static void registerRoot(GCRefBase* r);

    /**
     * @brief Unregisters a root reference.
     * @param r Pointer to the root reference.
     */
    static void unregisterRoot(GCRefBase* r);

    /**
     * @brief Performs a blocking garbage collection cycle.
     * @param major If true, performs a major (full) collection.
     */
    static void collectNow(bool major = false);

    /**
     * @brief Starts an incremental garbage collection cycle.
     */
    static void startIncrementalCollect();

    /**
     * @brief Performs a single incremental collection step.
     * @return True if the collection cycle has completed.
     */
    static bool incrementalCollectStep();

    /**
     * @brief Write barrier invoked on member reference updates.
     *
     * This method must be called whenever a GCObject updates a member
     * reference to another GCObject.
     *
     * @param owner Owning object.
     * @param child Referenced child object.
     */
    static void writeBarrier(GCObject* owner, GCObject* child);

    /**
     * @brief Sets the marking budget.
     * @param b New mark budget.
     */
    static void setMarkBudget(int b);

    /**
     * @brief Sets the sweeping budget.
     * @param b New sweep budget.
     */
    static void setSweepBudget(int b);

    /**
     * @brief Enables or disables debug output.
     */
    static bool debug;
};

#endif
