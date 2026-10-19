#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Sign, notarize and staple a .dmg whose .app bundles are already signed with
# the hardened runtime, then verify the result (v2.4.0 T10.7b). Any failure
# exits non-zero: a release never presents an unsigned or unnotarized image as
# signed.
#
# Usage: scripts/deploy/macos_notarize.sh IMAGE.dmg
# Environment: CODESIGN_IDENTITY, APPLE_ID, APPLE_APP_PASSWORD, APPLE_TEAM_ID

set -euo pipefail

DMG="${1:?usage: $0 IMAGE.dmg}"
for var in CODESIGN_IDENTITY APPLE_ID APPLE_APP_PASSWORD APPLE_TEAM_ID; do
    if [ -z "${!var:-}" ]; then
        echo "error: ${var} is not set" >&2
        exit 2
    fi
done

codesign --force --timestamp --sign "${CODESIGN_IDENTITY}" "${DMG}"
xcrun notarytool submit "${DMG}" \
    --apple-id "${APPLE_ID}" \
    --password "${APPLE_APP_PASSWORD}" \
    --team-id "${APPLE_TEAM_ID}" \
    --wait --timeout 30m
xcrun stapler staple "${DMG}"

codesign --verify --strict --verbose=2 "${DMG}"
xcrun stapler validate "${DMG}"
spctl --assess --type open --context context:primary-signature --verbose "${DMG}"
echo "${DMG}: signed, notarized and stapled"
