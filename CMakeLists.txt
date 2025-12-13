cmake_minimum_required(VERSION 3.15)
project(CSC2210GarabageCollector VERSION 1.0 LANGUAGES CXX)

# Library
add_library(GC STATIC
        src/GC.cpp
        src/GCObject.cpp
        include/GCRef.h
)

target_include_directories(GC
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(GC PUBLIC cxx_std_20)

# Install rules (unchanged)
install(TARGETS GC
        EXPORT GCTargets
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include)

install(DIRECTORY include/ DESTINATION include)

install(EXPORT GCTargets
        FILE GCTargets.cmake
        NAMESPACE GC::
        DESTINATION lib/cmake/GC)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/GCConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)

install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/GCConfigVersion.cmake"
        DESTINATION lib/cmake/GC
)

option(GC_BUILD_TESTS "Build GC unit tests" ON)

if (GC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
