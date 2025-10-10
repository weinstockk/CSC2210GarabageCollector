cmake_minimum_required(VERSION 4.0)
project(TermProject)

set(CMAKE_CXX_STANDARD 20)

add_executable(TermProject main.cpp
        gc/GC.cpp
        gc/GCObject.cpp
        testobjects/NodeGC.cpp
        gc/GCRef.h
        gc/GCRefBase.h
        testobjects/NodeNormal.cpp
        testobjects/NodeNormal.h
        testobjects/RandomObject.cpp
        testobjects/RandomObject.h
)
