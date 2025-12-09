// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GCRef.h
// ----------------------------------

#ifndef TERMPROJECT_GCREF_H
#define TERMPROJECT_GCREF_H

#include <utility>
#include "GCRefBase.h"
#include "GC.h"
#include <type_traits>

class GCObject;

/**
 * GCRef<T> represents either:
 *  - a root reference (owner == nullptr) which is registered with GC as a root
 *  - or a member reference owned by a GCObject (owner != nullptr) which is tracked by the owner
 *
 * When assigned to (or constructed) and owner != nullptr we invoke GC::writeBarrier(owner, child)
 * to keep incremental marking correct.
 */
template <typename T>
class GCRef : public GCRefBase {
    static_assert(std::is_convertible<T*, GCObject*>::value, "T must inherit GCObject");

    T* ptr = nullptr;
    GCObject* owner = nullptr;
    bool registeredRoot = false;

    void registerRootIfNeeded() {
        if (!owner && ptr && !registeredRoot) {
            GC::registerRoot(this);
            registeredRoot = true;
        }
    }

    void unregisterRootIfNeeded() {
        if (!owner && registeredRoot) {
            GC::unregisterRoot(this);
            registeredRoot = false;
        }
    }

    void detachOwnerOrRoot() {
        if (owner) {
            owner->removeMemberRef(this);
            owner = nullptr;
        } else {
            unregisterRootIfNeeded();
        }
    }

public:
    explicit GCRef(T* p = nullptr) : ptr(p), owner(nullptr) {
        registerRootIfNeeded();
    }

    GCRef(GCObject* owner_, T* p = nullptr) : ptr(p), owner(owner_) {
        if (owner) owner->addMemberRef(this);
        else registerRootIfNeeded();
        if (owner && ptr) GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
    }

    GCRef(const GCRef& other) : ptr(other.ptr), owner(other.owner), registeredRoot(other.registeredRoot) {
        if (owner) owner->addMemberRef(this);
        else if (registeredRoot) GC::registerRoot(this);
    }

    GCRef(GCRef&& other) noexcept
        : ptr(std::exchange(other.ptr, nullptr)),
          owner(std::exchange(other.owner, nullptr)),
          registeredRoot(std::exchange(other.registeredRoot, false)) {
        if (owner) owner->addMemberRef(this);
        else if (registeredRoot) GC::registerRoot(this);
    }

    ~GCRef() override {
        if (owner) owner->removeMemberRef(this);
        else unregisterRootIfNeeded();
    }

    void nullIfPointsTo(GCObject* obj) override {
        if (ptr == obj) {
            ptr = nullptr;
            if (!owner) unregisterRootIfNeeded();
        }
    }

    GCRef& operator=(const GCRef& other) {
        if (this == &other) return *this;
        detachOwnerOrRoot();
        ptr = other.ptr;
        owner = other.owner;
        registeredRoot = other.registeredRoot;
        if (owner) owner->addMemberRef(this);
        else if (registeredRoot) GC::registerRoot(this);
        if (owner && ptr) GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
        return *this;
    }

    GCRef& operator=(GCRef&& other) noexcept {
        if (this == &other) return *this;
        detachOwnerOrRoot();
        ptr = std::exchange(other.ptr, nullptr);
        owner = std::exchange(other.owner, nullptr);
        registeredRoot = std::exchange(other.registeredRoot, false);
        if (owner) owner->addMemberRef(this);
        else if (registeredRoot) GC::registerRoot(this);
        if (owner && ptr) GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
        return *this;
    }

    GCRef& operator=(T* o) {
        if (owner) {
            ptr = o;
            if (ptr) GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
        } else {
            unregisterRootIfNeeded();
            ptr = o;
            registerRootIfNeeded();
        }
        return *this;
    }

    GCRef& operator=(std::nullptr_t) {
        if (owner) ptr = nullptr;
        else {
            unregisterRootIfNeeded();
            ptr = nullptr;
        }
        return *this;
    }

    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }
    explicit operator bool() const { return ptr != nullptr; }
    T* get() const { return ptr; }

    GCObject* getObject() const override { return static_cast<GCObject*>(ptr); }
};

#endif // TERMPROJECT_GCREF_H
