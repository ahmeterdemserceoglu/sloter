cmake_minimum_required(VERSION 3.18)
project(SlotMachine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android NDK configuration
if(ANDROID)
    set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION clang)
    set(CMAKE_ANDROID_STL_TYPE c++_shared)
endif()

# Find required packages for desktop build
if(NOT ANDROID)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBPQ REQUIRED libpq)
    find_package(OpenSSL REQUIRED)
    find_package(CURL REQUIRED)
    find_package(OpenGL REQUIRED)
    
    # Find jsoncpp
    find_path(JSONCPP_INCLUDE_DIR json/json.h
        PATHS /usr/include/jsoncpp /usr/local/include/jsoncpp)
    find_library(JSONCPP_LIBRARY jsoncpp
        PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)
    
    if(NOT JSONCPP_INCLUDE_DIR OR NOT JSONCPP_LIBRARY)
        message(FATAL_ERROR "jsoncpp not found. Please install libjsoncpp-dev")
    endif()
endif()

# Include directories
include_directories(src)
include_directories(src/core)
include_directories(src/security)
include_directories(src/payment)
include_directories(src/ui)
include_directories(src/utils)
include_directories(src/database)
include_directories(src/auth)

if(NOT ANDROID)
    include_directories(${LIBPQ_INCLUDE_DIRS})
    include_directories(${JSONCPP_INCLUDE_DIR})
    include_directories(${OPENSSL_INCLUDE_DIR})
    include_directories(${CURL_INCLUDE_DIRS})
endif()

# Source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.h"
)

# Create executable
add_executable(SlotMachine ${SOURCES})

# Compiler definitions
if(NOT ANDROID)
    add_definitions(-DUSE_DATABASE)
    add_definitions(-DUSE_AUTH)
endif()

# Link libraries
if(ANDROID)
    target_link_libraries(SlotMachine
        android
        log
        EGL
        GLESv2
        OpenSLES
    )
else()
    # Desktop libraries
    target_link_libraries(SlotMachine 
        ${OPENGL_LIBRARIES}
        ${LIBPQ_LIBRARIES}
        ${JSONCPP_LIBRARY}
        ${OPENSSL_LIBRARIES}
        ${CURL_LIBRARIES}
        pthread
    )
    
    # Add library directories and compile flags
    target_link_directories(SlotMachine PRIVATE ${LIBPQ_LIBRARY_DIRS})
    target_compile_options(SlotMachine PRIVATE ${LIBPQ_CFLAGS_OTHER})
endif()

# Install rules
if(NOT ANDROID)
    install(TARGETS SlotMachine DESTINATION bin)
endif()