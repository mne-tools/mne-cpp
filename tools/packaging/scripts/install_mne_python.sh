#!/bin/bash
#
#  install_mne_python.sh
#  MNE-CPP Installer - Install MNE-Python via pip
#
#  Installs the MNE-Python package using pip.  Two modes are supported:
#
#    --user    Install into the current user's local site-packages
#              (pip install --user mne).  No root/sudo required.
#
#    --global  Install system-wide (uses sudo on Linux/macOS).
#              On PEP 668 systems the --break-system-packages flag
#              is added automatically.
#
#  Usage:
#    ./install_mne_python.sh --user
#    ./install_mne_python.sh --global
#
#  When called without arguments the script defaults to --user.
#

set -e

# ---------------------------------------------------------------------------
# Parse arguments
# ---------------------------------------------------------------------------
INSTALL_MODE="user"   # default
while [[ $# -gt 0 ]]; do
    case "$1" in
        --user)   INSTALL_MODE="user";   shift ;;
        --global) INSTALL_MODE="global"; shift ;;
        *)        shift ;;  # ignore unknown (e.g. install dir from QIF)
    esac
done

echo "================================================================"
echo "  MNE-CPP: Install MNE-Python  (mode: ${INSTALL_MODE})"
echo "================================================================"
echo ""

# ---------------------------------------------------------------------------
# Locate Python 3
# ---------------------------------------------------------------------------
PYTHON_CMD=""
for candidate in python3 python; do
    if command -v "$candidate" &> /dev/null; then
        PY_MAJOR=$("$candidate" -c "import sys; print(sys.version_info.major)" 2>/dev/null || echo "")
        if [ "$PY_MAJOR" = "3" ]; then
            PYTHON_CMD="$candidate"
            break
        fi
    fi
done

if [ -z "$PYTHON_CMD" ]; then
    echo "ERROR: Python 3 not found on the system PATH."
    echo ""
    echo "Please install Python 3 from https://www.python.org/downloads/"
    echo "and ensure it is added to your PATH, then run this script again."
    exit 1
fi

echo "Found Python: $($PYTHON_CMD --version 2>&1)"
echo ""

# ---------------------------------------------------------------------------
# Locate pip
# ---------------------------------------------------------------------------
PIP_CMD=""
if $PYTHON_CMD -m pip --version &> /dev/null; then
    PIP_CMD="$PYTHON_CMD -m pip"
elif command -v pip3 &> /dev/null; then
    PIP_CMD="pip3"
elif command -v pip &> /dev/null; then
    PIP_CMD="pip"
fi

if [ -z "$PIP_CMD" ]; then
    echo "ERROR: pip is not available for $PYTHON_CMD."
    echo ""
    echo "Please install pip first:"
    echo "  $PYTHON_CMD -m ensurepip --upgrade"
    exit 1
fi

echo "Found pip: $($PIP_CMD --version)"
echo ""

# ---------------------------------------------------------------------------
# Detect PEP 668 (externally-managed-environment)
# ---------------------------------------------------------------------------
BREAK_FLAG=""
if $PYTHON_CMD -c "
import sysconfig, os
marker = os.path.join(sysconfig.get_path('stdlib'), 'EXTERNALLY-MANAGED')
exit(0 if os.path.isfile(marker) else 1)
" 2>/dev/null; then
    BREAK_FLAG="--break-system-packages"
    echo "NOTE: PEP 668 externally-managed environment detected."
    echo ""
fi

# ---------------------------------------------------------------------------
# Install MNE-Python
# ---------------------------------------------------------------------------
if [ "$INSTALL_MODE" = "global" ]; then
    echo "Installing MNE-Python system-wide (requires sudo)..."
    echo ""
    SUDO_CMD=""
    if [ "$(id -u)" -ne 0 ]; then
        SUDO_CMD="sudo"
    fi
    $SUDO_CMD $PIP_CMD install --upgrade $BREAK_FLAG mne
else
    echo "Installing MNE-Python for the current user (--user)..."
    echo ""
    $PIP_CMD install --user --upgrade $BREAK_FLAG mne
fi

RET=$?
if [ $RET -eq 0 ]; then
    echo ""
    echo "================================================================"
    echo "  MNE-Python installed successfully!  (mode: ${INSTALL_MODE})"
    echo "================================================================"
    echo ""
    echo "Verify with:"
    echo "  $PYTHON_CMD -c \"import mne; print(mne.__version__)\""
    echo ""
    echo "Get started: https://mne.tools/stable/auto_tutorials/index.html"
else
    echo ""
    echo "ERROR: MNE-Python installation failed (exit code $RET)."
    exit 1
fi
