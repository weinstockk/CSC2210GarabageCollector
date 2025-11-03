# CSC2210GarabageCollector
Term Project for CSC 2210. Mark and Sweep Garbage Collector
* Generational


Any garbage collection algorithm must perform 2 basic operations. One, it should be able to detect all the unreachable objects and secondly, it must reclaim the heap space used by the garbage objects and make the space available again to the program. The operations are performed by Mark and Sweep Algorithm in two phases as listed and described further as follows:
* Mark phase
* Sweep phase

## How To Download 
This has only been tested on CLion on a Windows computer. This will not work for anything but a Windows OS as powershell is needed to run this.
cmake is set up so if you aren't using CLions default then it should give you an option to change the path, give the path to the folder that cmake.exe is located in.
This can work for both the MinGW (g++) and MSVC (cl.exe) compilers, if it can find these compilers it will ask what one you want to use and ask for a path if needed. This could be a little buggy.

You should be in your projects base folder when you run the command so it can set itself up properly. Your CMake file should also be in the root.

````
Invoke-WebRequest -Uri "https://github.com/weinstockk/CSC2210GarabageCollector/raw/Incremental/install.ps1" -OutFile "install.ps1"; .\install.ps1
````

This is downloading an installation file from the GitHub repo and running it, this should set up the cmake file as well as add a new file called GCinstall. Once done it should delete itself.

## How to use 
The first thing you should do is `#include GCRef.h` This is your smart pointer wrapper and will register objects with the garbage collector. If you want a object to use the GC then you need it to be in this wrapper.
Any object you build will also need to inherit from the `GCObject` Object. This will set everything up that needs to be in the GC.

Some notes when developing
* If you want to include a GCObject in another GC object make sure you initialize with an owner by doing this `Node(OtherGCObject* a) : a(this, a) {...}` This is so the GC has access to that object when it needs to be deleted.
* You are working with a reference not the object itself so you need to use `->` instead of `.` to call methods you create


Example:
````
#include "GCRef.h"

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

````

### Sources
[Mark-and-Sweep: Garbage Collection Algorithm](https://www.geeksforgeeks.org/java/mark-and-sweep-garbage-collection-algorithm/)
