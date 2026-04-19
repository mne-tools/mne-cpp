#!/usr/bin/env python3
"""Generate PWA icon PNGs from the MNE-CPP brain logo SVGs.

The SVG icons (icon-192.svg, icon-512.svg) contain the colorful polygon
brain from resources/design/logos/MNE-CPP_Logo.svg, centered on a dark
rounded-rectangle background. This script converts them to PNG using
rsvg-convert (librsvg) when available.

Usage:
    python3 generate_icons.py
"""
import subprocess
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

for size in [192, 512]:
    svg_path = os.path.join(SCRIPT_DIR, f'icon-{size}.svg')
    png_path = os.path.join(SCRIPT_DIR, f'icon-{size}.png')
    if not os.path.exists(svg_path):
        print(f'SVG not found: {svg_path}')
        continue
    try:
        subprocess.run(['rsvg-convert', '-w', str(size), '-h', str(size),
                       svg_path, '-o', png_path], check=True)
        print(f'Converted {svg_path} -> {png_path}')
    except FileNotFoundError:
        print(f'rsvg-convert not found; install librsvg or convert manually')
