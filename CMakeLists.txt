cmake_minimum_required(VERSION 3.16)
project(Simple3DGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SIMPLE3D_ENABLE_OPENGL "Build the interactive OpenGL/GLFW client" ON)
option(SIMPLE3D_ENABLE_ALSA "Enable the ALSA background music backend" ON)

set(simple3d_sources
    src/main.cpp
    src/audio/AudioEngine.cpp
)

add_executable(simple3d ${simple3d_sources})

target_include_directories(simple3d PRIVATE include)

if (SIMPLE3D_ENABLE_OPENGL)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 3.3 REQUIRED)
    target_link_libraries(simple3d PRIVATE OpenGL::GL glfw)
else()
    target_compile_definitions(simple3d PRIVATE SIMPLE3D_HEADLESS)
    target_include_directories(simple3d PRIVATE stubs)
endif()

if (SIMPLE3D_ENABLE_ALSA)
    if (UNIX AND NOT APPLE)
        find_package(ALSA)
        if (ALSA_FOUND)
            target_link_libraries(simple3d PRIVATE ALSA::ALSA)
        endif()
    endif()
else()
    target_compile_definitions(simple3d PRIVATE SIMPLE3D_NO_ALSA)
endif()

if (APPLE)
    target_link_libraries(simple3d PRIVATE "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
endif()
