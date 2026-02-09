#!/bin/bash
#
#  configure_environment.sh
#  MNE-CPP Installer - PATH and Environment Variable Configuration
#
#  Configures system PATH and environment variables for MNE-CPP:
#   - Adds bin/ to PATH (for applications)
#   - Adds lib/ to library search path
#   - Sets MNE_CPP_ROOT pointing to the installation directory
#   - Sets MNE_CPP_SDK (include + lib paths for development)
#   - Sets MNE_DATASETS_SAMPLE_PATH if sample data is installed
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
INSTALL_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BIN_DIR="${INSTALL_DIR}/bin"
LIB_DIR="${INSTALL_DIR}/lib"
INCLUDE_DIR="${INSTALL_DIR}/include"
DATA_DIR="${INSTALL_DIR}/data"

echo "================================================================"
echo "  MNE-CPP: Environment Configuration"
echo "================================================================"
echo ""
echo "Installation directory: ${INSTALL_DIR}"
echo ""

# Detect shell configuration file
SHELL_RC=""
SHELL_NAME=""
if [ -n "$ZSH_VERSION" ] || [ "$SHELL" = "$(which zsh 2>/dev/null)" ]; then
    SHELL_RC="$HOME/.zshrc"
    SHELL_NAME="zsh"
elif [ -n "$BASH_VERSION" ] || [ "$SHELL" = "$(which bash 2>/dev/null)" ]; then
    if [ -f "$HOME/.bashrc" ]; then
        SHELL_RC="$HOME/.bashrc"
    elif [ -f "$HOME/.bash_profile" ]; then
        SHELL_RC="$HOME/.bash_profile"
    else
        SHELL_RC="$HOME/.bashrc"
    fi
    SHELL_NAME="bash"
fi

if [ -z "$SHELL_RC" ]; then
    SHELL_RC="$HOME/.profile"
    SHELL_NAME="sh"
fi

echo "Shell config: ${SHELL_RC} (${SHELL_NAME})"
echo ""

# Helper: add a line to shell RC if not already present
add_to_rc() {
    local comment="$1"
    local line="$2"
    local marker="$3"

    if grep -q "${marker}" "$SHELL_RC" 2>/dev/null; then
        echo "  [skip] ${marker} already configured"
    else
        echo "" >> "$SHELL_RC"
        echo "# ${comment}" >> "$SHELL_RC"
        echo "${line}" >> "$SHELL_RC"
        echo "  [added] ${line}"
    fi
}

# Ensure shell RC exists
touch "$SHELL_RC"

echo "Configuring environment variables..."
echo ""

# --- MNE_CPP_ROOT ---
add_to_rc "MNE-CPP installation root" \
    "export MNE_CPP_ROOT=\"${INSTALL_DIR}\"" \
    "MNE_CPP_ROOT"

# --- PATH: Add binaries ---
if [ -d "$BIN_DIR" ]; then
    add_to_rc "MNE-CPP applications" \
        "export PATH=\"\${MNE_CPP_ROOT}/bin:\$PATH\"" \
        "MNE_CPP_ROOT}/bin"
else
    echo "  [skip] bin/ directory not found (applications not installed)"
fi

# --- Library path (macOS: DYLD_LIBRARY_PATH / Linux: LD_LIBRARY_PATH) ---
if [ -d "$LIB_DIR" ]; then
    if [[ "$OSTYPE" == "darwin"* ]]; then
        add_to_rc "MNE-CPP runtime libraries" \
            "export DYLD_LIBRARY_PATH=\"\${MNE_CPP_ROOT}/lib:\$DYLD_LIBRARY_PATH\"" \
            "MNE_CPP_ROOT}/lib"
    else
        add_to_rc "MNE-CPP runtime libraries" \
            "export LD_LIBRARY_PATH=\"\${MNE_CPP_ROOT}/lib:\$LD_LIBRARY_PATH\"" \
            "MNE_CPP_ROOT}/lib"
    fi
fi

# --- Development SDK paths ---
if [ -d "$INCLUDE_DIR" ]; then
    add_to_rc "MNE-CPP Development SDK" \
        "export MNE_CPP_SDK=\"${INSTALL_DIR}\"" \
        "MNE_CPP_SDK"

    add_to_rc "MNE-CPP SDK CMake hint" \
        "export CMAKE_PREFIX_PATH=\"\${MNE_CPP_SDK}:\$CMAKE_PREFIX_PATH\"" \
        "MNE_CPP_SDK.*CMAKE_PREFIX_PATH"

    echo ""
    echo "  SDK Usage Hints:"
    echo "    CMake:    find_package(MNE-CPP REQUIRED)"
    echo "    Include:  -I\${MNE_CPP_SDK}/include"
    echo "    Link:     -L\${MNE_CPP_SDK}/lib -lmne_utils -lmne_fiff ..."
else
    echo ""
    echo "  [skip] include/ directory not found (Development SDK not installed)"
fi

# --- MNE Sample Dataset ---
if [ -d "${DATA_DIR}/MNE-sample-data" ]; then
    add_to_rc "MNE Sample Dataset path (compatible with MNE-Python)" \
        "export MNE_DATASETS_SAMPLE_PATH=\"${DATA_DIR}\"" \
        "MNE_DATASETS_SAMPLE_PATH"
fi

echo ""
echo "================================================================"
echo "  Configuration complete!"
echo "================================================================"
echo ""
echo "Changes written to: ${SHELL_RC}"
echo ""
echo "To apply changes to your current session, run:"
echo "  source ${SHELL_RC}"
echo ""
echo "Or simply open a new terminal window."
echo "================================================================"
