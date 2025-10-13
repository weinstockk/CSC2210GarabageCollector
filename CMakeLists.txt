cmake_minimum_required(VERSION 3.15)
project(CSC2210GarabageCollector VERSION 1.0 LANGUAGES CXX)

add_library(GC STATIC
        src/GC.cpp
        src/GCObject.cpp
)

# Precompiled headers for faster compile time
target_precompile_headers(GC PRIVATE
        <utility>
        <iostream>
        "include/internal/GC.h"
        "include/internal/GCObject.h"
        "include/internal/GCRefBase.h"
)

# Internal includes (not installed)
target_include_directories(GC PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/internal
)

target_compile_features(GC PUBLIC cxx_std_20)

# Only install the public interface header (GCRef.h)
install(FILES include/GCRef.h DESTINATION include)

# Install the compiled library
install(TARGETS GC
        EXPORT GCTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)

# Export configuration for downstream CMake projects
install(EXPORT GCTargets
        FILE GCTargets.cmake
        NAMESPACE GC::
        DESTINATION lib/cmake/GC
)
