# Copyright / SPDX license-header tooling

Every first-party source file in mne-cpp carries a **3-line SPDX block** at
the very top:

```cpp
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2010-2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//   ...one line per substantive author of this file, alphabetical by surname.
```

The single source of truth for the full license remains the top-level
[`LICENSE`](../../LICENSE). The per-file SPDX block exists only to make the
license **machine-identifiable** (per the [SPDX](https://spdx.dev/) standard)
and to surface authorship.

Python, shell, CMake, YAML, and batch files use `#` (or `::` for `.bat`)
instead of `//`; the rest of the block is identical.

## Tooling

| Tool | Purpose |
| ---- | ------- |
| `python -m tools.license_headers.validate` | Walk the in-scope tree, fail if any file lacks a valid SPDX block. |
| `python -m tools.license_headers.validate --strict` | Also fail if the author list / year range drift from `git log --follow`. |
| `python -m tools.license_headers.migrate --dry-run --paths …` | Print a unified diff for the would-be rewrite. |
| `python -m tools.license_headers.migrate --paths …` | Rewrite files in place (idempotent). |

Both CLIs are stdlib-only Python 3.10+; no pip dependencies.

## Installing the pre-commit hook

From the repository root:

```bash
ln -s ../../tools/copyright/pre-commit .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit   # if your filesystem lost the +x bit
```

The hook runs `validate --strict` against the staged in-scope files only;
unrelated commits (e.g. doc-only edits, vendored code under `src/external/`)
are unaffected.

## CI

`.github/workflows/license-headers.yml` runs the same validator on every
push to `staging`/`main` and on PRs that touch in-scope paths. A red build
blocks merge.

## Re-running the migration

The migration script is deterministic: a fresh clone re-running
`python -m tools.license_headers.migrate` will produce a byte-identical
result. Author lists come from `git log --follow --format='%an <%ae>'`,
deduped by lower-cased email, with bot/no-reply addresses filtered out, then
sorted alphabetically by surname. Year ranges are floored at 2010 (the start
of the mne-cpp v0.x era) and bounded above by the current year.
