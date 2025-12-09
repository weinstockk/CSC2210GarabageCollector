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
 * Abstract base for GC-managed references.
 * Concrete template GCRef<T> implements getObject() to return the GCObject pointer.
 */
class GCRefBase {
public:
    virtual GCObject* getObject() const = 0;
    virtual void nullIfPointsTo(GCObject* obj) = 0;
    virtual ~GCRefBase() = default;
};

#endif // TERMPROJECT_GCREFBASE_H

