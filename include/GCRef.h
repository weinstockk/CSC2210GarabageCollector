// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GCRef.h  (FIXED - includes GCObject)
// ----------------------------------

#ifndef TERMPROJECT_GCREF_H
#define TERMPROJECT_GCREF_H

#include <utility>
#include <type_traits>

#include "GCObject.h"
#include "GCRefBase.h"
#include "GC.h"

class GCObject; // still okay, but GCObject.h ensures completeness

/**
 * GCRef<T> represents either:
 *  - a root reference (owner == nullptr) which is registered with GC as a root
 *  - or a member reference owned by a GCObject (owner != nullptr) which is tracked by the owner
 *
 * Important fixes:
 *  - registeredRoot is never copied
 *  - writeBarrier invoked on all member stores (including null)
 *  - move/copy properly adjust owner's memberRef list
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
    explicit GCRef(T* p = nullptr) : ptr(p), owner(nullptr), registeredRoot(false) {
        registerRootIfNeeded();
    }

    GCRef(GCObject* owner_, T* p = nullptr) : ptr(p), owner(owner_), registeredRoot(false) {
        if (owner) {
            owner->addMemberRef(this);
            GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
        } else {
            registerRootIfNeeded();
        }
    }

    GCRef(const GCRef& other) : ptr(other.ptr), owner(other.owner), registeredRoot(false) {
        if (owner) {
            owner->addMemberRef(this);
            if (ptr) GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
        } else {
            registerRootIfNeeded();
        }
    }

    GCRef(GCRef&& other) noexcept
        : ptr(std::exchange(other.ptr, nullptr)),
          owner(std::exchange(other.owner, nullptr)),
          registeredRoot(false)
    {
        other.unregisterRootIfNeeded();
        if (owner) {
            owner->addMemberRef(this);
            if (ptr) GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
        } else {
            registerRootIfNeeded();
        }
    }

    ~GCRef() override {
        if (owner) {
            owner->removeMemberRef(this);
            owner = nullptr;
        } else {
            unregisterRootIfNeeded();
        }
    }

    void nullIfPointsTo(GCObject* obj) override {
        if (ptr == obj) {
            if (owner) {
                ptr = nullptr;
                GC::writeBarrier(owner, nullptr);
            } else {
                unregisterRootIfNeeded();
                ptr = nullptr;
            }
        }
    }

    GCRef& operator=(const GCRef& other) {
        if (this == &other) return *this;
        detachOwnerOrRoot();
        ptr = other.ptr;
        owner = other.owner;
        registeredRoot = false;
        if (owner) {
            owner->addMemberRef(this);
            if (ptr) GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
        } else {
            registerRootIfNeeded();
        }
        return *this;
    }

    GCRef& operator=(GCRef&& other) noexcept {
        if (this == &other) return *this;
        detachOwnerOrRoot();
        ptr = std::exchange(other.ptr, nullptr);
        owner = std::exchange(other.owner, nullptr);
        other.unregisterRootIfNeeded();
        registeredRoot = false;
        if (owner) {
            owner->addMemberRef(this);
            if (ptr) GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
        } else {
            registerRootIfNeeded();
        }
        return *this;
    }

    GCRef& operator=(T* o) {
        if (owner) {
            ptr = o;
            GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
        } else {
            unregisterRootIfNeeded();
            ptr = o;
            registerRootIfNeeded();
        }
        return *this;
    }

    GCRef& operator=(std::nullptr_t) {
        if (owner) {
            ptr = nullptr;
            GC::writeBarrier(owner, nullptr);
        } else {
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
