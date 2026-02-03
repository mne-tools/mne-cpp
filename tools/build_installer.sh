#!/bin/bash

# Define build directory
BUILD_DIR="build_installer"
SOURCE_DIR="src"

# Get number of cores for parallel build
if [[ "$OSTYPE" == "darwin"* ]]; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=$(nproc)
fi

# Check for CMAKE_PREFIX_PATH
if [ -z "$CMAKE_PREFIX_PATH" ]; then
    echo "WARNING: CMAKE_PREFIX_PATH is not set."
    echo "If libraries (like Qt) are not found, please export CMAKE_PREFIX_PATH."
    echo "Example: export CMAKE_PREFIX_PATH=~/Qt/6.6.0/macos"
fi

# Check for Qt Installer Framework (binarycreator)
if ! command -v binarycreator &> /dev/null; then
    echo "WARNING: 'binarycreator' not found in PATH."
    echo "Please ensure Qt Installer Framework is installed and added to PATH."
    echo "Or set 'QtInstallerFramework_DIR' or 'CPACK_IFW_ROOT'."
    
    # Try to check a common location for IFW
    POSSIBLE_IFW_PATH="$HOME/Qt/Tools/QtInstallerFramework/4.0/bin" # Example path
    if [ -d "$POSSIBLE_IFW_PATH" ]; then
         export PATH=$PATH:$POSSIBLE_IFW_PATH
         echo "Found potential IFW at $POSSIBLE_IFW_PATH. Added to PATH."
    fi
fi

echo "-----------------------------------------------------------------"
echo "           MNE-CPP Installer Generation Script                   "
echo "-----------------------------------------------------------------"

# Configure project with installer enabled
echo "1. Configuring project with MNE_ENABLE_INSTALLER=ON..."
cmake -S $SOURCE_DIR -B $BUILD_DIR -DMNE_ENABLE_INSTALLER=ON -DCMAKE_BUILD_TYPE=Release

if [ $? -ne 0 ]; then
    echo "Error: Configuration failed."
    exit 1
fi

# Build the package
echo "2. Building Installer Package (using $CORES cores)..."
cmake --build $BUILD_DIR --target package -j$CORES

if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

echo "-----------------------------------------------------------------"
echo "Success! Installer generated in $BUILD_DIR"
echo "-----------------------------------------------------------------"
