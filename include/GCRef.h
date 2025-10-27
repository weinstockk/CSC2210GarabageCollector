// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GCRef.cpp
// ----------------------------------

#ifndef TERMPROJECT_GCREF_H
#define TERMPROJECT_GCREF_H

#include <utility>
#include "GC.h"
#include "GCRefBase.h"

using namespace std;

class GCObject;

template <typename T>
class GCRef : public GCRefBase {
    T* ptr;
    GCObject* owner = nullptr;
    bool registeredRoot = false;

    /// Registers the reference as a root if needed.
    void registerRootIfNeeded() {
        if (!owner && ptr && !registeredRoot) {
            GC::registerRef(this);
            registeredRoot = true;
        }
    }

    /// Unregisters the reference from the root set if needed.
    void unregisterRootIfNeeded() {
        if (registeredRoot) {
            GC::unregisterRef(this);
            registeredRoot = false;
        }
    }

    void detachFromOwnerOrRoot() {
        if (owner) {
            owner->removeMemberRef(this);
            owner = nullptr;
        } else {
            unregisterRootIfNeeded();
        }
    }

public:
    /**
     * @brief Constructs a root reference (not owned by any object).
     * @param p Raw pointer to the object being referenced.
     */
    explicit GCRef(T* p = nullptr) : ptr(p) {
        registerRootIfNeeded();
    }

    /**
     * @brief Constructs a member reference owned by a GCObject.
     * @param owner The owner object.
     * @param p Optional pointer to the referenced object.
     */
    GCRef(GCObject* owner, T* p = nullptr) : ptr(p), owner(owner) {
        if (owner) owner->addMemberRef(this);
    }

    /** @brief Copy constructor. */
    GCRef(const GCRef& other) : ptr(other.ptr), owner(other.owner), registeredRoot(other.registeredRoot) {
        if (owner) owner->addMemberRef(this);
        else if (registeredRoot) GC::registerRef(this);
    }

    /** @brief Move constructor. */
    GCRef(GCRef&& other) noexcept
        : ptr(exchange(other.ptr, nullptr)),
          owner(exchange(other.owner, nullptr)) {
        if (owner) owner->addMemberRef(this);
        else if (registeredRoot) GC::registerRef(this);
    }

    /** @brief Destructor. Cleans up ownership and registration. */
    ~GCRef() override {
        if (owner) owner->removeMemberRef(this);
        else unregisterRootIfNeeded();
    }

    void nullIfPointsTo(GCObject* obj) override {
        if (ptr == obj) {
            ptr = nullptr;
            // if this was a root, unregister it
            if (!owner) unregisterRootIfNeeded();
        }
    }

    /** @brief Copy assignment operator. */
    GCRef& operator=(const GCRef& other) {
        if (this == &other) return *this;

        detachFromOwnerOrRoot();

        ptr = other.ptr;
        owner = other.owner;
        registeredRoot = other.registeredRoot;

        if (owner) owner->addMemberRef(this);
        else if (registeredRoot) GC::registerRef(this);
        return *this;
    }

    /** @brief Move assignment operator. */
    GCRef& operator=(GCRef&& other) noexcept {
        if (this == &other) return *this;

        detachFromOwnerOrRoot();

        ptr = exchange(other.ptr, nullptr);
        owner = exchange(other.owner, nullptr);
        registeredRoot = exchange(other.registeredRoot, false);

        if (owner) owner->addMemberRef(this);
        else if (registeredRoot) GC::registerRef(this);
        return *this;
    }

    /** @brief Assigns from a raw pointer. */
    GCRef& operator=(T* obj) {
        if (owner) ptr = obj;
        else {
            unregisterRootIfNeeded();
            ptr = obj;
            registerRootIfNeeded();
        }
        return *this;
    }

    /** @brief Assigns from `nullptr`. */
    GCRef& operator=(nullptr_t) {
        if (owner) ptr = nullptr;
        else {
            unregisterRootIfNeeded();
            ptr = nullptr;
        }
        return *this;
    }

    /** @brief Dereference operator. */
    T& operator*() const { return *ptr; }

    /** @brief Member access operator. */
    T* operator->() const { return ptr; }

    /** @brief Boolean check for validity. */
    explicit operator bool() const { return ptr != nullptr; }

    /** @brief Gets the raw pointer. */
    T* get() const { return ptr; }

    /** @brief Returns the referenced GCObject for GC traversal. */
    GCObject* getObject() const override { return static_cast<GCObject*>(ptr); }
};

#endif // TERMPROJECT_GCREF_H
