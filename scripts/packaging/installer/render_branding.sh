#!/bin/bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Render the installer icons and sidebar logo in config/ from the website logo.
# The output is committed; rerun this after changing the logo.
# Needs rsvg-convert, ImageMagick 7 and, for the .icns, macOS iconutil.

set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "${HERE}/../../.." && pwd)"
IMG="${REPO_ROOT}/doc/website/static/img"
OUT="${HERE}/config"
FONT="/System/Library/Fonts/SFNS.ttf"
TMP="$(mktemp -d)"
trap 'rm -rf "${TMP}"' EXIT

# Brain mark without the dark wordmark paths, for the application icon
grep -v 'fill="#033045"' "${IMG}/logo.svg" > "${TMP}/mark.svg"
rsvg-convert -w 2048 "${TMP}/mark.svg" -o "${TMP}/mark-raw.png"
# macOS-style plate: white rounded square with a soft shadow, the mark centred on it
magick "${TMP}/mark-raw.png" -trim +repage -resize 640x640 "${TMP}/mark.png"
magick -size 1024x1024 xc:none -fill white -draw "roundrectangle 100,100 924,924 185,185" "${TMP}/plate.png"
magick "${TMP}/plate.png" \( +clone -background '#1b1f3a' -shadow 35x18+0+14 \) +swap \
    -background none -layers merge +repage -gravity center -extent 1024x1024 \
    "${TMP}/mark.png" -gravity center -geometry +0-6 -composite "${TMP}/icon-1024.png"

mkdir -p "${TMP}/installer.iconset"
for size in 16 32 128 256 512; do
    magick "${TMP}/icon-1024.png" -resize "${size}x${size}" "${TMP}/installer.iconset/icon_${size}x${size}.png"
    magick "${TMP}/icon-1024.png" -resize "$((size * 2))x$((size * 2))" "${TMP}/installer.iconset/icon_${size}x${size}@2x.png"
done
iconutil -c icns "${TMP}/installer.iconset" -o "${OUT}/installer.icns"
magick "${TMP}/icon-1024.png" -define icon:auto-resize=256,64,48,32,16 "${OUT}/installer.ico"
magick "${TMP}/icon-1024.png" -resize 256x256 "${OUT}/installer.png"

# Sidebar header: the full logo above the page list (IFW picks the @2x file on HiDPI)
rsvg-convert -w 1200 "${IMG}/logo.svg" -o "${TMP}/logo.png"
for scale in 1 2; do
    suffix=""; [ "${scale}" = 2 ] && suffix="@2x"
    magick "${TMP}/logo.png" -trim +repage -resize "$((150 * scale))x" -background none \
        -gravity center -extent "$((200 * scale))x$((112 * scale))" "${OUT}/sidebar${suffix}.png"
done

echo "Rendered branding into ${OUT}"
