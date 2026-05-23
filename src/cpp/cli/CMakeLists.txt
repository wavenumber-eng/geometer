add_executable(geometer main.cpp)
target_link_libraries(geometer PRIVATE geometer_lib)

if(EMSCRIPTEN)
    # Build as a Node.js CLI that reads/writes the real filesystem.
    target_link_options(geometer PRIVATE
        -sNODERAWFS=1
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=67108864
        -sSTACK_SIZE=8388608
        -sENVIRONMENT=node
        -sMODULARIZE=0
        -sEXIT_RUNTIME=1
    )
    set_target_properties(geometer PROPERTIES
        OUTPUT_NAME "geometer-node-test"
        SUFFIX ".js"
    )
endif()

# Copy outputs to dist/ after build.
add_custom_command(TARGET geometer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:geometer>"
        "${CMAKE_SOURCE_DIR}/dist/"
    COMMENT "Copying geometer to dist/"
)

if(EMSCRIPTEN)
    # Also copy the .wasm file.
    add_custom_command(TARGET geometer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:geometer>/geometer-node-test.wasm"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMENT "Copying geometer-node-test.wasm to dist/"
    )
endif()
