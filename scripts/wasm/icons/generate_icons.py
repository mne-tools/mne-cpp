#!/usr/bin/env python3
"""Generate PWA icon PNGs from the MNE Inspect application icon.

Resizes the 256x256 app icon to the sizes required by the PWA manifest
(192x192 and 512x512).  Uses ``sips`` on macOS or ``convert`` (ImageMagick)
elsewhere.

Usage:
    python3 generate_icons.py
"""
import subprocess
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.normpath(os.path.join(SCRIPT_DIR, '..', '..', '..'))
SOURCE_ICON = os.path.join(
    REPO_ROOT, 'src', 'applications', 'mne_inspect', 'resources',
    'images', 'appIcons', 'icon_mne_inspect_256x256.png')

if not os.path.exists(SOURCE_ICON):
    sys.exit(f'Source icon not found: {SOURCE_ICON}')

for size in [192, 512]:
    png_path = os.path.join(SCRIPT_DIR, f'icon-{size}.png')
    if sys.platform == 'darwin':
        subprocess.run(['sips', '-z', str(size), str(size),
                       SOURCE_ICON, '--out', png_path], check=True)
    else:
        subprocess.run(['convert', SOURCE_ICON, '-resize',
                       f'{size}x{size}', png_path], check=True)
    print(f'Generated {png_path} ({size}x{size})')
