# MNE-CPP user manual (Docusaurus)

This folder hosts the static documentation site that ships with MNE-CPP.

## Quick start

```bash
npm install        # one-time
make screenshots   # regenerate auto PNGs (see below)
make start         # local dev server (http://localhost:3000)
make build         # production build under build/
```

## Auto-generated screenshots

Every manual page references images under `/img/manual/auto/...`. Those PNGs
are **not** versioned: they are produced on every docs build by the
`mne_doc_shots` tool from a single source of truth:

- Manifest: [`screenshots/manifest.json`](screenshots/manifest.json)
- Output:   `static/img/manual/auto/` (ignored by git)
- Tool:     `src/tools/doc_shots/` → `mne_doc_shots`
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
[`src/tools/doc_shots/shot_app_common.{h,cpp}`](../../src/tools/doc_shots/) —
it forces every `QRhiWidget` descendant onto the Null backend (so the grab
works on headless macOS/CI without Metal) and pumps the event loop until the
window is laid out.

Today only `mne_inspect_app` is wired up. Its `setup` schema:

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

Follow-up sessions will add `mne_scan_app` and `mne_analyze_studio_app`
kinds on top of the same `shot_app_common` infrastructure (each app
exposes its `MainWindow` via a small static `*_app_core` library,
mirroring `mne_inspect_app_core` and `mne_align_app_core`).
