#!/bin/bash
#
#  download_sample_data.sh
#  MNE-CPP Installer - Download MNE Sample Dataset
#
#  Downloads the MNE sample dataset and sets the MNE_DATASETS_SAMPLE_PATH
#  environment variable (compatible with MNE-Python conventions).
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
INSTALL_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
DATA_DIR="${INSTALL_DIR}/data"
SAMPLE_DATA_DIR="${DATA_DIR}/MNE-sample-data"
DOWNLOAD_URL="https://osf.io/86qa2/download"
ARCHIVE_FILE="${DATA_DIR}/MNE-sample-data.tar.gz"

echo "================================================================"
echo "  MNE-CPP: Download MNE Sample Dataset"
echo "================================================================"
echo ""
echo "Download URL: ${DOWNLOAD_URL}"
echo "Destination:  ${DATA_DIR}"
echo ""

# Create data directory
mkdir -p "${DATA_DIR}"

# Check if already downloaded
if [ -d "${SAMPLE_DATA_DIR}" ]; then
    echo "MNE sample dataset already exists at: ${SAMPLE_DATA_DIR}"
    echo "To re-download, remove the directory and run this script again."
else
    echo "Downloading MNE sample dataset (~1.5 GB)..."
    echo "This may take a while depending on your internet connection."
    echo ""

    # Try curl first, then wget
    if command -v curl &> /dev/null; then
        curl -L -o "${ARCHIVE_FILE}" "${DOWNLOAD_URL}" --progress-bar
    elif command -v wget &> /dev/null; then
        wget -O "${ARCHIVE_FILE}" "${DOWNLOAD_URL}" --show-progress
    else
        echo "ERROR: Neither 'curl' nor 'wget' found. Please install one and try again."
        exit 1
    fi

    echo ""
    echo "Extracting archive..."
    tar -xzf "${ARCHIVE_FILE}" -C "${DATA_DIR}"
    rm -f "${ARCHIVE_FILE}"

    echo "Download and extraction complete."
fi

echo ""
echo "Sample data location: ${SAMPLE_DATA_DIR}"
echo ""

# Set environment variable hint
echo "================================================================"
echo "  Environment Variable Setup"
echo "================================================================"
echo ""
echo "To use this dataset with MNE-Python or MNE-CPP, set:"
echo ""
echo "  export MNE_DATASETS_SAMPLE_PATH=\"${DATA_DIR}\""
echo ""

# Offer to add to shell profile
if [ -n "$BASH_VERSION" ] || [ -n "$ZSH_VERSION" ]; then
    SHELL_RC=""
    if [ -f "$HOME/.zshrc" ]; then
        SHELL_RC="$HOME/.zshrc"
    elif [ -f "$HOME/.bashrc" ]; then
        SHELL_RC="$HOME/.bashrc"
    elif [ -f "$HOME/.bash_profile" ]; then
        SHELL_RC="$HOME/.bash_profile"
    fi

    if [ -n "$SHELL_RC" ]; then
        # Check if already set
        if grep -q "MNE_DATASETS_SAMPLE_PATH" "$SHELL_RC" 2>/dev/null; then
            echo "MNE_DATASETS_SAMPLE_PATH is already configured in ${SHELL_RC}"
        else
            echo "Adding MNE_DATASETS_SAMPLE_PATH to ${SHELL_RC}..."
            echo "" >> "$SHELL_RC"
            echo "# MNE-CPP: MNE Sample Dataset path" >> "$SHELL_RC"
            echo "export MNE_DATASETS_SAMPLE_PATH=\"${DATA_DIR}\"" >> "$SHELL_RC"
            echo "Done. Restart your terminal or run: source ${SHELL_RC}"
        fi
    fi
fi

echo ""
echo "================================================================"
echo "  Done!"
echo "================================================================"
