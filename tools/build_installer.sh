#!/bin/bash
#
# MNE-CPP Installer Generation Script
#
# Builds the MNE-CPP installer package using the Qt Installer Framework
# (binarycreator).
#
# By default the script downloads the latest dev release archives from
# GitHub Releases and stages them into the QIF package layout.  You can
# override the source with --archive <path-to-tar.gz/zip>.
#
# Installer Components:
#   1. Applications       - MNE-CPP binaries (required)
#   2. Runtime Libraries  - Shared libraries (required, hidden)
#   3. Development SDK    - Headers, static libs, CMake config (opt-in)
#   4. MNE Sample Dataset - Downloads ~1.5 GB sample data (opt-in)
#   5. MNE Python         - Installs MNE-Python via pip/venv (opt-in)
#   6. PATH Configuration - Sets PATH and environment variables (opt-in)
#
# Usage:
#   ./tools/build_installer.sh                      # download from GH Releases
#   ./tools/build_installer.sh --archive <FILE>     # use a local archive
#   ./tools/build_installer.sh --tag v0.1.0         # download a specific tag
#
# Prerequisites:
#   - Qt Installer Framework (binarycreator in PATH or QtInstallerFramework_DIR set)
#   - gh CLI (for downloading from GitHub Releases)
#

set -e

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
INSTALLER_DIR="${REPO_ROOT}/tools/packaging/installer"
PACKAGES_DIR="${INSTALLER_DIR}/packages"
SCRIPTS_DIR="${REPO_ROOT}/tools/packaging/scripts"
OUTPUT_DIR="${REPO_ROOT}/build_installer"
MNE_CPP_VERSION="2.0.0"

ARCHIVE=""
GH_TAG="dev_build"

# ---------------------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------------------
while [[ $# -gt 0 ]]; do
    case "$1" in
        --archive)
            ARCHIVE="$2"; shift 2 ;;
        --tag)
            GH_TAG="$2"; shift 2 ;;
        *)
            echo "Unknown option: $1"; exit 1 ;;
    esac
done

# ---------------------------------------------------------------------------
# Detect platform
# ---------------------------------------------------------------------------
if [[ "$OSTYPE" == "darwin"* ]]; then
    PLATFORM="macos"
    INSTALLER_SUFFIX="Darwin"
    ARCHIVE_PATTERN="*-macos-dynamic-*.tar.gz"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    PLATFORM="windows"
    INSTALLER_SUFFIX="win64.exe"
    ARCHIVE_PATTERN="*-windows-dynamic-x86_64.zip"
else
    PLATFORM="linux"
    INSTALLER_SUFFIX="Linux.run"
    ARCHIVE_PATTERN="*-linux-dynamic-x86_64.tar.gz"
fi

echo "-----------------------------------------------------------------"
echo "   MNE-CPP Installer Generation (Qt Installer Framework)        "
echo "-----------------------------------------------------------------"
echo ""
echo "  Platform      : ${PLATFORM}"
echo "  QIF config    : ${INSTALLER_DIR}/config/config.xml"
echo "  QIF packages  : ${PACKAGES_DIR}"
echo "  Output dir    : ${OUTPUT_DIR}"
echo ""
echo "  Components:"
echo "    [required] Applications (binaries)"
echo "    [required] Runtime Libraries (shared libs)"
echo "    [opt-in]  Development SDK"
echo "    [opt-in]  MNE Sample Dataset (download script)"
echo "    [opt-in]  MNE Python (venv install script)"
echo "    [opt-in]  PATH Configuration (environment setup)"
echo ""

# ---------------------------------------------------------------------------
# Check for binarycreator
# ---------------------------------------------------------------------------
if ! command -v binarycreator &> /dev/null; then
    echo "WARNING: 'binarycreator' not found in PATH."
    echo "Please ensure Qt Installer Framework is installed and added to PATH."
    echo "Or set 'QtInstallerFramework_DIR'."

    # Try common locations
    for candidate in \
        "$HOME/Qt/Tools/QtInstallerFramework/4.7/bin" \
        "/opt/Qt/Tools/QtInstallerFramework/4.7/bin" \
        "$QtInstallerFramework_DIR/bin"; do
        if [ -x "$candidate/binarycreator" ]; then
            export PATH="$PATH:$candidate"
            echo "Found binarycreator at $candidate. Added to PATH."
            break
        fi
    done

    if ! command -v binarycreator &> /dev/null; then
        echo "ERROR: binarycreator still not found. Aborting."
        exit 1
    fi
