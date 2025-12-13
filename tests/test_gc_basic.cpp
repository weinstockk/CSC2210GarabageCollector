// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock (AI Used)
// File: include/GC.h
// ----------------------------------

#include <catch2/catch_test_macros.hpp>

#include "GC.h"
#include "GCObject.h"
#include "GCRef.h"

#include <atomic>
#include <vector>

class CountingNode : public GCObject {
public:
    GCRef<CountingNode> next;
    static std::atomic<int> liveCount;

    CountingNode() : next(this, nullptr) { ++liveCount; }
    ~CountingNode() override { --liveCount; }
};

std::atomic<int> CountingNode::liveCount{0};

static void runIncrementalCollectionToCompletion() {
    GC::startIncrementalCollect();
    while (!GC::incrementalCollectStep()) {}
}

TEST_CASE("Deep linked list reachable from root is preserved") {
    GC::init(50, 50, 1000, 50);
    REQUIRE(CountingNode::liveCount.load() == 0);

    const int N = 100;
    std::vector<CountingNode*> nodes;
    nodes.reserve(N);

    for (int i = 0; i < N; ++i) {
        nodes.push_back(new CountingNode());
    }

    GCRef<CountingNode> root(nodes[0]);
    for (int i = 0; i < N - 1; ++i) {
        nodes[i]->next = nodes[i + 1];
    }

    runIncrementalCollectionToCompletion();
    REQUIRE(CountingNode::liveCount.load() == N);

    root = nullptr;
    runIncrementalCollectionToCompletion();
    REQUIRE(CountingNode::liveCount.load() == 0);
}

TEST_CASE("Branching object graph survives marking") {
    GC::init(50, 50, 1000, 50);
    REQUIRE(CountingNode::liveCount.load() == 0);

    CountingNode* rootRaw = new CountingNode();
    CountingNode* left = new CountingNode();
    CountingNode* right = new CountingNode();

    GCRef<CountingNode> root(rootRaw);
    root->next = left;
    left->next = right;

    runIncrementalCollectionToCompletion();
    REQUIRE(CountingNode::liveCount.load() == 3);

    root = nullptr;
    runIncrementalCollectionToCompletion();
    REQUIRE(CountingNode::liveCount.load() == 0);
}
