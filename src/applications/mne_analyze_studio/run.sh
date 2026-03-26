#!/bin/bash

# Convenience launcher for the three-process MNE Analyze Studio stack.

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
BIN_DIR="$SCRIPT_DIR/../../../out/Release/bin"
BUILD_DIR="$SCRIPT_DIR/../../../build"

WORKBENCH_APP="$BIN_DIR/mne_analyze_studio_workbench.app/Contents/MacOS/mne_analyze_studio_workbench"
WORKBENCH_BIN="$BIN_DIR/mne_analyze_studio_workbench"
KERNEL_APP="$BIN_DIR/mne_analyze_studio_neuro_kernel.app/Contents/MacOS/mne_analyze_studio_neuro_kernel"
KERNEL_BIN="$BIN_DIR/mne_analyze_studio_neuro_kernel"
SKILL_APP="$BIN_DIR/mne_analyze_studio_skill_host.app/Contents/MacOS/mne_analyze_studio_skill_host"
SKILL_BIN="$BIN_DIR/mne_analyze_studio_skill_host"

if [ -f "$WORKBENCH_APP" ]; then
    WORKBENCH_PATH="$WORKBENCH_APP"
elif [ -f "$WORKBENCH_BIN" ]; then
    WORKBENCH_PATH="$WORKBENCH_BIN"
else
    echo "Error: Could not locate mne_analyze_studio_workbench in $BIN_DIR"
    exit 1
fi

if [ -f "$KERNEL_APP" ]; then
    KERNEL_PATH="$KERNEL_APP"
elif [ -f "$KERNEL_BIN" ]; then
    KERNEL_PATH="$KERNEL_BIN"
else
    echo "Error: Could not locate mne_analyze_studio_neuro_kernel in $BIN_DIR"
    exit 1
fi

if [ -f "$SKILL_APP" ]; then
    SKILL_PATH="$SKILL_APP"
elif [ -f "$SKILL_BIN" ]; then
    SKILL_PATH="$SKILL_BIN"
else
    echo "Error: Could not locate mne_analyze_studio_skill_host in $BIN_DIR"
    exit 1
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
    unset QT_PLUGIN_PATH
    unset QT_QPA_PLATFORM_PLUGIN_PATH
    unset DYLD_FRAMEWORK_PATH
    unset DYLD_LIBRARY_PATH

    QT_ROOT=""
    if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
        QT6_DIR=$(sed -n 's/^Qt6_DIR:PATH=//p' "$BUILD_DIR/CMakeCache.txt" | head -n 1)
        if [ -n "$QT6_DIR" ] && [ -d "$QT6_DIR" ]; then
            QT_ROOT="$(cd "$QT6_DIR/../../.." && pwd)"
        fi
    fi

    if [ -z "$QT_ROOT" ] && command -v qmake6 >/dev/null 2>&1; then
        QT_ROOT="$(qmake6 -query QT_INSTALL_PREFIX 2>/dev/null || true)"
    fi

    if [ -n "$QT_ROOT" ] && [ -d "$QT_ROOT" ]; then
        QT_PLUGIN_DIR="$QT_ROOT/plugins"
        QT_LIB_DIR="$QT_ROOT/lib"

        if [ -d "$QT_PLUGIN_DIR" ]; then
            export QT_PLUGIN_PATH="$QT_PLUGIN_DIR"
            if [ -d "$QT_PLUGIN_DIR/platforms" ]; then
                export QT_QPA_PLATFORM_PLUGIN_PATH="$QT_PLUGIN_DIR/platforms"
            fi
        fi

        if [ -d "$QT_LIB_DIR" ]; then
            export DYLD_FRAMEWORK_PATH="$QT_LIB_DIR"
            export DYLD_LIBRARY_PATH="$BIN_DIR/../lib:$QT_LIB_DIR"
        fi
    fi
fi

pkill -f mne_analyze_studio_neuro_kernel || true
pkill -f mne_analyze_studio_skill_host || true
pkill -f mne_analyze_studio_workbench || true

echo "Launching Neuro-Kernel from $KERNEL_PATH"
"$KERNEL_PATH" &
KERNEL_PID=$!

echo "Launching Skill-Host from $SKILL_PATH"
"$SKILL_PATH" &
SKILL_PID=$!

cleanup() {
    kill -TERM "$KERNEL_PID" "$SKILL_PID" 2>/dev/null || true
    wait "$KERNEL_PID" 2>/dev/null || true
    wait "$SKILL_PID" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

echo "Launching Workbench from $WORKBENCH_PATH"
"$WORKBENCH_PATH" "$@"
