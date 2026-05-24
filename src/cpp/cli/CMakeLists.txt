add_executable(geometer main.cpp)
target_link_libraries(geometer PRIVATE geometer_lib)
target_include_directories(geometer PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/rapidjson/include
)

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

if(EMSCRIPTEN)
    # Copy Node CLI parity/test outputs to the platform-aware WASM dist folder.
    # Flat dist/ copies are retained as compatibility aliases during the
    # transition to the grouped dist layout.
    add_custom_command(TARGET geometer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GEOMETER_WASM_NODE_TEST_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer>"
            "${GEOMETER_WASM_NODE_TEST_DIST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:geometer>/geometer-node-test.wasm"
            "${GEOMETER_WASM_NODE_TEST_DIST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer>"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:geometer>/geometer-node-test.wasm"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMENT "Copying geometer-node-test outputs to dist/"
    )
else()
    # Copy native CLI to the platform-aware native dist folder. The flat dist/
    # copy is retained as a compatibility alias for existing source checkouts.
    add_custom_command(TARGET geometer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GEOMETER_NATIVE_DIST_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/dist"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer>"
            "${GEOMETER_NATIVE_DIST_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:geometer>"
            "${CMAKE_SOURCE_DIR}/dist/"
        COMMENT "Copying geometer to dist/"
    )
endif()
