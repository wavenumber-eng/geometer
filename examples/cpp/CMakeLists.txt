include(FetchContent)

find_package(OpenGL REQUIRED)

set(GEOMETER_IMGUI_VERSION "v1.92.8")
set(GEOMETER_IMGUI_COMMIT "8936b58fe26e8c3da834b8f60b06511d537b4c63")
set(GEOMETER_SDL_VERSION "release-3.2.30")
set(GEOMETER_SDL_COMMIT "f5e5f6588921eed3d7d048ce43d9eb1ff0da0ffc")

FetchContent_Declare(
    imgui
    URL "https://github.com/ocornut/imgui/archive/${GEOMETER_IMGUI_COMMIT}.tar.gz"
    URL_HASH SHA256=a1e4443190d78976e0901039dd2207e1d59a4b5b818489232568bdb69ff79ce6
)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    SDL3
    URL "https://github.com/libsdl-org/SDL/archive/${GEOMETER_SDL_COMMIT}.tar.gz"
    URL_HASH SHA256=7f80bce51b7179bab472e75b90ce8ac06b162b3da8e19feadecee5acea07dbfb
)

FetchContent_MakeAvailable(imgui SDL3)

if(TARGET SDL3::SDL3-static)
    set(GEOMETER_SDL_TARGET SDL3::SDL3-static)
elseif(TARGET SDL3::SDL3)
    set(GEOMETER_SDL_TARGET SDL3::SDL3)
else()
    message(FATAL_ERROR "SDL3 CMake target was not created")
endif()

set(GEOMETER_IMGUI_DIR "${imgui_SOURCE_DIR}")

add_library(geometer_imgui_sdl3_opengl3 STATIC
    "${GEOMETER_IMGUI_DIR}/imgui.cpp"
    "${GEOMETER_IMGUI_DIR}/imgui_draw.cpp"
    "${GEOMETER_IMGUI_DIR}/imgui_tables.cpp"
    "${GEOMETER_IMGUI_DIR}/imgui_widgets.cpp"
    "${GEOMETER_IMGUI_DIR}/backends/imgui_impl_sdl3.cpp"
    "${GEOMETER_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)
target_include_directories(geometer_imgui_sdl3_opengl3 PUBLIC
    "${GEOMETER_IMGUI_DIR}"
    "${GEOMETER_IMGUI_DIR}/backends"
)
target_link_libraries(geometer_imgui_sdl3_opengl3 PUBLIC
    ${GEOMETER_SDL_TARGET}
    OpenGL::GL
)

add_executable(geometer_hlr_preview hlr_preview.cpp)
target_link_libraries(geometer_hlr_preview PRIVATE
    geometer_lib
    geometer_imgui_sdl3_opengl3
)
target_include_directories(geometer_hlr_preview PRIVATE
    "${CMAKE_SOURCE_DIR}/third_party/rapidjson/include"
)

if(MSVC)
    target_compile_options(geometer_hlr_preview PRIVATE /W4)
    target_compile_definitions(geometer_hlr_preview PRIVATE
        _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
    )
else()
    target_compile_options(geometer_hlr_preview PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(geometer_hlr_preview PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/cpp"
)
