include(FetchContent)

cmake_minimum_required(VERSION 3.16)

# force universal binary on osx (must go BEFORE project call)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
# specify minimum support macOS version
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    # If no deployment target has been set default to the minimum supported
    # OS version (this has to be set before the first project() call)
    if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
        set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0 CACHE STRING "iOS Deployment Target")
    else()
        set(CMAKE_OSX_DEPLOYMENT_TARGET 10.10 CACHE STRING "macOS Deployment Target")
    endif()
endif()

project(
    ChuGL
    VERSION 0.1.0
    LANGUAGES CXX C
)

if(LINUX)
    add_compile_options(-fPIC)
    #set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined" )
endif()

set(
    CORE 
    src/core/macros.h
    src/core/log.h      src/core/log.c
    src/core/memory.h   src/core/memory.cpp
)

# chugl library
add_library(
    ${PROJECT_NAME} SHARED 
    src/chugl.cpp 
    src/app.cpp          src/app.h
    src/sync.cpp         src/sync.h
    src/shaders.h
    src/graphics.cpp     src/graphics.h
    src/implementations.cpp
    ${CORE}
)

# Defines
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:CHUGL_DEBUG>
    $<$<CONFIG:Release>:CHUGL_RELEASE>
    GLM_FORCE_DEPTH_ZERO_TO_ONE # glm force depth range to 0-1
    GLM_ENABLE_EXPERIMENTAL
    LOG_USE_COLOR
)

# Recommended extras
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

# increasing warning levels
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)
endif()

# Windows warnings
if (MSVC)
	# Ignore a warning that GLM requires to bypass
	# Disable warning C4201: nonstandard extension used: nameless struct/union
	target_compile_options(${PROJECT_NAME} PUBLIC /wd4201)

    # Required for GLM lookat()
    # Disable warning C4127: conditional expression is constant
    target_compile_options(${PROJECT_NAME} PUBLIC /wd4127)

	# Disable warning C4305: truncation from 'int' to 'bool' in 'if' condition
	target_compile_options(${PROJECT_NAME} PUBLIC /wd4305)

	# Ignore a warning that stb_image requires to bypass
	# Disable warning C4244: conversion from 'int' to 'short', possible loss of data
	target_compile_options(${PROJECT_NAME} PUBLIC /wd4244)

    # ignore fopen warning for cgltf
	target_compile_options(${PROJECT_NAME} PUBLIC /wd4996)

    # allow unused variables for the ChuGin DLL Query
	target_compile_options(${PROJECT_NAME} PUBLIC /wd4100)

endif (MSVC)


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
target_link_libraries(${PROJECT_NAME} PRIVATE webgpu glfw glfw3webgpu)


# add vendor to include path
target_include_directories(${PROJECT_NAME} 
    PRIVATE src
    PRIVATE vendor
)

# emscripten specific options =================================================
if (EMSCRIPTEN)
    # know to rebuild when assets/ directory changes
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/assets)

    # Make sure to re-link when the shell file changes
    # set(SHELL_FILE shell_minimal.html)
    # set_property(
    #     TARGET ${PROJECT_NAME}
    #     PROPERTY LINK_DEPENDS
    #     "${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
    # )

    # Add Emscripten-specific link options
    target_link_options(${PROJECT_NAME} PRIVATE
        -sUSE_GLFW=3 # Use Emscripten-provided GLFW
        -sUSE_WEBGPU # Handle WebGPU symbols
        -sASYNCIFY # Required for emscripten_sleep in the async wgpu adapter and device request
        -sALLOW_MEMORY_GROWTH # allow memory to grow dynamically

        # uncomment to include local files in the build
        # https://emscripten.org/docs/porting/files/packaging_files.html
        # https://emscripten.org/docs/getting_started/Tutorial.html#tutorial-files
        # To include files outside of the current working directory
        # use the `--preload-file srcpath@dstpath` syntax to explicitly 
        # specify the target location
        --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/assets@./assets

        # Can specify custom html template
        # https://github.com/emscripten-core/emscripten/blob/main/src/shell_minimal.html
        # --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
endif (EMSCRIPTEN)

# IDE setup ====================================================================
if(XCODE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal")
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
