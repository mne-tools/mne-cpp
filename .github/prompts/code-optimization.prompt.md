---
mode: agent
description: >
  Run a periodic code-quality and performance optimization pass on the
  mne-cpp codebase. Pick a small, focused batch of items from
  doc/dev-notes/optimization-requirements.md, apply fixes, verify the build
  is warning-free, and update the Progress Tracker.
---

# Code Optimization Pass

Use this prompt whenever you want to do a focused, repeatable code-quality or
performance optimization sweep on mne-cpp. Each invocation is one independent
iteration that should leave the codebase measurably better.

## Source of truth

All work items, priorities, and metrics live in
[`doc/dev-notes/optimization-requirements.md`](../../doc/dev-notes/optimization-requirements.md).

That document tracks:

- Raw pointers / RAII gaps (§ 2)
- C-style patterns: casts, `goto`, `printf`, `NULL`, `#define` constants (§ 3)
- Architecture / design smells (§ 4)
- Performance hotspots (§ 5)
- Eigen adoption & numerical types (§ 6)
- Smart pointer policy (§ 7)
- Thread safety (§ 8)
- Documentation quality (§ 9)
- Test quality & cross-validation gaps (§ 10)
- C++17 modernization (§ 11)
- `mne_analyze` plugin feature parity (§ 15)
- Live metrics dashboard (§ 13.6)
- Progress tracker — append every iteration (§ 14)

## Workflow for one optimization pass

1. **Pick a batch.** Open `optimization-requirements.md` and select **one** of:
   - The next-highest-impact P0/P1 cluster from the priority matrix (§ 12)
   - One concrete metric from § 13.6 to drive toward 0 (e.g. eliminate all
     remaining C-style casts in one library)
   - One `mne_analyze` task from § 15 (H1–H6, M1–M14, L1–L5)
   - A single non-idiomatic file flagged in § 2–§ 4
   Keep the batch small enough to verify in one full build.

2. **Scan the codebase** to confirm scope and find every occurrence
   (`grep_search` / `semantic_search`). Update the metric counts in § 13.6 if
   they have drifted.

3. **Apply fixes** following project conventions:
   - Qt 6.11+, C++17, BSD-3-Clause headers with `@since` and `@date`
   - Zero warnings — treat every compiler warning as a build break
   - `static_cast`/`reinterpret_cast` over C-style casts
   - `qInfo`/`qWarning`/`qCritical` over `printf`
   - `nullptr` over `NULL`
   - `inline constexpr` over `#define` numeric constants
   - `enum class` over plain `enum`
   - `override` on every overriding virtual
   - `[[nodiscard]]` on pure-query methods
   - Smart pointers (`std::unique_ptr`/`QSharedPointer`) over raw `new`
   - Prefer Eigen `Map<const ...>` for read-only views

4. **Build & verify.** Run a full incremental build of the affected targets:
   ```sh
   cd build/developer-dynamic && cmake --build . 2>&1 | tail -50
   ```
   Build must complete with **0 errors and 0 new warnings**. Run any
   relevant tests in `src/testframes/`.

5. **Update the Progress Tracker** (§ 14): add a dated row listing the files
   touched and the metrics moved.

6. **Commit on `staging`** with a message of the form:
   ```
   REFACTOR: <short summary> (optimization iteration N)

   - <bullet per file/group>
   - Metric movement: <before> → <after>
   - Build: 0 warnings
   ```
   Then `git push origin staging`.

## Constraints

- **Never** flag warnings or pre-existing errors as "out of scope" — fix them
  immediately or reduce the batch size until the build is clean.
- **Never** push directly to `main`; staging only.
- **Don't** mix unrelated refactors into one commit.
- **Don't** introduce new third-party dependencies as part of an optimization
  pass; raise that separately.
- **Do** add a focused unit test when fixing a bug uncovered during cleanup
  (see § 10 for test-quality bar).

## Reference docs

- [`doc/dev-notes/optimization-requirements.md`](../../doc/dev-notes/optimization-requirements.md) — work items, priorities, metrics
- [`doc/dev-notes/gap-analysis.md`](../../doc/dev-notes/gap-analysis.md) — feature gaps vs MNE-Python and MNE-C
- [`doc/dev-notes/v2.2.0-requirements.md`](../../doc/dev-notes/v2.2.0-requirements.md) — current release scope
- `CHANGELOG.md` — keep release notes synchronized when work is user-visible
