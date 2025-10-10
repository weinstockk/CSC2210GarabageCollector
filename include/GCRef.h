// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GCRef.h
// ----------------------------------

#ifndef TERMPROJECT_GCREF_H
#define TERMPROJECT_GCREF_H

#include <utility>
#include "GC.h"
#include "GCRefBase.h"

class GCObject;

/**
 * @class GCRef
 * @brief Smart reference type that integrates with the GC system.
 *
 * A @ref GCRef behaves like a smart pointer (`std::shared_ptr`-like) but
 * automatically registers itself with the garbage collector.
 *
 * - If created **without an owner**, it acts as a **root reference**.
 * - If created **with a GCObject owner**, it becomes a **member reference**,
 *   tracked as part of that object’s internal reference list.
 *
 * @tparam T The type of the GC-managed object being referenced.
 * @see GC
 * @see GCObject
 */
template <typename T>
class GCRef : public GCRefBase {
    T* ptr;                 ///< Raw pointer to the referenced object.
    GCObject* owner = nullptr; ///< Owner object (null if root ref).
    bool registeredRoot = false; ///< Whether this ref is currently a registered GC root.

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

public:
    /**
     * @brief Constructs a root reference (not owned by any object).
     * @param p Raw pointer to the object being referenced.
     */
    explicit GCRef(T* p) : ptr(p) {
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
    GCRef(const GCRef& other) : ptr(other.ptr), owner(other.owner) {
        if (owner) owner->addMemberRef(this);
        else registerRootIfNeeded();
    }

    /** @brief Move constructor. */
    GCRef(GCRef&& other) noexcept
        : ptr(std::exchange(other.ptr, nullptr)),
          owner(std::exchange(other.owner, nullptr)) {
        if (owner) owner->addMemberRef(this);
        else {
            registerRootIfNeeded();
            other.unregisterRootIfNeeded();
        }
    }

    /** @brief Copy assignment operator. */
    GCRef& operator=(const GCRef& other) {
        ptr = other.ptr;
        owner = other.owner;
        if (!owner && !registeredRoot) registerRootIfNeeded();
        return *this;
    }

    /** @brief Move assignment operator. */
    GCRef& operator=(GCRef&& other) noexcept {
        if (this == &other) return *this;
        if (owner) owner->removeMemberRef(this);
        else unregisterRootIfNeeded();

        ptr = std::exchange(other.ptr, nullptr);
        owner = std::exchange(other.owner, nullptr);
        registeredRoot = false;

        if (owner) owner->addMemberRef(this);
        else registerRootIfNeeded();

        return *this;
    }

    /** @brief Assigns from a raw pointer. */
    GCRef& operator=(T* obj) {
        if (!owner) unregisterRootIfNeeded();
        ptr = obj;
        if (!owner) registerRootIfNeeded();
        return *this;
    }

    /** @brief Assigns from `nullptr`. */
    GCRef& operator=(std::nullptr_t) {
        if (owner) ptr = nullptr;
        else {
            unregisterRootIfNeeded();
            ptr = nullptr;
        }
        return *this;
    }

    /** @brief Destructor. Cleans up ownership and registration. */
    ~GCRef() {
        if (owner) owner->removeMemberRef(this);
        else unregisterRootIfNeeded();
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
