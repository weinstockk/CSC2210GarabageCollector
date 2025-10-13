// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GCObject.h
// ----------------------------------

#ifndef TERMPROJECT_GCOBJECT_H
#define TERMPROJECT_GCOBJECT_H

#pragma once
#include <vector>

class GCRefBase;

/**
 * @class GCObject
 * @brief Base class for all garbage-collected objects.
 *
 * Every GC-managed object must derive from this class.
 * The GC system uses it to:
 * - Track reachability (via `marked`)
 * - Store internal references to other GC-managed objects
 *
 * @see GC
 * @see GCRefBase
 */
class GCObject {
public:
    /** @brief Indicates whether the object has been marked as reachable. */
    bool marked;

    /** @brief Constructs a new GCObject and registers it with the GC. */
    GCObject();

    /** @brief Virtual destructor (required for polymorphic deletion). */
    virtual ~GCObject();

    /**
     * @brief Adds a reference to this object's internal reference list.
     * @param ref Pointer to a @ref GCRefBase owned by this object.
     */
    void addMemberRef(GCRefBase* ref);

    /**
     * @brief Removes a reference from this object's internal reference list.
     * @param ref Pointer to the reference being removed.
     */
    void removeMemberRef(GCRefBase* ref);

    /**
     * @brief Returns a list of all GCRefBase pointers owned by this object.
     * @return Constant reference to the list of member references.
     */
    virtual const std::vector<GCRefBase*>& getRefs() const;

private:
    /** @brief Internal list of GC-managed references contained within this object. */
    std::vector<GCRefBase*> memberRefs;
};

#endif // TERMPROJECT_GCOBJECT_H
