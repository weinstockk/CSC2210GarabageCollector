// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: src/GCObject.cpp
// ----------------------------------

#include <algorithm>
#include "../include/GCObject.h"
#include "../include/GC.h"
#include "../include/GCRefBase.h"

/**
 * @brief Constructor that automatically registers this object with the GC.
 */
GCObject::GCObject() : marked(false), survivalCount(0), generation(Generation::Young) {
    GC::registerObject(this);
}

/**
 * @brief Default virtual destructor.
 */
GCObject::~GCObject() = default;

/**
 * @brief Adds a member reference to this object's tracking list.
 * @param ref Pointer to a GCRefBase reference owned by this object.
 */
void GCObject::addMemberRef(GCRefBase* ref) {
    memberRefs.push_back(ref);
}

/**
 * @brief Removes a member reference from the tracking list.
 * @param ref Pointer to the reference being removed.
 */
void GCObject::removeMemberRef(GCRefBase* ref) {
    memberRefs.erase(std::remove(memberRefs.begin(), memberRefs.end(), ref), memberRefs.end());
}

/**
 * @brief Retrieves this object's internal references.
 * @return A constant vector of GCRefBase pointers.
 */
const std::vector<GCRefBase*>& GCObject::getRefs() const {
    return memberRefs;
}
