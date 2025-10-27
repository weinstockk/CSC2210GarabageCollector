// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GCRefBase.h
// ----------------------------------

#ifndef TERMPROJECT_GCREFBASE_H
#define TERMPROJECT_GCREFBASE_H

class GCObject;

class GCRefBase {
public:
    /**
     * @brief Returns the GC-managed object this reference points to.
     * @return Pointer to a GCObject, or `nullptr` if none.
     */
    virtual GCObject* getObject() const = 0;

    virtual void nullIfPointsTo(GCObject* obj) = 0;

    /** @brief Virtual destructor. */
    virtual ~GCRefBase() = default;
};

#endif // TERMPROJECT_GCREFBASE_H
