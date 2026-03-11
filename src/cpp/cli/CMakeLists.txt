add_executable(geometer main.cpp)
target_link_libraries(geometer PRIVATE geometer_lib)

# Copy executable to dist/ after build.
add_custom_command(TARGET geometer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:geometer>"
        "${CMAKE_SOURCE_DIR}/dist/"
    COMMENT "Copying geometer to dist/"
)