fi

# ---------------------------------------------------------------------------
# Step 1: Obtain the release archive
# ---------------------------------------------------------------------------
if [ -z "$ARCHIVE" ]; then
    echo "1. Downloading release archive from GitHub Releases (tag: ${GH_TAG})..."

    if ! command -v gh &> /dev/null; then
        echo "ERROR: 'gh' CLI not found. Install it from https://cli.github.com/"
        echo "Or use --archive <path> to provide a local archive."
        exit 1
    fi

    mkdir -p /tmp/mne-cpp-dl
    gh release download "$GH_TAG" -p "$ARCHIVE_PATTERN" -D /tmp/mne-cpp-dl --clobber
    ARCHIVE=$(ls /tmp/mne-cpp-dl/$ARCHIVE_PATTERN 2>/dev/null | head -1)

    if [ -z "$ARCHIVE" ]; then
        echo "ERROR: No matching archive found for pattern ${ARCHIVE_PATTERN}"
        exit 1
    fi
    echo "   Downloaded: ${ARCHIVE}"
else
    echo "1. Using local archive: ${ARCHIVE}"
fi
echo ""

# ---------------------------------------------------------------------------
# Step 2: Stage the release into QIF package data directories
# ---------------------------------------------------------------------------
echo "2. Staging release into QIF package layout..."

# Clean previous data directories
for pkg in org.mnecpp.applications org.mnecpp.applications.cli org.mnecpp.applications.gui \
           org.mnecpp.runtime org.mnecpp.sdk \
           org.mnecpp.sampledata org.mnecpp.mnepython org.mnecpp.pathconfig; do
    rm -rf "${PACKAGES_DIR}/${pkg}/data"
done

# Extract to a temp directory
EXTRACT_DIR=$(mktemp -d)
if [[ "$ARCHIVE" == *.zip ]]; then
    unzip -q "$ARCHIVE" -d "$EXTRACT_DIR"
else
    tar xf "$ARCHIVE" -C "$EXTRACT_DIR"
fi

# Find the content root (archives may have a top-level folder)
CONTENT_DIR="$EXTRACT_DIR"
SUBDIRS=$(find "$EXTRACT_DIR" -mindepth 1 -maxdepth 1 -type d)
if [ "$(echo "$SUBDIRS" | wc -l)" -eq 1 ]; then
    CONTENT_DIR="$SUBDIRS"
fi

# -----------------------------------------------------------------------
# Applications — split into GUI (.app bundles / executables with GUI)
# and CLI (standalone command-line tools)
# -----------------------------------------------------------------------
GUI_DATA="${PACKAGES_DIR}/org.mnecpp.applications.gui/data"
CLI_DATA="${PACKAGES_DIR}/org.mnecpp.applications.cli/data"
mkdir -p "${GUI_DATA}/bin" "${CLI_DATA}/bin"

if [ -d "$CONTENT_DIR/apps" ]; then
    # macOS layout: apps/ contains both .app bundles and standalone executables
    # .app bundles go to GUI, standalone executables go to CLI
    for item in "$CONTENT_DIR/apps/"*; do
        name=$(basename "$item")
        if [[ "$name" == *.app ]]; then
            echo "   [GUI] $name"
            cp -R "$item" "${GUI_DATA}/bin/"
        elif [ -f "$item" ] && [ -x "$item" ]; then
            echo "   [CLI] $name"
            cp "$item" "${CLI_DATA}/bin/"
        elif [ -d "$item" ] && [[ "$name" == *_plugins ]]; then
            # MNE-CPP plugin directories — route to matching component
            if [[ "$name" == mne_scan_plugins ]] || [[ "$name" == mne_analyze_plugins ]]; then
                echo "   [GUI plugin] $name"
                cp -R "$item" "${GUI_DATA}/bin/"
            else
                echo "   [CLI plugin] $name"
                cp -R "$item" "${CLI_DATA}/bin/"
            fi
        fi
    done
