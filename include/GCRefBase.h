// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GCRefBase.h
// ----------------------------------

#ifndef TERMPROJECT_GCREFBASE_H
#define TERMPROJECT_GCREFBASE_H

class GCObject;

/**
 * @file GCRefBase.h
 * @brief Defines the abstract base class for GC-managed references.
 */

/**
 * @class GCRefBase
 * @brief Abstract base class for garbage-collector-managed references.
 *
 * Concrete reference types (e.g., GCRef<T>) must implement access
 * to the underlying GCObject and support nulling when the object
 * is collected.
 */
class GCRefBase {
public:
    /**
     * @brief Retrieves the GCObject this reference points to.
     * @return Pointer to the referenced GCObject, or nullptr.
     */
    virtual GCObject* getObject() const = 0;

    /**
     * @brief Sets the reference to null if it points to the given object.
     * @param obj The GCObject being checked or collected.
     */
    virtual void nullIfPointsTo(GCObject* obj) = 0;

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~GCRefBase() = default;
};

#endif
