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
offending package within the range its dependent accepts (`npm update <pkg>`),
or with an npm `override` when the dependent pins an older release. Check the
result with `npm audit` and a site build.

Two advisories have no fixed release yet (both published 18 Sep 2026); they
are reached only through Docusaurus' own tooling, never from the published
site:

- **`braces` <= 3.0.3 / GHSA-vfj7-8cjw-p6xm** - stack exhaustion on deeply
  nested brace patterns. Reached through `@docusaurus/utils -> micromatch`
  and `chokidar`, which match the repository's own file globs at build time.
  No attacker-supplied pattern reaches them.
- **`http-cache-semantics` <= 4.2.0 / GHSA-ch52-4w7c-c8xp** - a shared HTTP
  cache can serve one user's response to another. Reached only through
  `@docusaurus/core -> update-notifier -> got`, the CLI's "new version
  available" check, which caches nothing across users.

Re-check both when a patched release appears; until then they stay open with
this rationale rather than being force-resolved.

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

## Documentation gate (local and CI)

`doc/check-docs.sh --channel dev|stable` runs every documentation check in
order: API docs without Doxygen warnings and with current generated pages,
the `@snippet` contract, the quality page, the screenshots, the Docusaurus
build (broken links, anchors and images fail it) and the version routing.
`--skip-site` stops before the Node.js build. It needs Doxygen 1.16.1 and the
screenshots (`cmake --build <build_dir> --target doc-shots`).

In CI, [`_reusable-docs.yml`](../../.github/workflows/_reusable-docs.yml) runs
the `DocShots` job ([`_reusable-doc-shots.yml`](../../.github/workflows/_reusable-doc-shots.yml)),
then the same script, on pull requests, `staging` and `main`. It uploads the
screenshots (`doc-screenshots`), the built site (`website-<channel>`) and the
Doxygen warning log. The `Website` jobs of `staging` and `main` only deploy
that checked build. For a local site build without screenshots set
`MNECPP_ALLOW_MISSING_SCREENSHOTS=1`.
