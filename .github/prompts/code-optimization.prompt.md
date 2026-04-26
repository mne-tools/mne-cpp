---
mode: agent
description: >
  Run a periodic code-quality AND performance optimization pass on the
  mne-cpp codebase. Pick a small, focused batch of items from
  doc/dev-notes/optimization-requirements.md, apply fixes, measure perf
  impact when relevant, verify the build is warning-free, and update the
  Progress Tracker.
---

# Code & Performance Optimization Pass

Use this prompt whenever you want to do a focused, repeatable sweep on
mne-cpp. Each invocation is one independent iteration that should leave the
codebase **measurably better in code quality, runtime performance, or both**.

Idiomatic / beautiful C++ and raw runtime speed are co-equal goals here.
Making a hot loop 20% faster is as valuable as eliminating 50 C-style casts.

## Source of truth

All work items, priorities, and metrics live in
[`doc/dev-notes/optimization-requirements.md`](../../doc/dev-notes/optimization-requirements.md).

That document tracks:

- Raw pointers / RAII gaps (§ 2)
- C-style patterns: casts, `goto`, `printf`, `NULL`, `#define` constants (§ 3)
- Architecture / design smells (§ 4)
- **Performance hotspots & techniques (§ 5)** — reserve, deep copies, Eigen,
  parallelism (§ 5.7), SIMD/vectorization (§ 5.8), allocations (§ 5.9),
  profiling workflow (§ 5.10), build flags (§ 5.11), regression guards (§ 5.12)
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
   - **One performance hotspot from § 5** (e.g. parallelize an epoch loop,
     add `noalias()` to a matmul, replace an O(n²) algorithm with KD-tree)
   - One `mne_analyze` task from § 15 (H1–H6, M1–M14, L1–L5)
   - A single non-idiomatic file flagged in § 2–§ 4
   Keep the batch small enough to verify in one full build.

2. **Scan the codebase** to confirm scope and find every occurrence
   (`grep_search` / `semantic_search`). For perf work, also identify the
   representative dataset / benchmark you'll use to measure (§ 5.10).

3. **Measure baseline (perf changes only).** Before touching code:
   - Time the affected path 3× with `QElapsedTimer`, Instruments (macOS), or
     `perf` (Linux). Record min/median/max wall-clock, and peak RSS if
     allocation work.
   - Capture a flamegraph or top-N hot frames if the change targets a hot
     loop. This proves you're optimizing the right place.

4. **Apply fixes** following project conventions:
   - Qt 6.11+, C++17, BSD-3-Clause headers with `@since` and `@date`
   - **Zero warnings** — treat every compiler warning as a build break
   - `static_cast`/`reinterpret_cast` over C-style casts
   - `qInfo`/`qWarning`/`qCritical` over `printf`
   - `nullptr` over `NULL`
   - `inline constexpr` over `#define` numeric constants
   - `enum class` over plain `enum`
   - `override` on every overriding virtual
   - `[[nodiscard]]` on pure-query methods
   - Smart pointers (`std::unique_ptr` / `QSharedPointer`) over raw `new`
   - Prefer Eigen `Map<const ...>` for read-only views

   **Performance-specific moves (§ 5):**
   - `reserve()` before any known-size append loop
   - `.noalias()` on non-aliasing matrix multiplications
   - `QtConcurrent::blockingMappedReduced` for embarrassingly parallel loops
   - Hoist heap allocations out of hot loops; reuse thread-local buffers
   - Eigen array expressions over hand-rolled scalar loops (vectorization)
   - Replace O(n²) scans with KD-tree / hash / Dijkstra where called for
   - `std::as_const(container)` in range-fors to prevent Qt detach
   - `std::move` on rvalue arguments into `make_shared` / `emplace_back`

5. **Build & verify.** Run a full incremental build of the affected targets in
   **Release** (perf work demands optimized builds):
   ```sh
   cd build/developer-dynamic && cmake --build . 2>&1 | tail -50
   ```
   Build must complete with **0 errors and 0 new warnings**. Run any
   relevant tests in `src/testframes/`.

6. **Measure delta (perf changes only).** Re-time the same path with the
   same dataset:
   - Require **≥5% wall-clock improvement** (or a justified structural
     reason — e.g. unblocking a future change) before merging.
   - If the change regresses any other benchmark by >10%, revert or rework.
   - Capture before/after numbers verbatim in the commit message.

7. **Update the Progress Tracker** (§ 14): add a dated row listing the files
   touched, metrics moved, and (for perf) the before/after timings.

8. **Commit on `staging`** with a message of the form:

   For code-quality changes:
   ```
   REFACTOR: <short summary> (optimization iteration N)

   - <bullet per file/group>
   - Metric movement: <before> → <after>
   - Build: 0 warnings
   ```

   For performance changes:
   ```
   PERF: <short summary> (optimization iteration N)

   - <bullet per file/group>
   - Hotspot: <function / loop / file:line>
   - Dataset: <fixture used>
   - Wall-clock: <before> ms → <after> ms (−XX%)
   - Build: 0 warnings
   ```

   Then `git push origin staging`.

## Constraints

- **Never** flag warnings or pre-existing errors as "out of scope" — fix them
  immediately or reduce the batch size until the build is clean.
- **Never** push directly to `main`; staging only.
- **Never** claim a perf improvement without a measured before/after.
- **Don't** mix unrelated refactors into one commit.
- **Don't** sacrifice correctness or readability for marginal speedups.
  Prefer the simplest change that hits the target; document the trade-off if
  forced to write less idiomatic code for a measured win.
- **Don't** introduce new third-party dependencies as part of an optimization
  pass; raise that separately.
- **Do** add a focused unit test when fixing a bug uncovered during cleanup
  (§ 10), and a benchmark in `src/testframes/` whenever a hot path is
  optimized (§ 5.12).

## Reference docs

- [`doc/dev-notes/optimization-requirements.md`](../../doc/dev-notes/optimization-requirements.md) — work items, priorities, metrics, perf workflow
- [`doc/dev-notes/gap-analysis.md`](../../doc/dev-notes/gap-analysis.md) — feature gaps vs MNE-Python and MNE-C
- [`doc/dev-notes/v2.2.0-requirements.md`](../../doc/dev-notes/v2.2.0-requirements.md) — current release scope
- `CHANGELOG.md` — keep release notes synchronized when work is user-visible
