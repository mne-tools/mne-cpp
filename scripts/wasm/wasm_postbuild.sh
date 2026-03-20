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

# --- 2. Patch every .html file ---------------------------------------------
#
# Qt generates HTML like:
#   <body onload="init()">
#
# We inject the service-worker loader right after <head> so it runs before
# any WASM code is fetched.
#
SW_TAG='<script src="coi-serviceworker.js"></script>'

for html in "$OUTPUT_DIR"/*.html; do
    [ -f "$html" ] || continue

    if grep -q 'coi-serviceworker' "$html"; then
        echo "[wasm_postbuild] Already patched: $(basename "$html")"
        continue
    fi

    # Insert the service-worker script right after <head> ... opening tags
    if [[ "$(uname)" == "Darwin" ]]; then
        sed -i '' "s|<head>|<head>\\
    ${SW_TAG}|" "$html"
    else
        sed -i "s|<head>|<head>\n    ${SW_TAG}|" "$html"
    fi
    echo "[wasm_postbuild] Patched: $(basename "$html")"
done

echo "[wasm_postbuild] Done."
