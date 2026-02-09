#!/bin/bash
#
# MNE-CPP Installer Generation Script
#
# Builds the MNE-CPP installer package using CPack (Qt Installer Framework).
#
# Installer Components:
#   1. Applications       - MNE-CPP binaries (required)
#   2. Runtime Libraries  - Shared libraries (required, hidden)
#   3. Development SDK    - Headers, static libs, CMake config (opt-in)
#   4. MNE Sample Dataset - Downloads ~1.5 GB sample data (opt-in)
#   5. MNE Python         - Installs MNE-Python via pip (opt-in)
#   6. PATH Configuration - Sets PATH and environment variables (opt-in)
#
# Usage:
#   ./tools/build_installer.sh
#
# Prerequisites:
#   - CMake 3.15+
#   - Qt (set CMAKE_PREFIX_PATH)
#   - Qt Installer Framework (binarycreator in PATH or QtInstallerFramework_DIR set)
#

set -e

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
echo ""
echo "  Components included:"
echo "    [required] Applications (binaries)"
echo "    [required] Runtime Libraries (shared libs)"
echo "    [opt-in]  Development SDK (headers + static libs + CMake config)"
echo "    [opt-in]  MNE Sample Dataset (download script)"
echo "    [opt-in]  MNE Python (pip install script)"
echo "    [opt-in]  PATH Configuration (environment setup script)"
echo ""

# Ensure packaging scripts are executable
SCRIPTS_DIR="tools/packaging/scripts"
if [ -d "$SCRIPTS_DIR" ]; then
    chmod +x "$SCRIPTS_DIR"/*.sh 2>/dev/null || true
fi

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
echo ""
echo "Post-installation scripts available at:"
echo "  ${BUILD_DIR}/scripts/download_sample_data.sh"
echo "  ${BUILD_DIR}/scripts/install_mne_python.sh"
echo "  ${BUILD_DIR}/scripts/configure_environment.sh"
echo "-----------------------------------------------------------------"
