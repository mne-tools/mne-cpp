#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Build the MNE-CPP INTERNAL / maintainer documentation.
#
# Unlike build-api-docs.sh (which publishes the library-only public API to
# the website), this generates browsable HTML covering the application and
# plugin internals under src/applications, including ``@internal`` sections.
# The output is for maintainers only and is NEVER published to the website
# API index.
#
# Output: doc/internal_out/html/index.html (git-ignored).

set -euo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Doxyfile.internal uses paths relative to the doc/ directory
# (INPUT=../src/libraries ../src/applications), so doxygen must be invoked
# with doc/ as the working directory.
( cd doc && doxygen Doxyfile.internal )

echo "Internal maintainer docs generated at: doc/internal_out/html/index.html"
