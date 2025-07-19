#!/bin/bash

echo "=== Installing Slot Machine Dependencies ==="

# Update package list
sudo apt update

# Install build essentials
echo "Installing build tools..."
sudo apt install -y build-essential cmake git

# Install PostgreSQL and development libraries
echo "Installing PostgreSQL..."
sudo apt install -y postgresql postgresql-contrib postgresql-client libpq-dev

# Install OpenSSL development libraries
echo "Installing OpenSSL..."
sudo apt install -y libssl-dev

# Install JSON library
echo "Installing JSON library..."
sudo apt install -y libjsoncpp-dev

# Install CURL development libraries
echo "Installing CURL..."
sudo apt install -y libcurl4-openssl-dev

# Install OpenGL libraries (for desktop version)
echo "Installing OpenGL..."
sudo apt install -y libgl1-mesa-dev libglu1-mesa-dev

# Install additional security libraries
echo "Installing security libraries..."
sudo apt install -y libgcrypt20-dev

# Install JWT library (optional, for token handling)
echo "Installing JWT library..."
sudo apt install -y libjwt-dev

# Install Google Test (for unit testing)
echo "Installing Google Test..."
sudo apt install -y libgtest-dev

# Install pkg-config
sudo apt install -y pkg-config

# Install additional development tools
echo "Installing development tools..."
sudo apt install -y valgrind gdb

# For Android development (optional)
echo "Installing Java for Android development..."
sudo apt install -y openjdk-11-jdk

# Install Node.js and npm (for web dashboard - optional)
echo "Installing Node.js..."
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt install -y nodejs

echo ""
echo "=== Dependency Installation Complete ==="
echo ""
echo "Installed packages:"
echo "- Build tools: gcc, g++, cmake, make"
echo "- PostgreSQL: server, client, development libraries"
echo "- OpenSSL: development libraries"
echo "- JSON: jsoncpp development libraries"
echo "- CURL: development libraries"
echo "- OpenGL: Mesa development libraries"
echo "- Security: libgcrypt development libraries"
echo "- JWT: development libraries"
echo "- Testing: Google Test framework"
echo "- Java: OpenJDK 11 (for Android)"
echo "- Node.js: Runtime and npm"
echo ""
echo "Next steps:"
echo "1. Run: chmod +x database/setup_database.sh"
echo "2. Run: ./database/setup_database.sh"
echo "3. Build project: mkdir build && cd build && cmake .. && make"
echo ""