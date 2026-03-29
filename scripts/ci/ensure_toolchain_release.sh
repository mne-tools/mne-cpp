#!/usr/bin/env bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
    echo "usage: $0 <release-tag>" >&2
    exit 1
fi

release_tag="$1"
release_title="${QT_BINARY_RELEASE_TITLE:-Qt Toolchain Artifacts}"
release_notes="${QT_BINARY_RELEASE_NOTES:-Reusable Qt runtime, static, WASM and Installer Framework assets for CI.}"
repo="${GITHUB_REPOSITORY:?GITHUB_REPOSITORY is required}"

if gh release view "${release_tag}" -R "${repo}" >/dev/null 2>&1; then
    release_id="$(gh api "repos/${repo}/releases/tags/${release_tag}" --jq '.id')"
    gh api \
        --method PATCH \
        "repos/${repo}/releases/${release_id}" \
        -f name="${release_title}" \
        -f body="${release_notes}" \
        -F prerelease=true \
        -f make_latest=false \
        >/dev/null
else
    gh release create \
        "${release_tag}" \
        -R "${repo}" \
        --title "${release_title}" \
        --notes "${release_notes}" \
        --prerelease \
        --latest=false \
        >/dev/null
fi
