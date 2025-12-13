// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: include/GCRef.h
// ----------------------------------

#ifndef TERMPROJECT_GCREF_H
#define TERMPROJECT_GCREF_H

#include <utility>
#include <type_traits>

#include "GCObject.h"
#include "GCRefBase.h"
#include "GC.h"

/**
 * @file GCRef.h
 * @brief Defines the GCRef template for garbage-collector-managed references.
 */

/**
 * @class GCRef
 * @brief Smart reference type managed by the garbage collector.
 *
 * GCRef<T> may represent either a root reference (when no owner is specified)
 * or a member reference owned by a GCObject. Root references are registered
 * directly with the garbage collector, while member references are tracked
 * by their owning object.
 *
 * @tparam T Type of object referenced; must inherit from GCObject.
 */
template <typename T>
class GCRef : public GCRefBase {
    static_assert(std::is_convertible<T*, GCObject*>::value,
                  "T must inherit GCObject");

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
    /**
     * @brief Constructs a root GCRef.
     * @param p Pointer to the managed object.
     */
    explicit GCRef(T* p = nullptr) : ptr(p), owner(nullptr), registeredRoot(false) {
        registerRootIfNeeded();
    }

    /**
     * @brief Constructs a member GCRef owned by a GCObject.
     * @param owner_ Owning GCObject.
     * @param p Pointer to the managed object.
     */
    GCRef(GCObject* owner_, T* p = nullptr)
        : ptr(p), owner(owner_), registeredRoot(false) {
        if (owner) {
            owner->addMemberRef(this);
            GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
        } else {
            registerRootIfNeeded();
        }
    }

    /**
     * @brief Copy constructor.
     * @param other Reference to copy.
     */
    GCRef(const GCRef& other)
        : ptr(other.ptr), owner(other.owner), registeredRoot(false) {
        if (owner) {
            owner->addMemberRef(this);
            if (ptr) {
                GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
            }
        } else {
            registerRootIfNeeded();
        }
    }

    /**
     * @brief Move constructor.
     * @param other Reference to move from.
     */
    GCRef(GCRef&& other) noexcept
        : ptr(std::exchange(other.ptr, nullptr)),
          owner(std::exchange(other.owner, nullptr)),
          registeredRoot(false) {
        other.unregisterRootIfNeeded();
        if (owner) {
            owner->addMemberRef(this);
            if (ptr) {
                GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
            }
        } else {
            registerRootIfNeeded();
        }
    }

    /**
     * @brief Destructor.
     */
    ~GCRef() override {
        if (owner) {
            owner->removeMemberRef(this);
            owner = nullptr;
        } else {
            unregisterRootIfNeeded();
        }
    }

    /**
     * @brief Nulls the reference if it points to the specified object.
     * @param obj Object being checked or collected.
     */
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

    /**
     * @brief Copy assignment operator.
     * @param other Reference to copy from.
     * @return Reference to this object.
     */
    GCRef& operator=(const GCRef& other) {
        if (this == &other) return *this;
        detachOwnerOrRoot();
        ptr = other.ptr;
        owner = other.owner;
        registeredRoot = false;
        if (owner) {
            owner->addMemberRef(this);
            if (ptr) {
                GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
            }
        } else {
            registerRootIfNeeded();
        }
        return *this;
    }

    /**
     * @brief Move assignment operator.
     * @param other Reference to move from.
     * @return Reference to this object.
     */
    GCRef& operator=(GCRef&& other) noexcept {
        if (this == &other) return *this;
        detachOwnerOrRoot();
        ptr = std::exchange(other.ptr, nullptr);
        owner = std::exchange(other.owner, nullptr);
        other.unregisterRootIfNeeded();
        registeredRoot = false;
        if (owner) {
            owner->addMemberRef(this);
            if (ptr) {
                GC::writeBarrier(owner, static_cast<GCObject*>(ptr));
            }
        } else {
            registerRootIfNeeded();
        }
        return *this;
    }

    /**
     * @brief Assigns a raw pointer to this reference.
     * @param o Pointer to assign.
     * @return Reference to this object.
     */
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

    /**
     * @brief Assigns a null pointer to this reference.
     * @return Reference to this object.
     */
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

    /**
     * @brief Dereferences the managed object.
     * @return Reference to the managed object.
     */
    T& operator*() const { return *ptr; }

    /**
     * @brief Accesses the managed object.
     * @return Pointer to the managed object.
     */
    T* operator->() const { return ptr; }

    /**
     * @brief Checks whether the reference is non-null.
     * @return True if non-null, false otherwise.
     */
    explicit operator bool() const { return ptr != nullptr; }

    /**
     * @brief Returns the raw pointer.
     * @return Pointer to the managed object.
     */
    T* get() const { return ptr; }

    /**
     * @brief Returns the referenced object as a GCObject.
     * @return Pointer to the GCObject.
     */
    GCObject* getObject() const override {
        return static_cast<GCObject*>(ptr);
    }
};

#endif
