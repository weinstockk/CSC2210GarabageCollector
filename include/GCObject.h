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

enum class Generation { Young, Old };

/**
 * Base class for all GC-managed objects.
 *
 * You may either:
 *  - Override traceChildren(std::vector<GCObject*>& out) to explicitly push children,
 *  - OR add GCRef<> member fields (preferred) — GC will discover children via getMemberRefs().
 *
 * Defaults: traceChildren() collects children by iterating getMemberRefs().
 */
class GCObject {
public:
    bool marked = false;   // used as "gray" marker (discovered)
    bool black = false;    // true when object has been blackened (scanned)
    int survivalCount = 0;
    Generation generation = Generation::Young;

    GCObject();
    virtual ~GCObject();

    // Called by GC when tracing children. Default implementation uses getMemberRefs().
    virtual void traceChildren(std::vector<GCObject*>& out) const;

    // Member-ref management for GCRef ownership
    void addMemberRef(GCRefBase* r);
    void removeMemberRef(GCRefBase* r);
    const std::vector<GCRefBase*>& getMemberRefs() const;

private:
    std::vector<GCRefBase*> memberRefs;
};

#endif // TERMPROJECT_GCOBJECT_H
