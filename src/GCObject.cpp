// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: src/GCObject.cpp
// ----------------------------------

#include "../include/GCObject.h"
#include "../include/GCRefBase.h"
#include "../include/GC.h"

#include <algorithm>

GCObject::GCObject() : marked(false), survivalCount(0), generation(Generation::Young) {
    GC::registerObject(this);
}

GCObject::~GCObject() = default;

void GCObject::addMemberRef(GCRefBase* r) {
    memberRefs.push_back(r);
}

void GCObject::removeMemberRef(GCRefBase* r) {
    memberRefs.erase(std::remove(memberRefs.begin(), memberRefs.end(), r), memberRefs.end());
}

const std::vector<GCRefBase*>& GCObject::getMemberRefs() const {
    return memberRefs;
}

void GCObject::traceChildren(std::vector<GCObject*>& out) const {
    for (GCRefBase* r : memberRefs) {
        if (!r) continue;
        GCObject* child = r->getObject();
        if (child) out.push_back(child);
    }
}
