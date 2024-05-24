include(FetchContent)

cmake_minimum_required(VERSION 3.16)

set(CMAKE_VERBOSE_MAKEFILE OFF)

# force universal binary on osx (must go BEFORE project call)
# TODO: universal binary not working with webgpu-distribution
# set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "" FORCE)
# # specify minimum support macOS version
# if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
#     # If no deployment target has been set default to the minimum supported
#     # OS version (this has to be set before the first project() call)
#     if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
#         set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "iOS Deployment Target")
#     else()
#         set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10 CACHE STRING "macOS Deployment Target")
#     endif()
# endif()

project(
    ChuGL
    VERSION 0.1.0
    LANGUAGES CXX C
)

if(LINUX OR EMSCRIPTEN)
    add_compile_options(-fPIC)  # needed for emcripten
    #set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
endif()


if ( MSVC )
    # Set the startup project to be the actual project instead of ALL_BUILD 
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

    set(CHUCK_EXECUTABLE_DEBUG "${CMAKE_SOURCE_DIR}/chuck-debug.exe")
    set(CHUCK_EXECUTABLE_RELEASE "${CMAKE_SOURCE_DIR}/chuck-release.exe")
    set(CHUGIN_PATH_DEBUG "--chugin-path:${CMAKE_TARGET_DIR}./build/Debug")
    set(CHUGIN_PATH_RELEASE "--chugin-path:${CMAKE_TARGET_DIR}./build/Release")
    set_target_properties( ${PROJECT_NAME} PROPERTIES
        # VS_DEBUGGER_COMMAND $<$<CONFIG:Debug>:${CHUCK_EXECUTABLE_DEBUG}>$<$<CONFIG:Release>:${CHUCK_EXECUTABLE_RELEASE}>
        VS_DEBUGGER_COMMAND "C:/Program Files/ChucK/chuck.exe"

        VS_DEBUGGER_COMMAND_ARGUMENTS "$<$<CONFIG:Debug>:${CHUGIN_PATH_DEBUG}>$<$<CONFIG:Release>:${CHUGIN_PATH_RELEASE}> --chugin-probe"

        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()


set(
    CORE 
    src/core/log.c
    src/core/hashmap.c
    src/core/memory.cpp
)

set(
    VENDOR
    src/implementations.cpp
    vendor/mikktspace/mikktspace.c
)

set(
    RENDERER_TESTS
    # src/tests/obj.cpp
    src/tests/gltf.cpp
)

if (CHUGL_BUILD_RENDERER_TESTS)
    message(STATUS "Building Renderer Tester")
    add_executable(
        ChuGL-Renderer-Tester
        src/main.cpp
        ${CORE}
        ${VENDOR}
        ${RENDERER_TESTS}
    )

    # benchamrks TODO: put behind option flag, link with compile options 
    # add_executable(
    #     ChuGL-Benchmarks
    #     bench/hashmap.cpp
    #     ${CORE}
    # )

    set_target_properties(ChuGL-Renderer-Tester PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS OFF
        COMPILE_WARNING_AS_ERROR ON
        VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
    )

    # IDE setup
    if(XCODE)
        set_target_properties(ChuGL-Renderer-Tester PROPERTIES
            XCODE_GENERATE_SCHEME ON
            XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
            XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        )
    endif()
endif()

# chugl library
if (EMSCRIPTEN)
    # emcmake can't handle shared libraries. SIDE_LOAD executable is the only option
    add_executable(
        ${PROJECT_NAME} 
        src/chugl.cpp 
        ${CORE}
        ${VENDOR}
    )
else()
    add_library(
        ${PROJECT_NAME} SHARED 
        src/chugl.cpp 
        ${CORE}
        ${VENDOR}
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 11
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
    VS_DEBUGGER_ENVIRONMENT "DAWN_DEBUG_BREAK_ON_ERROR=1"
    POSITION_INDEPENDENT_CODE ON
    PREFIX ""
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX ".chug"
)

# =================================================================================
# interface library for CMake properties
# =================================================================================
add_library(chugl_shared_properties INTERFACE)

# Defines
target_compile_definitions(chugl_shared_properties INTERFACE 
    $<$<CONFIG:Debug>:CHUGL_DEBUG>
    $<$<CONFIG:Release>:CHUGL_RELEASE>
    GLM_FORCE_DEPTH_ZERO_TO_ONE # glm force depth range to 0-1
    GLM_ENABLE_EXPERIMENTAL
    LOG_USE_COLOR
)

# increasing warning levels
if (MSVC)
    target_compile_options(chugl_shared_properties INTERFACE /W4)
else()
    target_compile_options(chugl_shared_properties INTERFACE -Wall -Wextra -pedantic)

    # disable warnings
    target_compile_options(chugl_shared_properties INTERFACE 
        -Wno-unused-parameter
        -Wno-deprecated-declarations
        -Wno-compound-token-split-by-macro
    )
endif()

# Ignore Windows warnings
if (MSVC)
	target_compile_options(
        chugl_shared_properties INTERFACE
        # Ignore a warning that GLM requires to bypass
        # Disable warning C4201: nonstandard extension used: nameless struct/union
        /wd4201
        # Required for GLM lookat()
        # Disable warning C4127: conditional expression is constant
        /wd4127
        # Disable warning C4305: truncation from 'int' to 'bool' in 'if' condition
        /wd4305
        # Ignore a warning that stb_image requires to bypass
        # Disable warning C4244: conversion from 'int' to 'short', possible loss of data
        /wd4244
        # ignore fopen warning for cgltf
        /wd4996
    )
    target_compile_options(
        ${PROJECT_NAME} PRIVATE
        # allow unused variables for the ChuGin DLL Query
        /wd4100
    )
endif (MSVC)

# add include path
target_include_directories(chugl_shared_properties 
    INTERFACE src
    INTERFACE vendor
)

# vendor dependencies ==========================================================

if (NOT EMSCRIPTEN)
    add_subdirectory(vendor/glfw)
endif()
add_subdirectory(vendor/glfw3webgpu)

# TODO: host our own distribution on CCRMA
# switching to static library instead of dynami b/c
# chuck cannot find build/Debug/webgpu_native.dll.
# throws Error [126]: The specified module could not be found.
# i.e. ChuGL DLL needs wgpu DLL which cannot be not found.
# will need to fix this to implement ChuGLins
FetchContent_Declare(
  webgpu
  GIT_REPOSITORY https://github.com/AndrewAday/webgpu-distribution
  GIT_TAG        main
)
FetchContent_MakeAvailable(webgpu)

# linking
# target_link_libraries(${PROJECT_NAME} PRIVATE webgpu glfw glfw3webgpu)
target_link_libraries(chugl_shared_properties INTERFACE webgpu glfw glfw3webgpu)
target_link_libraries(${PROJECT_NAME} PRIVATE chugl_shared_properties)
if (CHUGL_BUILD_RENDERER_TESTS)
    target_link_libraries(ChuGL-Renderer-Tester PRIVATE chugl_shared_properties)
endif()

# emscripten specific options =================================================
if (EMSCRIPTEN)
    # know to rebuild when assets/ directory changes
    set(SHELL_FILE shell_minimal.html)
    set_property(
        TARGET ${PROJECT_NAME}
        PROPERTY LINK_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}
    )
    # set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets)

    # Add Emscripten-specific link options
    target_link_options(${PROJECT_NAME} PRIVATE
        -sUSE_GLFW=3 # Use Emscripten-provided GLFW
        -sUSE_WEBGPU # Handle WebGPU symbols
        -sALLOW_MEMORY_GROWTH # allow memory to grow dynamically
        -sSIDE_MODULE=1  # needed for chuck to dynamically load the chugin

        # add thread support
        # -pthread

        # removing because ASYNCIFY not supported in side module
        # using infinite loop instead
        # -sASYNCIFY # Required for emscripten_sleep in the async wgpu adapter and device request

        # -sDISABLE_EXCEPTION_CATCHING=0

        # uncomment to include local files in the build
        # https://emscripten.org/docs/porting/files/packaging_files.html
        # https://emscripten.org/docs/getting_started/Tutorial.html#tutorial-files
        # To include files outside of the current working directory
        # use the `--preload-file srcpath@dstpath` syntax to explicitly 
        # specify the target location
        # --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@./assets

        # Can specify custom html template
        # https://github.com/emscripten-core/emscripten/blob/main/src/shell_minimal.html
        # --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".wasm")

    # emscripten only generates .html properly if target is a MAIN_MODULE
    # set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
endif (EMSCRIPTEN)
