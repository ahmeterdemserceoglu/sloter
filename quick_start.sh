#!/bin/bash

echo "=== Slot Machine Quick Start ==="

# Check if PostgreSQL is running
if ! systemctl is-active --quiet postgresql; then
    echo "Starting PostgreSQL..."
    sudo systemctl start postgresql
fi

# Check if database exists
if ! psql -h localhost -U slotmachine_user -d slotmachine_db -c "SELECT 1;" &>/dev/null; then
    echo "Database not found. Setting up..."
    ./database/setup_database.sh
fi

# Build if needed
if [ ! -f "build/SlotMachine" ]; then
    echo "Building application..."
    ./build_ubuntu.sh
fi

# Run the application
if [ -f "build/SlotMachine" ]; then
    echo "Starting Slot Machine..."
    ./build/SlotMachine
else
    echo "Build failed. Please check the build output."
    exit 1
fi