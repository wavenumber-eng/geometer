cmake_minimum_required(VERSION 3.24)
project(geometer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OCCT must be pre-built (its CMake doesn't support subdirectory inclusion).
# Run scripts/build_occt.py first, then configure with:
#   cmake --preset default
find_package(OpenCASCADE REQUIRED)

add_subdirectory(src/cpp)
