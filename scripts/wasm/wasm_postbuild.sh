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

# --- 1. Copy coi-serviceworker.js -------------------------------------------
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

# --- 3. Inject service-worker <script> into every .html -------------------
SW_TAG='<script src="coi-serviceworker.js"></script>'

for html in "$OUTPUT_DIR"/*.html; do
    [ -f "$html" ] || continue

    if grep -q 'coi-serviceworker' "$html"; then
        echo "[wasm_postbuild] Already patched: $(basename "$html")"
        continue
    fi

    if [[ "$(uname)" == "Darwin" ]]; then
        sed -i '' "s|<head>|<head>\\
    ${SW_TAG}|" "$html"
    else
        sed -i "s|<head>|<head>\n    ${SW_TAG}|" "$html"
    fi
    echo "[wasm_postbuild] Patched: $(basename "$html")"
done

echo "[wasm_postbuild] Done."
