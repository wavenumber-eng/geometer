add_executable(geometer_planar_contours_test
    planar_contours_test.cpp
    ${CMAKE_SOURCE_DIR}/src/cpp/lib/planar_contours.cpp
)

target_include_directories(geometer_planar_contours_test PRIVATE
    ${CMAKE_SOURCE_DIR}/src/cpp/lib
)

add_test(NAME geometer_planar_contours_test COMMAND geometer_planar_contours_test)

add_executable(geometer_planar_solve_test
    planar_solve_test.cpp
)

target_link_libraries(geometer_planar_solve_test PRIVATE geometer_lib)

add_test(NAME geometer_planar_solve_test COMMAND geometer_planar_solve_test)

add_executable(geometer_projection_options_json_test
    projection_options_json_test.cpp
)

target_link_libraries(geometer_projection_options_json_test PRIVATE geometer_lib)

add_test(NAME geometer_projection_options_json_test COMMAND geometer_projection_options_json_test)

add_executable(geometer_projection_model_transform_test
    projection_model_transform_test.cpp
)

target_link_libraries(geometer_projection_model_transform_test PRIVATE geometer_lib)

target_compile_definitions(geometer_projection_model_transform_test PRIVATE
    GEOMETER_TEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

add_test(NAME geometer_projection_model_transform_test COMMAND geometer_projection_model_transform_test)

if(WIN32 AND OpenCASCADE_BINARY_DIR)
    set_tests_properties(
        geometer_planar_solve_test
        geometer_projection_options_json_test
        geometer_projection_model_transform_test
        PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${OpenCASCADE_BINARY_DIR}"
    )
endif()
