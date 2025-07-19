#!/bin/bash

echo "=== Testing Build Environment ==="

# Test basic commands
echo "Testing cmake..."
cmake --version

echo "Testing g++..."
g++ --version

echo "Testing pkg-config..."
pkg-config --version

echo "Testing PostgreSQL development libraries..."
pkg-config --exists libpq && echo "libpq: OK" || echo "libpq: MISSING"

echo "Testing OpenSSL..."
pkg-config --exists openssl && echo "openssl: OK" || echo "openssl: MISSING"

echo "Testing jsoncpp..."
pkg-config --exists jsoncpp && echo "jsoncpp: OK" || echo "jsoncpp: MISSING"

echo "Testing libcurl..."
pkg-config --exists libcurl && echo "libcurl: OK" || echo "libcurl: MISSING"

echo ""
echo "=== Creating minimal test build ==="

# Create a minimal test
mkdir -p test_build
cd test_build

cat > test.cpp << 'EOF'
#include <iostream>
#include <libpq-fe.h>
#include <openssl/sha.h>
#include <json/json.h>
#include <curl/curl.h>

int main() {
    std::cout << "Testing libraries..." << std::endl;
    
    // Test PostgreSQL
    std::cout << "PostgreSQL version: " << PQlibVersion() << std::endl;
    
    // Test JSON
    Json::Value root;
    root["test"] = "value";
    std::cout << "JSON test: " << root["test"].asString() << std::endl;
    
    // Test CURL
    curl_version_info_data* curl_info = curl_version_info(CURLVERSION_NOW);
    std::cout << "CURL version: " << curl_info->version << std::endl;
    
    std::cout << "All libraries working!" << std::endl;
    return 0;
}
EOF

# Create simple CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.18)
project(TestBuild)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQ REQUIRED libpq)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

find_path(JSONCPP_INCLUDE_DIR json/json.h
    PATHS /usr/include/jsoncpp /usr/local/include/jsoncpp)
find_library(JSONCPP_LIBRARY jsoncpp
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu)

include_directories(${LIBPQ_INCLUDE_DIRS})
include_directories(${JSONCPP_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CURL_INCLUDE_DIRS})

add_executable(test test.cpp)

target_link_libraries(test 
    ${LIBPQ_LIBRARIES}
    ${JSONCPP_LIBRARY}
    ${OPENSSL_LIBRARIES}
    ${CURL_LIBRARIES}
)

target_link_directories(test PRIVATE ${LIBPQ_LIBRARY_DIRS})
target_compile_options(test PRIVATE ${LIBPQ_CFLAGS_OTHER})
EOF

# Try to build
echo "Configuring with CMake..."
cmake .

if [ $? -eq 0 ]; then
    echo "CMake configuration successful!"
    echo "Building..."
    make
    
    if [ $? -eq 0 ]; then
        echo "Build successful!"
        echo "Running test..."
        ./test
    else
        echo "Build failed!"
    fi
else
    echo "CMake configuration failed!"
fi

cd ..
echo "Test completed."