# MNE-CPP user manual (Docusaurus)

This folder hosts the static documentation site that ships with MNE-CPP.

## Quick start

```bash
npm install        # one-time
make screenshots   # regenerate auto PNGs (see below)
make start         # local dev server (http://localhost:3000)
make build         # production build under build/
```

## Dependency advisories

Dependabot alerts on this `package-lock.json` are fixed by bumping the
offending package, except where the fixed release is not API-compatible with
the dependent that pulls it in. The current standing exception:

**`brace-expansion` / GHSA-mh99-v99m-4gvg (CVE-2026-14257), fixed in 5.0.8.**
The package reaches us only through
`@docusaurus/core -> serve-handler -> minimatch@3.1.5`, and `minimatch@3`
does `const expand = require('brace-expansion')` and then calls `expand(...)`
directly. From 3.0.0 onwards `brace-expansion` exports an object
(`{ expand, EXPANSION_MAX, EXPANSION_MAX_LENGTH }`) instead of a callable, so
forcing 5.0.8 through an npm `override` installs and loads without complaint
but throws `expand is not a function` for any pattern containing braces.

There is no compatible upgrade path today:

- `serve-handler` pins `minimatch 3.1.5` exactly, in every published version
  including `latest` (6.1.7).
- `minimatch` stays callable only up to the 5.x line, which depends on
  `brace-expansion ^2.0.1`; the 2.x maintenance branch tops out at 2.1.2 and
  has no backport of this fix.
- The only `brace-expansion` release carrying the fix is 5.0.8, on the
  incompatible object-export line.

Exposure is limited: `serve-handler` is reached exclusively from
`docusaurus serve`, the local preview server. Neither the staging nor the main
workflow runs it — both only run `npm run build`, which does not load
`serve-handler` at all. The advisory describes a denial of service against a
process serving attacker-supplied patterns, so no CI or published-site path is
affected.

Re-check when `serve-handler` relaxes its `minimatch` pin or a 1.x/2.x
backport appears; until then the alert should stay dismissed with this
rationale rather than force-resolved with a broken override.

## Auto-generated screenshots

Every manual page references images under `/img/manual/auto/...`. Those PNGs
are **not** versioned: they are produced on every docs build by the
`mne_doc_shots` tool from a single source of truth:

- Manifest: [`screenshots/manifest.json`](screenshots/manifest.json)
- Output:   `static/img/manual/auto/` (ignored by git)
- Tool:     `doc/tools/doc_shots/` → `mne_doc_shots`
- CMake:    `cmake --build <build_dir> --target doc-shots`
- Wrapper:  `make screenshots` (or `make screenshots-force`)

To add or change a manual screenshot:

1. Edit `screenshots/manifest.json` (add a new `id`, set the `kind` and
   `setup`).
2. Reference the image from the mdx page as
   `![alt](/img/manual/auto/<id>.png)`.
3. Run `make screenshots`.

No PNG should ever be checked into `static/img/manual/auto/`.

## Real-app screenshot kinds

Some manifest entries use a shot `kind` that constructs the full application
`MainWindow` under the offscreen QPA and grabs it. Shared plumbing lives in
[`doc/tools/doc_shots/shot_app_common.{h,cpp}`](../tools/doc_shots/) —
it forces every `QRhiWidget` descendant onto the Null backend (so the grab
works on headless macOS/CI without Metal).

Captures are deterministic ([`shot_capture.{h,cpp}`](../tools/doc_shots/)):
offscreen platform, device pixel ratio 1, 96 DPI, `en_US` locale, Fusion
style with its light palette, the bundled DejaVu Sans 2.35 at 13 px
(`doc/tools/doc_shots/fonts/`, Bitstream Vera licence), no cursor blink or
UI animations. Each shot waits until no thread-pool job is running and
the window has rendered identically for 300 ms (at most 15 s), then must be
exactly the declared `size`. A window that cannot shrink to that size fails
and names the widgets whose minimum size prevents it, so pick a `size` the
window can actually take. A failed shot exits non-zero and leaves no PNG
behind. Every run prints the capture environment, so two differing images
can be traced to their environment.

`mne_inspect_app` builds the mne_inspect main window. Its `setup` schema:

