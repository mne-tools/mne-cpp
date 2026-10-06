#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Build the MNE-CPP API documentation from a clean state:
#   1) Doxygen (pinned version) extracts XML from the public headers into a
#      freshly emptied doc/xml_out/.
#   2) tools/quality/check_doxygen_warnings.py fails on any Doxygen warning
#      outside tools/quality/doxygen_warning_allowlist.json.
#   3) tools/doxy2mdx/audit_registry.py fails when the XML and
#      doc/api_registry.json disagree.
#   4) tools/doxy2mdx/doxy2mdx.py turns XML into MDX + a sidebar fragment,
#      or with --check verifies the committed ones without rewriting them.
#   5) Docusaurus builds the static site in doc/website/build/.
#
# Usage (from any directory):
#   doc/build-api-docs.sh             # all steps, regenerating the pages
#   doc/build-api-docs.sh --no-site   # steps 1-4, no Node.js needed
#   doc/build-api-docs.sh --check     # CI: like --no-site, but fail on stale pages

set -euo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

DOXYGEN_VERSION="1.16.1"
BUILD_SITE=true
CHECK_ONLY=false
for arg in "$@"; do
  case "$arg" in
    --no-site) BUILD_SITE=false ;;
    --check) BUILD_SITE=false; CHECK_ONLY=true ;;
    -h|--help) sed -n '5,20p' "$0"; exit 0 ;;
    *) echo "error: unknown argument '$arg'" >&2; exit 2 ;;
  esac
done

require() {
  command -v "$1" >/dev/null 2>&1 || { echo "error: '$1' not found on PATH ($2)" >&2; exit 2; }
}
require doxygen "install Doxygen $DOXYGEN_VERSION"
require python3 "Python 3.9 or newer"
found="$(doxygen --version | awk '{print $1}')"
if [ "$found" != "$DOXYGEN_VERSION" ]; then
  echo "error: Doxygen $DOXYGEN_VERSION is required, found $found; warning texts differ between versions" >&2
  exit 2
fi
if $BUILD_SITE; then
  require npm "Node.js 22"
fi

# Doxyfile paths are relative to doc/ (INPUT=../src/libraries).
rm -rf doc/xml_out
( cd doc && doxygen Doxyfile )
python3 tools/quality/check_doxygen_warnings.py doc/xml_out/doxygen-warnings.log
python3 tools/doxy2mdx/audit_registry.py --check --xml-dir doc/xml_out/xml

if $CHECK_ONLY; then
  python3 tools/doxy2mdx/check_generated.py --xml-dir doc/xml_out/xml
  exit 0
fi
# Pages of removed classes would otherwise linger; index.mdx is hand-written (check_generated.py KEEP).
find doc/website/docs/api -type f ! -path doc/website/docs/api/index.mdx -delete
python3 tools/doxy2mdx/doxy2mdx.py \
    --xml-dir doc/xml_out/xml \
    --out-dir doc/website/docs/api \
    --registry doc/api_registry.json \
    --generate-sidebars \
    --strict

if $BUILD_SITE; then
  ( cd doc/website && npm ci && npm run build )
fi
