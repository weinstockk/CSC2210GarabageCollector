// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GCObject.h
// ----------------------------------

#ifndef TERMPROJECT_GCOBJECT_H
#define TERMPROJECT_GCOBJECT_H

#include <vector>

class GCRefBase;

/**
 * @file GCObject.h
 * @brief Defines the base class for garbage-collector-managed objects.
 */

/**
 * @enum Generation
 * @brief Represents the generational state of a GCObject.
 */
enum class Generation { Young, Old };

/**
 * @class GCObject
 * @brief Base class for all garbage-collector-managed objects.
 *
 * Derived classes may either override traceChildren() to explicitly
 * expose child objects, or declare GCRef<T> member fields, which are
 * automatically discovered by the garbage collector.
 */
class GCObject {
public:
    /**
     * @brief Indicates whether the object has been marked during GC.
     */
    bool marked = false;

    /**
     * @brief Indicates whether the object has been fully scanned.
     */
    bool black = false;

    /**
     * @brief Number of GC cycles the object has survived.
     */
    int survivalCount = 0;

    /**
     * @brief Current generational state of the object.
     */
    Generation generation = Generation::Young;

    /**
     * @brief Constructs a GC-managed object.
     */
    GCObject();

    /**
     * @brief Virtual destructor.
     */
    virtual ~GCObject();

    /**
     * @brief Traces child objects for garbage collection.
     *
     * The default implementation discovers children through
     * member GCRef instances.
     *
     * @param out Vector to receive child objects.
     */
    virtual void traceChildren(std::vector<GCObject*>& out) const;

    /**
     * @brief Registers a GCRef as a member reference.
     * @param r Pointer to the member reference.
     */
    void addMemberRef(GCRefBase* r);

    /**
     * @brief Unregisters a GCRef from the member reference list.
     * @param r Pointer to the member reference.
     */
    void removeMemberRef(GCRefBase* r);

    /**
     * @brief Returns all registered member references.
     * @return Vector of GCRefBase pointers.
     */
    const std::vector<GCRefBase*>& getMemberRefs() const;

private:
    std::vector<GCRefBase*> memberRefs;
};

#endif