| Key                     | Type / values                                | Effect |
|-------------------------|----------------------------------------------|--------|
| `load_demo_electrodes`  | bool                                         | Loads the synthetic depth-strip montage via `inspect_demo_fixtures::demoOneDepthStrip()`. |
| `load_demo_mri`         | bool                                         | Loads the synthetic Gaussian-blob volume via `inspect_demo_fixtures::demoMriSlab()`. |
| `focus_dock`            | `"pick"` \| `"layers"` \| `"overlay"`        | Raises the requested dock before the grab. |
| `simulate_pick`         | `{ kind: "contact"\|"voxel"\|"vertex", target: [...] }` | Injects a synthetic pick event into the scene so the pick dock shows a populated readout. |
| `fixtures`              | array of named fixture ids                   | Reserved for future named fixtures registered via `AppFixtureLoaders`. |

The four `inspect-multimodal/{overview,load-mri,load-electrodes,pick-dock}`
entries in `screenshots/manifest.json` are the current real grabs.

`mne_align_app` drives the seven-step coregistration wizard. Its `setup` schema:

| Key                       | Type / values                                              | Effect |
|---------------------------|------------------------------------------------------------|--------|
| `wizard_step`             | int 0..6 (Setup, Fiducials, EegCap, HeadShape, Verify, Save, Done) | `AlignWizard::goToStep()` is called with the matching `AlignStep` before the grab. |
| `load_demo_bem`           | bool                                                       | Sets a synthetic BEM path on the wizard so the Setup page reads as "BEM loaded" (no on-disk surfaces are touched). |
| `load_demo_cap`           | `"10-20"` \| null                                          | Hints which synthetic EEG montage to associate with the EEG-cap page. |
| `load_demo_digitisation`  | `"demo"` \| null                                           | Pushes the full demo digitisation session (3 fiducials + 8-electrode cap + 40 HSP points) into the shared `AcquiredPoints` store before the grab. |
| `simulate_capture`        | `{ kind: "fiducial"\|"eeg"\|"hsp", count: N }`             | Appends only the requested subset to the point store — used to show partial progress on each capture page. |
| `fixtures`                | array of named fixture ids                                 | Same `AppFixtureLoaders` escape hatch as `mne_inspect_app`. |

The eight `mne-align/{overview,step1-setup,step2-fiducials,step3-eeg-cap,step4-head-shape,step5-verify,step6-save,step7-done}`
entries in `screenshots/manifest.json` are the corresponding real grabs.

`mne_scan_app` builds the MNE Scan main window with the plugins of the build
(loaded from the `mne_scan_plugins` directory next to the built `mne_scan`):

| Key         | Type / values                                  | Effect |
|-------------|------------------------------------------------|--------|
| `pipeline`  | array of `{ plugin, x, y }`                    | Places the plugins (by display name, e.g. `"Fiff Simulator"`) at the scene positions; consecutive entries are connected. |
| `select`    | plugin display name                            | Selects that plugin, so its setup widget is shown. |
| `open_menu` | `"sensor"` \| `"algorithm"`                    | Paints the open plugin menu next to its tool button. |

`mne_analyze_studio_app` builds the Analyze Studio workbench in offline mode
(`MNE_ANALYZE_STUDIO_OFFLINE`: no Neuro Kernel / Skill Host processes, no
keychain access) and previews a workflow graph without executing it:

| Key           | Type / values                | Effect |
|---------------|------------------------------|--------|
| `workflow`    | file name                    | Workflow from `src/applications/mne_analyze_studio/examples/workflows/`. |
| `open_editor` | bool                         | Also opens the file in an editor tab. |
| `center`      | `"graph"` \| `"editor"`       | Which center tab is in front (default `graph`). |

Each app exposes its window through a static `*_app_core` library
(`mne_inspect_app_core`, `mne_align_app_core`, `mne_scan_app_core`,
`mne_analyze_studio_app_core`).

## Screenshots in CI

The `DocShots` job ([`_reusable-doc-shots.yml`](../../.github/workflows/_reusable-doc-shots.yml))
builds `mne_doc_shots`, renders the whole manifest, runs
`tools/quality/validate_screenshots.py` and uploads the PNGs as the
`doc-screenshots` artifact. The website jobs of pull requests, `staging` and
`main` wait for it and download the PNGs before `npm run build`. A missing
image fails the site build; for a local build without the screenshots set
`MNECPP_ALLOW_MISSING_SCREENSHOTS=1`.
