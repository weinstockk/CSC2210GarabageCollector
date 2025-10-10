// ----------------------------------
// Course: CSC 2210
// Section: 002
// Name: Keagan Weinstock
// File: main.cpp
// ----------------------------------

#include "gc/GC.h"
#include "gc/GCRef.h"
#include "testobjects/NodeGC.h"
#include "testobjects/NodeNormal.h"
#include "testobjects/RandomObject.h"

#include <chrono>
#include <iostream>

using namespace std;
using namespace std::chrono;

class Test {
public:
    // ============================================================
    // SECTION 1: Helper Timing
    // ============================================================
    template <typename Func>
    static long long measureTime(Func f, const string& label) {
        auto start = high_resolution_clock::now();
        f();
        auto duration = duration_cast<milliseconds>(high_resolution_clock::now() - start).count();
        cout << label << " took " << duration << " ms" << endl;
        return duration;
    }

    // ============================================================
    // SECTION 2: GC-Managed Object Test
    // ============================================================
    static long long runGCTest() {
        cout << "\n------ Running GC Test ------\n";
        long long totalTime = measureTime([] {
            {
                GCRef root(new NodeGC(1));
                root->left = new NodeGC(2);
                root->right = new NodeGC(3);

                root->left->addChild(new NodeGC(4));

                cout << "\nInitial GC Tree Structure (Before Collection):\n";
                cout << R"(
                 (1)
                /   \
             (2)     (3)
              |
             (4)
            )" << endl;

                cout << "\nFirst GC collect\n\n";
                GC::collect();

                // Simulate losing references
                cout << "\nBreaking reference: root->left.reset(nullptr);\n";
                root->left = nullptr;

                cout << "\nTree Structure After Breaking Reference:\n";
                cout << R"(
                 (1)
                    \
                    (3)

            (2) and (4) are now unreachable and will be GC'd.
            )" << endl;

                cout << "\nSecond GC collect\n\n";
                GC::collect();
            }

            cout << "\nFinal GC collect after root goes out of scope\n\n";
            GC::collect();
        }, "GC-managed test");

        cout << "-------------------------------\n";
        return totalTime;
    }

    // ============================================================
    // SECTION 3: Normal Manual Object Test
    // ============================================================
    static long long runNormalTest() {
        cout << "\n======= Running Normal C++ Test =======\n";
        long long totalTime = measureTime([] {
            NodeNormal* root = new NodeNormal(1);
            NodeNormal* n1 = new NodeNormal(2);
            NodeNormal* n2 = new NodeNormal(3);

            root->left = n1;
            root->right = n2;
            n2->right = new NodeNormal(4);

            cout << "\nInitial Tree Structure (Before Breaking Reference):\n";
            cout << R"(
             (1)
            /   \
         (2)     (3)
                    \
                    (4)
        )" << endl;

            cout << "\nBreaking Node 3\n";
            root->right = nullptr;

            cout << "\nTree Structure After Breaking Reference:\n";
            cout << R"(
             (1)
            /
         (2)
        )" << endl;

            // Manual cleanup
            cout << "\n--- Manual Cleanup Phase ---\n";
            //delete root->right->right; // Pointer is null
            cout << "Node 4 is leaked (can be fixed by removing in the destructor or by removing it first then Node 3)" << endl;
            delete n2; // manually free node 3 and its child
            delete n1;
            delete root;

        }, "Manual C++ test");

        cout << "===============================\n";
        return totalTime;
    }


    // ============================================================
    // SECTION 4: Combined Benchmark
    // ============================================================
    static void runCombined() {
        cout << "============= COMPARISON: GC vs Normal C++ =============\n";

        auto gcTime = runGCTest();
        auto normalTime = runNormalTest();

        cout << "\n========== Summary ==========\n";
        cout << "GC-managed time:   " << gcTime << " ms\n";
        cout << "Normal C++ time:   " << normalTime << " ms\n";
        cout << "GC difference:     " << (gcTime - normalTime) << " ms (negative means GC faster)\n";
        cout << "=============================\n";
    }

    // ============================================================
    // SECTION 5: Threshold Test
    // ============================================================
    static void runThresholdTest() {
        cout << "\n======= GC Threshold Stress Test =======\n";
        measureTime([] {
            for (int i = 0; i < 20; i++) {
                GCRef r(new RandomObject(i, new NodeGC(i + 10)));
            }
            GC::collect();
        }, "Threshold trigger");
        cout << "=======================================\n";
    }
};

class Tires final : public GCObject {
    string name = "";
    int width;
    int aspectRatio;
    int diameter;

public:
    explicit Tires(int width, int aspectRatio, int diameter) : width(width),aspectRatio(aspectRatio), diameter(diameter) {}
    ~Tires() override { cout << "Tires Disposed of Properly" << endl; }
};

class Vehicle final : public GCObject {
    string model = "";
    int speed;
    GCRef<Tires> tires;

    public:
    explicit Vehicle(Tires* tires, string model) : model(model), speed(0), tires(this, tires) {
        cout << model << " Created" << endl;
    }
    ~Vehicle() override { cout << "Vehicle Destroyed" << endl; }

    void setSpeed(int s) { speed = s; }
    int getSpeed() const { return speed; }

    void drive() {
        cout << "Driving Vehicle" << endl;
    }

    void changeTires(Tires* t) {
        cout << "Changing Tires" << endl;
        tires = t;
    }
};


int main() {
    // Testing
    /**
    {
        cout << "=============================================" << endl;
        cout << "            GC Test Program Start            " << endl;
        cout << "=============================================" << endl << endl;

        // Compare GC vs Manual C++ allocation
        Test::runCombined();

        // Stress-test GC threshold behavior
        //Test::runThresholdTest();

        cout << endl << "=============================================" << endl;
        cout << "            GC Test Program End              " << endl;
        cout << "=============================================" << endl;
    }
    **/

    // Car Demo
    {
        GCRef miata(new Vehicle(new Tires(195, 50, 15), "Miata"));
        miata->drive();
        GC::collect();
        miata->changeTires(new Tires(195, 55, 15));
        GC::collect();
    }

    return 0;
}