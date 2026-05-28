# `doxy2mdx` — Doxygen XML → Docusaurus MDX

Converts the XML emitted by `doc/Doxyfile` (TASK 18.1) into one MDX page
per registered public class plus a sidebar fragment consumed by
`doc/website/sidebars.ts`.  Port of `src/external/skigen/doc/doxygen2mdx.py`,
adapted for MNE-CPP's `FIFFLIB::` / `MNELIB::` namespaces, Qt
`Q_SIGNALS:` / `Q_SLOTS:` sections, SPDX author headers (TASK 17), and
the existing `doc/api_registry.json` (TASK 16.7 + 18.3).

## Usage

```bash
python3 tools/doxy2mdx/doxy2mdx.py \
    --xml-dir   doc/xml_out/xml \
    --out-dir   doc/website/docs/api \
    --registry  doc/api_registry.json \
    --generate-sidebars \
    [--classes FIFFLIB::FiffInfo,MNELIB::MNEForwardSolution] \
    [--sidebar-out doc/website/sidebars.api.generated.ts] \
    [--strict]
```

### Flags

| Flag | Purpose |
| --- | --- |
| `--xml-dir` | Doxygen XML output directory (must contain `index.xml`). |
| `--out-dir` | Target directory for MDX files; per-module sub-folders use `dir_slug`. |
| `--registry` | Path to `doc/api_registry.json`. |
| `--generate-sidebars` | Also write the Docusaurus sidebar fragment. |
| `--sidebar-out` | Override fragment path. Default: `<out-dir>/../../sidebars.api.generated.ts`. |
| `--classes` | Comma-separated qualified names to restrict to (smoke testing). |
| `--repo-root` | Repository root used for SPDX scraping. Defaults to the parent of the registry file. |
| `--strict` | Treat orphan / missing classes as a fatal error (CI gate). |

## Coverage policy

`doc/api_registry.json` is the single source of truth for public API
coverage:

* Every `class` entry with `documented: true` **must** appear in the
  Doxygen XML; otherwise the script logs a warning and (in `--strict`
  mode) exits non-zero.
* Every public class extracted from XML **must** have a registry entry,
  unless its namespace is on the built-in exclude list (`std::`,
  `Eigen::`, `boost::`, `Qt*::`, …) or the class lives under a
  `::detail::` / `internal` segment.
* Sidebar order is driven by `modules[*].sidebar_position` (top-level)
  and `classes[*].sidebar_position` (per-module).

## Running locally

```bash
doxygen doc/Doxyfile                       # TASK 18.1 (emits doc/xml_out/xml/)
python tools/doxy2mdx/doxy2mdx.py \
    --xml-dir doc/xml_out/xml \
    --out-dir doc/website/docs/api \
    --registry doc/api_registry.json \
    --generate-sidebars
( cd doc/website && npm ci && npm run build )
```

## Running in CI

`.github/workflows/api-docs.yml` (TASK 18.5) chains the same three
commands and additionally runs `python tools/validate_api_registry.py`
to enforce the registry invariants — including the new
`validate_documented_flag` check added in TASK 18.3.  Failure of any
step blocks merge.

## Implementation notes

* `class_slug("FIFFLIB::FiffInfo")` → `fiff-info`.
* `module_for("FIFFLIB::FiffInfo")` first looks up the leaf name
  (`FiffInfo`); on collisions it disambiguates via the lowercased
  namespace (`FIFFLIB` → `fiff`).
* `read_spdx_authors(path)` regex-scrapes the first 30 lines of the
  source file for `Name <email>` lines after an
  `SPDX-License-Identifier` marker.  Falls back to Doxygen `@author`
  blocks when no SPDX header is present (legacy files prior to
  TASK 17).
* `beautify_type` collapses `Eigen::Matrix<double,-1,-1,…>` →
  `Eigen::MatrixXd`, strips `class`/`struct`, drops `*_EXPORT` macro
  prefixes, and normalises `QSharedPointer< X >` spacing.
* Doxygen `@f[...]@f]` / `@f$...@f$` formulas pass through as `$$...$$`
  and `$...$` for the KaTeX plugin chain.
* Optional registry fields:
  * `guide` — emits a `:::tip[See also]` admonition linking to a
    user-guide page.
  * `python_equiv` / `python_url` — emits a `:::info[Python equivalent]`
    admonition cross-referencing MNE-Python.
