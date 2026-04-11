#!/usr/bin/env python3
"""Patch MLAS compute.cpp to guard FP16 template code for WASM builds.

MLFloat16::Negate/IsNegative/FromBits are unavailable in the
Emscripten/WASM build configuration and FP16 hardware acceleration
is not available on WASM anyway.  This script wraps the affected
template specialisations in ``#if !defined(__EMSCRIPTEN__)`` guards.
"""

import pathlib
import sys

path = pathlib.Path("onnxruntime/core/mlas/lib/compute.cpp")
lines = path.read_text().splitlines(True)

# Functions/instantiations to guard – identified by unique substring in function name line.
# Each entry: (identifying_substring, exclude_substring_or_None, is_function_def)
targets = [
    ("MlasComputeExp<MLAS_FP16>(", None, True),
    ("MlasComputeSoftmaxThreaded<MLAS_FP16>(", None, True),
    ("MlasComputeSoftmax<MLAS_FP16>(", "Threaded", False),  # explicit instantiation (;)
    ("MlasGQASupported<MLAS_FP16>(", None, True),
]

# Collect (start, end) line ranges to guard (0-indexed, inclusive)
ranges = []
for needle, exclude, is_func in targets:
    for i, line in enumerate(lines):
        if needle not in line:
            continue
        if exclude and exclude in line:
            continue
        # Walk backward up to 10 lines to find the 'template' keyword
        start = i
        for j in range(i, max(i - 10, -1), -1):
            if lines[j].lstrip().startswith("template"):
                start = j
                break
        if is_func:
            # Find matching closing brace
            depth = 0
            end = i
            opened = False
            for k in range(i, len(lines)):
                for ch in lines[k]:
                    if ch == "{":
                        depth += 1
                        opened = True
                    elif ch == "}":
                        depth -= 1
                if opened and depth == 0:
                    end = k
                    break
        else:
            # Explicit template instantiation – find the terminating semicolon
            end = i
            for k in range(i, len(lines)):
                if ";" in lines[k]:
                    end = k
                    break
        ranges.append((start, end))
        break  # only first match per target

if not ranges:
    print("ERROR: no MLAS_FP16 blocks found to guard", file=sys.stderr)
    sys.exit(1)

# Insert guards in reverse order so indices remain valid
for start, end in sorted(ranges, reverse=True):
    lines.insert(end + 1, "#endif  // !defined(__EMSCRIPTEN__)\n")
    lines.insert(start, "#if !defined(__EMSCRIPTEN__)\n")

path.write_text("".join(lines))
print(f"Patched {path}: guarded {len(ranges)} FP16 blocks for WASM")
