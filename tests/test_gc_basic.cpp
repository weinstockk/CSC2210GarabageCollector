// tests/test_gc_basic.cpp
#include <catch2/catch_test_macros.hpp>

#include "GC.h"
#include "GCObject.h"
#include "GCRef.h"

#include <atomic>

// A simple test GCObject that tracks live instances so tests can assert deletion.
class CountingNode : public GCObject {
public:
    // member reference (owned by this)
    GCRef<CountingNode> next;

    static std::atomic<int> liveCount;

    CountingNode()
    : next(this, nullptr) // initialize member GCRef with owner = this
    {
        ++liveCount;
    }

    ~CountingNode() override {
        --liveCount;
    }
};

std::atomic<int> CountingNode::liveCount{0};

// Helper: run an incremental collection to completion
static void runIncrementalCollectionToCompletion() {
    GC::startIncrementalCollect();
    // Keep calling incrementalCollectStep until it returns true (Idle)
    while (!GC::incrementalCollectStep()) {
        // loop — tests shouldn't busy-wait forever because collector should finish
    }
}

TEST_CASE("Basic allocation registers objects and collectNow frees unreachable") {
    // Make budgets low so test runs quickly (optional)
    GC::init(50, 50, 1000, 50); // doesn't hurt to tune here

    // start with zero live nodes
    REQUIRE(CountingNode::liveCount.load() == 0);

    {
        // Create a root pointer. GCRef<T> default-ctor registers as root when owner == nullptr
        CountingNode* raw = new CountingNode();
        GCRef<CountingNode> root(raw); // root registered automatically

        // create a child and link as member (owned by root object)
        CountingNode* child = new CountingNode();
        // create member GCRef inside object: use the object's member ref 'next'
        // Because 'root' is a GCRef to the object, use operator-> to access member
        root->next = child;

        // both objects are reachable, so they should remain alive after a GC
        runIncrementalCollectionToCompletion();
        REQUIRE(CountingNode::liveCount.load() == 2);

        // Now drop the root by assigning nullptr -> this will unregister the root
        root = nullptr;
    }

    // After root destroyed, both nodes are unreachable. Run collector until finished.
    runIncrementalCollectionToCompletion();

    REQUIRE(CountingNode::liveCount.load() == 0);
}

TEST_CASE("Cycle cleanup: two nodes referencing each other are collected when no roots") {
    GC::init(50, 50, 1000, 50);

    REQUIRE(CountingNode::liveCount.load() == 0);

    {
        // create two nodes and link them in a cycle, but do NOT keep any external root
        CountingNode* a = new CountingNode();
        CountingNode* b = new CountingNode();

        // create temporary local roots so we can wire them; use GCRef local variables that will go out of scope
        {
            GCRef<CountingNode> ra(a);
            GCRef<CountingNode> rb(b);

            ra->next = b;
            rb->next = a;

            REQUIRE(CountingNode::liveCount.load() == 2);
            // ra and rb go out of scope at end of this block -> roots unregistered
        }

        // At this point, there are no registered roots pointing to a or b; run GC.
        runIncrementalCollectionToCompletion();
        REQUIRE(CountingNode::liveCount.load() == 0);
    }
}

TEST_CASE("Write barrier: assigning a child to an already-marked owner preserves child during incremental mark") {
    GC::init(10, 10, 1000, 50);

    // sanity
    REQUIRE(CountingNode::liveCount.load() == 0);

    // Setup: create an external root and a separate child node
    CountingNode* owner_raw = new CountingNode();
    CountingNode* child_raw = new CountingNode();

    GCRef<CountingNode> owner(owner_raw); // root
    // do NOT create a root for child_raw; it's initially unreachable (only raw new), but not yet linked

    // Begin incremental collect and let marking seed the owner
    GC::startIncrementalCollect();
    // We need to seed roots into the mark stack; call one mark step so owner becomes marked
    GC::incrementalCollectStep(); // should run MarkRoots -> Marking step(s)

    // At this point, if owner_raw got marked by seeding, then it is "black/marked".
    // Now assign owner->next = child_raw; GC writeBarrier should mark child if owner is marked.
    owner->next = child_raw; // This uses GCRef::operator=(T*) and should call GC::writeBarrier

    // Run the rest of the incremental collection to completion
    while (!GC::incrementalCollectStep()) { /* continue */ }

    // Because we linked the child from a marked root, the child should have survived.
    // There are two objects still reachable via the root 'owner'.
    REQUIRE(CountingNode::liveCount.load() == 2);

    // Clean up: remove the root and collect to free memory
    owner = nullptr;
    runIncrementalCollectionToCompletion();
    REQUIRE(CountingNode::liveCount.load() == 0);
}
