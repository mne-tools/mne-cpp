#!/bin/bash
# ---------------------------------------------------------------------------
#  wasm_postbuild.sh — Post-build step for MNE-CPP WASM deployment
#
#  This script:
#    1. Copies coi-serviceworker.js into the WASM output directory.
#    2. Injects the service-worker <script> tag into every Qt-generated
#       .html file so that Chrome / Safari get the COOP/COEP headers
#       required for SharedArrayBuffer (multi-threaded Wasm).
#
#  Usage:
#    ./tools/wasm/wasm_postbuild.sh <output_dir>
#
#  Example:
#     ./scripts/wasm/wasm_postbuild.sh out/wasm/bin
# ---------------------------------------------------------------------------

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUTPUT_DIR="${1:?Usage: $0 <wasm-output-directory>}"

if [ ! -d "$OUTPUT_DIR" ]; then
    echo "[ERROR] Output directory does not exist: $OUTPUT_DIR"
    exit 1
fi

# --- 1. Copy support scripts ------------------------------------------------
echo "[wasm_postbuild] Copying coi-serviceworker.js → $OUTPUT_DIR/"
cp "$SCRIPT_DIR/coi-serviceworker.js" "$OUTPUT_DIR/coi-serviceworker.js"

# --- 2. Replace mne_browse.html with custom template -----------------------
#
# Qt generates a bare-bones HTML file.  We replace it with our branded
# loading screen that includes the MNE-CPP logo, floating polygon particles,
# mouse-reactive parallax, and a progress bar — while keeping the same
# Qt bootstrap JS (`mne_browse.js`, `qtLoad`, `window.mne_browse_entry`).
#
TEMPLATE="$SCRIPT_DIR/mne_browse_template.html"

if [ -f "$OUTPUT_DIR/mne_browse.html" ] && [ -f "$TEMPLATE" ]; then
    echo "[wasm_postbuild] Replacing mne_browse.html with custom template"
    cp "$TEMPLATE" "$OUTPUT_DIR/mne_browse.html"
fi

# --- 2b. Replace mne_inspect.html with custom template --------------------
INSPECT_TEMPLATE="$SCRIPT_DIR/mne_inspect_template.html"

if [ -f "$OUTPUT_DIR/mne_inspect.html" ] && [ -f "$INSPECT_TEMPLATE" ]; then
    echo "[wasm_postbuild] Replacing mne_inspect.html with custom template"
    cp "$INSPECT_TEMPLATE" "$OUTPUT_DIR/mne_inspect.html"
fi

# --- 2c. Copy PWA assets (manifest, service worker, icons) ----------------
echo "[wasm_postbuild] Copying PWA manifest → $OUTPUT_DIR/"
cp "$SCRIPT_DIR/manifest.json" "$OUTPUT_DIR/manifest.json"

echo "[wasm_postbuild] Copying sw.js → $OUTPUT_DIR/"
cp "$SCRIPT_DIR/sw.js" "$OUTPUT_DIR/sw.js"

# Replace __CACHE_VERSION__ placeholder with version from CITATION.cff
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
if [ -f "$REPO_ROOT/CITATION.cff" ]; then
    APP_VERSION=$(grep '^version:' "$REPO_ROOT/CITATION.cff" | head -1 | sed 's/version:[[:space:]]*//')
    if [ -n "$APP_VERSION" ]; then
        echo "[wasm_postbuild] Setting SW cache version to mne-inspect-v${APP_VERSION}"
        if [[ "$(uname)" == "Darwin" ]]; then
            sed -i '' "s|mne-inspect-v2.2.0|mne-inspect-v${APP_VERSION}|g" "$OUTPUT_DIR/sw.js"
        else
            sed -i "s|mne-inspect-v2.2.0|mne-inspect-v${APP_VERSION}|g" "$OUTPUT_DIR/sw.js"
        fi
    fi
fi

if [ -d "$SCRIPT_DIR/icons" ]; then
    echo "[wasm_postbuild] Copying icons/ → $OUTPUT_DIR/icons/"
    mkdir -p "$OUTPUT_DIR/icons"
    cp "$SCRIPT_DIR/icons/"*.svg "$OUTPUT_DIR/icons/" 2>/dev/null || true
    cp "$SCRIPT_DIR/icons/"*.png "$OUTPUT_DIR/icons/" 2>/dev/null || true
fi

# --- 3. Inject service-worker <script> into every .html --------------------
SW_TAG='<script src="coi-serviceworker.js"></script>'

for html in "$OUTPUT_DIR"/*.html; do
    [ -f "$html" ] || continue

    if grep -q 'coi-serviceworker' "$html"; then
        echo "[wasm_postbuild] Already patched: $(basename "$html")"
    else
        if [[ "$(uname)" == "Darwin" ]]; then
            sed -i '' "s|<head>|<head>\\
    ${SW_TAG}|" "$html"
        else
            sed -i "s|<head>|<head>\n    ${SW_TAG}|" "$html"
        fi
        echo "[wasm_postbuild] Patched SW: $(basename "$html")"
    fi
done

echo "[wasm_postbuild] Done."
