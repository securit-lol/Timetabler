cmake_minimum_required(VERSION 3.16)
project(Timetabler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

if(WIN32)
    set(TT_BACKEND "DX11" CACHE STRING "Render backend: DX11 or GL3")
else()
    set(TT_BACKEND "GL3" CACHE STRING "Render backend: DX11 or GL3")
endif()

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
)

set(BACKEND_SOURCES "")
set(BACKEND_LIBS "")

if(TT_BACKEND STREQUAL "DX11")
    list(APPEND IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    )
    set(BACKEND_SOURCES src/platform/backend_dx11.cpp)
    list(APPEND BACKEND_LIBS d3d11 dxgi d3dcompiler)
else()
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)

    find_package(OpenGL REQUIRED)
    list(APPEND IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    set(BACKEND_SOURCES src/platform/backend_gl3.cpp)
    list(APPEND BACKEND_LIBS glfw OpenGL::GL)
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC ${BACKEND_LIBS})

set(APP_SOURCES
    src/main.cpp
    src/model/model.cpp
    src/logic/scheduler.cpp
    src/ui/app_ui.cpp
    src/platform/fonts.cpp
    ${BACKEND_SOURCES}
)

add_executable(timetabler ${APP_SOURCES})
target_include_directories(timetabler PRIVATE src)
target_link_libraries(timetabler PRIVATE imgui)

if(TT_BACKEND STREQUAL "DX11")
    target_compile_definitions(timetabler PRIVATE TT_BACKEND_DX11)
else()
    target_compile_definitions(timetabler PRIVATE TT_BACKEND_GL3)
endif()

message(STATUS "Timetabler backend: ${TT_BACKEND}")

option(TT_BUILD_TESTS "Build scheduler unit tests" OFF)
if(TT_BUILD_TESTS)
    add_executable(scheduler_test
        tests/scheduler_test.cpp
        src/model/model.cpp
        src/logic/scheduler.cpp
    )
    target_include_directories(scheduler_test PRIVATE src)
    enable_testing()
    add_test(NAME scheduler_test COMMAND scheduler_test)
endif()