elif [ -d "$CONTENT_DIR/bin" ]; then
    # Linux/Windows layout: bin/ has executables
    # Heuristic: apps with .app or known GUI names go to GUI, rest to CLI
    GUI_APPS="mne_scan mne_analyze mne_browse mne_inspect mne_anonymize"
    GUI_PLUGINS="mne_scan_plugins mne_analyze_plugins"
    for item in "$CONTENT_DIR/bin/"*; do
        name=$(basename "$item")
        base_name="${name%.*}"  # strip extension
        if echo "$GUI_APPS" | grep -qw "$base_name"; then
            echo "   [GUI] $name"
            cp -r "$item" "${GUI_DATA}/bin/"
        elif echo "$GUI_PLUGINS" | grep -qw "$name"; then
            echo "   [GUI plugin] $name"
            cp -r "$item" "${GUI_DATA}/bin/"
        elif [[ "$name" == *_plugins ]]; then
            echo "   [CLI plugin] $name"
            cp -r "$item" "${CLI_DATA}/bin/"
        else
            echo "   [CLI] $name"
            cp -r "$item" "${CLI_DATA}/bin/"
        fi
    done
fi

# Also stage plugins/ into CLI if present (Linux)
if [ -d "$CONTENT_DIR/plugins" ]; then
    cp -r "$CONTENT_DIR/plugins" "${CLI_DATA}/"
fi

# Runtime — lib/ (MNE-CPP dylibs + Qt frameworks for CLI tools)
RT_DATA="${PACKAGES_DIR}/org.mnecpp.runtime/data"
mkdir -p "${RT_DATA}/lib"
if [ -d "$CONTENT_DIR/lib" ]; then
    cp -R "$CONTENT_DIR/lib/"* "${RT_DATA}/lib/" 2>/dev/null || true
fi

# SDK — include/ + lib/cmake/
SDK_DATA="${PACKAGES_DIR}/org.mnecpp.sdk/data"
mkdir -p "${SDK_DATA}"
if [ -d "$CONTENT_DIR/include" ]; then
    cp -r "$CONTENT_DIR/include" "${SDK_DATA}/" 2>/dev/null || true
fi
if [ -d "$CONTENT_DIR/lib/cmake" ]; then
    mkdir -p "${SDK_DATA}/lib"
    cp -r "$CONTENT_DIR/lib/cmake" "${SDK_DATA}/lib/" 2>/dev/null || true
fi

# Scripts
for comp in org.mnecpp.sampledata org.mnecpp.mnepython org.mnecpp.pathconfig; do
    mkdir -p "${PACKAGES_DIR}/${comp}/data/scripts"
done
cp "$SCRIPTS_DIR/download_sample_data.sh"  "${PACKAGES_DIR}/org.mnecpp.sampledata/data/scripts/"
cp "$SCRIPTS_DIR/download_sample_data.bat" "${PACKAGES_DIR}/org.mnecpp.sampledata/data/scripts/"
cp "$SCRIPTS_DIR/install_mne_python.sh"    "${PACKAGES_DIR}/org.mnecpp.mnepython/data/scripts/"
cp "$SCRIPTS_DIR/install_mne_python.bat"   "${PACKAGES_DIR}/org.mnecpp.mnepython/data/scripts/"
cp "$SCRIPTS_DIR/configure_environment.sh" "${PACKAGES_DIR}/org.mnecpp.pathconfig/data/scripts/"
cp "$SCRIPTS_DIR/configure_environment.bat" "${PACKAGES_DIR}/org.mnecpp.pathconfig/data/scripts/"

# Make scripts executable
chmod +x "${PACKAGES_DIR}"/*/data/scripts/*.sh 2>/dev/null || true

rm -rf "$EXTRACT_DIR"
echo "   Staging complete."
echo ""

# ---------------------------------------------------------------------------
# Step 3: Build the installer with binarycreator
# ---------------------------------------------------------------------------
echo "3. Building installer with binarycreator..."
mkdir -p "$OUTPUT_DIR"

INSTALLER_FILE="${OUTPUT_DIR}/MNE-CPP-${MNE_CPP_VERSION}-${INSTALLER_SUFFIX}"

binarycreator \
    --offline-only \
    -c "${INSTALLER_DIR}/config/config.xml" \
    -p "${PACKAGES_DIR}" \
    "$INSTALLER_FILE"

echo ""
echo "-----------------------------------------------------------------"
echo "  Success! Installer generated:"
echo "    ${INSTALLER_FILE}"
echo ""
echo "  To test, run the installer and select desired components."
echo "-----------------------------------------------------------------"
