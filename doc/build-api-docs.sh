#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Build the MNE-CPP API documentation site end-to-end:
#   1) Doxygen extracts XML from the public headers.
#   2) tools/doxy2mdx/doxy2mdx.py turns XML into MDX + a sidebar fragment.
#   3) Docusaurus builds the static site in doc/website/build/.

set -euo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Doxyfile uses paths relative to the doc/ directory (INPUT=../src/libraries),
# so doxygen must be invoked with doc/ as the working directory.
( cd doc && doxygen Doxyfile )

python3 tools/doxy2mdx/doxy2mdx.py \
    --xml-dir doc/xml_out/xml \
    --out-dir doc/website/docs/api \
    --registry doc/api_registry.json \
    --generate-sidebars
( cd doc/website && npm ci && npm run build )
