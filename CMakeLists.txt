cmake_minimum_required(VERSION 3.24)
project(geometer VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# OCCT dependency
#
# OCCT's CMake uses CMAKE_SOURCE_DIR internally, so it cannot be added as a
# subdirectory via FetchContent or add_subdirectory. Instead it is built as a
# standalone project and found via find_package.
#
# If OCCT is not found at configure time, scripts/build_occt.py is invoked
# automatically. This makes the first configure slow (~10-15 min) but
# subsequent configures are instant. The script can also be run manually:
#
#   python scripts/build_occt.py          # build
#   python scripts/build_occt.py --clean  # wipe and rebuild
# ---------------------------------------------------------------------------
set(_geometer_default_occt_dir "${CMAKE_SOURCE_DIR}/.deps/occt-install/cmake")
set(_geometer_occt_dir "${OpenCASCADE_DIR}")
if(NOT _geometer_occt_dir OR _geometer_occt_dir MATCHES "-NOTFOUND$")
    set(_geometer_occt_dir "${_geometer_default_occt_dir}")
endif()

find_package(OpenCASCADE QUIET NO_DEFAULT_PATH PATHS "${_geometer_occt_dir}")

if(NOT OpenCASCADE_FOUND)
    message(STATUS "OpenCASCADE not found. Building from source via scripts/build_occt.py ...")
    message(STATUS "This is a one-time operation and will take ~10-15 minutes.")

    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    execute_process(
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/build_occt.py"
        RESULT_VARIABLE _occt_build_result
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    if(NOT _occt_build_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to build OCCT. You can try manually:\n"
            "  python scripts/build_occt.py\n"
            "Check the output above for errors."
        )
    endif()

    set(OpenCASCADE_DIR "${_geometer_default_occt_dir}" CACHE PATH
        "The directory containing a CMake configuration file for OpenCASCADE." FORCE)
    find_package(OpenCASCADE REQUIRED NO_DEFAULT_PATH PATHS "${OpenCASCADE_DIR}")
endif()

message(STATUS "OpenCASCADE ${OpenCASCADE_VERSION} found at ${OpenCASCADE_INSTALL_PREFIX}")

include(CTest)
add_subdirectory(src/cpp)

if(BUILD_TESTING)
    add_subdirectory(tests/cpp)
endif()
