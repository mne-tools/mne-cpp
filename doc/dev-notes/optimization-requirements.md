# MNE-CPP — Code Quality & Optimization Requirements

**Purpose:** Living document for tracking code smells, non-idiomatic patterns,
performance bottlenecks, and test quality issues. Revisit periodically to
measure progress and re-prioritize.

Last updated: 13 April 2026

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Raw Pointers & Memory Management](#2-raw-pointers--memory-management)
3. [C-Style Patterns (Non-Idiomatic C++)](#3-c-style-patterns-non-idiomatic-c)
4. [Architecture & Design Smells](#4-architecture--design-smells)
5. [Performance Optimization Opportunities](#5-performance-optimization-opportunities)
6. [Eigen Adoption & Numerical Types](#6-eigen-adoption--numerical-types)
7. [Smart Pointer Policy](#7-smart-pointer-policy)
8. [Thread Safety](#8-thread-safety)
9. [Documentation Quality](#9-documentation-quality)
10. [Test Quality & Independent Verification](#10-test-quality--independent-verification)
11. [Priority Matrix](#11-priority-matrix)
12. [Audit Process — How We Scan & What Matters](#12-audit-process--how-we-scan--what-matters)
13. [Progress Tracker](#13-progress-tracker)

---

## 1. Executive Summary

The MNE-CPP codebase contains a mix of legacy C-style code (ported from the
original MNE-C/Fortran) and modern C++17. Key systemic issues:

- **120+ raw pointer instances** across libraries, tools, and examples
- **40+ C-style casts** in core numerical code (fiff, fs, math)
- **25+ `#define` constants** that should be `constexpr`
- **15+ `printf` calls** in libraries and tools instead of Qt/stream logging
- **`void*` callback pattern** in performance-critical forward/math code
- **Inconsistent smart pointer types** (`QSharedPointer` vs `std::shared_ptr`)
- **~40% of tests are weak** (smoke-only, no numerical assertions, no cross-validation)
- **Zero MNE-Python cross-validation** for critical algorithms

---

## 2. Raw Pointers & Memory Management

### 2.1 Raw `new`/`delete` Instances

| Priority | Location | Pattern | Recommended Fix |
|----------|----------|---------|-----------------|
| P0 | `lsl/lsl_stream_inlet.cpp` | `m_pSocket = new QTcpSocket()` with manual `delete` | `std::unique_ptr<QTcpSocket>` |
| P0 | `fwd/fwd_comp_data.h:65-70` | 6 raw members (`comp_coils`, `field`, `vec_field`, etc.) all `= nullptr` | Smart pointers or RAII wrappers |
| P0 | `fwd/fwd_coil_set.cpp:158` | `FwdCoil* def = nullptr` with unclear ownership | `std::unique_ptr<FwdCoil>` |
| P1 | `mne_analyze/libs/events/eventsharedmemmanager.cpp:150` | `new EventUpdate[bufferLength]` raw array | `std::vector<EventUpdate>` or `std::unique_ptr<EventUpdate[]>` |
| P1 | `mne_browse/main.cpp:74` | Global `MainWindow* mainWindow = NULL` | `std::unique_ptr<MainWindow>` |
| P1 | `tools/surface/mne_volume_source_space/main.cpp:167-185` | Multiple `delete sp;` | Smart pointer ownership |
| P1 | Examples (8+ files) | `new BrainView()` / `new BrainTreeModel()` without cleanup | Factory function returning smart ptrs |
| P2 | `examples/ex_inverse_mne_raw/main.cpp:421` | `epoch = new MNEEpochData()` then wrap in SPtr | `std::make_shared<MNEEpochData>()` directly |

### 2.2 Functions Returning Raw Pointers

| Priority | Location | Recommended Fix |
|----------|----------|-----------------|
| P0 | `mne/mne_surface.h:236-272` | 4 static methods returning `MNESurface*` → return `std::unique_ptr<MNESurface>` |
| P0 | `mne/mne_sss_data.h:119-131` | Static methods returning `MNESssData*` → `std::unique_ptr` |
| P0 | `fwd/fwd_eeg_sphere_model_set.h:113-136` | Functions returning raw ptrs → smart ptrs |
| P1 | `fwd/compute_fwd.cpp:128-609` | Multiple raw `QTextStream*` returns | RAII wrapper or `std::optional` |

### 2.3 `std::unique_ptr(new X)` Instead of `std::make_unique`

| Location | Current | Fix |
|----------|---------|-----|
| `mne/mne_cov_matrix.h:155-193` | `std::unique_ptr<MNECovMatrix>(new MNECovMatrix(...))` | `std::make_unique<MNECovMatrix>(...)` |
| `tools/preprocessing/mne_anonymize/fiffanonymizer.cpp:115` | `QSharedPointer<QStack<int32_t>>(new QStack<int32_t>)` | `QSharedPointer<QStack<int32_t>>::create()` |
| Various tools | `FiffStream::SPtr(new FiffStream(&file))` repeated 15+ times | Add `FiffStream::create()` factory |

---

## 3. C-Style Patterns (Non-Idiomatic C++)

### 3.1 C-Style Casts → `static_cast` / `reinterpret_cast`

**Critical density in FIFF byte-swap and tag handling:**

| Priority | Location | Count | Example |
|----------|----------|-------|---------|
| P1 | `fiff/fiff_byte_swap.h:57-161` | 14 | `(unsigned char*)(&source)` → `reinterpret_cast<unsigned char*>(&source)` |
| P1 | `fiff/fiff_tag.h:940-941` | 2 | `(float*)this->data()` → `reinterpret_cast<float*>(this->data())` |
| P1 | `fs/fs_surface.cpp:274-451` | 6 | `(float)iVal` → `static_cast<float>(iVal)` |
| P2 | `math/numerics.cpp:94-244` | 4 | `(int)(n*(n-1)*0.5)` → `static_cast<int>(...)` |
| P2 | `com/rt_client.cpp:178` | 2 | `((float)from)/m_pFiffInfo->sfreq` → `static_cast<float>(from)` |
| P2 | `fiff/fiff_cov.cpp:381` | 1 | `(unsigned) C.rows()` → `static_cast<unsigned>(...)` |

### 3.2 `printf` → Qt Logging / Streams

**Libraries: COMPLETE** — All printf calls eliminated from `src/libraries/`.
Each call categorized as `qInfo` (progress), `qCritical` (errors), `qWarning` (recoverable), or `qDebug` (internal).

**Remaining in tools/applications (lower priority):**

| Priority | Location | Count | Fix |
|----------|----------|-------|-----|
| P2 | `tools/inverse/mne_compute_mne/main.cpp` | 54 | Replace with `qInfo()` |
| P2 | `tools/inverse/mne_compute_raw_inverse/main.cpp` | 42 | Replace with `qInfo()` |
| P2 | `tools/forward/mne_flash_bem/flashbem.cpp` | 42 | Replace with `qInfo()` |
| P2 | `tools/inverse/mne_inverse_operator/main.cpp` | 38 | Replace with `qInfo()` |
| P2 | `tools/forward/mne_surf2bem/mne_surf2bem_settings.cpp` | 26 | Replace with `qInfo()` |
| P2 | `tools/forward/mne_make_source_space/main.cpp` | 20 | Replace with `qInfo()` |
| P2 | `tools/forward/mne_setup_forward_model/setupforwardmodel.cpp` | 18 | Replace with `qInfo()` |
| P2 | `applications/mne_scan/plugins/brainamp/brainampdriver.cpp` | 14 | Replace with `qInfo()` |
| P2 | Other tools (15+ files) | ~120 | Replace with `qInfo()`/`qCritical()` |

### 3.3 `#define` Constants → `constexpr`

| Priority | Location | Count | Fix |
|----------|----------|-------|-----|
| P2 | `mne_analyze_studio/.../rawsettings.h:71-127` | 25+ | `inline constexpr int NAME = value;` in namespace |
| P2 | `mne_browse/Utils/rawsettings.h` | 25+ | Same as above |
| P2 | `com/rt_cmd_client.cpp:52` | 1 | `#define USENEW 1` → `constexpr bool kUseNew = true;` |
| P2 | `tools/conversion/mne_make_cor_set/main.cpp:76-77` | 2 | `constexpr int kCorNSlice = 256;` |
| P2 | `tools/conversion/mne_ctf2fiff/main.cpp:80-86` | 7 | `constexpr int kCtfSenTypeMeg = 5;` etc. |

### 3.4 `typedef struct` → Modern Type Aliases

| Location | Fix |
|----------|-----|
| `mne/mne_sss_data.cpp:218` | `struct Name { ... };` (drop typedef) |
| `fiff/fiff_explain.h:61` | `struct FiffExplainEntry { ... };` |
| `inv/rap_music/inv_rap_music.h:83` | `struct Pair { ... };` |
| `inv/dipole_fit/inv_ecd_set.cpp:113` | `struct DipoleEntry { ... };` |
| `utils/layoutmaker.h:77` | `struct LayoutPoint { ... };` |

### 3.5 `NULL` → `nullptr`

| Location | Fix |
|----------|-----|
| `mne_browse/main.cpp:74` | `= nullptr` |
| `tools/server/.../fiffsimulator.cpp:94,326` | `= nullptr` |
| `tools/server/.../connectormanager.cpp:272` | `return nullptr` |

### 3.6 `memcpy`/`memset` → Safe Alternatives

| Location | Fix |
|----------|-----|
| `mne_analyze/libs/events/eventsharedmemmanager.cpp:315-316` | `std::copy_n` or `std::memcpy` with `static_assert` on triviality |
| `fs/fs_atlas_lookup.cpp:186,264` | `std::bit_cast<float>(bits)` (C++20) or keep `memcpy` with comment |

### 3.7 `std::srand`/`std::rand` → `<random>`

| Location | Fix |
|----------|-----|
| `mne_analyze/libs/events/eventgroup.cpp:128,140,153` | `std::mt19937` + `std::uniform_int_distribution` |

---

## 4. Architecture & Design Smells

### 4.1 God Classes (Lines > 500, Mixed Responsibilities)

| Class | File | Issue | Refactoring Strategy |
|-------|------|-------|---------------------|
| `FwdBemModel` | `fwd/fwd_bem_model.h` | ~1000+ lines, 50+ static methods, complex state | Split into `BemGeometry`, `BemSolver`, `BemIO` |
| `MNEForwardSolution` | `mne/mne_forward_solution.h` | Cluster computation + data model + I/O | Separate `ForwardSolutionData`, `ForwardSolutionCluster`, `ForwardSolutionIO` |
| `FiffStream` | `fiff/fiff_stream.h` | Reader + writer + tree ops in one class | Consider `FiffReader`/`FiffWriter` split |

### 4.2 `void*` Callback Anti-Pattern

C-style callback with type-erased user data. Replace with templates or `std::function`.

| Location | Current Signature | Recommended |
|----------|------------------|-------------|
| `math/simplex_algorithm.h:102-171` | `T (*func)(..., const void *user_data)` | `template<typename Func> ... Func&& func` (no void*) |
| `fwd/fwd_types.h:54-59` | Function pointers with `void *client` | `std::function<...>` or template |
| `fwd/fwd_bem_model.h:559-1021` | Multiple `void *client` parameters | Template parameter or `std::any` |
| `fwd/fwd_eeg_sphere_model.h:278-409` | `void *client` in static methods | Template |
| `math/sphere.h:171` | `float fit_eval(..., const void *user_data)` | Lambda/template |
| `mne/mne_types.h:91` | `typedef void (*mneUserFreeFunc)(void *)` | `std::function<void(void*)>` or custom deleter type |

### 4.3 Duplicated Code Patterns

| Pattern | Occurrences | Locations | Fix |
|---------|-------------|-----------|-----|
| `BrainView* + BrainTreeModel*` init | 10+ | 8 examples + 2 apps | Factory: `Disp3DFactory::createBrainView()` |
| `FiffStream::SPtr(new FiffStream(&file))` | 15+ | All tools | `FiffStream::create(QIODevice*)` static factory |
| Epoch collection loop | 5+ | Multiple inverse examples | Utility: `MNEEpochData::collectEpochs(raw, events, ...)` |

### 4.4 Inconsistent Error Handling

| Area | Pattern | Recommendation |
|------|---------|----------------|
| `mne_analyze_studio` | Modern: `throw WorkflowValidationError(...)` | **Keep — good pattern** |
| Forward library | Returns `int` error codes (C-style) | Migrate to exceptions or `std::expected` (C++23) / `tl::expected` |
| Tools | Mixed: some `qCritical()` + return, some silent | Standardize on qCritical() + return code |
| I/O functions | Some return bool, some return empty objects | Use `bool& ok` out-parameter (Qt-style) or overload: one throwing variant, one returning default + `bool& ok` |

---

## 5. Performance Optimization Opportunities

### 5.1 Missing `reserve()` on Containers

Many append/push_back loops lack pre-allocation. Grep for patterns:
```
QList<...> result;
for (...) { result.append(...); }
```
**Fix:** Add `result.reserve(expectedSize)` before the loop.

**Key locations where size is known ahead of time:**
- `conn/connectivity.cpp:95-127` — if-elseif chain of appends
- Multiple Model classes in `mne_browse`
- Graph node iteration in `mna/`

### 5.2 Unnecessary Deep Copies

| Location | Issue | Fix |
|----------|-------|-----|
| `conn/network/networknode.cpp:82-130` | Methods return `QList<QSharedPointer<...>>` by value creating copies | Return `const QList<...>&` or use `std::span` |
| Various SPtr creation | `SPtr(new T(localValue))` copies `localValue` | `std::make_shared<T>(std::move(localValue))` |
| Epoch loop pattern | `new MNEEpochData(); epoch->data = slice;` | Construct in-place with data |

### 5.3 Eigen Performance

| Optimization | Description | Impact |
|-------------|-------------|--------|
| **Lazy evaluation** | Ensure intermediate expressions aren't materialized unnecessarily (e.g. `.eval()` only when needed) | Medium |
| **`noalias()`** | Add `.noalias()` to matrix multiplications where aliasing is impossible (C = A * B) | High for large matmuls |
| **Column-major access** | Verify iteration patterns match storage order (Eigen default: column-major) | Medium |
| **Block operations** | Use `.block()`, `.topRows()`, `.segment()` instead of manual loops | Medium |
| **SIMD alignment** | Ensure `EIGEN_MAKE_ALIGNED_OPERATOR_NEW` where needed, or use `Eigen::aligned_allocator` | Low (mostly handled) |
| **Expression templates** | Avoid breaking expression chains with unnecessary temporaries | Medium |

### 5.4 String Operations

| Pattern | Locations | Fix |
|---------|-----------|-----|
| `QString + QString + ...` in loops | Various logging | Use `QStringLiteral` or `QStringBuilder` (`%` operator) |
| `qPrintable(...)` in printf | Tools (15+ locations) | Use `qInfo()` directly |
| `arg().arg().arg()` chains | Several dialogs/logging | Single `arg(a, b, c)` overload (Qt 5.14+) |

### 5.5 I/O Performance

| Optimization | Description |
|-------------|-------------|
| **Buffered FIFF reads** | Ensure `FiffStream` uses buffered I/O for sequential reads |
| **Memory-mapped files** | For large raw data files, consider `QFile::map()` / `mmap` |
| **Parallel epoch reads** | Epoch extraction loops are sequential; candidates for `QtConcurrent` |

### 5.6 Algorithm Complexity

| Location | Current | Potential |
|----------|---------|-----------|
| `inv/hpi/inv_hpi_fit.cpp` — coil ordering | Brute-force permutation | Hungarian algorithm for assignment |
| `sts/sts_adjacency.cpp` — nearest-neighbor | O(n²) pairwise distance | KD-tree or spatial hashing |
| `disp3D/helpers/geometryinfo.cpp` — SCDC | BFS per vertex (inefficient for dense meshes) | Dijkstra with priority queue |

---

## 6. Eigen Adoption & Numerical Types

### 6.1 `std::vector<double/float>` → Eigen Vectors

| Location | Current Type | Recommended |
|----------|-------------|-------------|
| `sts/sts_adjacency.cpp:90,104` | `std::vector<double> nnDist` | `Eigen::VectorXd` |
| `sts/sts_cluster.cpp:255` | `std::vector<int> indices` | `Eigen::VectorXi` |
| `inv/hpi/inv_hpi_fit_data.cpp:235` | `std::vector<double> fv, fv1` | `Eigen::VectorXd` |
| `lsl/lsl_stream_inlet.cpp:228` | `std::vector<float> sample` | `Eigen::VectorXf` or Eigen::Map |
| `lsl/lsl_stream_outlet.cpp:285` | `std::queue<std::vector<float>>` | `std::queue<Eigen::VectorXf>` |
| `utils/layoutmaker.h:139-140` | `std::vector<std::vector<float>>` | `Eigen::MatrixX3f` |
| `utils/layoutloader.h:112-113` | `std::vector<std::vector<float>>` | `Eigen::MatrixX3f` |

### 6.2 `QVector<float/double>` → Eigen

| Location | Current | Recommended |
|----------|---------|-------------|
| `mri/mri_vol_data.h:63-84` | `QVector<float> pixelsFloat` | `Eigen::VectorXf` (for numerical operations) or keep QVector if pure storage |

### 6.3 Raw Array Parameters → Eigen::Map / Eigen::Ref

Where functions accept `float*` or `double*` with size parameters, wrap with
`Eigen::Map` at call site or change signature to `Eigen::Ref<>`.

---

## 7. Smart Pointer Policy

### 7.1 Current State: Mixed `QSharedPointer` and `std::shared_ptr`

The codebase currently uses `QSharedPointer` via `SPtr` / `ConstSPtr` typedefs
on most classes. This is Qt-idiomatic but limits interop with STL algorithms.

### 7.2 Recommended Policy

| Context | Use |
|---------|-----|
| **Owning, single owner** | `std::unique_ptr<T>` |
| **Shared ownership** | `QSharedPointer<T>` (match existing `SPtr` pattern) |
| **Non-owning reference** | Raw pointer or `T&` (documented as non-owning) |
| **Optional value** | `std::optional<T>` (not pointer) |
| **Factory functions** | Return `std::unique_ptr<T>` (caller decides ownership) |
| **New code (no Qt dependency)** | Prefer `std::shared_ptr` / `std::unique_ptr` |
| **Qt widget trees** | Raw `new` with Qt parent (Qt manages lifetime) — document clearly |

### 7.3 Migration Path

1. **Phase 1:** Add factory methods (`::create()`) to high-frequency classes
   (`FiffStream`, `FiffRawData`, `MNEForwardSolution`)
2. **Phase 2:** Convert functions returning raw `T*` to `std::unique_ptr<T>`
3. **Phase 3:** Gradually replace `QSharedPointer` with `std::shared_ptr` in
   non-Qt code (libraries without Qt dependency: math, sts, ml)

---

## 8. Thread Safety

### 8.1 Known Concerns

| Location | Issue | Fix |
|----------|-------|-----|
| `lsl/lsl_stream_inlet.cpp:190` | `m_rawBuffer.append(data)` without mutex in streaming context | Add `std::mutex` guard or use lock-free queue |
| `mne_browse/main.cpp:74` | Global `MainWindow*` in signal handler | `std::atomic<MainWindow*>` or redesign |
| `mna/mna_graph_executor.h` | Parallel node execution with unclear synchronization | Audit thread safety of shared graph state |

### 8.2 General Rules (Not Yet Enforced)

- All shared mutable state must be protected by `std::mutex` or `std::atomic`
- Prefer `std::scoped_lock` over manual `lock()`/`unlock()`
- Document thread safety guarantees in class-level Doxygen (thread-safe, main-thread-only, etc.)

---

## 9. Documentation Quality

### 9.1 Missing Doxygen

- Many newer files (sts, mna, ml libraries) have minimal or boilerplate Doxygen
- Forward library (`fwd/`) has C-style comments from MNE-C port
- Tool main() functions often lack `@brief` and usage documentation

### 9.2 Misleading Comments

| Location | Issue |
|----------|-------|
| `mne/mne_hemisphere.cpp:71,588` | Old `NULL` references in comments |
| `eventgroup.cpp:128` | Three `std::srand()` calls without explaining why in each constructor |
| `fiffanonymizer.h:603` | Comment scope doesn't match method scope |

### 9.3 Recommendations

- Add `@thread_safety` custom Doxygen tag to all public classes
- Require `@brief` for all public methods
- Remove commented-out code blocks (replace with TODO/issue references)

---

## 10. Test Quality & Independent Verification

### 10.1 Weak/Meaningless Tests (Need Strengthening)

| Test | Issue | Action |
|------|-------|--------|
| `test_connectivity_metrics` | Constructor-only, no computation verification | Add metric computation + numerical assertions |
| `test_geometryinfo` | Smoke tests, empty input verification only | Add known-answer tests for SCDC |
| `test_rtprocessing_averaging` | "Verify no crash" — no output validation | Assert averaged output against hand-computed reference |
| `test_fiff_coord_trans` | Trivial round-trip only | Add known transformation verification |
| `test_mne_channel_selection` | Only checks field existence | Add channel pick + verify content |
| `test_inv_loreta` | Sets method but doesn't verify result | Compare against MNE-Python eLORETA output |

### 10.2 Tests Missing Edge Cases

| Test | Missing |
|------|---------|
| `test_dsp_resample` | Extreme ratios (100x up/down), single-sample input |
| `test_filter_data` | Single-sample input, zero-length kernel |
| `test_interpolation` | Empty mesh, degenerate geometry |
| `test_utils_mnemath` | Overflow for very large numbers |

### 10.3 Hard-Coded Test Data Paths

**Current pattern:**
```cpp
QString base = QCoreApplication::applicationDirPath()
    + "/../resources/data/mne-cpp-test-data";
```

**Problems:**
- Fails silently with `QSKIP` if data folder is missing
- No environment variable fallback
- No CI validation that data exists

**Recommended fix:**
```cpp
static QString testDataPath() {
    QString envPath = qEnvironmentVariable("MNE_CPP_TEST_DATA");
    if (!envPath.isEmpty() && QDir(envPath).exists())
        return envPath;
    QString appPath = QCoreApplication::applicationDirPath()
        + "/../resources/data/mne-cpp-test-data";
    if (QDir(appPath).exists())
        return appPath;
    qFatal("Test data not found. Set MNE_CPP_TEST_DATA env variable.");
    return {};
}
```

### 10.4 MNE-Python Cross-Validation Requirements

**Rationale:** MNE-Python is the reference implementation. Numerical algorithms
should produce results within tolerance of Python output. The `PythonRunner`
utility (already in `utils/`) can execute Python scripts from C++ tests.

#### High Priority (Core Algorithms)

| C++ Module | Test | MNE-Python Verification | Tolerance |
|-----------|------|------------------------|-----------|
| **FIR/IIR Filters** | `test_dsp_firfilter` | Compare coefficients vs `scipy.signal.firwin()` / `scipy.signal.butter()` | 1e-10 |
| **Resampling** | `test_dsp_resample` | Compare output vs `scipy.signal.resample_poly()` on same input | 1e-8 |
| **Multitaper PSD** | `test_utils_spectral` | Compare vs `mne.time_frequency.psd_array_multitaper()` | 1e-6 |
| **CSD** | `test_dsp_csd` | Compare vs `mne.time_frequency.csd_array_multitaper()` | 1e-6 |
| **ICA** | `test_dsp_ica` | Compare unmixing matrix vs `mne.preprocessing.ICA()` (Infomax) | 1e-4 (stochastic) |
| **Forward Solution** | `test_mne_forward_solution` | Compare vs `mne.make_forward_solution()` | 1e-6 |
| **Inverse (dSPM)** | `test_compute_raw_inverse` | Compare STC vs `mne.minimum_norm.apply_inverse()` | 1e-4 |
| **Inverse (eLORETA)** | `test_inv_loreta` | Compare vs `mne.minimum_norm.apply_inverse(..., method='eLORETA')` | 1e-4 |
| **LCMV Beamformer** | `test_inv_beamformer` | Compare vs `mne.beamformer.make_lcmv()` + `apply_lcmv()` | 1e-4 |
| **Covariance** | `test_fiff_cov` | Compare vs `mne.read_cov()` and `mne.compute_covariance()` | 1e-10 |
| **Projections (SSP)** | `test_mne_projections` | Compare vs `mne.io.read_raw_fif().info['projs']` | 1e-10 |

#### Medium Priority (Data Structures)

| C++ Module | MNE-Python Verification |
|-----------|------------------------|
| FIFF Raw I/O | `mne.io.read_raw_fif()` — compare channel data, info fields |
| FIFF Evoked I/O | `mne.read_evokeds()` — compare data matrices |
| FreeSurfer Surface I/O | `mne.read_surface()` — compare vertices, faces |
| BEM Model | `mne.make_bem_model()` — compare surface geometry |
| Source Space | `mne.read_source_spaces()` — compare vertex indices |
| Annotations | `mne.read_annotations()` — compare onset, duration, description |

#### Cross-Validation Test Pattern

```cpp
void TestFirFilter::test_crossValidatePython()
{
    // 1. Generate test signal in C++ and save to temp file
    // 2. Run Python script via PythonRunner that:
    //    a. Loads same signal
    //    b. Applies scipy.signal.firwin() with same params
    //    c. Saves Python result to temp file
    // 3. Load Python result in C++
    // 4. Compare: QVERIFY(cppResult.isApprox(pythonResult, tolerance))
}
```

### 10.5 Data-Driven Test Template

Use Qt's `_data()` mechanism for parameterized tests:

```cpp
void TestResampling::test_resample_data()
{
    QTest::addColumn<double>("inputSfreq");
    QTest::addColumn<double>("outputSfreq");
    QTest::addColumn<int>("nSamples");

    QTest::newRow("downsample-2x")  << 1000.0 << 500.0  << 10000;
    QTest::newRow("upsample-3x")    << 100.0  << 300.0  << 5000;
    QTest::newRow("fractional")     << 600.0  << 1000.0 << 8000;
    QTest::newRow("extreme-down")   << 10000.0 << 100.0 << 50000;
    QTest::newRow("single-sample")  << 1000.0 << 500.0  << 1;
}
```

### 10.6 Exemplary Tests (To Replicate)

These tests demonstrate best practices — use them as templates:

| Test | Why It's Good |
|------|--------------|
| `test_mne_forward_solution` | Computes result, writes to file, reloads, compares row-by-row vs reference |
| `test_dsp_firfilter` | Generates synthetic sine waves, measures frequency response via DFT |
| `test_inv_beamformer` | Synthetic data grid, multiple configurations, symmetric power tests |
| `test_mne_cov_matrix` | Real data loading, packed-index verification, eigendecomposition checks |

---

## 11. Priority Matrix

### P0 — Safety & Correctness (Do First)

- [x] Replace `void*` callbacks with type-safe alternatives (math/simplex_algorithm.h + 3 callers → templates+lambdas) ✅ 2026-04-13
- [ ] Fix raw pointer ownership in `fwd_comp_data` (6 members with unclear lifetime)
- [ ] Add MNE-Python cross-validation for forward solution, inverse operators
- [ ] Fix thread safety in LSL stream inlet buffer operations
- [ ] Make test data paths robust (environment variable fallback)

### P1 — Code Quality & Maintainability (Next Sprint)

- [x] Replace C-style casts in `fiff/fiff_byte_swap.h` (14 casts → reinterpret_cast) ✅ 2026-04-13
- [x] Replace C-style casts in `fiff/fiff_tag.h` + const-correctness for `toFloat()`/`toDouble()` return types ✅ 2026-04-13
- [x] Replace C-style casts in `fs/fs_surface.cpp` (11 casts → static_cast/reinterpret_cast) ✅ 2026-04-13
- [x] Replace C-style casts in `math/numerics.cpp` (3 casts → static_cast) ✅ 2026-04-13
- [x] Replace C-style casts in `com/rt_client.cpp` (2 casts → static_cast) ✅ 2026-04-13
- [x] Replace `printf` in `math/linalg.cpp` → `qInfo` (6 calls) ✅ 2026-04-13
- [x] Replace `printf` in `com/rt_client.cpp` → `qInfo` (2 calls) ✅ 2026-04-13
- [x] Replace `printf` in `com/rt_data_client.cpp` → `qInfo` (4 calls) ✅ 2026-04-13
- [x] Const-correctness cascade: 10 files updated for `toFloat()`/`toDouble()` const return ✅ 2026-04-13
- [x] **All printf eliminated from libraries**: 20 files, ~380 calls → `qInfo`/`qCritical`/`qWarning` ✅ 2026-04-13
- [ ] Replace remaining C-style casts in `fiff/fiff_cov.cpp`
- [ ] Replace all remaining `printf` in tools and applications
- [ ] Convert functions returning raw `T*` to smart pointers
- [ ] Add `::create()` factory methods to `FiffStream`, `FiffRawData`
- [ ] Strengthen weak tests (add numerical assertions to smoke tests)
- [ ] Add edge case tests for DSP, interpolation, math utilities

### P2 — Performance & Polish (Ongoing)

- [ ] Add `.noalias()` to identified matrix multiplications
- [ ] Convert `#define` constants to `constexpr`
- [ ] Replace `typedef struct` with modern struct declarations
- [ ] Replace `NULL` with `nullptr` everywhere
- [ ] Add `reserve()` to container loops with known sizes
- [ ] Convert eligible `std::vector<double>` to Eigen types
- [ ] Replace `std::srand`/`std::rand` with `<random>`
- [ ] Profile and optimize hot paths (SCDC computation, BEM solver)

### P3 — Architecture (Plan & Execute Carefully)

- [ ] Extract `BemSolver` from `FwdBemModel` god class
- [ ] Create `BrainVisualizationFactory` for duplicated view creation
- [ ] Standardize error handling pattern across libraries
- [ ] Evaluate `QSharedPointer` → `std::shared_ptr` migration for non-Qt libs

---

## 12. Audit Process — How We Scan & What Matters

To ensure nothing slips through and the codebase converges toward a single,
beautiful, idiomatic standard, every audit cycle must follow this checklist.

### 12.1 Automated Scans (Run Every Cycle)

| Scan | Command / Method | What It Catches |
|------|-----------------|-----------------|
| **Raw pointers** | `grep -rn '\bnew \b' src/ --include='*.cpp' --include='*.h' \| grep -v external/` | Unmanaged `new`, missing smart pointers |
| **C-style casts** | `grep -rn '([a-z]*\*)\|((int)\|(float)\|(double)\|(char)\|(unsigned))' src/libraries/ src/tools/ src/applications/ --include='*.cpp' --include='*.h'` | `(int)x`, `(float*)p` patterns |
| **NULL usage** | `grep -rnw 'NULL' src/ --include='*.cpp' --include='*.h' \| grep -v external/` | Pre-C++11 null pointer |
| **printf family** | `grep -rn '\bprintf\b\|\bsprintf\b\|\bfprintf\b' src/ --include='*.cpp' \| grep -v external/` | C-style I/O |
| **#define constants** | `grep -rn '#define [A-Z_]* [0-9]' src/ --include='*.h' \| grep -v external/` | Macros that should be `constexpr` |
| **typedef struct** | `grep -rn 'typedef struct' src/ --include='*.h' --include='*.cpp' \| grep -v external/` | C-style type definitions |
| **void pointer params** | `grep -rn 'void\s*\*' src/libraries/ --include='*.h'` | Type-erased callbacks |
| **std::vector numerical** | `grep -rn 'std::vector<\s*\(double\|float\)>' src/libraries/ --include='*.h' --include='*.cpp'` | Candidates for Eigen migration |
| **Missing reserve** | Look for `append\|push_back` inside `for`/`while` loops without prior `reserve()` | Container reallocation churn |
| **Compiler warnings** | `cmake --build build/ 2>&1 \| grep -i 'warning'` | Implicit conversions, unused variables, sign compare |
| **clang-tidy** | `clang-tidy -p build/ src/libraries/**/*.cpp -checks='modernize-*,performance-*,bugprone-*,cppcoreguidelines-*'` | Comprehensive modern C++ conformance |

### 12.2 Manual Review Checklist (Per Library / Module)

For each directory in `src/libraries/`, `src/applications/`, `src/tools/`,
`src/examples/`, and `src/testframes/`:

- [ ] **Ownership clarity**: Every pointer member has documented ownership
      (owning → smart ptr, non-owning → raw ptr + `@note Non-owning`)
- [ ] **Const-correctness**: All read-only methods are `const`, all read-only
      parameters are `const&` or `const T*`
- [ ] **RAII everywhere**: No manual `delete`, no manual `fclose()`, no `free()`
- [ ] **Naming consistency**: Classes use `UpperCamelCase`, methods use
      `lowerCamelCase`, members use `m_` prefix, constants use `k` prefix
- [ ] **Single Responsibility**: No class exceeds ~500 lines; if it does,
      document why or plan to extract
- [ ] **No dead code**: No commented-out blocks, no `#if 0` sections, no
      unused includes
- [ ] **Error handling**: All I/O operations check return values; all public
      entry points validate inputs
- [ ] **Doxygen complete**: Every public class and method has `@brief`,
      parameters documented, return values documented
- [ ] **Thread safety documented**: Classes that may be used from multiple
      threads state their guarantees explicitly

### 12.3 Test Quality Criteria (Per Test File)

Every test must satisfy **all** of these to pass review:

| Criterion | Description | Anti-Pattern |
|-----------|-------------|-------------|
| **Meaningful assertion** | Every test function contains `QVERIFY`/`QCOMPARE` that checks a computed result | "Just verify no crash" |
| **Known-answer verification** | At least one test per module compares against a pre-computed reference value | Only checking dimensions or non-null |
| **Edge cases** | Empty input, single element, maximum size, boundary values | Only happy path |
| **Data-driven** | Parameterized via `_data()` / `QTest::addColumn` for multiple configurations | Single hardcoded test case |
| **Independent verification** | Numerical algorithms cross-validated against MNE-Python or analytical solutions | Self-referencing (C++ vs C++ only) |
| **No silent skips** | `QSKIP` only when truly optional; critical tests `qFatal` on missing data | `QSKIP("data not found")` for core tests |
| **Deterministic** | Tests produce same result every run (seed RNGs, control floating-point env) | Random failures |
| **Performance baseline** | Hot-path tests log execution time; regressions caught by CI | No timing measurement |

### 12.4 Beauty Standards — What "Beautiful Code" Means for MNE-CPP

The goal is code that a new contributor can read top-to-bottom and understand
without referring to external documentation. Concretely:

1. **Uniform style**: Every file reads like it was written by the same author.
   Same brace style, same naming, same include order (`<system>`, `<Qt>`,
   `<Eigen>`, `"project"`).
2. **Intent over mechanism**: Prefer `Eigen::VectorXd` over `double*` + `int n`,
   `bool& ok` out-parameter or method overloading over error-code return values,
   `for (const auto& x : container)` over index loops.
3. **Minimal surface area**: Functions do one thing. Classes own one resource.
   Headers expose only what clients need.
4. **Self-documenting names**: `computeForwardSolution(...)` not `compute(...)`.
   `m_channelCount` not `m_n`. `isValid()` not `check()`.
5. **No surprises**: No hidden side effects, no global state mutation, no
   implicit conversions. Every function is clear about what it reads, writes,
   and owns.
6. **Performance by default**: Use `const&` for parameters ≥ pointer size, move
   semantics for transfers, `noalias()` for Eigen products, `reserve()` when
   size is known.
7. **Textbook design patterns**: Every non-trivial design decision should map
   to a recognized architectural or design pattern. A reader familiar with
   GoF, POSA, or modern C++ idioms should immediately recognize the structure.
   Code should read like a reference implementation, not like an ad-hoc
   solution. See §12.4.1 for the pattern catalogue.

#### 12.4.1 Architectural & Design Pattern Catalogue

We emphasize the use of well-known design patterns so that every structural
choice is immediately recognizable and understandable — textbook-style.
Patterns are not decoration; they must serve clarity **and** performance.

| Pattern | Where It Applies in MNE-CPP | Why |
|---------|-----------------------------|-----|
| **Factory Method** | `FiffStream::create()`, `BrainVisualizationFactory`, `MNEEpochData::collect()` | Encapsulates construction complexity, returns smart pointers, eliminates raw `new` at call sites |
| **Strategy** | Inverse solvers (dSPM, eLORETA, LCMV), connectivity metrics (PLI, wPLI, coherence) | Swap algorithms at runtime without touching client code; each solver is a self-contained class with a common interface |
| **Observer / Signal-Slot** | Qt signal/slot across all GUI code, `MNA` graph node notifications | Decouples producers from consumers; Qt's mechanism is zero-cost when no slot is connected |
| **RAII (Resource Acquisition Is Initialization)** | Every resource: files (`QFile`), streams (`FiffStream`), GPU buffers, mutexes (`std::scoped_lock`) | Guarantees cleanup on all exit paths including exceptions; eliminates `delete`, `fclose`, `free` |
| **Pimpl (Pointer to Implementation)** | Public library headers with stable ABI | Hides implementation details, reduces compile-time coupling, keeps headers minimal |
| **Builder** | Complex configuration objects (`ConnectivitySettings`, `InverseOperator` assembly) | Step-by-step construction of immutable objects; validates invariants at build time, not scattered across setters |
| **Template Method** | Base test class `initTestCase()` → derived `runTest()` → base `cleanupTestCase()` | Shared setup/teardown with customizable test body; Qt Test framework's natural pattern |
| **Iterator / Range** | Eigen block expressions (`.topRows()`, `.segment()`), FIFF tag iteration | Zero-copy views over data; avoid materializing subsets; compose with STL algorithms |
| **Facade** | Tool `main()` functions wrapping library calls into a simple CLI interface | One-function entry point hides multi-library orchestration; keeps tools thin |
| **Singleton (Scoped)** | Logger configuration, ONNX Runtime session pool | Use sparingly; prefer dependency injection. When used, scope lifetime to application, not global static |
| **Command** | MNA graph nodes (each node = encapsulated operation with inputs/outputs) | Enables undo, replay, serialization of processing pipelines; natural fit for graph-based architectures |
| **Flyweight** | Shared channel info, coil definitions, coordinate transforms | Avoid duplicating large read-only data across hemispheres/channels; share via `std::shared_ptr<const T>` |
| **Type Erasure (Modern)** | Replace `void*` callbacks with `std::function` or template parameters | Type-safe, debuggable, composable — eliminates the most dangerous C-style pattern in the codebase |

**Rules for pattern application:**

1. **Name it explicitly**: When a class implements a pattern, mention it in the
   class-level Doxygen: `@note Implements the Strategy pattern for inverse solvers.`
2. **Don't force it**: If a simple function suffices, don't wrap it in a pattern.
   Patterns exist to manage complexity, not to create it.
3. **Prefer compile-time patterns**: Templates (Strategy via CRTP, Policy-based
   design) over runtime polymorphism when the set of alternatives is known at
   compile time — this yields zero-overhead abstractions.
4. **Combine for performance**: Factory + Flyweight for shared immutable data.
   RAII + Move semantics for efficient resource transfer. Strategy + Templates
   for inlined algorithm dispatch.
5. **Test the pattern**: If a class uses Strategy, test that swapping the
   strategy produces the expected behavior change. If a class uses Builder,
   test that invalid build sequences are rejected.

### 12.5 Periodic Audit Schedule

| Frequency | Scope | Method |
|-----------|-------|--------|
| **Every PR** | Changed files only | Automated clang-tidy + manual reviewer checklist (§12.2) |
| **Monthly** | Full automated scan | Run all commands from §12.1, compare counts vs previous month |
| **Per release** | Full manual audit | Walk every library with §12.2 checklist, update this document |
| **Quarterly** | Test quality deep-dive | Score every test against §12.3 criteria, identify weakest 10 |
| **Annually** | Architecture review | Evaluate god classes, dependency graph, API surface stability |

### 12.6 Metrics to Track

| Metric | Current (2026-04-13) | Target |
|--------|---------------------|--------|
| Raw `new` outside Qt widgets | 120+ | 0 |
| C-style casts | 40+ | 0 |
| `printf` calls (non-external) | ~~15+~~ 0 in libraries | 0 |
| `#define` numeric constants | 25+ | 0 |
| `NULL` usage | 5+ | 0 |
| `void*` parameters (non-external) | 12+ | 0 |
| `typedef struct` | 5 active | 0 |
| Weak tests (smoke-only) | ~40% | <5% |
| Tests with Python cross-validation | 0 | 11 (see §10.4) |
| Compiler warnings (non-external) | TBD | 0 |
| clang-tidy findings (modernize-*) | TBD | 0 |

---

## 13. Progress Tracker

| Date | Changes | Author |
|------|---------|--------|
| 2026-04-13 | Initial document created from full codebase audit | — |
| 2026-04-13 | Added §12: Audit process, beauty standards, scan methods, metrics | — |
| 2026-04-13 | **Iteration 1 — Highest Impact Fixes (21 files):** | — |
| | P0: `simplex_algorithm.h` — eliminated `void*` callback → type-safe templates + lambdas in `sphere.cpp`, `fwd_eeg_sphere_model.cpp`, `inv_dipole_fit_data.cpp` | — |
| | P1: `fiff_byte_swap.h` — 14 C-style casts → `reinterpret_cast` | — |
| | P1: `fiff_tag.h` — C-style casts → `reinterpret_cast`, `printf` → `qWarning`, `toFloat()`/`toDouble()` return `const float*`/`const double*` | — |
| | P1: `fs_surface.cpp` — 11 C-style casts → `static_cast`/`reinterpret_cast` | — |
| | P1: `math/numerics.cpp` — 3 casts → `static_cast` | — |
| | P1: `com/rt_client.cpp` — `printf` → `qInfo`, 2 casts → `static_cast` | — |
| | P1: `com/rt_data_client.cpp` — `printf` → `qInfo` (4 calls), `Map<MatrixXf>` → `Map<const MatrixXf>` | — |
| | P1: `math/linalg.cpp` — `printf` → `qInfo` (6 calls) | — |
| | Const cascade (10 files): `fiff_raw_data.cpp`, `fiff_stream.cpp`, `mne_source_space.cpp`, `mne_source_spaces.cpp`, `mne_sss_data.cpp`, `mne_cov_matrix.cpp`, `mne_raw_data.cpp`, `mne_inverse_operator.cpp`, `annotationmodel.cpp`, `mne_fiff_exp_set.cpp` | — |
| | Full build: **0 errors** | — |
| 2026-04-13 | **Iteration 2 — Library printf Elimination (20 files, ~380 calls):** | — |
| | `mne/mne_inverse_operator.cpp` — 66 printf → qInfo/qCritical/qWarning | — |
| | `mne/mne_source_space.cpp` — 53 printf → qInfo/qCritical/qWarning | — |
| | `mne/mne_ctf_comp_data_set.cpp` — 31 printf → qInfo/qCritical/qWarning | — |
| | `mne/mne_raw_data.cpp` — 39 printf → qInfo/qCritical/qWarning/qDebug | — |
| | `mne/mne_surface_or_volume.cpp` — 21 printf → qInfo/qWarning/qCritical | — |
| | `inv/dipole_fit/inv_guess_data.cpp` — 16 printf → qInfo/qCritical | — |
| | `inv/dipole_fit/inv_ecd_set.cpp` — 9 printf → qInfo/qCritical | — |
| | `mne/mne_msh_display_surface.cpp` — 14 printf → qInfo | — |
| | `mne/mne_meas_data.cpp` — 14 printf → qInfo/qCritical | — |
| | `mne/mne_raw_info.cpp` — 7 printf → qCritical/qInfo | — |
| | `mne/mne.cpp` — 7 printf → qInfo/qCritical | — |
| | `mne/mne_cov_matrix.cpp` — 6 printf → qInfo | — |
| | `mne/mne_surface.cpp` — 4 printf → qInfo/qCritical | — |
| | `mne/mne_msh_display_surface_set.cpp` — 4 printf → qInfo/qCritical | — |
| | `mne/mne_proj_op.cpp` — 2 printf → qCritical | — |
| | `mne/mne_ctf_comp_data.cpp` — 2 printf → qCritical | — |
| | `mne/mne_sss_data.cpp` — 1 printf → qCritical | — |
| | `mri/mri_mgh_io.cpp` — 10 printf → qInfo/qWarning | — |
| | `com/rt_command/command_parser.cpp` — 7 printf → qInfo | — |
| | `com/rt_command/command.cpp` — 1 printf → qCritical | — |
| | `fiff/fiff_tag.h` — 3 printf → qWarning/qCritical | — |
| | **Result: 0 printf remaining in all libraries** | — |
| | Full build: **0 errors** | — |

---

*This document should be reviewed and updated at least once per release cycle.*
