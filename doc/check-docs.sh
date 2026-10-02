#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# The documentation gate (T4.4): one script, run the same way locally and by
# _reusable-docs.yml on pull requests, staging and main.
#
#   1. API docs: Doxygen without warnings, registry == XML, committed pages
#      current (doc/build-api-docs.sh --check) and the @snippet contract.
#   2. Quality page matches doc/release/v2.4.0/*.json.
#   3. Screenshots: every manifest shot present and valid (validate_screenshots.py).
#   4. Docusaurus build: broken links, anchors and images fail it.
#   5. Version routing of the built site for the requested channel.
#
# Usage (from any directory):
#   doc/check-docs.sh --channel dev|stable [--screenshots <dir>] [--skip-site]
#
#   --screenshots <dir>  generated PNGs (default: doc/website/static/img/manual/auto,
#                        produced by `cmake --build <build> --target doc-shots`)
#   --skip-site          stop after step 3 (no Node.js needed)
#
# Exit codes: 0 all checks passed, 1 a check failed, 2 usage or setup error.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

CHANNEL=""
SCREENSHOTS="doc/website/static/img/manual/auto"
BUILD_SITE=true
while [ $# -gt 0 ]; do
  case "$1" in
    --channel) CHANNEL="${2:-}"; shift 2 ;;
    --screenshots) SCREENSHOTS="${2:-}"; shift 2 ;;
    --skip-site) BUILD_SITE=false; shift ;;
    -h|--help) sed -n '5,23p' "$0"; exit 0 ;;
    *) echo "error: unknown argument '$1'" >&2; exit 2 ;;
  esac
done
case "$CHANNEL" in
  dev|stable) ;;
  *) echo "error: --channel dev|stable is required" >&2; exit 2 ;;
esac

step() { printf '\n== %s\n' "$1"; }

step "API docs (Doxygen, registry, generated pages)"
doc/build-api-docs.sh --check

step "Doxygen @snippet contract"
MNE_REQUIRE_DOXYGEN=1 python3 -m unittest tools/quality/tests/test_doxygen_snippets.py

step "Quality page"
python3 tools/quality/render_quality_pages.py --check

step "Screenshots"
python3 tools/quality/validate_screenshots.py --images "$SCREENSHOTS"

if ! $BUILD_SITE; then
  echo "PASS: documentation checks (site build skipped)."
  exit 0
fi

# The site build reads the screenshots from the static directory.
if [ "$(cd "$SCREENSHOTS" && pwd)" != "$REPO_ROOT/doc/website/static/img/manual/auto" ]; then
  rm -rf doc/website/static/img/manual/auto
  mkdir -p doc/website/static/img/manual
  cp -R "$SCREENSHOTS" doc/website/static/img/manual/auto
fi

step "Docusaurus build ($CHANNEL)"
command -v npm >/dev/null 2>&1 || { echo "error: npm not found (Node.js 22)" >&2; exit 2; }
if [ "$CHANNEL" = dev ]; then
  export MNECPP_SITE_ENV=dev DOCUSAURUS_BASE_URL=/dev/
else
  export MNECPP_SITE_ENV=stable
fi
( cd doc/website && npm ci && npm run build )

step "Version routing"
python3 tools/quality/check_website_build.py doc/website/build --channel "$CHANNEL"

echo "PASS: documentation checks ($CHANNEL)."
