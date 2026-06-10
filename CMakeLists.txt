cmake_minimum_required(VERSION 3.24)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment target")
endif()

set(GEOMETER_RELEASE_DATE "2026-06-09" CACHE STRING "Geometer UTC release date" FORCE)
set(GEOMETER_RELEASE_VERSION "2026.6.9" CACHE STRING "Geometer package/build version" FORCE)
set(GEOMETER_ABI_VERSION "20260609" CACHE STRING "Geometer C ABI generation date" FORCE)

project(geometer VERSION ${GEOMETER_RELEASE_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(_geometer_dist_os "windows")
elseif(APPLE)
    set(_geometer_dist_os "macos")
elseif(UNIX)
    set(_geometer_dist_os "linux")
else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" _geometer_dist_os)
endif()

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _geometer_dist_arch)
if(_geometer_dist_arch MATCHES "^(amd64|x86_64)$")
    set(_geometer_dist_arch "x64")
elseif(_geometer_dist_arch MATCHES "^(aarch64|arm64)$")
    set(_geometer_dist_arch "arm64")
elseif(_geometer_dist_arch MATCHES "^(i386|i686|x86)$")
    set(_geometer_dist_arch "x86")
endif()

set(GEOMETER_NATIVE_DIST_PLATFORM "${_geometer_dist_os}-${_geometer_dist_arch}"
    CACHE STRING "Geometer native dist platform directory")
set(GEOMETER_NATIVE_DIST_DIR
    "${CMAKE_SOURCE_DIR}/dist/native/${GEOMETER_NATIVE_DIST_PLATFORM}"
    CACHE PATH "Geometer platform-specific native dist directory")
set(GEOMETER_WASM_BROWSER_DIST_DIR
    "${CMAKE_SOURCE_DIR}/dist/wasm/browser"
    CACHE PATH "Geometer full browser WASM dist directory")
set(GEOMETER_WASM_NODE_TEST_DIST_DIR
    "${CMAKE_SOURCE_DIR}/dist/wasm/node-test"
    CACHE PATH "Geometer Node CLI test WASM dist directory")
set(GEOMETER_WASM_PLANAR_BROWSER_DIST_DIR
    "${CMAKE_SOURCE_DIR}/dist/wasm/planar-browser"
    CACHE PATH "Geometer planar browser WASM dist directory")

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
#   python scripts/build_occt.py --clean  # wipe and rebuild current platform
# ---------------------------------------------------------------------------
set(GEOMETER_OCCT_LIBRARY_TYPE "Static" CACHE STRING "OCCT library type to auto-build when missing")
set_property(CACHE GEOMETER_OCCT_LIBRARY_TYPE PROPERTY STRINGS Static Shared)
option(GEOMETER_COPY_OCCT_RUNTIME "Copy shared OCCT runtime DLLs into the native dist folder" OFF)

if(APPLE)
    set(GEOMETER_MACOS_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}"
        CACHE STRING "Minimum macOS deployment target for Geometer and bundled native dependencies")
endif()

set(_geometer_native_deps_dir "${CMAKE_SOURCE_DIR}/.deps/native/${GEOMETER_NATIVE_DIST_PLATFORM}")
if(GEOMETER_OCCT_LIBRARY_TYPE STREQUAL "Shared")
    set(_geometer_default_occt_install "${_geometer_native_deps_dir}/occt-shared-install")
else()
    set(_geometer_default_occt_install "${_geometer_native_deps_dir}/occt-install")
endif()

set(_geometer_default_occt_dir "${_geometer_default_occt_install}/lib/cmake/opencascade")
set(_geometer_legacy_occt_dir "${_geometer_default_occt_install}/cmake")

set(_geometer_occt_dirs)
if(OpenCASCADE_DIR AND NOT OpenCASCADE_DIR MATCHES "-NOTFOUND$")
    list(APPEND _geometer_occt_dirs "${OpenCASCADE_DIR}")
endif()
list(APPEND _geometer_occt_dirs "${_geometer_default_occt_dir}" "${_geometer_legacy_occt_dir}")

find_package(OpenCASCADE QUIET NO_DEFAULT_PATH PATHS ${_geometer_occt_dirs})

if(NOT OpenCASCADE_FOUND)
    message(STATUS "OpenCASCADE not found. Building from source via scripts/build_occt.py ...")
    message(STATUS "This is a one-time operation and will take ~10-15 minutes.")

    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(_geometer_occt_build_args
        --library-type ${GEOMETER_OCCT_LIBRARY_TYPE}
        --platform-tag ${GEOMETER_NATIVE_DIST_PLATFORM}
    )
    if(APPLE)
        list(APPEND _geometer_occt_build_args
            --macos-deployment-target ${GEOMETER_MACOS_DEPLOYMENT_TARGET}
        )
    endif()

    execute_process(
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/scripts/build_occt.py" ${_geometer_occt_build_args}
        RESULT_VARIABLE _occt_build_result
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    if(NOT _occt_build_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to build OCCT. You can try manually:\n"
            "  python scripts/build_occt.py --library-type ${GEOMETER_OCCT_LIBRARY_TYPE}\n"
            "Check the output above for errors."
        )
    endif()

    set(OpenCASCADE_DIR "${_geometer_default_occt_dir}" CACHE PATH
        "The directory containing a CMake configuration file for OpenCASCADE." FORCE)
    find_package(OpenCASCADE REQUIRED NO_DEFAULT_PATH PATHS "${OpenCASCADE_DIR}" "${_geometer_legacy_occt_dir}")
endif()

message(STATUS "OpenCASCADE ${OpenCASCADE_VERSION} found at ${OpenCASCADE_INSTALL_PREFIX}")

set(GEOMETER_OCCT_RUNTIME_FILES "")
if(WIN32 AND GEOMETER_OCCT_LIBRARY_TYPE STREQUAL "Shared")
    set(_geometer_occt_runtime_dir "${OpenCASCADE_INSTALL_PREFIX}/win64/vc14/bin")
    if(EXISTS "${_geometer_occt_runtime_dir}")
        file(GLOB GEOMETER_OCCT_RUNTIME_FILES CONFIGURE_DEPENDS
            "${_geometer_occt_runtime_dir}/*.dll")
    elseif(GEOMETER_COPY_OCCT_RUNTIME)
        message(WARNING
            "GEOMETER_COPY_OCCT_RUNTIME is ON, but OCCT runtime DLL directory "
            "was not found: ${_geometer_occt_runtime_dir}")
    endif()
endif()

function(geometer_copy_occt_runtime_to_native_dist target_name)
    if(NOT EMSCRIPTEN AND GEOMETER_COPY_OCCT_RUNTIME AND GEOMETER_OCCT_RUNTIME_FILES)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${GEOMETER_NATIVE_DIST_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${GEOMETER_OCCT_RUNTIME_FILES}
                "${GEOMETER_NATIVE_DIST_DIR}/"
            COMMAND_EXPAND_LISTS
            COMMENT "Copying OCCT runtime DLLs to native dist/"
        )
    endif()
endfunction()

include(CTest)
add_subdirectory(src/cpp)

if(BUILD_TESTING)
    add_subdirectory(tests/cpp)
endif()

option(GEOMETER_BUILD_EXAMPLES "Build optional native Geometer examples" OFF)
if(GEOMETER_BUILD_EXAMPLES AND NOT EMSCRIPTEN)
    add_subdirectory(examples/cpp)
endif()
