cmake_minimum_required(VERSION 3.24)
project(geometer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static linkage
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# OCCT via FetchContent
include(FetchContent)
FetchContent_Declare(
    occt
    GIT_REPOSITORY https://github.com/Open-Cascade-SAS/OCCT.git
    GIT_TAG        V7_8_1
    GIT_SHALLOW    TRUE
)

# Disable OCCT modules we do not need
set(BUILD_MODULE_Visualization  OFF CACHE BOOL "" FORCE)
set(BUILD_MODULE_ApplicationFramework OFF CACHE BOOL "" FORCE)
set(BUILD_MODULE_Draw           OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(occt)

add_subdirectory(src/cpp)
