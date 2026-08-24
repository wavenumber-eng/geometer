cmake_minimum_required(VERSION 3.24)

project(geometer_occt_xcaf_custom_driver_matrix LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenCASCADE REQUIRED)

if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR "The selected Windows matrix requires the OCCT MSVC v143 ABI.")
endif()

add_executable(geometer_xcaf_custom_driver_matrix
    ../step_topology_xcaf_custom_driver_test.cpp
)
target_include_directories(geometer_xcaf_custom_driver_matrix PRIVATE
    ${OpenCASCADE_INCLUDE_DIR}
)
target_link_directories(geometer_xcaf_custom_driver_matrix PRIVATE
    ${OpenCASCADE_LIBRARY_DIR}
)

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    string(REPLACE ";" "," _occt_link_group "${OpenCASCADE_LIBRARIES}")
    target_link_libraries(geometer_xcaf_custom_driver_matrix PRIVATE
        "$<LINK_GROUP:RESCAN,${_occt_link_group}>"
        Threads::Threads
        ${CMAKE_DL_LIBS}
    )
else()
    target_link_libraries(geometer_xcaf_custom_driver_matrix PRIVATE
        ${OpenCASCADE_LIBRARIES}
    )
endif()

if(MSVC)
    target_compile_options(geometer_xcaf_custom_driver_matrix PRIVATE /EHsc)
endif()
