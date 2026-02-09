#!/bin/bash
#
#  install_mne_python.sh
#  MNE-CPP Installer - Install MNE-Python via pip
#
#  Installs the MNE-Python package using pip.
#  Requires Python 3 and pip to be available on the system.
#

set -e

echo "================================================================"
echo "  MNE-CPP: Install MNE-Python"
echo "================================================================"
echo ""

# Find Python 3
PYTHON_CMD=""
if command -v python3 &> /dev/null; then
    PYTHON_CMD="python3"
elif command -v python &> /dev/null; then
    # Check if it's Python 3
    PY_VER=$(python --version 2>&1 | sed 's/[^0-9].*//' | head -c 1)
    if [ "$PY_VER" = "3" ]; then
        PYTHON_CMD="python"
    fi
fi

if [ -z "$PYTHON_CMD" ]; then
    echo "ERROR: Python 3 not found on the system PATH."
    echo ""
    echo "Please install Python 3 from https://www.python.org/downloads/"
    echo "and ensure it is added to your PATH, then run this script again."
    exit 1
fi

echo "Found Python: $($PYTHON_CMD --version)"
echo ""

# Check for pip (try python -m pip first, then fall back to pip3/pip commands)
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
    echo "Neither 'python3 -m pip', 'pip3', nor 'pip' were found."
    echo "Please install pip:"
    echo "  $PYTHON_CMD -m ensurepip --upgrade"
    echo "or"
    echo "  $PYTHON_CMD -m pip install --upgrade pip"
    exit 1
fi

echo "Found pip: $($PIP_CMD --version)"
echo ""

# Install MNE-Python
echo "Installing MNE-Python..."
echo ""
$PIP_CMD install --upgrade mne

if [ $? -eq 0 ]; then
    echo ""
    echo "================================================================"
    echo "  MNE-Python installed successfully!"
    echo "================================================================"
    echo ""
    echo "You can verify the installation with:"
    echo "  $PYTHON_CMD -c \"import mne; print(mne.__version__)\""
    echo ""
    echo "To get started, visit: https://mne.tools/stable/auto_tutorials/index.html"
else
    echo ""
    echo "ERROR: MNE-Python installation failed."
    echo "Please try running manually: $PIP_CMD install mne"
    exit 1
fi
