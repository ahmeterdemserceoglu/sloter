#!/bin/bash

echo "=== Building Slot Machine for Ubuntu ==="

# Check if dependencies are installed
echo "Checking dependencies..."

# Check for required packages
REQUIRED_PACKAGES=("cmake" "g++" "libpq-dev" "libssl-dev" "libjsoncpp-dev" "libcurl4-openssl-dev")
MISSING_PACKAGES=()

for package in "${REQUIRED_PACKAGES[@]}"; do
    if ! dpkg -l | grep -q "^ii  $package "; then
        MISSING_PACKAGES+=("$package")
    fi
done

if [ ${#MISSING_PACKAGES[@]} -ne 0 ]; then
    echo "Missing required packages: ${MISSING_PACKAGES[*]}"
    echo "Please run: ./install_dependencies.sh"
    exit 1
fi

# Check if PostgreSQL is running
if ! systemctl is-active --quiet postgresql; then
    echo "PostgreSQL is not running. Starting..."
    sudo systemctl start postgresql
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo "Configuring build with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_DATABASE=ON -DUSE_AUTH=ON

if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    exit 1
fi

# Build the project
echo "Building project..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

# Run tests if available
if [ -f "test/SlotMachineTests" ]; then
    echo "Running tests..."
    ./test/SlotMachineTests
fi

cd ..

echo ""
echo "=== Build Successful ==="
echo ""
echo "Executable: build/SlotMachine"
echo ""
echo "To run the application:"
echo "1. Make sure PostgreSQL is running: sudo systemctl start postgresql"
echo "2. Setup database: ./database/setup_database.sh"
echo "3. Run application: ./build/SlotMachine"
echo ""
echo "For development:"
echo "- Debug build: cmake .. -DCMAKE_BUILD_TYPE=Debug"
echo "- Run with debugger: gdb ./build/SlotMachine"
echo "- Memory check: valgrind ./build/SlotMachine"
echo ""