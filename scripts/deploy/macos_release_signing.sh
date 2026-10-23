#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Developer ID signing of a deployed macOS release tree and of its disk image,
# shared by scripts/deploy/deploy.sh and the macOS signing pipeline test.
#
#   macos_release_signing.sh tree OUT_DIR   sign bin/ and lib/ inside-out with the hardened runtime,
#                                           verify, notarize them as one submission, staple the apps
#   macos_release_signing.sh dmg IMAGE.dmg  sign, notarize, staple and assess a disk image
#
# Environment: MACOS_SIGNING_MODE (production|mock), CODESIGN_IDENTITY, KEYCHAIN_PATH (optional),
# APPLE_ID, APPLE_ID_PASSWORD, APPLE_TEAM_ID, MACOS_NOTARY_EVIDENCE (JSON file to append to).
# Every failure exits non-zero; see doc/wiki/MacOS-Code-Signing.md.

set -euo pipefail

TOOL="python3 $(cd "$(dirname "$0")/../.." && pwd)/tools/release/macos_signing.py"
MODE="${MACOS_SIGNING_MODE:?MACOS_SIGNING_MODE must be production or mock}"
IDENTITY="${CODESIGN_IDENTITY:?CODESIGN_IDENTITY is not set (macos-codesign-setup)}"
EVIDENCE="${MACOS_NOTARY_EVIDENCE:?MACOS_NOTARY_EVIDENCE is not set}"
KEYCHAIN_ARGS=()
if [ -n "${KEYCHAIN_PATH:-}" ]; then
    KEYCHAIN_ARGS=(--keychain "${KEYCHAIN_PATH}")
fi

case "${1:-}" in
tree)
    OUT="${2:?usage: $0 tree OUT_DIR}"
    ${TOOL} sign --mode "${MODE}" --identity "${IDENTITY}" ${KEYCHAIN_ARGS[@]+"${KEYCHAIN_ARGS[@]}"} \
        "${OUT}/bin" "${OUT}/lib"
    ${TOOL} verify --mode "${MODE}" "${OUT}/bin" "${OUT}/lib"
    ZIP="$(mktemp -d)/mne-cpp-macos-code.zip"
    (cd "${OUT}" && /usr/bin/zip -qry "${ZIP}" bin lib)
    ${TOOL} notarize --mode "${MODE}" --evidence "${EVIDENCE}" "${ZIP}"
    rm -f "${ZIP}"
    ${TOOL} staple --mode "${MODE}" "${OUT}"/bin/*.app
    ;;
dmg)
    IMAGE="${2:?usage: $0 dmg IMAGE.dmg}"
    ${TOOL} sign --mode "${MODE}" --identity "${IDENTITY}" ${KEYCHAIN_ARGS[@]+"${KEYCHAIN_ARGS[@]}"} "${IMAGE}"
    ${TOOL} verify --mode "${MODE}" "${IMAGE}"
    ${TOOL} notarize --mode "${MODE}" --evidence "${EVIDENCE}" "${IMAGE}"
    ${TOOL} staple --mode "${MODE}" "${IMAGE}"
    ;;
*)
    echo "usage: $0 tree OUT_DIR | dmg IMAGE.dmg" >&2
    exit 2
    ;;
esac
