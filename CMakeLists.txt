cmake_minimum_required(VERSION 3.20)
project(spaceinvaders LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(engine
    src/engine/world.cpp
    src/engine/spatial_hash.cpp
    src/engine/batch.cpp
)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(engine PRIVATE -Wall -Wextra -Wpedantic)

add_executable(engine_tests
    src/tests/test_main.cpp
    src/tests/test_ecs.cpp
    src/tests/test_spatial.cpp
    src/tests/test_batch.cpp
)
target_link_libraries(engine_tests PRIVATE engine)
target_compile_options(engine_tests PRIVATE -Wall -Wextra -Wpedantic)

option(SPACEINVADERS_BUILD_GL "Build the OpenGL game" ON)

if (SPACEINVADERS_BUILD_GL)
    include(FetchContent)

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
    )

    set(GLM_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW TRUE
    )

    FetchContent_MakeAvailable(glfw glm)

    add_library(glad STATIC third_party/glad/src/gl.c)
    target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include)

    find_package(OpenGL REQUIRED)

    add_executable(spaceinvaders
        src/main.cpp
        src/engine/renderer.cpp
        src/game/atlas.cpp
        src/game/game.cpp
        src/game/stress.cpp
    )
    target_link_libraries(spaceinvaders PRIVATE engine glfw glad OpenGL::GL glm::glm)
    target_compile_definitions(spaceinvaders PRIVATE GLFW_INCLUDE_NONE)
    target_compile_options(spaceinvaders PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME engine_tests COMMAND engine_tests)
