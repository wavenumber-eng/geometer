add_subdirectory(lib)
add_subdirectory(cli)

if(NOT EMSCRIPTEN)
    add_executable(geometer_sdk_link_probe EXCLUDE_FROM_ALL sdk/link_probe.cpp)
    target_link_libraries(geometer_sdk_link_probe PRIVATE geometer_lib)
endif()
