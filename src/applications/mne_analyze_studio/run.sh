#!/bin/bash

# Convenience launcher for the three-process MNE Analyze Studio stack.

set -euo pipefail

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
BIN_DIR="$SCRIPT_DIR/../../../out/Release/bin"

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

    if [ -d "/opt/homebrew/share/qt/plugins" ]; then
        export QT_PLUGIN_PATH="/opt/homebrew/share/qt/plugins"
        export QT_QPA_PLATFORM_PLUGIN_PATH="/opt/homebrew/share/qt/plugins/platforms"
    elif [ -d "/opt/homebrew/Cellar/qtbase/6.10.2/share/qt/plugins" ]; then
        export QT_PLUGIN_PATH="/opt/homebrew/Cellar/qtbase/6.10.2/share/qt/plugins"
        export QT_QPA_PLATFORM_PLUGIN_PATH="/opt/homebrew/Cellar/qtbase/6.10.2/share/qt/plugins/platforms"
    fi

    export DYLD_FRAMEWORK_PATH="/opt/homebrew/opt/qtbase/lib"
    export DYLD_LIBRARY_PATH="$BIN_DIR/../lib:/opt/homebrew/opt/qtbase/lib:/opt/homebrew/lib"
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
