# Changelog

All notable changes to MNE-CPP will be documented in this file.

## [Unreleased]

### For developers

**Breaking changes**

Library headers are included with an `mne/` prefix: `#include <mne/fiff/fiff_raw_data.h>`
instead of `#include <fiff/fiff_raw_data.h>`, which marks them as MNE-CPP headers
the way `Q` marks Qt's. The library sources moved from `src/libraries/<lib>` to
`src/libs/mne/<lib>`; `src/libs` is the include root, and installed headers land in
`include/mne/<lib>`. The library named `mne` keeps its name (`<mne/mne/...>`).
The other source directories follow suit: `src/applications` is `src/apps`,
`src/testframes` is `src/tests` and `src/external` is `src/3rdparty`, the one
place for third-party code (SOUP); `src/3rdparty/init.sh` downloads Qt, Eigen
and ONNX Runtime there.

All libraries share one namespace, `mne`. The per-library namespaces are gone;
type names already carry their library prefix (`FiffInfo`, `BidsChannel`,
`MnaProject`), so the qualification only repeated it. The Qt-based display
libraries and the application libraries keep a namespace of their own. The
entries below still use the 2.x namespace names; translate them with this
table.

| Before | Now |
|---|---|
| `BASELIB`, `FIFFLIB`, `MNELIB`, `FWDLIB`, `INVLIB`, `DSPLIB`, `ACQLIB`, `BIDSLIB`, `IEEGLIB`, `MNALIB`, `FSLIB`, `MRILIB`, `MATHLIB`, `STSLIB`, `CONNECTIVITYLIB`, `DECODINGLIB`, `MLLIB`, `LSLLIB`, `COMLIB`, `CHANNELSLIB`, `PYBRIDGELIB`, `REPORTLIB` | `mne` |
| `DISPLIB`, `DISP3DLIB` | `mne::viz::qt` (library `mne_viz_qt`, see below) |
| `SURFACEKEYS` | `mne::viz::qt::surface_keys` |
| `ANSHAREDLIB` (MNE Analyze) | `mne::analyze` |
| `EVENTSLIB` (MNE Analyze events) | `mne::analyze::events` |
| `EVENTSINTERNAL` | `mne::analyze::events::detail` |
| `SCSHAREDLIB`, `SCMEASLIB`, `SCDISPLIB` (MNE Scan) | `mne::scan` |
| `using namespace FIFFLIB; using namespace MNELIB; ...` | `using namespace mne;` |
| `FIFFLIB::buildDateTime()`, `buildHash()`, `buildHashLong()` and the same three in every other library | `mne::buildDateTime()`, `buildHash()`, `buildHashLong()` from `base/base_global.h` only; all libraries are built from one commit |
| `ACQLIB::buildDateTime()`, `buildHash()`, `buildHashLong()` | `mne::acq_build_date_time()`, `acq_build_hash()`, `acq_build_hash_long()`; `mne_acq` builds without `mne_base` |

Visualization is one toolkit-free library with backends, the way MNE-Python
has `mne.viz` and its backends:

```text
src/libs/mne/viz/            mne_viz      mne::viz      figures/, views/, gpu/, forms/, models/
  backends/qt/               mne_viz_qt   mne::viz::qt  plots/, viewers/, 3d/
```

`mne_viz` describes what the viewers show and how they behave without a GUI
toolkit (figures as display lists, the trace and brain view logic, GPU frame
descriptions with a software renderer, settings forms and view models).
`mne_viz_qt` is its Qt backend and the only library that uses Qt. A backend
depends on `mne_viz` only, never on another backend; `dependency_policy.json`
enforces this. The former `mne_scene` and `mne_viewmodels` are `mne_viz`, and
`mne_disp` and `mne_disp3D` are `mne_viz_qt`; the 3-D view lives in its `3d/`.

| Before | Now |
|---|---|
| `mne_scene`, `mne_viewmodels` (`mne::scene`, `mne::viewmodels`) | `mne_viz` (`mne::viz`) |
| `mne_disp`, `mne_disp3D` (`mne::disp`) | `mne_viz_qt` (`mne::viz::qt`) |
| `<mne/scene/canvas.h>`, `<mne/scene/trace_view.h>`, `<mne/scene/gpu_frame.h>`, ... | `<mne/viz/figures/canvas.h>`, `<mne/viz/views/trace_view.h>`, `<mne/viz/gpu/gpu_frame.h>`, ... |
| `<mne/viewmodels/channel_table.h>` | `<mne/viz/models/channel_table.h>` |
| `<mne/disp/...>` | `<mne/viz/backends/qt/...>` |
| `<mne/disp3D/...>` | `<mne/viz/backends/qt/3d/...>` |
| `DISPSHARED_EXPORT`, `DISP3DSHARED_EXPORT`, `SCENESHARED_EXPORT` | `VIZQTSHARED_EXPORT`, `VIZSHARED_EXPORT` |
| `MNE_DISP3D` (defined when the 3-D view is available) | `MNE_VIZ_QT_3D` |

The base library's functions are snake_case like the standard library's:

| Before | Now |
|---|---|
| `toInt`, `toDouble`, `toLower`, `indexOf`, `equalsIgnoreCase` | `to_int`, `to_double`, `to_lower`, `index_of`, `iequals` |
| `pathFromUtf8`, `utf8FromPath` | `path_from_utf8`, `utf8_from_path` |
| `contains(list, name)` | `std::ranges::contains(list, name)` (C++23, `<algorithm>`) |
| `ByteStream::isOpen`, `atEnd`, `readBigEndian`, `writeBigEndian` | `is_open`, `at_end`, `read_big_endian`, `write_big_endian` |
| `byteSwap`, `byteSwapInPlace`, `toBigEndian`, `fromBigEndian` | `byte_swap`, `byte_swap_in_place`, `to_big_endian`, `from_big_endian` |
| `ThreadPool::parallelFor`, `parallelMap`, `threadCount` | `parallel_for`, `parallel_map`, `thread_count` |
| `buildDateTime`, `buildHash`, `buildHashLong` | `build_date_time`, `build_hash`, `build_hash_long` |
| `timeNowNow`, `dateToday`, `dateTimeNow`, `gitHash`, `gitHashLong` (`utils/buildinfo.h`, removed) | `build_date_time`, `build_hash`, `build_hash_long` (`base/base_global.h`), the date and commit of the build |
| `IeegImplantation::indexOf` | `IeegImplantation::index_of` |

The library interfaces use the C++ standard library instead of Qt for strings,
paths, containers, smart pointers and logging. There are no compatibility
aliases; replace the old spelling as below.

| Before | Now |
|---|---|
| `QString` names, comments and descriptions | `std::string` (UTF-8) |
| `QString` file names, `QFile&` / `QIODevice&` readers | `std::filesystem::path` |
| `QList`, `QVector`, `QStringList`, `QMap`, `QHash` in signatures | `std::vector`, `BASELIB::StringList`, `std::map` |
| `bool read(..., T& out)` and filled `std::unique_ptr` out-parameters | `BASELIB::Expected<T>` / `std::optional<T>` return values |
| `T::SPtr`, `T::ConstSPtr` (`QSharedPointer`) | `std::shared_ptr<T>`, `std::shared_ptr<const T>`, `std::make_shared<T>()` |
| `T::UPtr` | `std::unique_ptr<T>` |
| `ptr.data()`, `ptr.isNull()`, `ptr.clear()` | `ptr.get()`, `!ptr`, `ptr.reset()` |
| `QJsonDocument`, `QJsonObject`, `QJsonArray`, `QJsonValue`, `QJsonParseError` | `BASELIB::JsonDocument`, `JsonObject`, `JsonArray`, `JsonValue`, `JsonParseError` in `base/json.h`, with snake_case methods (`from_json()`, `to_json()`, `to_int()`, `is_undefined()`, ...); incremental NDJSON / SAX parsing with `BASELIB::JsonStreamParser` in `base/json_stream_parser.h` |
| `QCommandLineParser`, `QCommandLineOption` | `mne::CommandLineParser`, `CommandLineOption` in `base/command_line.h` (`add_option()`, `process()`, `is_set()`, `value()`, `positional_arguments()`) |
| `qInstallMessageHandler(MNELogger::customLogWriter)` | nothing: programs log through `mne::Log` (`qDebug()` / `qWarning()` → `mne::Log::debug()` / `warning()`); a log window installs `mne::Log::set_sink()` |
| `MNELogger::setLogFile(QString)`, `logFile()` | `BASELIB::Log::set_file(path)`, `Log::file()` |
| `utils/generics/mne_logger.h`, `utils/generics/log.h` | `base/log.h` |
| `qWarning() << ...` in library code | `BASELIB::Log::warning("... {}", value)` (std::format syntax) |
| `QTest::ignoreMessage` for library messages | `BASELIB::LogCapture` |
| `utils/error.h`, `byte_stream.h`, `endian.h`, `string_utils.h`, `thread_pool.h` and `UTILSLIB::Expected`, `Error`, `ByteStream`, `StringList`, `ThreadPool`, ... | `base/...` and `BASELIB::...`, in the new `mne_base` library |
| `utils/layoutloader.h`, `layoutmaker.h`, `selectionio.h`, `montage/standard_montage.h` and `UTILSLIB::LayoutLoader`, `LayoutMaker`, `SelectionIO`, `StandardMontage` | `channels/...` and `CHANNELSLIB::...`, in the new `mne_channels` library |
| `LayoutLoader::readAsaElcFile(path, names, loc3D, loc2D, unit)` | `LayoutLoader::read_elc(path)` → `std::optional<ElectrodeLayout>` (`names`, `positions_3d`, `positions_2d`, `unit`); `parse_elc(text)` |
| `LayoutMaker::makeLayout(in, out, names, file, do_fit, prad, w, h, writeFile, mirrorX, mirrorY)` | `LayoutMaker::make_layout(positions, LayoutOptions)` → box corners; `write_lout(path, corners, names, options)` |
| `SelectionIO::readMNESelFile`, `readBrainstormMonFile`, `writeMNESelFile`, `writeBrainstormMonFiles` (`QMultiMap` / `std::multimap`) | `SelectionIO::read_sel`, `read_mon`, `write_sel`, `write_mon` with `std::vector<ChannelSelection>` in file order |
| `StandardMontage::getMontage`, `getElectrodeNames`, `findElectrode(name, pos)`, `electrodeCount` | `montage`, `electrode_names`, `find_electrode(name)` → `std::optional<Eigen::Vector3d>`, `electrode_count` |
| `utils/ioutils.h`, `IOUtils::write_eigen_matrix(m, path, desc)`, `IOUtils::read_eigen_matrix(m, path)` | `math/matrix_io.h`, `MATHLIB::write_matrix(m, path, desc)`, `MATHLIB::read_matrix(m, path)` |
| `utils/python_runner.h`, `python_test_helper.h`, `UTILSLIB::PythonRunner`, `PythonTestHelper` | `pybridge/...`, `PYBRIDGELIB::...`, in the new `mne_pybridge` library |
| `PythonRunner::runCode`, `isPackageAvailable`, `ensureVenv`, ...; `result.stdOut`, `exitCode`, ...; `config.pythonExe`, `timeoutMsec`, `extraEnv` (`KEY=VALUE` list), ... | `run_code`, `is_package_available`, `ensure_venv`, ...; `std_out`, `exit_code`, ...; `python_exe`, `timeout` (`std::chrono`), `extra_env` (map), ... |
| `PythonTestHelper::evalDouble(code, &ok)`, `evalVector`, `evalMatrix`, `evalMatrixViaFile` | `eval_double(code)` etc., returning `std::optional` |
| `PythonTestHelper::writeMatrix`, `readMatrix` | `MATHLIB::write_matrix`, `MATHLIB::read_matrix` |
| `GUARD_PYTHON`, `GUARD_PYTHON_PACKAGE` from `pybridge/python_test_helper.h` (QtTest macros) | removed from the library, which no longer needs QtTest: skip or fail with your test framework when `!helper.is_available()`, failing when `PythonTestHelper::is_python_required()`; MNE-CPP's own tests take the macros from `src/testframes/common/python_guard.h` |
| `PythonRunner` signals `lineReceived`, `progressUpdated`, `finished` | the line and progress callbacks; `PythonRunner` is no longer a `QObject` |
| `MLTrainer::checkPrerequisites(QStringList)`, `InvCMNE::trainLstm(QString...)` | `check_prerequisites(StringList)`, `train_lstm(std::filesystem::path...)` |
| `utils/report.h`, `UTILSLIB::Report` (`QString`, `QList<QPair>`); `addText`, `addTable`, `addKeyValue`, `addCode`, `sectionCount`, `toHtml`; `ReportSection` | `report/report.h`, `REPORTLIB::Report` (`std::string`, `std::vector<std::pair>`), in the new Qt-free `mne_report` library; `add_text`, ..., `to_html`; the section type is private |
| `utils/generics/observerpattern.h`, `commandpattern.h`, `UTILSLIB::Subject`, `IObserver`, `ICommand` | removed from the libraries; `mne_rt_server` keeps them for its commands |
| `Subject::t_Observers` (`QSet`), `observerNumDebug()`, the global `Subject::notifyEnabled` | `observers()` (`std::vector`, attach order), `observers().size()`, per-subject `set_notify_enabled()` |
| `ex_utils` | `ex_channels`; the other sections moved to `ex_base`, `ex_math`, `ex_pybridge`, `ex_report` and `ex_com` |
| `UTILSLIB::KMeans`, `Linalg`, `Numerics`, `SimplexAlgorithm`, `Spectral`, `Sphere`, `Warp` (in `math/`) | `MATHLIB::...` |
| `UTILSLIB::` and `RTPROCESSINGLIB::` names in `dsp/` (`FilterKernel`, `ICA`, `RtAveraging`, ...) | `DSPLIB::...` |
| `mne_utils`, `utils/utils_global.h`, `UTILSLIB::buildDateTime()` | removed: link the library you use; build stamps come from each library's `<NS>::buildDateTime()` |
| `utils/generics/circularbuffer.h`, `CircularBuffer_Matrix_double` | `base/circular_buffer.h`, `BASELIB::CircularBuffer<Eigen::MatrixXd>` |
| `CircularBuffer::push(ptr, n)`, `getFreeElementsRead()`, `pause()` | `push(std::span)`, `size()`; `pause` is removed |
| `MNE_LOG_LEVEL=FATAL` | removed; `CRIT` is the highest level |
| `ACQLIB::PolhemusConnection`, `PolhemusSerialConfig`, `FastrakParser`, `FastrakSample` (Qt SerialPort) | `AcqDevice::open("polhemus://<port>?stations=1,2")`: a stream of positions and quaternions per station, stylus button presses as event code 1 |
| `PolhemusCoregistration::setConnection(conn)`, `connection()` | feed the stream: `on_pose(station, position, orientation)`, `on_button(station, position, orientation)` |
| `com/rt_client/`: `COMLIB::RtClient`, `RtCmdClient`, `RtDataClient`, `MetaData` (Qt Network) | `AcqDevice::open("rtserver://host:4217?simfile=...&buffer=...")`: the measurement info as `fiff.MeasInfo/1` extension (`toFiffInfo()`), calibrated samples, `STI 014` changes as events |
| `com/rt_command/`: `COMLIB::Command`, `CommandManager`, `CommandParser`, `RawCommand`, `Subject`, `IObserver`, `ICommand` | removed from the libraries: the command vocabulary of `mne_rt_server` lives in its `rtserver_connector` (`RTSERVER::...`); clients send commands through `rtserver://` |

- FIFF channel names are read as stored: `"MEG 0113"`, `"STI 014"`, as in
  MNE-Python and MNE-C. They used to lose their spaces (`"MEG0113"`). Look-ups
  match the way MNE-C does, ignoring spaces and case, so files that spell a
  channel differently still combine (for example a forward solution written
  with `"MEG0113"` and a raw file with `"MEG 0113"`). Compare names with
  `FIFFLIB::same_channel(a, b)` and find them with
  `FIFFLIB::channel_index(names, name)` or `has_channel` (`fiff_ch_name.h`)
  instead of `==`, `BASELIB::indexOf` or stripping spaces.
- The test of the MNE Scan plugins links with MSVC again: it used the
  EEGoSports and TMSI impedance widgets and electrode items, which their
  plugins did not export. The plugins now hide what they do not export on
  every platform, as Windows does, so such a test fails on macOS and Linux
  too; the connector templates that MNE Scan casts across plugins keep one
  type identity per process (`SCSHARED_TEMPLATE_VISIBLE`).
- `Log` keeps working while the program exits: a message that Qt or another
  library logged after static destruction had begun locked a destroyed mutex
  and aborted the program (`mne_anonymize --help` with Qt 6.10 on macOS,
  where `QCommandLineParser` exits with the `QApplication` still alive).
- `run_process()` builds with GCC's `-Wmaybe-uninitialized` in the coverage
  configuration; it held its deadline in a `std::optional` that GCC 14
  misread.
- On Windows, `FileByteStream` opens files shared, as on the other
  platforms: it locked out every other handle, so a FIFF file could not be
  read twice at once (reading an evoked file with its projections crashed),
  and a file held by a `QFile` or `QTemporaryFile` could not be written
  (`filterFile`, `mne_cov2proj`, `mne_make_source_space`).
- Text files written by the libraries (events, selections, layouts, matrices,
  dipoles, source-estimate CSV, cluster info, the log file) end their lines
  with LF on Windows too, like the files of MNE-C, MNE-Python and FreeSurfer;
  a quality test refuses a text-mode `std::ofstream` in the libraries.
- `os_pretty_name()` names Windows Server by its year, like QSysInfo
  (`Windows Server 2025 (10.0.26100)`); it reported Windows 11.
- `PythonRunner` hands out the lines of a Python child without the CR that
  Python on Windows writes before each LF, in `std_out`, `std_err`, the line
  callback and the progress parser; results compared with `"5"` or matrices
  parsed from the output failed on Windows.
- `FileByteStream::resize()` cuts the file through the stream's own handle.
  It used to resize through the path, which Windows refuses while the stream
  has the file open, so FIFF writing could not drop an old directory there,
  and which hit another file once the open one had been renamed.
- The acquisition benchmark gate no longer fails on the slow and fast phases
  of the shared runner, which moved ring, DMA and sustained throughput by up
  to 2x for both builds alike and failed commits that did not touch
  `mne_acq`. A gated metric compares the median over the rounds of head
  relative to base within a round (both run back to back), not the best of
  each; a regression must also show in five further rounds before it fails.
  `acq-bench.json` keeps every run's value.
- Every include sits under the banner of its kind: INCLUDES (the file's own
  header, then headers next to the file, then MNE-CPP and other headers),
  QT INCLUDES, EIGEN INCLUDES and STL INCLUDES, in that order; 980 files had
  standard headers under INCLUDES, Qt or Eigen headers under the wrong banner,
  empty banners, duplicate includes or the old STD/C++/MNE INCLUDES banners.
  `check_format.py` checks it (`scripts/quality/include_sections.py`) and
  `--fix` regroups the includes.
- `M_PI` is replaced by `std::numbers::pi` everywhere, so no file defines
  `_USE_MATH_DEFINES` before `<math.h>` any more and the build no longer
  defines it for MSVC: a C++ standard library without the POSIX constants,
  as on embedded targets, compiles MNE-CPP. Code that used `M_PI` through an
  MNE-CPP header defines it itself or uses `std::numbers::pi`.
- The libraries need nothing beyond Eigen and the standard library, so they
  can also be built for embedded targets: Skigen is no longer a dependency.
  The Ledoit-Wolf and OAS shrinkage and the Factor Analysis EM of `mne_sts`
  and the Local Outlier Factor of `mne_dsp` are implemented in those
  libraries (adapted from Skigen; the results agree to 1e-12) and work on the
  `(n_channels, n_samples)` data directly instead of a transposed copy. They
  are header templates on the scalar type, so an embedded target estimates
  float data in float: `ledoit_wolf_covariance()`, `oas_covariance()` and
  `factor_analysis_covariance()` (which also returns the loadings, the noise
  variances and the iterations) in `sts/sts_cov_estimators.h`, and
  `local_outlier_factor()` in `dsp/bad_channels_lof.h`. `StsCovEstimators`
  and `computeLofScores()` call them in double precision.
  `init.sh`/`init.bat` and CI no longer clone Skigen, `SKIGEN_RELEASE_REF` and
  `SKIGEN_DEV_REF` are gone from `src/external/external_deps.env`, and the
  dependency audit refuses a `Skigen::Skigen` link. The API registry no
  longer records Skigen ports: `skigen_candidate` and `skigen_target` are
  gone from `doc/api_registry.json`, `validate_api_registry.py` rejects them
  and no longer cross-checks a Skigen registry (`--skigen-registry` is gone).
  Migration: delete an old `src/external/skigen` clone.
- Compiler caches work across commits. Every library and plugin had the git
  hash in its compile definitions, and `base/base_global.h` expanded
  `__DATE__` and `__TIME__` in every file that included it, so each commit
  and build changed nearly every compile and ccache hit about 2 % of the
  files in CI. Only `base_global.cpp` and `acq_global.cpp` see them now;
  `base/buildinfo.h` is gone. `scripts/quality/tests/test_compiler_cache.py`
  keeps it that way.
- Medical-device readiness (IEC 62304, ISO 14971): a SOUP inventory of every
  third-party component with version, license, purpose, failure effect and
  mitigation (`scripts/compliance/soup_catalog.json`, generated into
  `doc/compliance/soup.json`, a CycloneDX SBOM and the "Third-Party Software"
  page); CI refuses a third-party CMake target the catalog does not list and
  uploads an SBOM with the versions the Linux build used. A class-to-test
  traceability matrix from `doc/api_registry.json` with per-library floors
  that may only rise (`scripts/compliance/traceability.py`). The
  "Medical-Device Readiness" page lists what is in place and the roadmap.
- `scripts/bench/` measures MNE-CPP for the website's Performance page:
  acquisition throughput, wake-up latency and loss-free paced rates
  (`mne_acq_bench`), forward and inverse computation against MNE-Python on
  the same sample data, and start-up time and memory. `refresh.sh` measures,
  writes the results JSON, renders SVG charts that name the machine, OS,
  compiler, commit and the command to reproduce them, and regenerates the
  page. A run that loses samples is reported on the chart, not hidden.
  The forward comparison runs MNE-Python both serially and with one job per
  core. On every push to staging, CI measures the acquisition suite and keeps
  results and charts as the `benchmarks` artifact.
- `mne::CommandLineParser` (`base/command_line.h`) parses a tool's options
  in standard C++ and agrees with `QCommandLineParser` on the forms the tools
  use. `mne_acq_bench` uses it and no longer needs Qt, so the benchmark job
  builds without downloading Qt.
- `mne::format_number()` formats a number like `QString::number()`, and
  `mne::tie_away_from_zero()` lets `std::format` round decimal ties away from
  zero as Qt does (`base/string_utils.h`). They replace eleven copies in the
  ported command-line tools.
- `mne::to_little_endian()`, `from_little_endian()` (`base/endian.h`) and
  `ByteStream::read_little_endian()` replace the byte-order code of six
  conversion tools.
- `mne::RasterImage` (`base/raster_image.h`) fills anti-aliased polygons,
  draws text in a built-in bitmap font and writes PNG files, without a window
  system or Qt. Fills take an opacity, `fill_polygons()` fills overlapping
  polygons as one area and `blend_pixel()` mixes a colour into a pixel;
  `decode_png()` and `load_png()` read 8-bit RGB and RGBA PNG files, e.g.
  golden images of the Qt-free tests.
- New library `mne_viz` (`mne/viz/`), Qt-free: `mne::viz::Canvas`
  records drawing calls that follow `QPainter` (pens, brushes with linear
  gradients, fonts, transforms, clips, opacity, cubic curves, text and images)
  into a `DisplayList`, which `to_svg()` writes as a deterministic SVG document
  and `to_raster()` renders into a `RasterImage`. It holds the plot models
  `LineChart`, `BarChart`, `SplinePlot`, `ImagePlot`, `VectorPlot`,
  `ButterflyPlot` and `AverageTile`, axis ticks (`nice_ticks()`,
  `tick_label()`) and the colour maps (`map_color()`, moved from
  `viz::qt::ColorMap`, which now calls them). `ex_scene` writes a plot as SVG and
  PNG without Qt.
- The plot widgets of `mne_viz_qt` (`LinePlot`, `Bar`, `Spline`, `ImageSc`,
  `TFplot`, `Plot`, `ButterflyView` and the channel tiles of
  `AverageLayoutView`) draw their `mne_viz` model through a QPainter backend
  (`viz/backends/qt/plots/helpers/scenepainter.h`: `paintScene()`, `QtTextMetrics`). On
  screen they draw the same pixels as before; `test_scene_render` compares
  every model's SVG and PNG output with golden files. `ButterflyView` saves
  SVG screenshots as vector graphics instead of an embedded bitmap. Removed
  with the move: `Graph::drawLabels()`, the protected data members of `Plot`
  and `ImageSc` (`ImageSc::normalize()`, `updateMaps()`), the public members
  `Spline::m_dMinAxisX` and `m_dMaxAxisX`, the protected helpers of
  `ColorMap` and `plots/detail/niceticks.h` (use `viz::nice_ticks()`).
  The Qt usage ceilings of `disp` drop accordingly (tokens 5149 to 4883).
- The coverage gate holds acq, dsp, fiff, fwd, inv and mne to their own line
  coverage floors (`library_floors` in `scripts/quality/coverage_policy.json`),
  so a weak library on the signal path cannot hide behind the total.
- Packages and the Qt builds leave out Qt Svg: the deploy scripts drop the
  svg image format and icon engine plugins (and with them QtSvg), and the Qt
  configure steps skip `qtsvg`. MNE-CPP writes SVG with its own writer and
  uses no SVG icons.
- The SOUP inventory lists the Qt modules the libraries, applications and
  tools request (also as sub-components in the CycloneDX SBOM) and the system
  `curl` program that MNE Analyze Studio's assistant runs.
- The GPU views run without Qt Widgets in their core: `viz::qt::ChannelRhiRenderer`
  holds the state, input handling and QRhi rendering of the raw-data trace
  view, and `disp3D::BrainViewCore` those of the 3-D brain view. Each has two
  hosts: the existing `ChannelRhiView` and `BrainView` (still `QRhiWidget`s,
  same API) and the new `QWindow` hosts `ChannelWindow` and `BrainWindow`,
  which create their own QRhi and swap chain (embed them with
  `QWidget::createWindowContainer()`). The crosshair, scalebars and ruler of
  the trace view and the FPS, info and viewport labels and separators of the
  brain view are drawn in the GPU pass instead of by child widgets.
  `LiveMarker` moved to `viz/backends/qt/3d/view/brainviewcore.h`.
- `audit_qt_modules.py --check` keeps the rendering code of `mne_viz_qt` and
  `mne_viz_qt` free of Qt Widgets: the colour maps, scene painter, SVG writer,
  item models, `ChannelRhiRenderer`, `BrainViewCore` and the `QWindow` hosts
  (`widgets_free` in `scripts/quality/qt_usage_policy.json`).
- The sensor-layout views of `mne_viz_qt` no longer use `QGraphicsView`:
  `mne::viz::SensorLayout` (sensors as labelled dots, with click and
  rubber-band selection as in a `QGraphicsScene`) and
  `mne::viz::AverageLayout` (an `AverageTile` per channel at its layout
  position) are the Qt-free models; `viz::qt::SceneViewport` shows such a model
  with wheel and pinch zoom, right-button panning, a rubber band and
  double-click fitting, and keeps it fitted until the user zooms or pans.
  `ChannelSelectionView` picks sensors in a `SensorLayoutWidget`,
  `AverageLayoutView` and mne_browse's average window draw an
  `AverageLayoutWidget`, and their SVG export writes vector graphics.
  Breaking: `ChannelSelectionView::selectionChanged()` and
  `AverageLayoutView::channelSelectionManagerChanged()` take a
  `SelectionItem` instead of a list of `QGraphicsItem`; `SelectionItem` moved
  to `viz/backends/qt/viewers/helpers/selectionitem.h` and copies all its fields;
  `LayoutScene`, `SelectionScene`, `SelectionSceneItem`, `AverageScene` and
  `AverageSceneItem` are removed (mne_browse keeps a private `LayoutScene`
  for its butterfly plot). Highlighted sensors now show an orange ring.
- `mne::viz::FilterResponsePlot` draws the magnitude response of a filter
  in dB with its cut-off frequencies; the new `viz::qt::FilterPlot` widget shows
  it, and `FilterDesignView` uses that widget instead of a `QGraphicsView`.
  Its plot export writes vector SVG. Removed: `FilterPlotScene` and
  `FilterDesignView::resizeEvent()`.
- `mne::viz::TraceView` (`viz/views/trace_view.h`) holds the viewport and
  interaction state of the raw-data trace view without a GUI toolkit: scroll
  position, zoom, file bounds and clamping, the channel window, wheel zoom and
  scrolling, drag scrolling with inertia, the ruler with its axis snapping,
  annotation range selection and boundary resizing, and the crosshair. Input
  arrives as toolkit-neutral `PointerEvent` and `WheelEvent`
  (`viz/views/input_event.h`) with timestamps; what the host has to do (animate a
  scroll or zoom, change the cursor, redraw, report clicks and edits) comes
  back through callbacks. `draw_overlays()` draws the crosshair, scalebars,
  ruler and annotation selection into a `Canvas`. `viz::qt::ChannelRhiRenderer`
  delegates to it and paints the overlays with `paintScene()`; its API and
  signals are unchanged. `test_scene_trace_view` tests it without Qt, with
  golden SVG files of the overlays. The Qt usage ceiling of `disp` drops
  accordingly (tokens 4885 to 4680).
- The rest of the trace view's logic moved to `mne_viz` as well:
  `decimate_trace()` (the min/max decimation and detrending that
  `ChannelDataModel::decimatedVertices()` now calls), `visible_channels()`,
  and `TraceGeometry` (`viz/views/trace_geometry.h`), which streams the trace
  vertices over the prefetch window and lists the traces of a frame with
  their shader parameters (`TraceUniforms`, the 52-byte uniform block of the
  trace shader). `TraceSource` (now `viz/views/trace_source.h`) supplies colours,
  bad flags and vertices, and computes the butterfly lanes by default.
  `TraceMarkers` draws the annotation, event and epoch overlay into a
  `Canvas`; `draw_trace_figure()` draws the whole view as a display list.
  `ViewAnimator` runs the scroll, inertial and zoom animations (cubic
  ease-out) over explicit timestamps. `viz::qt::ChannelRhiRenderer` only uploads
  vertices and uniforms, paints the display lists into its textures and
  `renderToImage()` result, and ticks the animator per frame and on a timer
  instead of using `QPropertyAnimation`; its API and signals are unchanged.
  The dotted guide lines of `renderToImage()` use the scene's dot pattern.
- The trace view no longer refetches all trace vertices on every scroll step:
  the refetch margin equalled the prefetch, so any scroll left it. It now
  refetches once half of the prefetch on a side is used up, and never on a
  side where the fetched window already ends with the data.
- `mne_viz` describes GPU frames without a graphics API (`viz/gpu/gpu_frame.h`):
  a view keeps its vertex and index buffers, textures (RGBA pixels or a
  `DisplayList` the backend rasterizes) and pipelines (shader id, vertex
  layout, topology, blending, culling, depth state) in `GpuResources` with
  per-resource revisions, and builds one `GpuFrame` per frame from passes and
  draws (uniform blocks in std140 layout, textures, viewport, scissor,
  instancing). `SoftwareRenderer` (`viz/gpu/software_renderer.h`) executes
  frames on the CPU into an RGBA `Image`: clipped, perspective-correct
  triangles with depth testing, culling and blending, one-pixel lines and
  points, and C++ counterparts of the shaders (`channeldata` and `overlay` of
  the trace view so far). `to_rgba_image()` rasterizes a figure with its
  opacity. `TraceFrameBuilder` (`viz/gpu/trace_frame.h`) describes the frames
  of the trace view: traces, the marker overlay with the per-second bands
  and the crosshair, scalebar, ruler and selection overlay.
  `test_scene_gpu_frame` tests the software backend; `test_scene_trace_view`
  renders a trace view headless against a golden PNG.
- `viz::qt::RhiFrameRenderer` (`viz/backends/qt/plots/helpers/rhiframerenderer.h`) is the
  QRhi backend of the GPU frame description: one QRhi buffer, texture and
  pipeline per resource handle, uploads of what changed, the uniform blocks
  of all draws in one dynamic buffer, shaders as precompiled `.qsb` files
  found by id, figure textures painted with `QPainter`. `checkShaders()`
  compares a frame with the reflection of its shaders (uniform block sizes,
  samplers, vertex inputs). `ChannelRhiRenderer` renders the frames of
  `viz::TraceFrameBuilder` through it instead of its own QRhi code; its API
  is unchanged except that `hudImage()` returns the image by value.
- `viz::BrainFrameBuilder` (`viz/gpu/brain_frame.h`) describes frames of the
  3-D brain view as `GpuFrame`: surfaces with the standard, holographic
  (back faces, then front faces), anatomical, x-ray and normals shaders,
  instanced dipole and electrode glyphs, and a screen overlay, pane by pane;
  mesh streams are uploaded only when their generation changes. The software
  backend gains C++ counterparts of these disp3D shaders, so the brain view
  renders headless (`test_scene_brain_view` compares a golden PNG), and
  `test_disp3d_brain_window` checks the frames against the reflection of the
  `.qsb` shaders.
- New tool `mne_render` renders raw data traces (PNG through the trace view's
  GPU frames, or SVG) or a FreeSurfer surface with a source estimate (PNG)
  headless, without Qt, a window system or a GPU, on the software backend of
  `mne_viz`; `--autoscale` scales channel types to their signals.
- New example `ex_scene_sdl` (`-DBUILD_EXAMPLES_SDL=ON`, needs SDL3) runs the
  trace view in an SDL3 window instead of Qt: the same `TraceView`,
  `TraceBuffer` and `TraceFrameBuilder`, frames executed by the software
  backend and shown as an SDL texture, SDL input translated to scene input
  events, a synthetic recording streaming in. The Qt-free CI job builds and
  runs it on macOS (SDL's dummy video driver).
- Settings panels are described without a GUI toolkit: `viz::SettingsForm`
  (`viz/forms/settings_form.h`) lists the fields of a panel (bool, int and double
  with range, step, unit and spin box or slider, choice, text, file path,
  action button, or a host-provided custom widget), their groups, and
  enabled/visible conditions, keeps the values valid and tells listeners
  about changes. `viz::qt::SettingsPanel` builds the Qt Widgets of any form and
  persists its values in `QSettings`, so another toolkit needs one builder
  instead of a hand-written view per panel. Flags (`add_flag()`) hold host
  modes that conditions refer to without being shown or stored, fields can
  opt out of persistence, and group boxes hide while none of their fields
  shows. `CovarianceSettingsView`, `SpectrumSettingsView`,
  `AveragingSettingsView`, `FilterSettingsView`, `ArtifactSettingsView` and
  `TfSettingsView` are built from forms; their APIs, signals and `QSettings`
  keys are unchanged and three `.ui` files are gone. The Qt usage ceiling of
  `disp` drops to 4519 tokens.
- The display pre-processing of the raw and evoked viewers moved out of Qt
  into `mne_dsp`: `DisplayCorrection` (`dsp/display_correction.h`) builds and
  applies the SSP projector (bad channels zeroed) and the CTF compensator,
  and `RtRawDisplay` (`dsp/rt/rt_raw_display.h`) adds SPHARA, FIR filtering
  of a channel selection with overlap-add across blocks, and trigger
  detection for streamed blocks. `viz::qt::RtFiffRawViewModel` and
  `viz::qt::EvokedSetModel` are adapters that place the results into their
  buffers.
- `mne_viz` holds toolkit-independent view models (`models/`): `ChannelTable` (channel information table
  with layout name mapping), `SpectrumTable` (spectra, channel selection,
  frozen copy, linear or log frequency axis and band) and `BidsTree`
  (subjects, sessions, data-type folders and items with the BIDS naming
  rules). `viz::qt::ChannelInfoModel`, `viz::qt::FrequencySpectrumModel` and
  `viz::qt::BidsViewModel` adapt them to Qt item models; items of the BIDS model
  carry their tree node in `BIDS_ITEM_NODE`. The Qt usage ceiling of `disp`
  drops to 4403 tokens.
- The data behind the raw-data views moved to `mne_viz` as well.
  `TraceBuffer` (`viz/views/trace_buffer.h`) stores the samples of the trace view
  (replaced, or appended to with a capacity), resolves the channel
  descriptions a host gives (`ChannelSpec`: name, type, colour, scale keys,
  bad flag, virtual) against a scale map and a signal colour, computes the
  RMS, and is a `TraceSource` that decimates and detrends its traces; changes
  reach listeners as callbacks. `viz::qt::ChannelDataModel` wraps it, describes
  the channels of its `FiffInfo` (per-kind scales, colours and type labels)
  and turns the callbacks into its unchanged signals; its API is unchanged
  apart from the new `buffer()`, which `ChannelRhiRenderer` now reads as its
  trace source. `SweepBuffer` (`viz/views/sweep_buffer.h`) is the sweeping window
  of the real-time view: raw and filtered layers written at a wrapping write
  position, new sweeps, the last block, the frozen copy and the sample under
  a column; `viz::qt::RtFiffRawViewModel` keeps its SSP, compensator, SPHARA,
  filter and trigger processing (they need `mne_fiff` and `mne_dsp`) and
  writes into it. `reconcile_conditions()` and `pre_stimulus_samples()`
  (`viz/models/evoked_conditions.h`) do the per-condition colour and visibility
  bookkeeping and the onset search of `viz::qt::EvokedSetModel`.
  `test_scene_trace_buffer` tests them without Qt. The Qt usage ceiling of
  `disp` drops to 4529 tokens.
- The camera, input handling, pane layout and head-up display of the 3-D brain
  view moved to `mne_viz` (which now links Eigen): `OrbitCamera`
  (`viz/views/camera.h`) computes the projection, view and model matrices, the
  seven view presets and the orbit and pan of a drag; `PaneCamera` holds the
  preset, zoom, pan and rotation of a pane (`viz::qt::SubView` derives from it,
  so its `perspectiveRotation` and `QVector2D pan` are now the Eigen
  `rotation` and `pan`, and `SubView::load()` takes an `Eigen::Quaternionf`
  fallback). `ray_from_pixel()`, `intersect_ray_triangle()` and
  `intersect_ray_sphere()` (`viz/views/ray_picking.h`) do the picking maths of
  `BrainSurface`, `DipoleObject`, `ElectrodeObject` and `RayPicker`.
  `PaneLayout` (`viz/views/pane_layout.h`) lays out 1 to 4 panes with
  their separators and splitters. `BrainViewInteraction` is the input state
  machine (orbit, pan, wheel zoom per pane, pane selection, preset-menu
  clicks, splitter drags, R and S keys) with effects through callbacks, and
  `BrainViewOverlay` draws the FPS, info, region and pane-name labels and the
  separators as a display list. `viz/views/input_event.h` gains `KeyEvent` and the
  buttons held during a pointer event; `toScenePointerEvent()` and
  `toSceneWheelEvent()` (`viz/backends/qt/plots/helpers/sceneinput.h`) translate Qt
  events for the trace and brain views. `viz::qt::BrainViewCore` translates
  events, converts the matrices for its QRhi uniforms and paints the overlay
  with `paintScene()`; the APIs and signals of `BrainViewCore`, `BrainView`
  and `BrainWindow` are unchanged, and `CameraController`, `RayPicker` and
  `viz::qt::MultiViewLayout` remain as Qt-typed adapters. `test_scene_brain_view`
  tests it without Qt, with a golden SVG of the overlay. The Qt usage ceiling
  of `disp3D` drops to 1807 tokens.
- `mne::write_mat_file()` (`math/mat_file.h`) writes matrices to a MATLAB
  Level 5 MAT-file; `mne_raw2mat` and `mne_epochs2mat` use it instead of two
  copies of the writer (their files are byte-identical).
- The inverse operator's decomposition of the whitened lead field uses Eigen's
  divide-and-conquer SVD, as MNE-Python uses LAPACK's: about 2.5 times less
  CPU time for the MNE sample data, with the same source estimates to single
  precision.
- `Linalg::thin_svd()` decomposes strongly rectangular matrices on all cores
  (blocked tall-skinny QR, then a divide-and-conquer SVD of the small factor)
  and `Linalg::parallel_product()` multiplies by column blocks. The inverse
  operator uses both, reads and assembles without copying the source spaces,
  and `FiffStream` writes arrays and matrices in one call instead of one per
  value. Building the MNE sample inverse operator takes about 0.7 s instead of
  5 s, faster than MNE-Python on the same machine. The benchmark disables the
  tool's MNE-C style noise-covariance regularisation, so both compute the same
  operator.
- The BEM forward model multiplies its coil and electrode solution matrices
  with the infinite-medium potentials in one matrix-vector product instead of
  one strided dot product per sensor. The MNE sample forward solution (oct-6,
  MEG and EEG, three-layer BEM) takes 3.5 s instead of 19 s and 7 s of CPU
  time instead of 93 s; MNE-Python takes 13 s. With `--accurate`, the coil
  definitions MNE-Python uses, both agree to 2e-5; the benchmark passes it.
- Reading source spaces computes the triangle geometry and the vertex and
  triangle neighbours on all cores and moves each hemisphere into place
  instead of copying it; named matrices are transposed while they are
  widened, and the inverse operator writes its eigenleads with one copy
  instead of three. `thin_svd()` and `parallel_product()` split their work
  into a fixed number of blocks, so the result no longer depends on the
  number of cores.
- Comparing two hemispheres (`==`) turned the sparse source-space distance
  matrix into a dense np x np matrix, which for a full cortical surface does
  not fit in memory. It compares the sparse matrices.
- `FiffRawData::read_raw_segment()` skipped the buffer a segment starts in
  when the segment starts on that buffer's last sample, and returned
  uninitialised data for it.
- The command-line tools are standard C++: they parse their options with
  `mne::CommandLineParser` and use `std::filesystem`, `std::print` and
  `mne::Json` instead of Qt Core, and link only the MNE-CPP libraries they
  need. Their options, output files and exit codes are unchanged; the help
  text is laid out differently. Number output rounds halfway values away from
  zero as before. Only `mne_screenshot_regression`, which renders with
  disp3D, and the live `view` of `mne_acq` still need Qt.
- `mne_make_movie` renders its frames with `mne::RasterImage` instead of
  `QPainter` and no longer needs Qt; frames differ from before only at
  anti-aliased triangle edges and in the font of the time stamp.
- `mne_acq` is standard C++; only its `view` command, built with the display
  library, uses Qt Widgets.
- `mne_rt_server` is standard C++ on `mne_com` sockets and threads. The FIFF
  simulator is built in instead of being loaded as a Qt plugin, so the
  `mne_rt_server_plugins` folder is gone; commands and wire protocol are
  unchanged, and replies go out when a command finishes instead of on the
  next 100 ms poll.
- `mne_anonymize` is a Qt-free command-line tool; its graphical interface is
  the new application `mne_anonymize_gui`. `--no-gui` is still accepted.
  `-md`, `-mdo`, `-sb` and `-sbo` also work with a single dash, and a file
  that cannot be opened is reported instead of crashing the tool.
- A default-constructed `FiffInfo` left `proj_id` and the microseconds of
  `meas_date` uninitialised, so the raw files the converters write differed
  from run to run in `FIFF_PROJ_ID`. They start as -1 and 0.
- `mne_make_cor_set` read float (`MRI_FLOAT`) mgh voxels as 8-byte doubles
  and could not read gzipped `.mgz` volumes; it reads 4-byte floats and
  decompresses `.mgz`.
- `mne_volume_data2mri` read the values of `.w` files as 8-byte doubles; they
  are 4-byte floats, as MNE-Python reads them.
- Completing a volume source space, which has no triangles, read past the
  per-vertex triangle lists and crashed `mne_list_source_space` and
  `mne_make_uniform_stc`; the lists have one entry per vertex.
- `MNESourceSpace::filter_source_spaces()` crashed on a bounding surface
  without triangle geometry, as `mne_volume_source_space` builds it; the
  geometry is computed when missing.
- Reading a source space ignored its type, so volume and discrete source
  spaces came back as surfaces; `FIFF_MNE_SOURCE_SPACE_TYPE` is read, and a
  space without it is a surface, as in MNE-Python.
- `mne_volume_source_space` wrapped the source space in a second source-space
  block, so readers found an empty source space; it writes one block.
- Volume source spaces are written with their parent-MRI block, as
  MNE-Python writes and requires it; without an MRI it holds the grid's voxel
  transform and dimensions and an empty interpolator. MNE-Python reads the
  output of `mne_volume_source_space`, which matches MNE-C's
  `volume-7mm-src.fif` point for point.
- `mne_convert_lspcov` aborted with an uncaught exception when `--meas`
  could not be read; it reports the file and exits with 1.
- `mne_mna_bids_converter pack` built every session it scanned in a BIDS
  tree without a project sidecar but never added it to its subject, so the
  packed project was empty.
- Minimum-norm estimates on a volume source space listed every vertex
  twice, so the `.stc` written by `mne_compute_mne` could not be read back.
  `InvMinimumNorm` takes the vertices of all source spaces, and
  `InvSourceEstimate::write` refuses data whose rows do not match the vertices.
- `mne_compute_mne` names a volume estimate `<out>-vl.stc`, which MNE-Python
  reads as a `VolSourceEstimate`.
- `mne_compute_mne --pred` multiplied a forward solution with the normalised,
  orientation-combined estimate, which crashed or wrote wrong values, and
  skipped the output for measured data. It predicts the data from the inverse
  operator as MNE-C does; `--predfwd` is gone.
- Every object created with `new` has an owner: a Qt parent, an
  ownership-taking Qt call, or a `std::unique_ptr` that hands it over with
  `release()`. The maintainability audit counts raw `new`, raw `delete` and
  unowned Qt objects, and all three ceilings are 0. MNE Scan plugins offer
  their settings widgets with `AbstractPlugin::offerControlWidgets()`, which
  keeps the ones no control view adopted instead of leaking them; MNE Analyze
  plugins hand out the menus, docks and views they built the same way.
- The `USE_FFTW` option is gone. It never reached the compiler (no build
  defined `EIGEN_FFTW_DEFAULT`), so every FFT has always run on Eigen's
  built-in KISS FFT; the dead FFTW plumbing in the build is removed.
- `disp3D` uses the standard library wherever Qt is not the API it talks to.
  Data, keys, names and paths are `std::` types: `std::shared_ptr` instead of
  `QSharedPointer` (`GeometryInfo`, `Interpolation`, the source workers and
  controller, `SourceEstimateOverlay`, `StcLoadingWorker`), `std::vector`,
  `std::map`, `std::unordered_map` and `std::pair` instead of the Qt
  containers, `std::string` keys (the surface map of `BrainView`,
  `SensorFieldMapper`, `RayPicker`/`RayHit`, `SceneLayer::id`/`displayName`,
  `MultimodalScene`, `surface_keys`, colormap, shader and overlay names) and
  `std::filesystem::path` for every file `DataLoader`, `BrainView`,
  `SourceEstimateManager`, `SourceEstimateOverlay`, `BrainSurface` and
  `StcLoadingWorker` read or write; `probeEvokedSets` returns
  `std::vector<std::string>`. The signals carrying colours, keys and picks
  use the same types. Signals consumed by widgets (`stcLoadingProgress`,
  `hoveredRegionChanged`, `electrodeContactClicked`, `shaderModeChanged`),
  the tree model and `PickResult` keep `QString`. The real-time controllers
  stream on a `mne::viz::qt::PeriodicTask` (`std::jthread`) and recompute
  interpolation matrices in order on a one-thread `mne::ThreadPool` instead of
  `QThread`s; locks are `std::mutex`, timing `std::chrono`, logging
  `mne::Log`.
- `disp` uses the standard library wherever Qt is not the API it talks to.
  Scale maps are `std::map<int, float>` (`ScalingView`, `getScalingValue`,
  `ButterflyView`, `AverageLayoutView`, `RtFiffRawView`, `ChannelDataView`);
  average colours and activations, modality, projector, compensator and
  artifact-threshold maps, layout maps and stim-channel maps are `std::map`
  keyed by `std::string`; channel names and selections are
  `std::vector<std::string>` and `std::vector<int>` (`ChannelInfoModel`,
  `ChannelSelectionView`, `SelectionItem`, `ApplyToView::getSelectedViews`,
  `showSelectedChannelsOnly`, `selectRows`, `setChannelFilter`); `LinePlot`
  takes `std::vector<double>`; `ChannelDisplayInfo`, `EventMarker`,
  `AnnotationSpan` and the time-ruler marks carry `std::string` labels;
  `RtFiffRawViewModel::addData` and `RtFiffRawView::addData` take a
  `std::span<const Eigen::MatrixXd>`; detected triggers are
  `std::map<int, std::vector<std::pair<int, double>>>`; the trigger and stim
  channel names, `EvokedSetModel::getBaselineInfo` (`std::pair<float, float>`),
  the HPI coil frequencies and errors (`std::vector`) follow. Files are
  `std::filesystem::path`: `FigureExporter::directory`/`save`, the coregistration
  file signals, `HpiSettingsView::digitizersChanged`/`loadCoilPresets`,
  `FwdSettingsView::atlasDirChanged` and the CMNE model checkpoint of
  `MinimumNormSettingsView`; `SvgWriter` takes UTF-8 `std::string` file names,
  titles and descriptions and writes through `std::ofstream`. Widget text,
  model data, `QSettings` and signals consumed by widgets keep `QString`;
  logging is `mne::Log`, locks `std::shared_mutex`, timing `std::chrono`.
- Qt Concurrent, Qt Network, Qt Multimedia, Qt XML, Qt Print Support and Qt
  SVG are no longer needed; MNE-CPP builds against Qt Core, Gui, Widgets and
  Shader Tools (plus Qt Test for the tests). The SVG export of the viewers,
  MNE Browse, MNE Analyze and the impedance widgets of MNE Scan goes through
  the new `mne::viz::qt::SvgWriter`, a paint device that writes SVG 1.1 with Qt
  Gui only, in place of `QSvgGenerator`. Background work runs on
  `mne::ThreadPool` (new: `active_count()`), sockets are `mne_com`'s:
  `mne_rt_server` serves its command and data ports through them, and MNE
  Analyze Studio's services talk over loopback TCP whose port and access
  token the server writes to an owner-only file in the user's runtime
  directory. Studio's LLM requests go through the system `curl` (7.76 or
  newer), with the API key and body in owner-only temporary files so they do
  not show in the process list. MNE Browse requested Print Support without
  using it. Every target of `disp`, `disp3D`
  and the applications has an allowlist of Qt modules (`qt_modules` in
  `scripts/quality/qt_usage_policy.json`), which
  `scripts/quality/audit_qt_modules.py --check` enforces in CI, together with
  the per-file audit of what each module is used for.
- macOS packages need macOS 26 or later (was 14.4). The libraries parse
  floating-point numbers with `std::from_chars`, which Apple's C++ library
  provides from macOS 26 on. `scripts/quality/tests/test_macos_availability.py`
  compiles every unit that uses it for the minimum macOS.
- The libraries build, install and link without Qt. `MNE_BUILD_QT` (default
  `ON`) builds the Qt-based parts: `mne_viz_qt`, `mne_viz_qt`, the applications
  with their plugins, the CLI tools and the QtTest tests and examples. With
  `-DMNE_BUILD_QT=OFF`, or when CMake does not find Qt (it says so and
  switches the option off), every other library is built and installed with
  the examples that use the libraries only, which then run as the tests. The
  installed `MNE-CPPConfig.cmake` looks for Qt only for `disp` and `disp3D`,
  so `find_package(MNE-CPP COMPONENTS ...)` of any other component needs no
  Qt, also from an SDK built with Qt. `scripts/ci/qt_free_sdk.sh` builds,
  tests and installs the libraries with Qt hidden from CMake and builds a
  consumer against the install tree; CI runs it on Linux, macOS and Windows,
  and the SDK consumer of the Qt builds no longer has Qt on its prefix path.
  The dependency audit refuses a Qt link outside the display layers
  (`DEP022`, `qt_layers` in `dependency_policy.json`) and allows only the Qt
  modules the display libraries use.
  Migration: a downstream project that relied on MNE-CPP's package to bring
  Qt along (for example `Qt6::Core` through `MNE-CPP::mne_fiff`) calls
  `find_package(Qt6 ...)` itself.
- 3D View builds with MSVC again: the real-time source interpolation worker
  kept FreeSurfer labels in vectors of a type its header only declared, which
  MSVC rejects in an exported class. A quality test finds exported classes
  that hold forward-declared types without declaring their destructor.
- The Windows build and test run get further: MNE Analyze's `EventGroup`
  defines its destructor in the events library, so the events test links
  with MSVC; the real-time DSP and FIFF sniffer examples, `mne_map_data` and
  a label test no longer hide locals or narrow `size_t`; and the base example
  closes its log file and its byte stream before it removes its directory,
  which Windows refused (the example now checks that the removal succeeds).
- The BrainAmp and EEGoSports MNE Scan plugins export their classes again on
  Windows: their export macro waited for a definition that no build set, so
  the plugins declared their own classes as imported. A quality test now
  checks that the build defines every plugin's and application's export
  macro.
- More of what only CI's compilers reject, now also checked locally: MSVC's
  narrowing of `size_t` (C4267) in the MNE-C raw-data buffer sizes and cluster
  counts, names that hide a local or a member (C4456, C4458) in `BidsRawData`
  and `MNEChSelection`, glibc's unchecked `write()` and a `qint64*` passed as
  `std::int64_t*` (different types on Linux) in tests, and stale API uses in
  the WebAssembly-only code of MNE Browse, MNE Analyze, MNE Inspect and MNE
  Anonymize (which now reports a file it cannot write or read in the browser).
- `lsl://` maps LSL time to the host clock from one instant: it reads the
  steady clock between two LSL clock readings and takes their midpoint, the
  narrowest of three tries. It read them one after the other, so a reader
  thread preempted in between shifted the stream's clock by the wait (5.6 ms
  on a loaded CI runner). `test_acq_lsl` computes its expected values the same
  way.
- `lsl://` uses only the in-tree LSL implementation (`mne_lsl`): liblsl is no
  longer loaded, and the `library=` parameter is gone (it is now rejected as
  unknown; `mne::lslRuntime()` is removed). Channel labels, types and units
  come from the stream description and samples keep their native format
  (float32, double64, int32, int16), as they did with liblsl; the in-tree path
  used to name channels `EEG 1..n` and read everything as float. For this
  `mne_lsl` gained `stream_info::channels()` / `set_channels()` and
  `as_xml()` (the full info with `desc/channels/channel`, served to fullinfo
  requests), `stream_inlet::info()` (fetches the full info),
  `stream_inlet::pull_chunk_double()` and `pull_chunk<T>` for any numeric
  type, and `stream_outlet::push_chunk_double()` and `push_sample<T>` /
  `push_chunk<T>`. liblsl and pylsl remain only in `test_acq_lsl`, as the
  reference peer whose outlets the in-tree inlet must read.
- The `run_mna_graph` skill of MNE Analyze Studio reads its graph inputs from
  a JSON document it keeps alive; it iterated over the members of a temporary
  that compilers before C++23's lifetime extension (GCC 14) destroy first.
- The libraries build with MSVC, GCC 14 and Emscripten, checked locally now
  against MSVC's standard library and with GCC 14 (the oldest compiler the build
  guide supports). The JSON and CBOR containers export their functions one by
  one: exporting the whole class made MSVC instantiate their vectors while the
  element type was still incomplete. `std::filesystem::path` is converted to
  UTF-8 with `mne::utf8_from_path` where a `std::string` takes it (on Windows a
  path is not a `std::string`), headers include `FiffChInfo` where exported
  classes hold it, the BrainVision codepage pattern no longer needs
  `std::regex::multiline` (missing in MSVC's library), and `mne::executable_dir()`
  compiles for WebAssembly. A missing channel in `MNEInverseOperator` is logged
  without formatting a `std::vector`, which libstdc++ 14 cannot.
- `test_com` skips its two multicast cases on a host without a route for
  multicast, such as the macOS CI runners, instead of failing them.
- The sanitizer job (`scripts/ci/sanitizers`) builds again: it compiled
  `test_acq_devices` without `mne_acq_devices` and its stand-in SDK
  libraries. In that static build the test found no `streaming_board://` or
  `brainflow_file://` source, because it registered every scheme but those
  two by hand; it registers them and checks `register_device_schemes()`.
- The libraries build again with the compilers CI uses. Including Qt used to
  switch off MSVC's warnings about implicit narrowing (C4244), members without
  dll-interface (C4251, C4275), assignments in conditions (C4706) and forcing
  values to bool (C4800) and to set `/utf-8`; the Qt-free libraries lost both.
  The build now sets them itself, with `_CRT_SECURE_NO_WARNINGS`, and the
  public headers no longer narrow implicitly in inline code (`fiff_cov.h`,
  `linalg.h`, `simplex_algorithm.h`, `inv_rap_music.h`, the `float` defaults of
  `MNEForwardSolution::compute_orient_prior` and `MNEIcp`), so MSVC users of
  the SDK see no such warnings either. GCC 14 rejected a redundant `std::move`
  in the CBOR parser, a false `-Wmaybe-uninitialized` in the JSON stream
  parser and designated initializers of `MNEForwardSolution::ReadOptions` and
  `RtAveraging::Settings` that left members without defaults; WebAssembly
  rejected an unused helper of `hardware_addresses()`.
- `test_api_registry` reads the registry from the source tree it was
  configured from; it used to look for the repository above its own binary and
  failed when `BINARY_OUTPUT_DIRECTORY` was outside the repository.
- A build with tests but without applications (`-DBUILD_APPLICATIONS=OFF
  -DBUILD_TESTS=ON`) configures, builds and passes. 95 tests and
  examples linked `mne_events`, a library of MNE Analyze that none of them
  used; the tests built on MNE Analyze Studio and MNE Scan libraries, and
  `test_mne_surf2bem`, now skip themselves where those are not built, and
  `mne_doc_shots` is built only with the application cores it links. The test
  inventory check rejects a test (`TI015`) or example (`TI016`) that depends on
  an application or tool target without such a guard.
- The pull request, staging and main workflows run again: a step name with
  an unquoted `mne::` made the shared test workflow invalid YAML, so every
  run failed before starting a job. The quality unit tests reject such values.
- `mne_base` no longer falls back to Qt's private zlib (`Qt::ZlibPrivate`)
  when the system has no zlib. It downloads the zlib release pinned by
  `MNE_CPP_ZLIB_VERSION` (1.4, checked against `MNE_CPP_ZLIB_SHA256`) at
  configure time instead and links it statically, so configuring the libraries
  does not need Qt. Offline builds set `FETCHCONTENT_SOURCE_DIR_MNECPP_ZLIB`
  to an unpacked zlib source tree.
- `mne_utils` is gone. Its contents moved to `mne_base`, `mne_math`,
  `mne_channels`, `mne_pybridge`, `mne_report` and `mne_com`, so each library
  has one purpose and one namespace.
- `mne_base` is the new lowest library: logging, error results, byte streams,
  string and path helpers, the thread pool and the circular buffer. It depends
  on the standard library, zlib and Qt Core only, so `mne_lsl` logs through it.
- `MNE_LOG_LEVEL` now sets the initial `Log::level()`; it can be changed at
  run time with `Log::set_level`, and messages below it are not formatted.
- `MATHLIB::write_matrix` accepts any dense Eigen expression and writes the
  shortest text that reads back bit-identical (it used to round to 6 digits).
  `read_matrix` is locale-independent, reads `nan`/`inf`, and fails instead of
  crashing on ragged rows, non-numeric values or a fixed-size target that does
  not fit.
- `PythonTestHelper::eval` and `run_script` honour their timeout; it used to be
  ignored. `PythonRunner` installs venv packages with `python -m pip`, and
  `MATHLIB::read_matrix` also reads from a `std::istream`.
- `Report` embeds figures (`add_image`, `add_image_file` for PNG, JPEG and SVG)
  as base64 data, so the page stays one self-contained file, and links its
  sections from a table of contents. It escapes single quotes too, and its
  footer shows the generation time in UTC.
- `mne_channels` no longer depends on Qt; the standard_1005 table is compiled
  in. `LayoutLoader` also reads MNE-Python's `.elc` layout (plain coordinates,
  one label per line) and Windows line ends, and fails on malformed numbers
  instead of reading them as zero. `SelectionIO::write_mon` refuses group names
  that would leave the target directory.
- `ex_make_layout` honours `--mirrorY`; it used to mirror x twice.
- Device control in `mne_acq` (`acq/acq_control.h`): a device publishes the
  parameters, commands and status values it offers while open
  (`AcqDevice::controls()`), which programs `read()`, `write()` and
  `invoke()` by id, with type, range and device state checked first.
  Changes, status updates, alerts and finished commands arrive as
  `AcqControlEvent` records through `pollControlEvents()`, published by the
  backend from any thread without locks or allocation, and a change that
  applies to samples names the first sample it applies to. `synthetic://`
  offers `gain`, `inject_event`, `refresh_info` (a new stream description
  version) and the `samples` status.
- New `mne_acq_devices` library with the `brainamp://` source for Brain
  Products BrainAmp amplifiers. `brainamp://?simulate=1` emits the amplifier's
  int16 wire format from `synthetic://` signals, so it runs without hardware
  on every platform; on Windows the source talks to the BrainAmp driver when
  `BrainAmpIoCtl.h` is in `MNE_BRAINAMP_SDK_DIR`.
- The BrainAmp MNE Scan plugin reads through `brainamp://` and is built by
  default; its setup has a simulate switch, the channel count and the
  resolution. It delivers the amplifier's native 5 kHz instead of dropping
  samples to lower rates, which aliased, and the marker channel holds the
  marker codes instead of codes scaled as volts.
- `tmsi://` streams TMSi Refa and Porti amplifiers, with each channel's
  gain, offset and exponent as its scale, and measures their impedances;
  `tmsi://?simulate=1` simulates a Refa on every platform. On Windows it
  loads the TMSi SDK from the system directory only.
- The TMSI MNE Scan plugin reads through `tmsi://` and is built by default,
  with a simulate switch. The impedance view is updated from the GUI thread
  instead of the acquisition thread. Trigger codes are no longer rewritten
  for display (254, 253 and 252 became 4000, 8000 and 2000 in recordings),
  the placeholder projectors (all-ones vectors named PCA-v1 to PCA-v3) are
  gone, and the beep interval starts at 5 s instead of an uninitialised
  value. The samples always apply the channels' gain, offset and exponent;
  the options to skip them and the driver debug file are removed.
- `gusbamp://` streams up to four synchronised g.tec g.USBamp amplifiers,
  the master's inputs first, as floats in microvolts, with the master's
  trigger line and the electrode impedances. `gusbamp://?simulate=1`
  simulates them on every platform. On Windows it loads `gUSBamp.dll` from
  the system directory, so the g.tec header and import libraries are no
  longer needed to build.
- The GUSBAmp MNE Scan plugin reads through `gusbamp://` and is built by
  default (`WITH_GUSBAMP` is gone). It used to dereference a measurement
  info it never created on start, ignore the chosen sampling rate (its slot
  matched no Qt 6 signal) and print every sample. Its setup takes the serial
  numbers and inputs as text, the block size, the trigger line and a
  simulate switch; the empty project dialog is removed.
- `eego://` streams ANT Neuro eego amplifiers as doubles in volts: the
  referential and bipolar inputs, the trigger input as `STI 014` with its
  changes as events, and the sample counter. It measures impedances and
  checks the rate and input ranges against the amplifier. It loads the eego
  SDK library at run time on Windows and Linux (`sdk=` or
  `MNE_CPP_EEGO_SDK`), so building no longer needs the SDK; it is tested
  against a stand-in SDK library. `eego://?simulate=1` simulates an
  amplifier on every platform.
- The EEGoSports MNE Scan plugin reads through `eego://` and is built by
  default (`WITH_EEGO` is gone), with a simulate switch instead of the driver
  debug file. Its streams no longer carry an `EEG REF` channel that was
  always zero. The impedance view used to stay empty unless MNE Scan was
  started from `bin/` and the cap had no more electrodes than the amplifier
  inputs; it now finds its layouts next to the application and colours the
  measured electrodes.
- `neuromag://` reads Neuromag / MEGIN Vectorview and TRIUX systems from
  their acquisition software, as the Neuromag plugins of `mne_rt_server` and
  MNE Scan did until they were removed in 2020: it registers with the data
  server, takes the buffers from its shared memory or files, and starts and
  stops measurements through the collector. The channels and their
  calibration come from the measurement info, `STI 014` (or `STI101`)
  changes become events. `neuromag://?simulate=1` simulates a system on
  every platform.
- The Neuromag MNE Scan plugin is back and reads through `neuromag://`.
- `polhemus://` reads Polhemus Fastrak and FastSCAN digitizers on a serial
  port without Qt: the positions in metres and orientation quaternions of
  every station, and stylus button presses as events. It sets up the
  stations, the output list and the tracking hemisphere.
  `polhemus://?simulate=1` sweeps a stylus over a head. MNE Align reads its
  digitizer through it; `mne_acq_polhemus` no longer needs Qt SerialPort.
- Channel kinds `Position` and `Orientation` and the unit `Meter` describe
  tracked sensors.
- `ftbuffer://` reads FieldTrip buffer servers without Qt. With the Neuromag
  header of `neuromag2ft` it carries the acquisition's channels, measurement
  info and digitizer points, and `STI 014` changes as events; numeric events
  of the buffer become events. `ftbuffer://?simulate=1` relays a simulated
  Neuromag system. The FtBuffer MNE Scan plugin reads through it; it used to
  treat every sample as float32 whatever the buffer held, call every
  channel MEG without the Neuromag header, and hand on half a second of
  samples at a time.
- `mne_com` is a standard-C++ transports library without Qt: `TcpSocket`
  (connect, read and write with timeouts), `UdpSocket` (receive datagrams)
  and `SerialPort` (raw 8N1, `list_serial_ports()`), each reporting
  `std::error_code`s through `std::expected`. The acquisition devices use it
  for their network and serial connections; `ex_com` shows it.
- `brainflow://<board>` opens any board BrainFlow supports through an
  installed BrainFlow 5, whose `BoardController` library is loaded at run
  time (`library=`, `MNE_CPP_BRAINFLOW_LIB`, `pip install brainflow`, the
  system loader) and never built or shipped with MNE-CPP. The board is a
  BrainFlow id or name, the parameters are BrainFlow's input parameters.
  Every row is a channel in BrainFlow's units, the timestamp row is the
  clock and the marker row gives events. Other BrainFlow versions and
  boards this platform's BrainFlow lacks are refused with the reason.
- `streaming_board://<address>:<port>?board=<id or name>` receives what a
  BrainFlow session streams with `add_streamer`, without BrainFlow: the
  channels of `brainflow://`, the sender's markers as events. The layouts of
  the synthetic board, Cyton, Ganglion and Cyton + Daisy are built in,
  generated from BrainFlow by `scripts/acq/generate_brainflow_boards.py`;
  other boards take theirs from an installed BrainFlow. `simulate=1` streams
  a synthetic session to itself. `mne_acq stream --brainflow-multicast`
  sends with the same code and no longer needs Qt Network, and knows the
  same boards without BrainFlow.
- `brainflow_file:///<file>?board=<id or name>` replays a file BrainFlow
  wrote (its file streamer or `DataFilter.write_file`) without BrainFlow,
  paced (`realtime`, `speed`) or as fast as it is read, once or in a
  `loop`; `mne_acq record "brainflow_file://...&realtime=0" out.fif`
  converts it to FIFF with the markers as events.
- `COMLIB::UdpSocket::bind` joins a multicast group given as address,
  sharing its port, on the system's multicast interface and on that of the
  default route, which differ behind a VPN (BrainFlow's streamer sends on
  the first, its receiver listens on the second); `send_to` sends a
  datagram, to a group through the interface of the default route.
- `mne_com` serves too: `mne::TcpServer` listens and accepts connections
  with a timeout; `TcpSocket::read_some()` reads what has arrived,
  `is_open()` and `close()`; `UdpSocket::receive_from()` reports the
  sender (`mne::UdpDatagram`) so a service can answer it, `bind(..., shared)`
  lets several services share a port, `join_group()` joins further groups
  on a chosen interface, `set_multicast_interface()` picks the interface
  multicast leaves through, and datagrams can go to the broadcast address.
- Device URI schemes may contain `_`, as BrainFlow's `streaming_board://`
  does.
- The BrainFlow MNE Scan plugin is back as a client of `brainflow://`: it
  streams any BrainFlow board through an installed BrainFlow, set up by
  board name or id and BrainFlow's input parameters, with every row of the
  board as a channel. It no longer builds BrainFlow as a submodule.
- `mne_acq` offers the device schemes (`brainflow://`, `eego://`,
  `natus://`, ...); it registered only `synthetic://`, `fiff://` and
  `lsl://`. Its BrainFlow output finds BrainFlow through
  `MNE_CPP_BRAINFLOW_LIB`, formerly `MNE_CPP_BRAINFLOW`.
- `babymeg://` reads the acquisition server of a BabyMEG system without Qt:
  its channels, sensor positions, coil types and current gains from the
  command port, the data from the data port, and the trigger lines
  `TRG001` to `TRG008` as a trigger word in `DTRG01` and as events.
  `babymeg://?simulate=1` serves a synthetic session. The BabyMEG MNE Scan
  plugin reads through it and gains host, port and simulation settings; it
  used to connect only to `localhost` and to give every sensor the x
  coordinate as its whole position in the coil transform. Its SQUID control
  dialog, which no button opened, and the `Update Fiff Info` action are
  gone: the gains are read at every start.
- `natus://` receives a Natus Quantum amplifier from the UDP datagrams of
  its streaming machine without Qt, in volts, and stops with a message when
  the machine sends other channels than set. `natus://?simulate=1` streams a
  synthetic session. The Natus MNE Scan plugin reads through it; it used to
  start every block one sample late and drop the samples of the datagram
  that completed a block.
- `rtserver://` reads the active connector of an `mne_rt_server` without Qt:
  it selects the connector, the replayed file and the buffer size, takes the
  measurement info with projectors, compensators and digitizer points, and
  streams the calibrated buffers with `STI 014` changes as events; the end of
  the measurement ends the stream. `rtserver://?simulate=1` replays a
  simulated Neuromag recording. The FiffSimulator MNE Scan plugin reads
  through it; it used to wait a fixed second for its data connection, spin
  on an empty buffer, and need a separate connect step before starting.
- `acq_fiff` maps channel kinds, units and coordinate frames through one
  table shared with the devices that speak FIFF.
- The TMSI impedance view finds its cap layouts wherever MNE Scan was
  started; it stayed empty unless the working directory was `bin/`. With a
  cap of more electrodes than the amplifier measures it colours the measured
  ones instead of none.
- `ACQLIB::start_thread(policy, body)` starts a thread under an
  `AcqThreadPolicy` and returns it, or the error of a refused policy without
  running `body`. Every acquisition backend and the recorder start their
  threads with it.
- `Sphere::fit_sphere_simplex` returns a zero centre when the fit fails; it was
  uninitialised. `LayoutMaker` then projects around the origin, as MNE-C does.
- The export macros of `mne_bids`, `mne_com`, `mne_connectivity`, `mne_dsp`,
  `mne_fiff`, `mne_fs`, `mne_fwd`, `mne_inv`, `mne_lsl`, `mne_mna` and `mne_mne`
  no longer come from Qt. Libraries whose public headers still include Qt
  headers now link Qt publicly, so code using them no longer has to add Qt to
  its own link line.
- `Subject::detach` of an observer that is not attached no longer crashes, and
  observers may detach themselves while they are notified.
- `mne_rt_server` no longer aborts in debug builds when a client requests the
  measurement info: the connector interface `RTSERVER::IConnector` lives in the
  new `rtserver_connector` library, so the server and its plugins share one
  type. Its ids are `FIFF_SIMULATOR_CONNECTOR` and `DEFAULT_CONNECTOR`.
- `RtCmdClient` returns an empty reply when the server closes the connection or
  does not answer within 30 s; it used to spin forever.
- `FiffTag::toInt`, `toJulian`, `toIntMatrix` and `toFloatMatrix` are no longer
  inline.
- The `fiff` headers use `std::int32_t` and the other `<cstdint>` types instead
  of `qint32`, ..., and include no Qt header they do not need (Qt includes in
  public headers 56 -> 13); `fiff_long_t` is `std::int64_t`. Their
  `Q_DECLARE_METATYPE` declarations are gone: Qt 6 puts the types into a
  `QVariant` without them. Code that took `QString`, `QSharedPointer` or
  `QDebug` from a `fiff` header includes them itself.
- `FiffCov` and `FiffNamedMatrix` are plain value types; their `SDPtr`
  aliases and `QSharedData` base are gone. `FiffProj::data`,
  `FiffCtfComp::data`, `MNEForwardSolution::sol`/`sol_grad` and the
  `MNEInverseOperator` matrices and covariances (`eigen_leads`,
  `eigen_fields`, `noise_cov`, `source_cov`, `orient_prior`, `depth_prior`,
  `fmri_prior`) are members, not shared pointers, and compare by value.
  Migration: `x.sol->data` becomes `x.sol.data`, `*x.noise_cov.data()`
  becomes `x.noise_cov`, a null check `if (x.sol)` becomes
  `if (!x.sol.empty())`, and `FiffCov::SDPtr(new FiffCov(c))` becomes `c`.
  `SCMEASLIB::RealTimeFwdSolution` loses `setSol`, `getSol`, `setSolGrad`
  and `getSolGrad` (`setSolGrad` overwrote `sol`); use `getValue()->sol`.
- `FiffStream`, `FiffIO` and `DSPLIB::filterFile` take a
  `std::filesystem::path` or a `BASELIB::ByteStream` instead of a `QIODevice`;
  `fiff_stream.h` and `fiff_io.h` no longer include Qt. `FiffStream(path)`
  reads a file, `FiffStream(std::make_unique<BASELIB::MemoryByteStream>(bytes))`
  bytes in memory; `start_file` and `open_update` take a path (or bytes for
  `start_file`), `open()` takes no mode, and `byte_stream()` replaces
  `device()`. `FiffStream(QByteArray*, mode)` and `FiffStream::byteStream` are
  gone. `FiffIO` reads paths or bytes, its lists are `raw` and `evoked`
  (were `m_qlistRaw`, `m_qlistEvoked`), it is no longer a `QObject`, and
  `write(path, FIFFB_EVOKED)` now writes the evoked sets instead of nothing.
  Migration: `QFile f(name); FiffStream s(&f);` becomes
  `FiffStream s(name.toStdU16String());`, and
  `start_writing_raw(FiffStream::byteStream(file, QIODevice::WriteOnly), ...)`
  becomes `start_writing_raw(file.fileName().toStdU16String(), ...)`.
  `BASELIB::FileByteStream::resize` cuts or extends a file.
- `FIFFLIB::FiffFileSharer` is a plain class without Qt and no longer watches
  its directory: `copy_realtime_file(path)` shares a recording and returns
  the shared file (or an `Io` error), `new_files()` lists the files shared
  since the last call, and `clear()` empties the directory. A shared file
  only appears under its name once it is complete. Migration: replace
  `copyRealtimeFile` with `copy_realtime_file`, and `initWatcher()` plus the
  `newFileAtPath` signal with `clear()`, a `QFileSystemWatcher` on
  `directory()`, and `new_files()` in its `directoryChanged` handler.
  The fiff library no longer runs moc.
- `FiffTag` is no longer a `QByteArray`: it holds its payload in a
  `std::vector<std::byte>`, with `data()` (now `std::byte*`), `size()`
  (`std::size_t`), `empty()`, `resize()`, `bytes()` (a `std::span`) and
  `assign(span)`. `fiff_tag.h` no longer includes Qt. Migration:
  `tag.append(ptr, n)` becomes `tag.assign(std::as_bytes(std::span(values)))`,
  `tag.left(n)` becomes `tag.bytes().first(n)`, `*a == *b` becomes
  `std::ranges::equal(a->bytes(), b->bytes())`, `QString(tag.data())` becomes
  `QString::fromStdString(tag.toString())`, and `char* p = tag.data()` becomes
  `std::byte* p = tag.data()`.
- `FiffTag`, `FiffStream` and the FIFF record types use snake_case member
  names, without aliases. Migration on `FiffTag`: `getType` -> `data_type`,
  `isMatrix` -> `is_matrix`, `getMatrixCoding` -> `matrix_coding`,
  `getMatrixDimensions` -> `matrix_dimensions`, `getInfo` -> `info`, and every
  decoder `toXxx` -> `to_xxx` (`toInt` -> `to_int`, `toFloatMatrix` ->
  `to_float_matrix`, `toSparseFloatMatrix` -> `to_sparse_float_matrix`,
  `toFiffID` -> `to_fiff_id`, `toChInfo` -> `to_ch_info`, `toDauPack16` ->
  `to_dau_pack16`, ...). On `FiffStream`: `isOpen` -> `is_open`,
  `streamName` -> `stream_name`, `setLittleEndian`/`isLittleEndian` ->
  `set_little_endian`/`is_little_endian`, `copyProcessingHistory` ->
  `copy_processing_history`, `seekTo` -> `seek`, `seekToEnd` -> `seek_to_end`,
  `readRaw`/`writeRaw` -> `read_raw`/`write_raw`. `storageSize()` of `FiffTag`,
  `FiffId`, `FiffTime`, `FiffChInfo`, `FiffChPos`, `FiffDigPoint`,
  `FiffDirEntry` and `FiffDataRef` is `storage_size()`, and `FiffId::toString`
  is `to_string`.
- The average colour and activation maps of `AverageSelectionView`,
  `AverageLayoutView`, `ButterflyView`, `EvokedSetModel` and `AverageScene`
  (`getAverageColor`, `setAverageColor`, `newAverageColorMap`, ...) are
  `std::shared_ptr<QMap<...>>` instead of `QSharedPointer<QMap<...>>`.
  Migration: `QSharedPointer<QMap<QString, QColor>>::create()` becomes
  `std::make_shared<QMap<QString, QColor>>()`. The `mne` and `inv` headers
  no longer include `<QSharedPointer>`, `InvGuessData` takes its guess and
  surface files as `std::filesystem::path`, and `MNEClusterInfo` uses
  `std::int32_t` instead of `qint32`.
- `MNERawData::pick_data`, `pick_data_proj` and `pick_data_filt` fill an
  `Eigen::Ref<MNERawData::RowMajorMatrixXf>` (channels x samples) instead of a
  `float**` of caller-built row pointers plus a sample count; the number of
  samples is the number of columns. A buffer whose row count does not match
  the channel selection is refused with `FAIL` and a warning; it used to be
  written past its end. Migration: `std::vector<float*> rows{...};
  raw.pick_data(sel, first, ns, rows.data())` becomes
  `MNERawData::RowMajorMatrixXf values(nchan, ns); raw.pick_data(sel, first, values)`.
- `MNEMeasDataSet::getValuesFromChannelData` reads a row-major channels x
  samples Eigen matrix and writes an `Eigen::Ref<Eigen::VectorXf>` instead of
  taking `float** data, int nsamp, int nch` and a `float*` output. An output
  vector without one element per channel is refused with -1 and a warning.
  Migration: `getValuesFromChannelData(t, integ, rows, ns, nch, tmin, sfreq,
  abs, out)` becomes `getValuesFromChannelData(t, integ, values, tmin, sfreq,
  abs, out)`, with `values` a `MNERawData::RowMajorMatrixXf` and `out` an
  `Eigen::VectorXf(nch)`.
- `MNEMeasDataSet::getValuesAtTime(time, integ, use_abs, value)` writes an
  `Eigen::Ref<Eigen::VectorXf>` instead of taking a channel count and a
  `float*`; it picks the first `value.size()` channels and refuses a vector
  longer than the data has channels, which used to be read past its end.
  Migration: `ds.getValuesAtTime(t, integ, nch, abs, out.data())` becomes
  `ds.getValuesAtTime(t, integ, abs, out)` with `out` an
  `Eigen::VectorXf(nch)`.
- `MNERawData::nbad`, `MNEMeasData::nbad` and `MNECovMatrix::nbad` are gone;
  they duplicated the size of `badlist` / `bads`. Migration: use
  `badlist.size()` (`bads.size()` for the covariance). Raw files read their
  bad channels through `FiffStream::read_bad_channels` on the already open
  stream instead of reopening the file, and the bad-channel names are logged
  on the same line as the count instead of one log message per name.
- `MNERawInfo::ndir` is gone; it always equalled `rawDir.size()`. Migration:
  use `rawDir.size()`.
- `MNEProjOp` and `MNEProjItem` take channel lists as `std::span` instead of a
  list plus a count, `MNEProjItem::affect` returns `bool`, and `MNEProjOp`
  loses `nitems` and `nch` (use `items.size()` and `names.size()`).
  `assign_channels` returns nothing. `makeProjection(files, chs, nch, result)`
  is `make_projection(files, chs)` and returns
  `BASELIB::Expected<std::unique_ptr<MNEProjOp>>`: nullptr when the projection
  would not affect the channels, an `Io` error for an unreadable file.
  Migration: `op.affect(list, n)` becomes `op.affect(list)` (or
  `op.affect(std::span(list).first(n))` for a prefix);
  `MNEProjOp::makeProjection(files, chs, nch, proj)` becomes
  `auto proj = MNEProjOp::make_projection(files, chs)`.
- `MNEProjOp::add_item` takes the vectors by reference and an optional
  `is_active` (was `add_item_active`); `combine` takes a `const MNEProjOp&` and
  returns nothing; `make_proj(bad = {})` replaces `make_proj_bad`;
  `make_proj`, `project_vector`, `project_dvector` and `apply_cov` return
  `bool` instead of `OK`/`FAIL` and are `[[nodiscard]]`; the projections are
  `const`, and `apply_cov` takes the covariance by reference. Migration:
  `op.add_item_active(&m, kind, desc, a)` becomes `op.add_item(m, kind, desc, a)`,
  `op.combine(&other)` becomes `op.combine(other)`,
  `op.make_proj_bad(bad) == FAIL` becomes `!op.make_proj(bad)`.
- `MNEProjItem::vecs` is an `MNENamedMatrix` value instead of a
  `std::unique_ptr`, and `MNEProjItem::nvec` is the function `nvec()`
  (`vecs.nrow`) instead of a second copy of the count. `MNENamedMatrix` and
  `MNEProjItem` no longer declare their own copy constructors and destructors,
  so both are cheap to move. Migration: `item.vecs->data` becomes
  `item.vecs.data`, `item.nvec` becomes `item.nvec()`.
- `MNENamedVector::pick(names, require_all, res)` takes the names as a
  `std::span` instead of a list plus a count and returns `[[nodiscard]] bool`
  instead of `OK`/`FAIL`. A result vector that does not hold one value per
  name is refused; it used to be written past its end. `MNENamedVector::nvec`
  is gone (use `names.size()`), and the header no longer includes
  `QStringList`. Migration: `vec.pick(names, n, all, res) == FAIL` becomes
  `!vec.pick(names, all, res)`.
- `MNECovMatrix::pick_chs_omit(names, omit_meg_eeg, chs = {})` takes the names
  and channel infos as `std::span` instead of a list plus a count, and
  `omit_meg_eeg` as `bool`. Fewer channel infos than names are refused; they
  used to be read past their end. `mne_cov_matrix.h` no longer includes
  `QStringList`. Migration: `cov.pick_chs_omit(names, n, omit, chs)` becomes
  `cov.pick_chs_omit(names, omit, chs)` (or `std::span(names).first(n)`).
- `MNENamedMatrix::nrow` and `ncol` are the functions `nrow()` and `ncol()`
  (the dimensions of `data`) instead of separately stored counts that could
  disagree with it. `build(rowlist, collist, data)` drops the count
  arguments and refuses name lists that do not fit the data, and
  `pick(rows, cols)` takes the names as `std::span` without counts.
  `MNEProjOp::read` refuses a projection item whose stored vectors do not
  match its vector and channel counts. Migration: `m.nrow` becomes
  `m.nrow()`, `MNENamedMatrix::build(r, c, rows, cols, data)` becomes
  `MNENamedMatrix::build(rows, cols, data)`, `m.pick(rows, nr, cols, nc)`
  becomes `m.pick(rows, cols)`.
- `MNESparseNamedMatrix::nrow` and `ncol` are the functions `nrow()` and
  `ncol()` (the dimensions of `data`), and `data` is a `FiffSparseMatrix`
  value instead of a `std::unique_ptr`, so copying an `MNEDeriv` copies its
  derivation matrix without hand-written cloning. `mne_sparse_named_matrix.h`
  no longer includes `QStringList`. Migration: `m.nrow` becomes `m.nrow()`,
  `m.data->eigen()` becomes `m.data.eigen()`; drop assignments to the counts.
- `InvSourceEstimateIO::writeCsv`, `readCsv` and `writeMatrix` are
  `write_csv`, `read_csv` and `write_matrix`, take a `std::filesystem::path`
  and use the standard library instead of `QFile`/`QTextStream`. Values are
  written in the shortest form that reads back exactly (they were rounded to
  9 digits), and `read_csv` returns an empty estimate for a row that is not
  a number or lacks values (such rows used to be skipped silently, shifting
  the time axis). The write functions are `[[nodiscard]]`. Migration:
  `writeCsv(stc, qpath)` becomes `write_csv(stc, qpath.toStdU16String())`.
- `InvEcdSet::read_dipoles_dip` reads `.dip` files written by
  `mne.Dipole.save`: it read none of their dipoles because they carry extra
  `khi^2` and `free` columns, which are now read into `khi2` and `nfree`.
  Data lines need no leading blank, and a malformed line yields an empty
  set instead of being skipped. `save_dipoles_bdip` writes the record's
  `end` time and zeros in the error-analysis fields it does not compute
  (they held whatever was in memory) and is `const`. The three functions
  take a `std::filesystem::path` and use the standard library instead of
  `QFile`/`QTextStream`; the save functions are `[[nodiscard]]`.
  Migration: pass `qpath.toStdU16String()`.
- `MNEEventList` methods are snake_case: `read_fif`, `read_text`,
  `write_fif` (now `[[nodiscard]]`), `select_onsets`, `max_onset`,
  `to_matrix` and `from_matrix`. The text reader uses the standard library
  instead of `QFile`/`QTextStream`, and the header no longer includes
  `QString`. Migration: rename the calls, for example
  `MNEEventList::readFif(path)` becomes `MNEEventList::read_fif(path)`.
- `MNESourceSpace::read_label(path)` returns
  `std::optional<Eigen::VectorXi>` instead of filling an out-parameter and
  returning a status code, and reads with `std::ifstream` instead of
  `QFile`/`QTextStream`. A label file with a negative point count is
  refused; it used to resize the vertex vector to a negative size.
  Migration: `read_label(path, sel) == 0` becomes
  `const auto sel = read_label(path); if (sel) ...`.
- `MNEDescriptionParser::parseAverageFile(path, desc)` and
  `parseCovarianceFile(path, desc)` are `read_average(path)` and
  `read_covariance(path)` and return the description as a
  `std::optional`; `mne_description_parser.h` no longer includes
  `QString`/`QTextStream`. Migration: `AverageDescription d;
  parseAverageFile(path, d)` becomes
  `const auto d = MNEDescriptionParser::read_average(path); if (d) ...`.
- `mne_bem.h` no longer declares Qt metatypes for `MNEBem` and
  `std::shared_ptr<MNEBem>`, and `mne_surface_or_volume.h` no longer
  includes `QStringList` and `QDebug` (both unused). Migration: code that
  stores an `MNEBem` in a `QVariant` declares the metatype itself.
- The fs library headers are Qt-free: `qint16`/`qint32`/`qint64` are
  `std::int16_t`/`std::int32_t`/`std::int64_t`, and `fs_label.h` no longer
  declares a Qt metatype for `FsLabel`. `mne_inverse_operator.h` no longer
  declares metatypes for `MNEInverseOperator` and its `std::shared_ptr`;
  `rt_inv_op.h`, whose queued signals carry it, declares the by-value one.
  Unused `QSharedPointer` includes are gone from the fs, fwd and
  `filter_thread_arg.h` headers. Migration: code that put these types in a
  `QVariant` declares the metatype itself.
- New `MATHLIB::KMeans::seed(value)` makes a clustering reproducible, like
  scikit-learn's `random_state`; the preliminary clustering of the
  `"cluster"` start draws its seed from the object's engine instead of its
  own random device. `test_math_kmeans` failed in about 1 of 200 runs
  because every replicate landed in a local minimum.
- `filterkernel.h`, `inv_ecd_set.h`, `inv_hpi_fit.h` and `rt_inv_op.h` no
  longer declare Qt metatypes for `FilterKernel`, `FilterParameter`,
  `InvEcdSet`, `HpiFitResult` and `MNEInverseOperator`: Qt 6 registers a
  type on its first use in a `QVariant`, `qRegisterMetaType<T>()` or a
  signal, so the declarations were redundant. Migration: none.
- `MNEClusterInfo::write(path)` returns `[[nodiscard]] bool` (it returned
  nothing, so a file that could not be written went unnoticed) and writes with
  `std::ofstream` instead of `QFile`/`QTextStream`. Migration: check the
  result, e.g. `if (!info.write(path)) ...`.
- `MNELayout` and `MNELayoutPort` use Eigen geometry instead of Qt:
  `channelPositions()` maps to `Eigen::Vector2d`, `portAt` takes an
  `Eigen::Vector2d`, `confine` takes an `Eigen::AlignedBox2d`, and
  `extent()`, `visibleArea()` and `MNELayoutPort::rect()` return
  `Eigen::AlignedBox2d` (min/max corners) instead of `QRectF`. `confine` no
  longer swaps corners given in the wrong order: pass a box whose `min()` is
  below its `max()`. Migration: `QPointF(x, y)` becomes
  `Eigen::Vector2d(x, y)`, `QRectF(x, y, w, h)` becomes
  `Eigen::AlignedBox2d(Eigen::Vector2d(x, y), Eigen::Vector2d(x + w, y + h))`,
  `rect.left()`/`top()` become `box.min().x()`/`min().y()`.
- `MNEEpochDataList` derives from `std::vector` instead of `QList`.
  Migration: `isEmpty()` becomes `empty()`, `first()`/`last()` become
  `front()`/`back()`, `append(epoch)` and `list << epoch` become
  `push_back(epoch)`; `size()` is unsigned, so compare it with
  `std::ssize(list)` in signed loops.
- The mne library no longer uses Qt outside the remaining public headers:
  forward- and inverse-operator clustering and the HPI coil fit run on
  `BASELIB::ThreadPool` instead of `QtConcurrent`, the artifact check of
  `MNEEpochDataList` loops over the channels directly, and the remaining
  `QString`, `QStringList`, `QFile`, `Q_UNUSED` and `Q_ASSERT` uses in the
  CTF compensation, covariance, forward-solution and raw-data code are std
  code. Headers that included `QString`, `QList` or `QDebug` without using
  them no longer do. Migration: code that relied on those headers for its
  own `QDebug`/`QString` includes them itself.
- The remaining `Q_DECLARE_METATYPE` declarations in the connectivity and
  disp libraries, MNE Scan (`scMeas`, LSL adapter), MNE Analyze
  (`anShared`), MNE Browse, the MNE Analyze Studio browser extension and
  `mne_rt_server` are gone, and so is `anShared/Utils/metatypes.h`, which held
  only such declarations. Qt 6 registers a type on first use; the named
  `qRegisterMetaType` calls that `QMetaType::fromName` relies on stay.
  Migration: code that included `anShared/Utils/metatypes.h` for its
  transitive includes includes the headers it uses.
- New `BASELIB::executable_dir()` (directory of the running executable,
  without a `QCoreApplication`) and `BASELIB::resource_path(relative)`
  (`<executable_dir>/../resources/<relative>`, else relative to the working
  directory). The forward, inverse, HPI, disp3D and test-helper code finds
  `coil_def.dat`, `icos.fif` and the test data with them instead of
  `QCoreApplication::applicationDirPath()`, so these lookups also work in
  programs without a Qt application object.
- New `BASELIB::read_text_file(path)` and `write_text_file(path, text)` read
  and replace a whole file byte for byte (no line-ending or encoding
  conversion) and report failures as `BASELIB::Expected`.
- The BIDS library's sidecar API uses std types instead of Qt, and its JSON
  goes through `BASELIB::JsonDocument`. Migration:
  - `BIDSPath` takes and returns `std::string` labels and a
    `std::filesystem::path` root; `file_path()` and `directory()` return
    `std::filesystem::path`; methods are snake_case (`set_subject`,
    `file_path`, `with_suffix`, `channels_tsv_path`, `electrodes_tsv_path`,
    `events_tsv_path`, `coordsystem_json_path`, `sidecar_json_path`,
    `is_valid_entity_value`); `match()` returns `std::vector<BIDSPath>`.
  - `BidsChannel`, `BidsElectrode`, `BidsEvent`, `BidsDatasetDescription`
    and `BidsCoordinateSystem` hold `std::string` fields; their
    `read_tsv`/`write_tsv`, `read`/`write` and `read_json`/`write_json`
    take `std::filesystem::path` and `std::vector`/`std::span`;
    `to_fiff_dig_points` and `to_fiff_coord_trans` replace the camelCase
    names.
  - `BidsTsv::read_tsv`/`write_tsv` work on `std::vector<BidsTsvRow>` with
    `BidsTsvRow = std::map<std::string, std::string>`; `BidsTsv::value(row,
    column, fallback)` looks a cell up.
  - `BidsRawData` events and electrodes are `std::vector`, `eventIdMap` is a
    `std::map<std::string, int>`, the metadata fields are `std::string`,
    `write` takes the source as `std::filesystem::path`, and `isValid`,
    `setValid`, `createReader` are `is_valid`, `set_valid`, `create_reader`.
  - `bids_const.h` holds `constexpr` tables (`BIDS_DATATYPES`,
    `FIFF_KIND_TO_BIDS_TYPE`, `BIDS_COORD_TO_FIFF_FRAME`, ...) looked up
    with `bids_lookup(table, key)` instead of functions returning `QMap`
    and `QStringList`.
- `EDFReader` and `BrainVisionReader` set `FiffInfo::meas_date` like
  MNE-Python; it was left unset. EDF takes the EDF+ "Startdate" field, else
  the two-digit header year (85-99 is 1985-1999, 00-84 is 2000-2084; the old
  parser put every year in 2000-2099 and never used it). BrainVision takes the
  first dated "New Segment" marker, to the microsecond.
  `BrainVisionMarker::date` is a
  `std::optional<std::chrono::sys_time<std::chrono::microseconds>>` instead of
  a `QDateTime`, empty for the all-zero placeholder. New
  `FiffInfo::set_meas_date(time)` sets the date from a UTC time point.
- `DISP3DLIB::SliceOrientation` is removed; `SliceObject` takes the
  `MRILIB::SliceOrientation` that `MriSlicer` already returns (same
  enumerators and values), so no conversion is needed between them.
  Migration: replace `DISP3DLIB::SliceOrientation` with
  `MRILIB::SliceOrientation`.
- New `BASELIB::CborValue`, `CborArray`, `CborMap` and `CborParseError`
  (`base/cbor.h`): CBOR (RFC 8949) values in standard C++, the counterpart
  of Qt's `QCborValue`, `QCborArray` and `QCborMap`, with the same encoded
  bytes and JSON conversion. Decoding rejects malformed input, trailing
  bytes and nesting deeper than 1024 without throwing.
- New `mne::Sha256` (`base/sha256.h`) and `mne::to_hex()`
  (`base/string_utils.h`): SHA-256 digests in one call or incrementally,
  the counterpart of `QCryptographicHash::Sha256` and `QByteArray::toHex()`.
  The MNA graph executor fingerprints node inputs with it.
- New `mne::write_file_bytes()` (`base/byte_stream.h`) writes a whole file
  from bytes, the counterpart of `read_file_bytes()`.
- New `mne::split_whitespace()` (`base/string_utils.h`) splits a line of a
  whitespace-separated table into views of its fields, the counterpart of
  `QString::split(QRegularExpression("\\s+"), Qt::SkipEmptyParts)`.
- New `JsonArray::from_string_list()` / `to_string_list()`,
  `CborArray::from_string_list()` and `JsonObject::without(keys)`, the
  counterparts of `QJsonArray::fromStringList()` and
  `QCborArray::fromStringList()`; `CborMap::to_json_object()` takes keys to
  leave out, so large byte strings need not become base64 text.
- New `mne::to_base64()` and `mne::from_base64()` (`base/string_utils.h`):
  base64 in the standard or URL-safe alphabet, the counterpart of
  `QByteArray::toBase64()` / `fromBase64()`; decoding rejects invalid text
  instead of skipping characters.
- New `mne::format_iso8601()` and `mne::parse_iso8601()` with `mne::UtcTime`
  (`base/string_utils.h`): ISO 8601 text for UTC times with millisecond
  resolution, the counterpart of `QDateTime::toString(Qt::ISODate)` /
  `Qt::ISODateWithMs` and `QDateTime::fromString()`. Text without an offset
  is read as UTC.
- New `mne::run_process()` with `ProcessOptions` and `ProcessResult`
  (`base/process.h`): runs a program without a shell, with working
  directory, extra environment, timeout and a live output callback, the
  counterpart of `QProcess`; `host_name()`, `os_pretty_name()` and
  `cpu_architecture()` replace `QSysInfo`. `PythonRunner` and the MNA graph
  executor use them, so `mne_pybridge` no longer links Qt.
- New `mne::hardware_addresses()` (`base/process.h`): the 48-bit hardware
  addresses (`mne::HardwareAddress`) of the running network interfaces that
  are neither loopback nor point-to-point, in interface order, the
  counterpart of `QNetworkInterface::hardwareAddress()`.
- New `mne::parse_xml()` and `mne::read_xml_file()` with `mne::XmlElement`
  (`base/xml.h`): reads a well-formed XML document into an element tree
  (name, attributes, text, children), the counterpart of
  `QXmlStreamReader`. Malformed documents give a `Format` error with the
  line; UTF-8 and ISO-8859-1 documents are read.
- MNA script and IPC nodes whose program cannot be started report exit code
  -1 and the reason in `stderr`; they reported a made-up exit code before.
- The `mna` library uses standard C++ only and links just `mne_base`; the
  `.mna` and `.mnx` files are unchanged. Migration:
  - All structs hold `std::string`, `std::vector`, `mne::StringList`,
    `std::optional<mne::UtcTime>` and `std::vector<std::byte>` instead of
    Qt types. Node attributes, port extras and provenance attributes are
    `mne::JsonObject`; runtime port values are `mne::MnaValues`
    (`std::map<std::string, std::any>`, read with `std::any_cast`).
  - Fields and methods are snake_case: `opType` → `op_type`, `execMode` →
    `exec_mode`, `dataKind` → `data_kind`, `sizeBytes` → `size_bytes`,
    `toJson`/`fromJson`/`toCbor`/`fromCbor` → `to_json`/`from_json`/
    `to_cbor`/`from_cbor`, `addNode` → `add_node`, `paramTree` →
    `param_tree`, `setParam` → `set_param`, `evaluateExpression` →
    `evaluate_expression`, and so on.
  - `mnaFileRoleToString`, `mnaFileRoleFromString`, `mnaDataKindToString`,
    `mnaDataKindFromString` are `to_string`, `file_role_from_string` and
    `data_kind_from_string`; `data_kind_key` gives the snake_case file
    spelling.
  - `MnaIO::read`/`write` and `MnaProject::read`/`write` take a
    `std::filesystem::path` and return `mne::Expected`, so the reason of a
    failure reaches the caller. New `MnaIO::to_mnx()` encodes a project in
    memory, and `MnaFileRef::embed()`/`extract()` move file content in and
    out of a project.
  - `MnaOpSchema::validate()` returns the list of problems instead of
    filling an out-parameter; attribute types are `MnaAttrType`.
    `MnaGraph::validate()`, `topological_sort()`, `upstream_nodes()`,
    `downstream_nodes()` and `dirty_nodes()` return `StringList`;
    `MnaGraph::node()` throws `std::out_of_range` for an unknown id and
    `find_node()` returns a pointer or `nullptr`.
  - `MnaOpRegistry::OpFunc` is
    `std::function<MnaValues(const MnaValues&, const JsonObject&)>`.
    `MnaGraphExecutor::set_progress_callback()` takes a plain
    `std::function`; the stream plugin factory returns
    `std::shared_ptr<void>`.
  - `MnaProvenance::qtVersion` is removed; `compiler_info` and `os_info`
    identify the build.
- `mne_inverse_pipeline` exits with code 1 when a node aborts the pipeline,
  and the MNE Studio MNA skills, MNE Inspect, MNE Browse and
  `mne_mna_bids_converter` report why a project could not be read or
  written.
- `RtNoise` is a synchronous class with no Qt; callers that want a worker
  thread run it on theirs. It accepts blocks of any length: before, every
  block had to be as long as the first one, and a block of another length
  was written past the end of its buffer. Migration:
  `RtNoise(nfft, std::shared_ptr<FiffInfo>, blocks)` →
  `RtNoise(nfft, sfreq, blocks)`; `append()` plus the `SpecCalculated`
  signal → `estimate_spectrum(block)`, which returns
  `std::optional<Eigen::MatrixXd>`; `start`, `stop`, `wait`, `isRunning` and
  `RtNoiseWorker` are removed.
- `RtHpi` (`dsp/rt/rt_hpi.h`, was `rt_hpis.h`) tracks the head position of a
  stream with no Qt: it keeps the newest `window` samples, fits the HPI coils
  every `hop` new samples, so the position is current to within one hop
  instead of one window, and starts each fit from the last accepted position.
  A fit with a mean coil error above `max_error` is returned but neither moves
  the head nor seeds the next fit; with fewer than three coil frequencies
  nothing is fitted. The MNE Scan HPI plugin uses it and fits every half
  window. Migration: `RtHpi(sensorSet)` plus `setModelParameters`,
  `setProjectionMatrix`, `setHpiDigitizer`, `append(data)` and the
  `newHpiFitResultAvailable` signal → `RtHpi(info, {.coil_frequencies,
  .window, .hop, .max_error})` and `update(data)`, which returns the fit when
  one is due; `accepted(fit)`, `head_transform()`, `set_projectors`,
  `set_coil_frequencies`, `order_coil_frequencies`, `set_info`, `set_window`,
  `set_max_error` and `reset()`; `restart`, `stop` and `RtHpiWorker` are
  removed.
- `EogRegression::apply()` finds the fitted channels by name in the info of
  the data, like `mne.preprocessing.EOGRegression.apply`. Before, it reused
  the row numbers of the fitted data and ignored the info, so data with
  another channel order was corrected on the wrong channels. Data missing a
  fitted channel is left unchanged with a warning.
- `filterChpi(data, info, sfreq)` removes the HPI coil frequencies of the
  measurement info (`FiffInfo::hpi_coil_freqs`); before, it only warned.
- The dsp library no longer uses or links Qt. Programs that got Qt through
  `mne_dsp` link it themselves. `Spectrogram::make_spectrogram` computes
  the windows on the global thread pool, each straight into its column,
  instead of one full-size matrix per task summed up afterwards; a window
  size of 0 is the default. Migration: `Spectrogram::makeSpectrogram` →
  `Spectrogram::make_spectrogram`; `SpectogramInputData`, `FilterObject` and
  `filterChannel` are removed; `computeFilteredAverage` takes the event type
  as `int`.
- Coherence and imaginary coherence list the edges of the network row by row,
  like every other metric. Before, the rows were added by parallel tasks in
  whatever order they finished, so `Network::edges()[k]` was a different
  channel pair from run to run.
- The connectivity library no longer uses or links Qt itself. The metrics run
  their trials on the global `mne::ThreadPool` instead of `QtConcurrent`, and
  correlation sums the trials in trial order, so it gives the same result on
  every run. `Network` and `ConnectivitySettings` are no longer registered
  with `QMetaType`. Programs that got Qt through `mne_connectivity` link it
  themselves. Migration: the protected per-trial helpers of the metrics
  (`PhaseLagIndex::compute` and the like) take a `std::mutex&` instead of a
  `QMutex&`; call `qRegisterMetaType<mne::Network>()` yourself before sending
  a `Network` through a queued Qt connection.
- The forward library no longer uses or links Qt itself. The MEG and EEG
  field computations run their source-space slices on the global
  `mne::ThreadPool` instead of `QtConcurrent`. Migration: `fwd_eeg_sphere_model.h`
  and `fwd_eeg_sphere_layer.h` no longer include `<QDebug>`; programs that got
  Qt through `mne_fwd` link it themselves.
- The mne library no longer uses or links Qt itself; its public headers spell
  `qint32` as `std::int32_t` (the same type, so callers compile unchanged).
  Programs that got Qt through `mne_mne` link it themselves.
- The fiff library no longer uses or links Qt, so `mne_fiff`, `mne_mne`,
  `mne_fwd`, `mne_dsp` and `mne_connectivity` no longer link Qt
  themselves (`mne_base` still does, for its logging). The machine
  identifier of a new file ID comes from `mne::hardware_addresses()` and
  skips interfaces without a hardware address instead of giving up on the
  first one; BrainVision CapTrak and EGI montages are read with
  `mne::read_xml_file()`; the `FiffExplain` table is compiled in by CMake
  instead of a Qt resource. Migration: `FiffDirNode::print(QTextStream&)` →
  `print(std::ostream&)`; `FiffEvokedFile::size()` returns `std::size_t`;
  `FiffCov` and `FiffEvokedSet` are no longer registered with `QMetaType`
  (Qt 6 registers types of signal arguments itself); programs that got Qt
  through `mne_fiff` link it themselves.
- The session state of `PolhemusCoregistration` keeps the probe station, and
  restoring a state without stations keeps the current assignment. Before,
  the probe station was lost and a missing entry swapped pen and tracker
  (station 1 became the tracker).
- The session state of `PolhemusCoregistration` stores each fiducial under
  its own name. Before, the LPA was stored as `NAS`, the NAS as `RPA` and the
  RPA as `CZ`; a state written by hand or another tool was read shifted.
- The Polhemus coregistration library (`mne_acq_polhemus`) no longer uses or
  links Qt. `AcquiredPoints` and `PolhemusCoregistration` are plain classes
  with snake_case members; events arrive through callbacks, and the session
  state is a `mne::JsonObject`. MNE Align wraps the points in a QObject
  (`AlignPoints`) that turns the change callback into its `pointsChanged`
  signal. Migration:
  - `AcquiredPoints`: `pointsChanged` signal → `set_on_changed(callback)`;
    `emitChanged()` → `notify_changed()`; `points()` returns
    `std::vector<DigitizedPoint>`; `hasFiducial`, `countOf`, `undoLast`,
    `removeFiducial`, `hasAllFiducials`, `setPosition`, `setTwinFiducial`,
    `clearTwinFiducial`, `hasTwinFiducial`, `twinFiducial`,
    `hasAllTwinFiducials` → `has_fiducial`, `count_of`, `undo_last`, … .
    `DigitizedPoint::label` is `std::string`, `identNumber` → `ident_number`.
  - `PolhemusCoregistration`: no `QObject` parent; every member is
    snake_case (`computeRegistration()` → `compute_registration()`,
    `penPosition()` → `pen_position()`, …); `acquiredPoints()` returning a
    pointer → `acquired_points()` returning a reference; the nine signals →
    the members of `PolhemusCoregistration::Listener`, set with
    `set_listener()`; `saveSessionState(QSettings&, prefix)` /
    `restoreSessionState(QSettings&, prefix)` → `session_state()` /
    `restore_session_state(const JsonObject&)`;
    `OpticalCalibSample::trackerPos`, `trackerOri`, `focusPoint` →
    `tracker_pos`, `tracker_ori`, `focus_point`.
- The acquisition FIFF bridge (`mne_acq_fiff`) no longer uses or links Qt:
  the recorder and the FIFF replay source write and read with standard C++
  streams and buffers.
- The MRI, iEEG and statistics libraries (`mne_mri`, `mne_ieeg`, `mne_sts`) no
  longer link Qt.
- `mne_base` no longer links Qt, so the libraries that do not use Qt
  themselves load without it. Without a sink, `mne::Log` prints every
  message itself (results to stdout, diagnostics to stderr, the log file
  and colours as before) instead of handing it to Qt's message handler; the
  new `Log::print()` prints a message that way. There is no bridge to Qt's
  message handler: the applications, tools, examples and tests log through
  `mne::Log` instead of `qDebug()` / `qInfo()` / `qWarning()` /
  `qCritical()`, and MNE Analyze and MNE Browse install a `Log` sink that
  writes to their log windows. Migration: `Log::install_qt_message_handler()`
  is gone, log with `mne::Log`; code that read library messages through Qt
  (`QTest::ignoreMessage`, `QTest::failOnWarning`, a custom
  `qInstallMessageHandler`) installs a sink with `mne::Log::set_sink()` or
  records them with `mne::LogCapture` (the test frames use
  `ScopedLogCapture` from `src/testframes/common/scoped_log_capture.h`).
- The LSL library (`mne_lsl`) and the `lsl://` source (`mne_acq_lsl`) no
  longer use or link Qt. Outlets, inlets, discovery and time correction run
  on the sockets of `mne_com`, stream infos are parsed with
  `mne::parse_xml()`, and new streams get their UUID and host name without
  Qt; the wire protocol is unchanged and still interoperates with liblsl.
  `mne_lsl` moves from the core to the support layer of the dependency
  policy, above `mne_com`.
  With this, no library outside `disp` and `disp3D` links Qt.
- `RtAveraging` is a synchronous streaming average with no Qt: `update(block)`
  cuts an epoch at every onset of the stim channel and returns the event
  codes whose moving mean of the newest `averages` epochs changed, read from
  `evoked()`. The running sum costs one epoch per update instead of summing
  the whole window, and is summed up afresh once per window so it does not
  drift. Fixes of the streamed result: a second onset of a code before its
  epoch was complete, an onset on the first sample of a block, and a pulse
  longer than 100 samples were lost or counted twice; an epoch whose
  pre-stimulus part lies before the first sample was averaged with zeros
  and is now dropped, like `mne.Epochs`. Another window length, baseline or
  rejection updates the evoked responses at once. Event codes are integers.
  The MNE Scan averaging plugin runs it on its processing thread, and each
  evoked set travels with the codes that changed it. Migration:
  `RtAveraging(numAverages, pre, post, baselineFromMs, baselineToMs,
  triggerRow, info)` plus `append`, the `evokedStim` signal and the setters →
  `RtAveraging(info, {.pre_stim, .post_stim, .trigger_channel,
  .trigger_threshold, .averages, .baseline, .reject})`, `update(block)`,
  `evoked()`, `settings()`, `set_settings()` and `reset()`; the baseline is
  in seconds and `reject` holds peak-to-peak limits without an `Active` key;
  `restart`, `stop` and `RtAveragingWorker` are removed.
- The MNE Scan connectivity plugin no longer warns on every network of a
  time-domain metric such as correlation, which has no frequency band to
  select, and computes each trial with the settings current when it arrives.
- `HpiFitResult` starts without head movement: `bIsLargeHeadMovement`,
  `fHeadMovementDistance` and `fHeadMovementAngle` were left uninitialized.
- `RtInvOp` is a synchronous streaming inverse with no Qt: it computes the
  MNE, dSPM or sLORETA source estimates of each block and follows each new
  noise covariance. It does the forward-only work once per channel set and
  applies the kernel in factored form, so a new covariance costs one
  channels × channels eigendecomposition plus the noise normalization
  instead of the SVD of the whitened gain: 130 ms instead of 3.1 s for the
  305 sample MEG channels and 7928 sources. The estimates equal those of
  `MNEInverseOperator` and `InvMinimumNorm` to 1e-6. The MNE Scan source
  localization plugin uses it on its processing thread, so the newest
  covariance applies to the next block, and localizes averages at SNR 3
  (lambda2 1/9) as `mne.minimum_norm.apply_inverse` does instead of SNR 1.
  Its CMNE path now gets the gain, covariances and data of the operator's
  channels; it got the gain of every forward channel and a source covariance
  vector where a matrix is expected. Migration: `RtInvOp(info, fwd)` →
  `RtInvOp(info, fwd, {.method = ..., .lambda2 = ...})`; `append(cov)` and
  the `invOperatorCalculated` signal → `update(cov)`, then `apply(data, nave)`
  for the source estimates; `restart`, `stop`, `setFwdSolution` (construct a
  new one), `RtInvOpWorker` and `RtInvOpInput` are removed.
- `MNEForwardSolution::pick_channels` and `pick_types` left the names of every
  channel in `info.ch_names`, so code matching channels by name against a
  picked forward solution still found the dropped ones.
- `Linalg::get_whitener`, used by `FiffCov::prepare_noise_cov`, takes the
  rank from the eigenvalues instead of a second SVD of the covariance: a
  305-channel covariance is prepared in 10 ms instead of 104 ms.
- The MNE Analyze Studio workflow test filtered the shared test recording and
  wrote its output into the shared test data, where `test_fiff_rwr`, running
  in parallel, listed the file and then failed to open it once removed. It
  now filters a copy in its own temporary directory.
- The frequency bins of the spectral connectivity metrics and whether they
  keep the per-trial spectra belong to each `ConnectivitySettings` instead of
  being process-wide statics, so two analyses (e.g. two MNE Scan plugins) no
  longer change each other's band, and a metric no longer resets the band of
  every other analysis to the whole spectrum. Changing the bins drops the kept
  spectra. Migration: `AbstractMetric::m_iNumberBinStart` /
  `m_iNumberBinAmount` → `settings.set_frequency_bins(start, count)` and
  `settings.frequency_bins()`; `AbstractMetric::m_bStorageModeIsActive` →
  `settings.set_keep_trial_data(keep)` and `settings.keeps_trial_data()`.
- With the per-trial spectra kept, coherence and imaginary coherence after a
  phase-lag metric (PLI, wPLI, USPLI, dwPLI) on the same trials skipped the
  power spectra the phase-lag metric does not compute and divided by an empty
  sum: the MNE Scan connectivity plugin showed wrong coherence after switching
  from a phase-lag metric.
- `RtConnectivity` is a synchronous streaming estimator with no Qt: it keeps
  a sliding window of trials, and each `update(trial)` computes only that
  trial's spectra, takes the oldest trial out of the running sums and returns
  the networks, which equal those of `compute_connectivity` for the trials in
  the window. The MNE Scan connectivity plugin runs it on its processing
  thread: it no longer copies all trials to a worker for every block, nor
  drops the trials that arrived while the worker was busy when the worker's
  copy came back. Migration: `RtConnectivity()` plus `append(settings)` and
  the `newConnectivityResultAvailable` signal → `RtConnectivity(settings,
  trials)` and `update(trial)`, which returns the networks; `networks()`,
  `set_trial_count`, `settings()` and `reset()`; `restart`, `stop` and
  `RtConnectivityWorker` are removed.
- `RtCov` is a plain class with no Qt that follows the noise as it changes.
  With a half-life, older samples count exponentially less (the weighted
  numpy.cov with aweights); without one, every sample counts the same. Each
  block is one rank update of the running mean and centred scatter, so memory
  no longer grows with the number of samples, and a large DC offset no longer
  costs precision: the old sums of raw outer products lost a fifth of a 1 nV
  covariance on a 10 mV offset. The MNE Scan covariance plugin uses a
  half-life of about its sample count and keeps estimating instead of
  starting over after each estimate. Migration:
  `RtCov(std::shared_ptr<FiffInfo>)` → `RtCov(const FiffInfo&, halfLife)`;
  `estimateCovariance(data, n)` → `update(data)`, then `covariance()`, which
  returns `std::optional<FiffCov>`, and `reset()` to start over.
  `RtCovComputeResult` is removed.
- Trigger detection (`dsp/rt/rt_detect_trigger.h`) uses standard C++ only.
  The gradient detector subtracted the first sample's value from every
  sample-to-sample change when asked to remove an offset, so on a trigger
  line idling above 0 V it missed flanks; a change has no offset, and the
  parameter is gone. Migration: `detectTriggerFlanksMax` /
  `detectTriggerFlanksGrad` / `toEventMatrix` are
  `detect_trigger_flanks_max` / `detect_trigger_flanks_grad` /
  `to_event_matrix`; a flank is a `TriggerFlank{sample, value}` instead of a
  `QPair<int, double>`, results are `std::vector` and `std::map` instead of
  `QList` and `QMap`, channel lists are `std::span<const int>`, and the
  gradient detector takes `FlankDirection::Rising`/`Falling` instead of the
  strings "Rising"/"Falling" and no `bRemoveOffset`.
- `FilterKernel` reports the pass band of low-pass and high-pass filters the
  right way round: a 100 Hz low-pass runs from 0 to 100 Hz, a high-pass from
  its cutoff to the Nyquist frequency. Before, `highpass_freq()` and
  `lowpass_freq()` were swapped for them, so the description read "100Hz to
  0Hz" and the filter design view showed a loaded low-pass as *from* 100 *to*
  0 Hz.
- `FilterKernel`, `FilterParameter`, `FilterIO` and `ParksMcClellan` use
  standard C++ only. `FilterIO` writes coefficients with all their digits,
  so they read back exactly, keeps spaces in filter names, and rejects a
  value that is no number. `FilterKernel::prepare_filter` no longer
  re-transforms the coefficients on every call. Migration:
  - `FilterKernel` methods are snake_case: `getName`/`setName` →
    `name`/`set_name` (`std::string`), `getSamplingFrequency` →
    `sampling_frequency`, `getFilterOrder` → `filter_order`,
    `getCoefficients` → `coefficients` (a const reference),
    `getDesignMethod`/`getFilterType` → `design_method`/`filter_type`,
    `getShortDescription` → `short_description`, `prepareFilter`,
    `applyConvFilter`, `applyFftFilter` → `prepare_filter`,
    `apply_conv_filter`, `apply_fft_filter`, and so on for every getter and
    setter. `designMethodIndex`/`filterTypeIndex` →
    `design_method_index`/`filter_type_index` (take `std::string_view`),
    `m_designMethods`/`m_filterTypes` → `design_methods`/`filter_types`.
    `FilterParameter::getName` → `name`.
  - `FilterIO::readFilter(QString, FilterKernel&)` → `read_filter(path)`
    returning `Expected<FilterKernel>`; `writeFilter(QString, kernel)` →
    `write_filter(path, kernel)` returning `Expected<void>`.
  - `ParksMcClellan` is no `QObject`; `FirCoeff` → `coefficients`, and the
    Remez exchange steps (`init`, `CalcParkCoeff2`, `Remez2`, ...) are
    private.
- Cosine FIR filters (`FilterKernel` with the "Cosine" design method) match
  MNE-C `mne_apply_filter` again: the lower band edge had no cos² taper but
  a hard step one half-width above the edge, so band-pass and high-pass
  filters were narrower than asked for. A cosine notch filter is the
  complement of the band-pass; it read past the end of an empty kernel
  before. Kernels of odd order were missing their centre tap, and a band
  above the Nyquist frequency wrote past the end of the frequency response.
- `MaxwellMovementComp` reads and writes MaxFilter head position files
  (`maxfilter -headpos`) like `mne.chpi.read_head_pos` and
  `write_head_pos`. Before, it wrote eight columns that MNE-Python cannot
  read, dropped error and velocity, and wrote a rotation whose quaternion
  had a negative scalar part as a different rotation. Migration:
  `readHeadPos(QString)` / `writeHeadPos(QString, ...)` are
  `read_head_pos(path)` returning `Expected<std::vector<HeadPosEntry>>`
  (an error for a line without ten values) and `write_head_pos(path, span)`
  returning `Expected<void>`; `HeadPosEntry` gained `dError` and
  `dVelocity`.
- `FineCalibration::write` writes bare channel numbers (`113`) like
  MaxFilter and `mne.preprocessing.write_fine_calibration`, not `0113`, and
  `read` rejects a line without 14 or 16 values like MNE-Python instead of
  skipping it. Migration: `read(QString)` / `write(QString)` take a
  `std::filesystem::path` and return `Expected<FineCalibration>` /
  `Expected<void>`; `findEntry(n, entry)` is `find(n)` returning a pointer
  or `nullptr`, `addEntry` is `push_back`, `gainVector` and
  `imbalanceMatrix` are `gain_vector` and `imbalance_matrix`.
- The MNE Scan HPI plugin offers the Neuromag VectorView coil-frequency
  presets again; its preset file has not been installed since the move to
  CMake, so the list was empty. `HpiSettingsView` now has them built in, and
  `loadCoilPresets()` returns whether the file could be read, keeps the
  presets if not, and refreshes the list for the loaded coils.
- `BrainVisionReader` decodes `.vhdr` and `.vmrk` files like MNE-Python:
  UTF-8, Windows-1252 for `Codepage=ANSI`, and Latin-1 when the bytes are
  not valid in that encoding. Latin-1 headers from older recorders kept raw
  bytes, so a `µV` unit was not recognised and those channels were read
  10^6 times too large.
- New `BASELIB::latin1_to_utf8(text)` and `BASELIB::is_utf8(text)` for
  headers stored in Latin-1 (EDF) or of unknown encoding.
- The BIDS format readers use std types, and `mne_bids` no longer links Qt
  itself. Migration:
  - `AbstractFormatReader`, `EDFReader`, `BrainVisionReader`: `open()` takes a
    `std::filesystem::path`; `getInfo`, `readRawSegment`, `getSampleCount`,
    `getFrequency`, `getChannelCount`, `toFiffRawData`, `formatName`,
    `supportsExtension` are `info`, `read_raw_segment`, `sample_count`,
    `sfreq`, `channel_count`, `to_fiff_raw_data`, `format_name` (returns
    `std::string_view`) and `supports_extension` (takes
    `std::string_view`). `read_raw_segment` takes `long` sample indices.
  - `EDFReader::getAllChannelInfos`/`getMeasurementChannelInfos` and
    `BrainVisionReader::getChannelInfos`/`getMarkers` are
    `all_channel_infos`, `measurement_channel_infos`, `channel_infos` and
    `markers`, returning `const std::vector<...>&` instead of a `QVector`
    copy.
  - `EDFChannelInfo`, `BrainVisionChannelInfo` and `BrainVisionMarker` hold
    `std::string` fields in UTF-8; `toFiffChInfo`, `toSi`, `unitScale`,
    `isVoltageUnit` are `to_fiff_ch_info`, `to_si`, `unit_scale`,
    `is_voltage_unit`.
- FIFF annotation channel names and extras, and `FiffAnnotations::read_json`/
  `write_json`, use `BASELIB::JsonDocument` instead of Qt's JSON classes; the
  files are unchanged. The helpers in `fiff/fiff_annotations_json.h` are now
  `names_to_json`, `names_from_json`, `extras_to_json` and `extras_from_json`
  and take or return `BASELIB::JsonArray`/`JsonObject`.
- The inv library no longer uses or links Qt; it reads the CMNE model
  metadata with `BASELIB::JsonDocument`. Migration, per class:
  - `InvMinimumNorm(inverse, lambda, method)` and `setMethod(method)` take
    `std::string_view`; `doInverseSetup` takes `int`. Pass
    `method.toStdString()` for a `QString`.
  - The convenience functions are snake_case and use std types:
    `apply_inverse_epochs(std::vector<MatrixXd>, ...)` returns
    `std::vector<InvSourceEstimate>`, `apply_inverse_raw`, `apply_forward`,
    `estimate_snr`, `compute_whitener` and `compute_source_psd` return
    `std::pair`, `compute_source_band_power` takes and returns `std::map`
    keyed by `std::string`; `method` is a `std::string_view`.
  - `InvVolumeSourceEstimate`: `set_shape(Eigen::Vector3i)`, `shape()`,
    `has_shape()`, `to_volume()`, `centre_of_mass()` (was `setShape(QVector)`,
    `hasShape`, `toVolume`, `centreOfMass`).
  - `InvLabelTimeCourse::extract` takes the mode as `std::string_view`.
  - `activeVertices` of `InvMxne`, `InvGammaMap` and `InvTfMxne` results,
    `InvTrapMusic::compute` results and the RAP-MUSIC dipole lists are
    `std::vector`; `InvLCMV::applyLCMVEpochs`/`InvDICS::applyDICSEpochs`
    take and return `std::vector`; `InvBeamformer::kind`, `srcType` and
    `subject` are `std::string`.
  - `InvHpiModelParameters` takes the coil frequencies as
    `std::vector<int>` and is a plain value type with defaulted `==`;
    `HpiFitResult::hpiFreqs`/`errorDistances` are `std::vector`, and
    `sFilePathDigitzers` is now `digitizerFile` (`std::filesystem::path`).
  - `InvDipoleFitSettings` file names, `projnames`,
    `InvDipoleFitData::setup_dipole_fit_data` and `bemname`, and
    `InvDipoleFit::fit_dipoles(_raw)` data names are `std::string` (UTF-8).
  - `InvCMNESettings::onnxModelPath` and `InvCMNE::applyCmne` take a
    `std::string`.
- `InvEcdSet` is a `std::vector`-backed container: `addEcd(ecd)` and
  `set << ecd` become `push_back(ecd)`, `empty()`, `begin()` and `end()`
  are new, `size()` returns `int`, and `dataname` is a `std::string`. The
  index operator no longer returns the first dipole with a warning for an
  index past the end (it could not on an empty set); like `std::vector` it
  requires a valid index. The header no longer includes `QList`/`QString`.
  Migration: `set.addEcd(ecd)` becomes `set.push_back(ecd)`,
  `set.dataname.isEmpty()` becomes `set.dataname.empty()`.
- `mne_fs` no longer links Qt Core. Migration: targets that used Qt only
  through `mne_fs` link `Qt::Core` themselves.
- `MNEMghTag` is a plain struct whose `data` is a `std::vector<std::byte>`
  instead of a `QByteArray`, and `MNEMghTagGroup::tags` holds the tags by
  value (`std::vector<MNEMghTag>`) instead of through `std::unique_ptr`.
  The FreeSurfer surface, curvature and tag readers behind
  `MNESourceSpace::load_surface` use `std::ifstream` instead of `QFile`,
  and `filter_source_spaces` runs its per-hemisphere work on `std::jthread`
  instead of `QtConcurrent`. `find_source_space_hemi()` returns `int`.
  Migration: `tag->data.constData()` becomes `tag.data.data()`.
- `MNESourceSpace::load_surface` refuses surface and curvature files whose
  vertex, triangle, quad or value counts are negative or exceed the file;
  a negative count used to throw `std::bad_alloc` out of the reader, and a
  huge one allocated memory for data the file did not hold.
- New `MNEChSelection::these(name, channels)` and
  `MNEChSelection::assign_channels(chs, deriv = nullptr)` (MNE-C
  `mne_ch_selection_these`, `mne_ch_selection_assign_chs` and
  `mne_ch_selection_assign_chs_info`) build a channel selection and match it
  to a recording; they were private helpers of the dipole fit. The retry
  that matches a channel by the part of its name before a dash (`EEG 001`
  for `EEG 001-REF`) compared the whole name again and never matched; it now
  matches like MNE-C. `mne_ch_selection.h` no longer includes `QString`.
- `MNEChSelection::ndef` is gone (use `chdef.size()`), and `nchan` is the
  function `nchan()` returning `chspick.size()` instead of a count kept next
  to the picked names. Migration: `sel.nchan` becomes `sel.nchan()`; a
  selection built by hand sets `chspick` (and `pick`) instead of `nchan`.
- `MNEChSelection::nderiv` is the function `nderiv()` counting the entries of
  `pick_deriv` that pick a derivation. A selection built by hand used to read
  no derived channels unless it also set the count. Migration: `sel.nderiv`
  becomes `sel.nderiv()`; drop assignments to it.
- `mne/mne_types.h` holds only the MNE-C constants and no longer includes
  20 other headers (fiff stream, projection, covariance, CTF compensation,
  layout, events, filter, ...) and `<QStringList>`. `MNE_DEFAULT_TRIGGER_CH`
  is a `std::string_view` and `MNE_ENV_TRIGGER_CH` a `const char*` constant
  instead of macros; the unused `MNE_ENV_ROOT` is gone. Migration: include
  the headers you use (for example `mne/mne_proj_op.h`) yourself.
- `MNEMshDisplaySurface` loses `user_data` and `user_data_free`, and
  `mne_types.h` loses `mneUserFreeFunc`: MNE-C's `mne_analyze` cached a contour
  map there, nothing in mne-cpp set them. Migration: keep per-surface state in
  your own map keyed by the surface.
- A default-constructed `MNEMshLight`, `MNEMshPicked` or `MNEMshColorScaleDef`
  is zero-initialised (light off at the origin, vertex 0, empty scale); their
  members used to hold whatever was in memory. The `MNEMshLight` constructor
  takes `state` as a `bool`.
- `MNEProjOp` tells MEG, EEG and stimulus channels apart by the name prefix
  (`MEG...`, `EEG...`, `STI...`) like MNE-C. It used to match the text anywhere
  in the name, so a projector zeroed its weights on channels such as `REF_EEG`
  or `TRIG_STI` and could classify an item as EEG.
- `InvDipoleFitData::print_fields` writes to a `std::ostream` instead of a
  `QTextStream`; `inv_dipole_fit_data.h` no longer includes Qt's text stream.
- `MNESourceSpace::filter_source_spaces` writes the omitted points to a
  `std::ostream`, one `%10.3f %10.3f %10.3f` line per point like MNE-C (the
  `--mindistout` file of the forward solution padded its separators too). With
  threads, both source spaces wrote to the stream at once and their lines could
  interleave; each now logs into its own buffer, so the file matches a serial
  run.
- `MNEProjOp::report`, `MNEProjOp::report_data` and `MNESssData::print` write
  to a `std::ostream` instead of a `QTextStream` and print exactly what MNE-C
  prints (`%-10s` channel names, `%10.5g` weights). The library logs these
  reports through `BASELIB::Log` instead of writing to `stderr`. Migration:
  `QTextStream s(&str); op.report(s, tag);` becomes
  `std::ostringstream s; op.report(s, tag);`.
- `mne_doc_shots` no longer captures a window before its pending timers have
  run: when grabbing a frame was slower than the settle time, a repaint that
  fell due during the grab was skipped, so screenshots on a loaded machine
  could show an earlier state and differ between runs.
- `MNEMeasData` and `MNERawData` find the trigger channel (`STI 014` or
  `MNE_TRIGGER_CH_NAME`) with `FIFFLIB::same_channel`, like every other
  channel lookup: spaces and case are ignored.
- `FiffStream::read_rt_tag` is gone: nothing called it, and the real-time
  server reads its socket with `read_tag_info(false)` and `read_tag_data`.
- `fiff/fiff.h`, `connectivity/connectivity.h` and `mne/mne.h` are umbrella
  headers for their library.
- `lsl://?simulate=1` publishes a synthetic EEG session as an LSL stream
  (`channels=`, `sfreq=`, `seed=`) and reads it. It used to fail for want of a
  stream.
- `ChannelDataView` draws magnetometers at their own default scale (1.2 pT);
  they used the gradiometer scale, 33 times larger, so the traces of
  magnetometer systems such as BabyMEG looked flat.
- `mne_fix_stim14` does what MNE-C's tool does: in place, `STI 014` gets the
  range 1 and keeps the low byte of its 16-bit samples; `--32` keeps the low
  16 bits of `STI101` and `STI201`. It used to rebuild `STI 014` from
  `STI 001` to `STI 006` into a copy, with a threshold the calibrated lines
  never reached, and so cleared `STI 014`. A refused file stays unchanged and
  the tool exits with 1. Migration: `mne_fix_stim14 --raw in.fif --out
  out.fif` becomes `cp in.fif out.fif && mne_fix_stim14 out.fif`.
- `mne_toggle_skips` turns skips off and on as MNE-C does, by renaming
  `FIFF_DATA_SKIP` to `FIFF_MNE_DATA_SKIP_NOP` and back. It used to write the
  negated skip count in host byte order, which a skip of one buffer read back
  as -1 and others as garbage.
- The MNE Scan acquisition plugins keep streaming when a block is
  overwritten before they read it (the computer was busy); they used to stop
  for good.
- The raw data browser of MNE Analyze Studio saves samples at their true
  scale; it divided them by the channel range (3276.8 for most channels of
  the sample data).
- `mne_anonymize` writes replacement texts with characters outside ASCII
  whole; it cut each of them short by one byte per such character, leaving
  invalid UTF-8 (a HIS ID `Jörg-Müller-ß` became `Jörg-Müller`).
- `mne_anonymize --help` names the value of `--measurement_date` and
  `--subject_birthday` a date and of the two offsets a number of days; the
  first two were swapped.
- RAP-MUSIC takes the vertices of every source space of the forward solution;
  with a single (volume) space it read a second hemisphere that does not exist
  and returned estimates without vertices.
- The Options tables of the command-line tool manual pages list the options
  the tools have (`mne_compute_mne`, `mne_inverse_operator`,
  `mne_make_source_space`, `mne_convert_surface` and others documented
  options that do not exist or missed ones that do).
  `scripts/doc/sync_tool_options.py` rewrites them from `--help`; `--check`
  lists differences.

## [2.4.0] - 2026-10-27

**Theme: correctness you can check.** v2.4.0 compares MNE-CPP against
MNE-Python 1.11 wherever both compute the same thing. That comparison found and
fixed more than 20 numerical defects in inverse, forward, beamformer, filtering
and spectral code and more than 200 other bugs. The release adds sEEG/ECoG
support, about 60 new MNE-Python ports, a WebAssembly renderer that matches the
desktop, and a signing pipeline for the Windows and macOS installers. 970
commits since v2.3.0.

This release replaces the v2.4.0 tag of 2026-10-22, which was withdrawn before
its Windows downloads could be signed.

### Highlights

- **Code signing** — every Windows executable and DLL carries a version
  resource with product name MNE-CPP and the release version, checked before
  each SignPath request against the artifact configurations in `.signpath/`.
  The macOS apps, `.dmg` images and installer are Developer ID signed and
  notarized. The Windows downloads of 2.4.0 are not yet Authenticode signed;
  signed Windows builds follow with 2.4.1 once the SignPath certificate is
  issued. The
  [code signing policy](https://mne-cpp.github.io/code-signing-policy/) states
  what is signed, by whom, and what the applications send over the network.
- **Connected documentation** — the manual, API reference and examples link to
  each other by scientific topic; the examples page can be filtered by task,
  recording type, level and library; a new Get Started guide builds a first
  program against an installed MNE-CPP, and every example states what it
  computes.
- **sEEG and ECoG** — new `mne_ieeg` library (implantations, leads, bipolar
  montages, projection of contact values onto the cortex). MNE Inspect draws
  and picks depth and grid electrodes from BIDS `*_electrodes.tsv` and paints
  per-contact values onto the brain, on the desktop and in the browser.
- **Validated against MNE-Python** — about 60 cross-validation test suites
  compare MNE-CPP with MNE-Python 1.11, SciPy, NiBabel and MNE-BIDS on committed
  reference values, from FIFF reading to inverse solutions. A versioned
  inventory lists, for 443 MNE-Python functions, whether MNE-CPP has an
  equivalent, a partial port or nothing yet, with the test that proves it.
- **Inverse and forward results corrected**:
  - Minimum-norm kernels were 0.8 % off.
  - eLORETA was off by a factor of 2 to 4; loose eLORETA was 24 % off.
  - Loose dSPM norms were 17 % off.
  - LCMV/DICS whitening was wrong.
  - The sphere-model forward crashed.
  - Magnetic-dipole fits ended 18 mm off.
  - Details are listed under *Fixed*.
- **File formats corrected**:
  - EDF channels were scaled by 1e6, and BDF was decoded as 16-bit.
  - MNE Browse saved EEG/EOG/STI channels 3276.8 times too large.
  - FIF event files were written scrambled.
  - Raw files with acquisition skips crashed.
  - FIFF writes used host byte order.
- **New MNE-Python ports** — FIR filter design (`create_filter`, `filter_data`,
  `firwin2`), ECG/EOG detection and projectors, Welch, multitaper and Morlet
  PSD/CSD/TFR, muscle and amplitude annotation, EOG regression, permutation
  t-test, `combine_evoked`, `apply_forward`, `compute_proj_evoked`,
  `estimate_snr`, `grade_to_vertices`, `grow_labels`, event utilities and six
  digitizer formats.
- **3D in the browser** — WebAssembly and OpenGL now use the same rendering path
  as the desktop. A long-standing cause of garbled or blank WebGL frames is
  fixed. Source-estimate animation is about twice as fast and picking is about
  1000 times faster.
- **MNE Scan** shows the head-movement path from continuous HPI in 3D.
- **MNE Analyze** gains event durations and trigger codes, averaging per
  condition, and saving of averages.
- **Documentation you can run** — every public class has a compiled, tested
  example (29 new example programs). The API reference has zero Doxygen
  warnings. Manual screenshots are generated by CI from the current build.
- **Trustworthy releases** — signed Windows installers and archives, signed and
  notarized macOS disk images, published SHA-256 checksums, and CI that
  installs MNE-CPP on clean Linux, macOS and Windows machines following the
  build guide.

### New library: `mne_ieeg`

- `IeegImplantation`:
  - Groups contacts into leads.
  - Reads and writes the BIDS iEEG `group` and `dimension` columns.
  - Transforms between coordinate frames.
  - Supports declared lead types (depth, strip, grid).
- `IeegReference` builds bipolar montages along each lead, named as in
  MNE-Python (`LA1-LA2`), as a sparse derivation matrix.
- `IeegSurfaceMap` projects contact values onto the cortex like
  `mne.stc_near_sensors` (sum, single, nearest and weighted modes).
- Example `ex_ieeg`.

### New APIs

**Reading and writing data**
- `FiffExplain` names FIFF tag kinds, block kinds, units, unit multipliers
  and channel kinds; `FiffDirNode::print` writes the block tree to a stream.
- Digitizer readers on `FiffDigPointSet`:
  - Polhemus Isotrak (`.elp`/`.hsp`/`.eeg`) and Polhemus FastSCAN.
  - BrainProducts CapTrak (`.bvct`).
  - EGI MFF.
  - Localite (`.csv`).
  - Neuroscan (`.dat`).
  - `transformToHead`.
- Head → MNI → Talairach transforms (`FiffCoordTransSet::readMniTransform`,
  `headToMni`, `mniToTalairach`); `mne_collect_transforms --mgh`.
- Events (`FiffEvents`): `pick`, `merge`, `count`, `concatenate`,
  `find_stim_steps`, `make_fixed_length`.
- Channels:
  - `channel_indices_by_type`, `pick_channels_regexp` and
    `pick_data_channels`.
  - Every MNE-Python channel type, including seeg, ecog, dbs, fNIRS and eye
    tracking.
- `FiffEvoked::combine` (`combine_evoked`), `FiffEvokedSet::save`,
  `FiffProj::compute_from_evoked`, `FiffCov::read` without exceptions, and
  covariance from epochs already in memory.
- HPI coil frequencies are read from the measurement info.
- MNE-C formats: layouts (`.lout`), event lists, derivations (`MNEDerivSet`),
  and morph maps (`MNEMorphMap`, used by `mne_make_morph_maps`).
- BEM solutions written by MNE-CPP are readable by MNE-C and MNE-Python.

**Source estimation**
- `applyForward`, `estimateSnr`, `MNEForwardSolution::convert_to_surf_ori`
  and `MNESourceSpace::gradeToVertices`.
- `InvSourceEstimate::writeHemispherePair` writes MNE-Python-compatible
  `-lh.stc`/`-rh.stc` files, also for LCMV and DICS results.
- The MNE-C minimum-norm quantities: eigenfield projection, SNR, lambda² and
  predicted data.

**Signal processing and statistics**
- `FirFilter::designMne`, `filterData` and `firwin2`, with window choice.
- `ArtifactDetect`: `computeProjEcg`/`computeProjEog` and `peakFinderMne`.
- `setEegReference` can add an average-reference projector instead of changing
  the data.
- Morlet wavelets (`MorletTfr::buildWavelet`), multitaper TFR on arrays,
  DPSS with `sym`/`lowBias` and concentration ratios, and Welch with DC
  removal.
- Statistics:
  - Permutation t-test.
  - `StatsAdjacency::combine` and `chain`.
  - Benjamini-Yekutieli FDR (`negcorr`) and reject masks.
  - `Numerics::chi2Isf`.
- `FsLabelUtils::growCircularLabel` (`grow_labels`). `KMeans` matches MATLAB
  for start centroids, Hamming distance and empty clusters.

**Workflows (MNA)**
- The graph executor runs verification checks and records provenance:
  versions, host, a SHA-256 hash per input, and timing.
- Parameter expressions support comparisons.

**Display**
- `BrainView`:
  - Electrodes: `setElectrodes`, `setElectrodeValues` and the
    `electrodeContactClicked` signal.
  - `setHeadMovementPath`, `takeScreenshot` and `setAutomatedRotation`.
- `RtFiffRawView` shows a crosshair with the value under the cursor.
  `ChannelDataView::renderToImage` renders traces without a GPU.
- Events carry a duration and a trigger code (`addRangedEvent`,
  `setEventDuration`, `setEventCode`).
- `MNELogger::setLogFile`. Debug and info messages go to stdout; warnings and
  errors go to stderr.

### Applications

**MNE Scan**
- Shows the head-movement path from continuous HPI fitting in the 3D view.
  Coil frequencies are taken from the recording. Digitizer points appear when
  the HPI view opens.
- Writes a log file. Plugin load errors are reported, and `MNE_SCAN_PLUGIN_DIR`
  overrides the plugin location.
- The clustered-MNE plugin is renamed `rtclustmne` and accepts unclustered
  forward solutions.
- Robustness:
  - Survives configurations naming unknown plugins.
  - Plugins stop cleanly before any data arrived.
  - Saving to an unwritable path is reported.
  - The dark theme and font are bundled again.

**MNE Analyze**
- Events have a duration, a trigger code and a type. Ranged annotations are
  drawn as bands, and codes are shared between instances.
- Averaging computes one evoked response per trigger code. *Save average* is
  implemented.
- Opens BEM files under any name and `.dip` dipole files.
- *View ▸ Plugins* menu. Automated 3D rotation works.
- Raw and event export work in the browser build.

**MNE Inspect** (desktop and browser)
- sEEG/ECoG electrodes, *File ▸ Load Electrode Values…*, dipoles placed in
  MRI coordinates.
- View layout and zoom are restored when you reopen a session.

**MNE Analyze Studio**
- Offline mode, a workflow preview, and FIFF files opening in the raw browser
  on Linux.
- Kernel and skill errors are reported instead of failing silently.

**MNE Browse**
- Event markers use the colours chosen for each event type.

**Command-line tools**
- `mne_edf2fiff` converts through the corrected EDF reader.
- `mne_eximia2fiff` works again; every conversion used to fail.
- `mne_prepare_bem_model` and `mne_setup_forward_model` write BEM solutions
  that other tools can read.
- `mne_make_morph_maps` writes real morph maps. `mne_make_derivations` writes
  the MNE-C format.
- `mne_collect_transforms` collects the full transform chain.
  `mne_transform_points` accepts multi-transform files.
- `mne_dipole_fit` honours `--grid`, the guess radius and `--magdip`.
- `mne_process_raw`'s grand average now matches MNE-C.
- Every tool returns a non-zero exit code on failure and writes errors to
  stderr.

**Real-time**
- `mne_rt_server` writes the UTC offset correctly, and the real-time client
  calibrates CTF compensators like the file reader.
- LSL streams are found on the same machine without multicast routing.

### Fixed

**Source estimation**
- Minimum-norm kernels:
  - 0.8 % off: the wrong noise eigenvalues were dropped.
  - eLORETA read its eigenfields transposed (off by a factor of 2 to 4).
  - Fixed-orientation dSPM/sLORETA had an empty noise normalisation.
  - The depth prior summed per channel instead of per source.
- Loose inverses on a Cartesian forward silently became loose = 1 (dSPM norms
  17 % off). Loose eLORETA lacked a renormalisation step (24 % off).
- Fixed-orientation solutions:
  - Forward reads used vertex instead of patch normals (1-2 % error).
  - Inverses built from free-orientation forwards were wrong.
  - `force_fixed` reads overwrote left-hemisphere rows.
- The sample-covariance whitener kept rank 58 of 360. CMNE whitening dropped
  every MEG direction.
- Beamformers:
  - Loading factor wrong.
  - `pick_ori="normal"` gave an empty filter.
  - NAI normalisation and rank handling wrong.
  - The whitener was transposed.
  - Max-power orientation signs were flipped.
  - LCMV used the wrong channels when the evoked channel order differed.
- Label-restricted inverses offset right-hemisphere vertices and kept one
  orientation instead of three.
- Sparse solvers:
  - MxNE stopped 43 % above the optimum.
  - Gamma-MAP pruned too early.
  - TF-MxNE converged short.
  - TRAP-MUSIC missed sources.
  - RAP-MUSIC read out of bounds.
- CMNE re-weighted the wrong quantity.

**Forward modelling and dipole fitting**
- Every sphere-model forward computation crashed.
- MEG/EEG gradients were stored in the wrong matrix.
- Magnetic-dipole fits computed fields at EEG electrodes and fitted current
  dipoles (ending 18 mm off).
- Icosahedron downsampling used the wrong base: only 63 of 2562 vertices
  matched.
- Volume source spaces listed excluded points.
- Volume centre of mass swapped x and z.
- Dipole fit:
  - Skipped or doubled the SSP projection.
  - The baseline used the end time for both bounds.
  - `--grid` and the guess radius were ignored.
  - MNE Analyze defaulted the sphere radius to 0.09 mm.
- On Windows, the Berg-Scherg sphere fit converged to a worse minimum.

**Preprocessing, epochs and averaging**
- SSP vectors from raw data spanned the wrong subspace. Covariance removed the
  grand mean instead of the evoked response, and ignored rejection limits.
- Epochs started one sample early. Baselines dropped the stimulus sample.
- Epoch rejection with a limit for only some channel types (e.g. gradiometers)
  rejected every epoch.
- Averages dropped their last sample, and the real-time average mixed stale
  pre-stimulus data across trigger codes (12 % error).
- Saved filtered raw files were shifted by half the filter length, and trigger
  channels were low-pass filtered with the data.
- SSS swapped its internal and external bases. Fine-calibration files now use
  the Elekta format.
- Spectra:
  - Morlet power was about 88 times too large.
  - PSD/CSD scaling and DC handling were wrong.
  - Real-time noise PSD used |X| instead of |X|².
- The surface Laplacian was centred on the electrodes instead of the head
  sphere.
- Standard montages used invented positions; they now use MNE-Python's
  `standard_1005`.
- Statistics:
  - The one-tailed cluster threshold was wrong.
  - The TFCE grid was wrong.
  - p-value tails below 1e-16 were wrong.
  - FDR ignored `alpha`.
  - Permutation p-values left out the observed statistic.
- Connectivity: correlation did not return Pearson r, and node positions came
  from the wrong channel.

**Files**
- `FiffDirNode::print` split every fragment into its own log line.
- EDF/BDF:
  - Every EDF channel was divided by 1e6.
  - Padded header fields read as 0.
  - BDF was decoded as 16-bit.
  - Channel kinds were wrong.
- BrainVision: unknown units read as zero, channel types were wrong, and ASCII
  files were read as binary.
- BIDS:
  - Electrode positions were attached to the wrong channels.
  - Units were wiped.
  - `recording` was written as `rec`.
  - Run numbers were padded.
  - The digitizer frame was wrong.
- FIFF:
  - Writes used host byte order.
  - UTF-8 strings were truncated.
  - Saving picked channels twice and calibrated evoked data twice.
  - Event files were scrambled.
  - The UTC offset and the inverse source unit were lost.
  - Old-style evoked files could not be read.
  - Corrupt tags were read out of bounds.
  - Files with acquisition skips or no data crashed.
- CTF compensation: the inverse transform was wrong, calibration was
  re-applied on write, and partial reads were returned silently.
- FreeSurfer:
  - Surface normals were overwritten instead of summed (affects
    `mne_surf2bem`).
  - Quad surfaces were scrambled.
  - Curvature or truncated files crashed the reader.
  - Asking for a hemisphere that was not loaded crashed.
- MRI: truncated `.nii.gz` files looped forever, and the Talairach transform
  frames were wrong.

**Applications**
- macOS (Apple silicon): the apps of the dynamic package and the installer were
  killed at launch because packaging invalidated their signatures.
- macOS and Linux dynamic packages: `resources` (layouts, styles, server
  configuration) was a link to the build machine instead of the files.
- macOS packages only started on the build machine's macOS (26); they now
  run on macOS 14.4 and later, the minimum of the bundled Qt.
- MNE Browse:
  - Saved EEG/EOG/STI channels 3276.8 times too large.
  - Could overwrite its own input.
  - Wrote empty annotation and event files.
  - Annotation durations grew on every save.
- MNE Analyze and MNE Scan: the butterfly plot showed an empty (on some screens
  magenta) area instead of the averages, and its screenshots were empty.
- MNE Analyze:
  - Events: event files never opened, trigger detection found no onsets, and
    reselecting a recording discarded its events.
  - Coregistration paired fiducials by position, ran ICP widgets from a
    worker thread, and read weights as 0 in decimal-comma locales.
  - Crashes fixed: adding an average without a recording, jumping to an event
    with nothing selected, channel selection, and a 13 GB allocation from an
    uninitialised pointer.
- MNE Scan:
  - Crashed on shutdown and on unknown plugins.
  - Reported 4294967295 channels before the channel info arrived.
  - Took screenshots of the wrong view.
  - Did not restore scalings.
- The real-time raw view drew markers offset after the buffer wrapped. The
  butterfly view compressed long averages. The spectrum view drew upside
  down. Several settings views lost their values without a settings file, and
  colour dialogs turned items black on *Cancel*.
- 3D view:
  - Thresholded source estimates showed the colormap's zero colour instead of
    the cortex.
  - Real-time sensor colouring never reached the head.
  - Dipoles were about 4 cm off in MRI coordinates.
  - Screenshots were written to `/`.
- Coregistration: the ICP nasion pin pulled exact alignments 25 mm off;
  optical calibration ended 24 mm RMS off.
- `FiffFileSharer` never delivered files (MNE Scan → MNE Analyze hand-over).

### Performance

- Source-estimate animation on two pial surfaces: 4.4 → 1.9 ms per frame.
  Hover highlighting: 10 ms → under 1 µs. Ray picking: 1.6 ms → about 1 µs.
- FIR filtering uses power-of-two FFTs (a test filter went from 13 s to 18 ms).
  The Hilbert envelope is FFT-based instead of O(n²).
- The browser build no longer re-uploads geometry every frame.

### For developers

**Breaking changes**
- Renamed:
  - macOS and Linux deploy with `scripts/deploy/deploy.sh`; `deploy.bat` is
    Windows only.
  - disp3D types live in the `DISP3DLIB` namespace; `PickResult` is now
    `RayHit`.
  - MNE Scan plugin `rtcmne` is now `rtclustmne` (`RtClustMne`).
- Changed defaults:
  - `FiffCoordTrans::read` returns the first transform in a file; use
    `readTransform(from, to)` to choose one.
  - `MNEForwardSolution::read` keeps bad channels by default.
  - `WelchPsd` removes the DC by default, and CSD no longer returns the DC bin.
  - `KMeans` uses the "singleton" empty-cluster action.
  - `StandardMontage` positions are now MNE-Python's.
- Changed values:
  - ECoG/sEEG/DBS channel kinds use the FIFF-standard values 902/802/803.
  - The BIDS entity is written as `recording`.
- Changed signatures:
  - FIFF writers, `checkIntegrity()` and `FiffEvokedSet::compensate_to` return
    `bool`.
  - Ownership-transferring factories return `std::unique_ptr`.
  - `CircularBuffer` cannot be copied.
- Removed:
  - `ElectrodeArray`/`ElectrodeContact`/`ElectrodeLayout` (use
    `IeegImplantation`).
  - `EDFReader` scale factor and `mne_edf2fiff --scaleFactor`.
  - `MNEHemisphere::getTriCoords`.
  - `FiffDirNode::explain`/`explain_block`/`get_tag_explanation`/
    `get_unit_name` and `FiffExplainEntry` (use `FiffExplain`).
  - mne_browse `MnxProject` (use the MNA project container).
  - Several unused internals.
- MNE Scan's bad-channel resource is renamed `babymeg_bad_channels.txt`. The
  MNE Analyze event shared-memory key is versioned.

**Quality**
- Every compiler warning is an error on all platforms, including the
  WebAssembly build.
- The source tree is formatted with clang-format 21.1.8 (one reformat commit,
  ignored by `git blame`), and clang-tidy findings may not grow.
- No raw `new`, `NULL`, C-style casts, `printf` or numeric `#define`s remain.
  Ownership uses smart pointers.
- Tests:
  - 34 tests that had never run in CI now run, and 26 unregistered tests were
    found.
  - Every test is registered with a label and a timeout.
  - Tests run against both the oldest and the newest supported Qt.
- Line coverage: libraries 89 %, applications 56 %, tools 61 %. Library branch
  coverage: 65 %.
- A layering check keeps the libraries free of dependency cycles.

**Build and release**
- Qt 6.10.0 to 6.12.0, Emscripten 6.0.12, ONNX Runtime 1.31.0. Versions are
  kept in `src/external/external_deps.env`.
- `doc/wiki/Release-Process.md` describes:
  - A release-candidate check before every release.
  - Windows Authenticode signing through SignPath, and macOS signing and
    notarization.
  - `SHA256SUMS` with per-file signing status.
  - Verification of the published downloads on all three platforms.
- A weekly onboarding job follows the build guide on clean Linux, macOS and
  Windows machines. It found three setup bugs, fixed in this release.
- `SECURITY.md` added.

### Documentation

- 29 new example programs, compiled and run by CI. Every public class has an
  example snippet in its API page; internal types show the examples that
  exercise them.
- The API reference builds with zero Doxygen warnings (1,726 fixed).
- Manual screenshots are generated from the current build in CI and compared
  against reference images.
- The website build uses a patched `braces` (nesting-depth bound for
  GHSA-vfj7-8cjw-p6xm, which upstream has not fixed), `follow-redirects`
  1.16.1 and `launch-editor` 2.14.2.
- The website fails to build on broken links or missing images, and has a
  generated quality page.
- The build guide is complete for all three platforms; CI runs exactly the
  commands it shows.

### Contributors

Christoph Dinh, Eric Larson (leaner CI on staging, #1013), ag-fieldline (FIFF
files without raw data no longer crash, #1003); dependency updates by
Dependabot.

## [2.3.0] - 2026-06-13

### Highlights

- **MNE Inspect multimodal visualization** — Unified `MultimodalScene` integrating sEEG depth electrodes, ECoG grids/strips (`ElectrodeObject`), MRI orthogonal slices (`SliceObject`), cortical surfaces with STC overlays, dipoles, connectivity arcs, and sensor locations; cross-modality picking with shared `PickResult` and timeline; data-driven colormap controls.
- **MNE Scan production hardening** — Plugin audit, real-time CMNE wiring (MNE/dSPM/sLORETA/CMNE method selector), recording/replay/resume, quickstart/hardware/deployment documentation, packaging refinements.
- **MNE Analyze cortical workflow** — FreeSurfer surface plugin, STC overlay with threshold/mid/max colormap and time slider, vertex picking and timecourse display.
- **MNE Align coregistration wizard** — 7-step wizard-guided coregistration tool with Polhemus FASTRAK serial driver, fiducial/HSP/EEG-cap digitization, Kabsch SVD + ICP alignment, `-trans.fif` export; session persistence; optical calibration engine with constrained solver.
- **Video overlay & probe tracking** — Tracker-driven video overlay projection on head surface with billboard fallback and vertex-displaced depth relief; catheter probe visualization (station 3) with tip-only crosshair, glow aura, and ethereal instrument shader.
- **MRI slice rendering** — 3D orthogonal MRI slice rendering with per-viewport visibility, intensity windowing controls, voxel-to-world transforms, and session persistence.
- **MNE-Python algorithm parity (~92%)** — Covariance estimation (auto/shrunk/OAS/FA/PCA), inverse convenience API, preprocessing & artifact detection, standard montages (10-20/10-10/10-05), Maxwell movement compensation, multitaper TFR, Picard ICA, surface Laplacian, auto ICA classification, depth priors, resolution metrics, SourceMorph, TRAP-MUSIC, TF-MxNE, CSP/SPoC/SSD decoding, simulation API.
- **Skigen integration** — ML algorithms (StandardScaler, MinMaxScaler, LOF, LedoitWolf, OAS, EmpiricalCovariance, FactorAnalysis, CSP, SPoC, SSD) delegated to skigen; API provenance registry with CI validation.
- **SPDX license-header normalization** — 1013 files migrated to unified SPDX+Doxygen headers; CI gate enforces compliance on every PR.
- **Doxygen-to-Docusaurus API docs** — Complete automated pipeline (`doxy2mdx.py`) generating 355 MDX pages (336 classes + 19 library indexes) with Mermaid inheritance diagrams and cross-reference hyperlinks.

### New Features

- **Covariance estimation parity**: Auto/shrunk/OAS/FA/PCA/regularize/whitener methods; `apply_inverse_epochs/raw/cov`, beamformer epochs, source PSD/power convenience API
- **Preprocessing & artifact detection parity**: Maxwell bad-channel detection, EOG regression, EEG re-referencing, cHPI filter, stim artifact removal, muscle/amplitude annotation, bridged electrode detection, LOF bad-channel detection, fine calibration, annotation↔event conversion
- **Source estimate types**: `VectorSourceEstimate`, `VolSourceEstimate`, `MixedSourceEstimate`; label operations (`extract_label_time_course` with 5 modes, stc↔label, grow/split labels)
- **Decoding**: CSP/SPoC/SSD/SlidingEstimator/GeneralizingEstimator/ReceptiveField; I/O export (EDF/BrainVision), AEC/PEC connectivity, HTML Report, linear regression, RM-ANOVA, bootstrap CI, fixed-length epochs
- **Standard montages**: 10-20/10-10/10-05 electrode montage support
- **Maxwell movement compensation**: Full Maxwell filter movement compensation
- **Multitaper TFR**: Multitaper time-frequency representation
- **Picard ICA**: Picard-based ICA decomposition algorithm
- **Surface Laplacian**: Current source density via surface Laplacian
- **Auto ICA classification**: Automated ICA component labeling
- **Depth priors & resolution metrics**: PSF/CTF resolution metrics for source estimation
- **Full SourceMorph**: Complete cross-subject cortical morphing
- **TRAP-MUSIC**: TRAP-MUSIC source localization algorithm
- **TF-MxNE**: Time-frequency mixed-norm estimates
- **Simulation API**: `simulate_raw`/`_stc`/`_evoked`, `addNoise`/`Ecg`/`Eog`/`Chpi` in new `sim` library
- **MNE Inspect plugins**: Electrode visualization plugin (`inspect_electrodes`), MRI slices plugin (`inspect_mri_slices`)
- **MNE Analyze Studio skills**: MNA-aware agent skills extension (`mna_skills`)
- **MNE Analyze cortical surface plugin**: STC load with fThresh/fMid/fMax colormap, time slider, play/pause
- **Real-time CMNE**: CMNE method added to MNE Scan minimum norm settings (combo: MNE/dSPM/sLORETA/CMNE) with model-checkpoint picker
- **Polhemus FASTRAK driver**: Serial backend with auto-detection, pivot calibration, optical path calibration engine, Kabsch SVD registration with vertex disambiguation
- **NIfTI reader**: NIfTI-1 single-file reader (`.nii`/`.nii.gz`) for MRI slice plugins
- **Documentation screenshot tool**: `mne_doc_shots` manifest-driven screenshot generation for manual pages
- **macOS .dmg generation**: `create-dmg` integration with codesign and notarization hooks
- **API provenance registry**: `doc/api_registry.json` with 90+ entries; `tools/validate_api_registry.py` CI validation

### MNE Inspect

- **Multimodal visualization** — Integrated `MultimodalScene` with electrode, MRI slice, brain surface, STC overlay, dipole, connectivity, and sensor renderables
- **Electrode plugin** — sEEG depth electrodes and ECoG grid/strip visualization with instanced contact spheres
- **MRI slices plugin** — 3D orthogonal MRI slice rendering with per-viewport visibility, intensity windowing controls, voxel-to-world transforms, and session persistence
- **Video overlay** — Tracker-driven video texture projection on head surface with billboard fallback and vertex-displaced depth relief
- **Probe tracking** — Catheter probe visualization (station 3) with tip-only rotating crosshair, glow aura, and ethereal instrument shader

### MNE Scan

- **Real-time CMNE** — Cascaded MNE method added to minimum norm plugin with MNE/dSPM/sLORETA/CMNE selector and ONNX model-checkpoint picker
- **Quickstart documentation** — Hardware setup, deployment, and quickstart manual pages

### MNE Align

- **New coregistration application** — 7-step wizard: Load FIFF digitizer points → Verify → Acquire fiducials → Acquire head shape → Acquire EEG cap → ICP alignment → Save `-trans.fif`
- **Polhemus FASTRAK integration** — Live serial acquisition with auto-detect, pivot calibration, hemisphere configuration
- **3D visualization** — Interactive BrainView with signal-driven sync, session persistence, marker rendering
- **Session persistence** — Full save/restore of coregistration state including acquired points and nested group prefixes
- **Optical calibration** — Constrained solver with known tracker-to-objective distance, sample persistence, direct objective center capture

### Skigen Integration

- **Delegated algorithms**: StandardScaler, MinMaxScaler, LOF, LedoitWolf, OAS, EmpiricalCovariance, FactorAnalysis, CSP, SPoC, SSD
- **Policy**: Never reimplement sklearn algorithms in mne-cpp; implement in skigen first
- **Init scripts**: Updated `init.sh`/`init.bat` to clone skigen automatically
- **CI**: API registry validation workflow

### CI/CD

- Fixed polyglot `.bat` SPDX headers causing bash syntax errors on Linux/macOS — `REM Copyright (c)` replaced with `:;#` comment style valid in both shells
- Fixed `mne_doc_shots` link errors on Linux with code coverage (`__gcov_exit` unresolved) — added `--coverage` compile/link flags
- Fixed `mne_doc_shots` MSVC RuntimeLibrary mismatch on Windows (`MD_DynamicRelease` vs `MT_StaticRelease`) — set `MSVC_RUNTIME_LIBRARY` to match project libraries
- Added SPDX license-header CI gate (`license-headers.yml`)
- Added API registry validation CI gate (`api-registry-validate.yml`)
- Added Doxygen API docs CI pipeline (`api-docs.yml`)
- Added screenshot regression CI (`screenshot-regression.yml`)
- Fixed CI deploy auth: replaced expired GIT_CREDENTIALS with GH_PAT
- Fixed Linux Qt builds: added XKB/XCB/EGL/GLES dev library dependencies
- Fixed Windows CI: use `vswhere` for VS toolchain discovery
- Fixed macOS CI: clear stale pip cache before setup-python
- Fixed 15 pipeline annotations in staging/codeql workflows
- Removed broken auto-fix-build workflow
- Consolidated license-headers workflow into staging pipeline
- Qt 6.11.2 toolchain across all CI workflows

### Documentation

- **SPDX license headers**: 1013 files migrated to unified SPDX+Doxygen format with chronological author order and rename history
- **Doxygen API docs pipeline**: `tools/doxy2mdx/doxy2mdx.py` (1394 lines) generating 355 MDX pages with Mermaid inheritance diagrams, XML-backed cross-references, and per-library indexes
- **MNE Align manual** (`mne-align.mdx`): 7-step wizard documentation
- **MNE Scan manual** (`mne-scan-hardware.mdx`, `mne-scan-quickstart.mdx`, `mne-scan-deployment.mdx`): 18 plugins enumerated
- **MNE Analyze Studio manual** (`mne-analyze-studio.mdx`): Agent-based workflow documentation
- **MNE Inspect multimodal manual** (`mne-inspect-multimodal.mdx`): Multimodal visualization guide
- **Build guide**: Updated for init-script workflow, Skigen dependency, and script paths
- **API sidebar**: Restructured — Namespaces/Classes/Files/Hierarchy/Members nested under Overview category
- **Thin-plate-spline warp example** (Closes #475)

### Refactoring

- Renamed `conn` library to `connectivity` to match mne-python naming
- Removed redundant null checks before `delete` (Closes #857)
- Modernized header format: unified v3 SPDX+Doxygen with `@author`/`@since`/`@date` tags
- Renamed FastTrak → Fastrak (correct product name)

### Bug Fixes

- Fixed `QWARN` deprecation warnings in `test_mne_io_coverage` (replaced with `qWarning()`)
- Fixed FIFF coordinate transformations: retain all transforms read from a file
- Fixed high-DPI rendering on fractional-scale monitors
- Fixed HPI digitizer sphere visibility in disp3D
- Fixed MNE Scan record action label/tooltip toggle
- Fixed stale static-analysis findings
- Fixed init.sh index error (#994)
- Fixed segfault when opening FIF files saved from MNE Python (gantry angle type mismatch, #993)
- Fixed cortical_surface plugin DLL resolution on Windows
- Fixed connectivity registry header paths after library rename
- Fixed WASM SerialPort, Doxygen MathJax, and deploy auth
- Fixed WASM file dialog guard for MRI loading (`onLoadMri`)
- Fixed Polhemus serial port disconnect, vertex-based frame flip, calibration flow
- Fixed session restore: group existence check for nested prefix, repopulate AcquiredPoints
- Fixed API docs: escaped `operator[]` headings causing broken Markdown links

## [2.2.1] - 2026-04-28

### Bug Fixes

- **mne_inspect — STC scrolling broken on desktop** (regression introduced in 2.2.0). Scrubbing the STC time-slider froze playback, produced flicker / "stretched-triangle" geometry artefacts, and could crash the application after a few seconds.
  - Root cause: commit `b58c18a8c2` ("Energy/computational savings…") switched the per-surface vertex buffer from `QRhiBuffer::Immutable` to `QRhiBuffer::Dynamic` to enable in-place colour updates. `Dynamic` is documented for *small, frequently-updated* payloads (UBOs, a few hundred bytes); using it for a multi-MB brain-surface VBO triggers two failure modes on Metal/Vulkan:
    1. Each in-flight frame slot keeps its own physical buffer. Skipping the per-frame `updateDynamicBuffer()` (because `m_gpu->dirty` was false) caused the GPU to sample the wrong slot — visible as stuck frames and wildly out-of-range vertex positions.
    2. Allocating a 4-5 MB Dynamic buffer × in-flight slots × every visible surface multiplied GPU memory pressure by roughly 3×, leading to allocation failures during interactive scrubbing.
  - The WebAssembly build was unaffected because the WASM merged-surface renderer uses `Immutable` buffers re-uploaded every frame.
  - Fix: revert just the desktop VBO type to `QRhiBuffer::Immutable` (re-created when vertex data is dirty). All other improvements from `b58c18a8c2` (demand-driven render, cached vertex count, IBO upload gating, removal of redundant `updateBuffers` in `renderSurface`) are kept.
  - Files: `src/libraries/disp3D/renderable/brainsurface.cpp`.

## [2.2.0] - 2026-04-26

### Highlights

- **MNX as MNE Scan project storage** — Pipeline configurations now load and save as `.mna` (JSON) or `.mnx` (CBOR) files via `MnaIO`; legacy XML pipeline format removed. Default auto-save path is `default.mna`.
- **Unified File-menu naming across applications** — `mne_scan`, `mne_inspect`, and `mne_browse` now use the same "Open Project… / Save Project… / Export Project as MNX…" pattern.
- **MNE-C CLI parity (82 tools)** — every command-line tool from MNE-C has a mne-cpp counterpart; final 4 ports landed in this release (`mne_make_movie`, `mne_convert_lspcov`, `mne_convert_ncov`, `mne_dacq_annotator`).
- **Lossless MNA / MNX round-trip** — every MNA struct preserves unknown JSON/CBOR keys via an `extras` field, so newer schema fields survive a round-trip through older software.
- **Performance as a co-equal optimization goal** — periodic `code-optimization` prompt and matching `optimization-requirements.md` doc cover both idiomatic-code refactors and quantitative perf wins (≥ 5 % wall-clock).

### New Features

- **MNA graph-execution library** (`mne_mna`): New library for the `.mna` analysis node-graph format with YAML-based operator definitions, topological execution, and `MnaOpRegistry` for built-in and user-registered operators
- **MNX binary container** — CBOR-based binary equivalent of `.mna` (`MNX1` magic header), with embedded-data support and identical key structure for lossless format conversion
- **MNA Scan integration** — `MnaGraph m_pPipelineGraph` mirrors the live plugin scene in `PluginSceneManager`; `MnaDataKind` ↔ `ConnectorDataType` mapping shipped in `mna_scan_types`; `PluginGui::loadConfig`/`saveConfig` route through `MnaIO::read`/`MnaIO::write`
- **Stream-mode MnaGraphExecutor** — `MnaGraphExecutor::startStream()` / `stopStream()` with `StreamContext` for real-time acquisition pipelines
- **Machine-learning library** (`mne_ml`): New library with `MlTrainer` for sklearn-compatible model training via embedded Python, supporting SVM, Random Forest, Logistic Regression, and custom estimators
- **STS covariance estimators**: Added Ledoit-Wolf and Oracle Approximating Shrinkage (OAS) estimators to the `sts` library with cross-validation support against scikit-learn
- **Source-space cluster permutation tests** — `StatsCluster::oneSamplePermutationTest()`, `fTestPermutationTest()`, `tfce()` (threshold-free cluster enhancement); `StatsAdjacency::fromSourceSpaceTemporal()` for spatio-temporal adjacency
- **Spectral connectivity expansion** — `GrangerCausality` (`"GC"`), `DirectedTransferFunction` (`"DTF"`), `PartialDirectedCoherence` (`"PDC"`); shared MVAR fitting via `MvarModel`
- **CMNE sparse inverse**: Added Cascaded MNE (`InvCMNE`) solver to the inverse library for spatially sparse source estimation
- **DSP Infomax ICA**: Extended the `dsp` library with an Infomax-based ICA decomposition algorithm
- **BIDS coordinate system**: Added `BidsCoordinateSystem` class for round-trip parsing and writing of `coordinatesystem.json`, and `BidsElectrode::toFiffDigPoints()` for converting BIDS electrodes to FIFF digitizer point sets
- **FreeSurfer atlas lookup**: Added `FsAtlasLookup` for Desikan-Killiany / Destrieux atlas region querying
- **MRI slicer**: Added `MriSlicer` class for extracting orthogonal slices (Axial/Sagittal/Coronal) from NIfTI volumes
- **3D electrode visualization**: `ElectrodeObject` now includes QRhi GPU buffer management (`updateBuffers`, `vertexBuffer`, `indexBuffer`, `instanceBuffer`) following the BrainSurface pattern; new `electrode.vert` / `electrode.frag` shaders with instanced contact spheres and Blinn-Phong lighting
- **3D MRI slice visualization**: New `SliceObject` renderable for textured MRI volume slices with voxel-to-world transforms and intensity windowing; new `slice.vert` / `slice.frag` shaders
- **FiffAnnotations integration**: `AnnotationModel` refactored to use `FiffAnnotations` as backing store instead of internal `QVector<AnnotationEntry>`, with `fiffAnnotations()` / `setFiffAnnotations()` public accessors
- **Python cross-validation tests**: Added `GUARD_PYTHON` / `GUARD_PYTHON_PACKAGE` macros ensuring Python-dependent tests fail in CI (`MNE_REQUIRE_PYTHON=true`) instead of silently skipping
- **PythonTestHelper extensions**: Added `writeMatrix()`, `readMatrix()`, and `evalMatrixViaFile()` convenience methods for Eigen↔NumPy round-trip validation
- **MNA Scan plugin registry**: Added `scan-plugins.json` schema and `MnaScanTypes` class for declaring MNE Scan plugins as MNA operator schemas
- **W-file I/O**: Added `read_w` and `write_w` methods to `FiffStream` for FreeSurfer `.w` format surface data
- **Cortical surface mapping**: New `MNECorticalMap` class with `icoDownsample()` for icosahedral downsampling, `smoothOperator()` for nearest-neighbor surface smoothing, and `computeMorphMaps()` for cross-subject cortical morphing
- **Scalp surface generation**: Added `makeScalpSurfaces()` to `MNEBemSurface` for creating decimated scalp meshes at multiple resolutions
- **Processing history**: Added `copyProcessingHistory()` to `FiffStream` for transferring processing provenance between FIFF files

### MNE Inspect

- **UI overhaul** — Native `QMenuBar` (File / View / Tools / Help) with macOS system-menu integration, Recent Projects submenu (max 10), Ctrl+O / Ctrl+Q / Space shortcuts; closable + movable Controls and Loaded Files dock widgets with custom flat title bars; status bar with progress; `saveState()` / `restoreState()` persistence
- **Loaded Files panel** — live `QTreeWidget` with Name / Type / Path columns and context menu (Remove, Copy Path, Show in Finder); double-click STC entry activates it in the source-estimate combo
- **Progressive-disclosure controls** — load buttons removed from sidebar; menu actions are the single entry point; control groups disabled until matching data loads; data unload via Loaded Files context menu
- **WASM compatibility** — menu bar renders inline; `QFileDialog` guarded for WASM; recent-files menu disabled on WASM

### MNE Scan

- **File menu uses Project terminology** — `&New Project` (Ctrl+N), `&Open Project…` (Ctrl+O, accepts `*.mna *.mnx`), `&Save Project…` (Ctrl+S, defaults `.mna`), `&Export Project as MNX…` (Ctrl+Shift+E, always binary CBOR)
- **Auto-save default path** — `default.xml` → `default.mna` (auto-loaded on startup if present)
- **Foreign-data preservation** — round-trip saves preserve unknown fields via `m_loadedMnaProject`
- **Modernised pipeline canvas** — rounded-rect nodes with gradient fills, drop shadows, type labels (`SENSOR` / `ALGORITHM`), centred name, hollow-input / filled-output ports, Bezier connections with tangent arrowheads, dot-grid background, Tailwind-inspired palette, selection glow

### MNE Browse

- **Project reset on standalone open** — opening a raw file without a project resets `m_mnxProject` to prevent stale-project contamination across all 5 file-open entry points (desktop menu, WASM picker, recent files, drag-drop, CLI)

### WASM Progressive Web App

- **PWA manifest + service worker** — `scripts/wasm/manifest.json`, `scripts/wasm/sw.js`, 192×192 / 512×512 icons; offline caching with versioned cache keys; combined COOP/COEP header injection
- **No server required** — deploy bundle to any static host; service worker injects required headers for `SharedArrayBuffer`

### Command-Line Tools

- **Final MNE-C CLI parity** (4 new): `mne_make_movie` (STC frames → PNG via `FsSurface` orthographic projection), `mne_convert_lspcov` (LISP S-expression covariance → FIFF `FiffCov`), `mne_convert_ncov` (ASCII ncov → FIFF), `mne_dacq_annotator` (event annotation replacing legacy Motif GUI)
- **Inverse tools** (7 new): `mne_compute_cmne`, `mne_inverse_pipeline`, `mne_label_ssp`, `mne_average_estimates`, `mne_process_stc`, `mne_make_uniform_stc`, `mne_map_data`
- **Preprocessing tools** (7 new): `mne_add_triggers`, `mne_change_baselines`, `mne_change_nave`, `mne_copy_processing_history`, `mne_fix_stim14`, `mne_make_derivations`, `mne_toggle_skips`
- **Info tools** (6 new): `mne_check_eeg_locations`, `mne_evoked_data_summary`, `mne_list_coil_def`, `mne_list_proj`, `mne_sensor_locations`, `mne_show_mna`
- **Forward tools** (3 new): `mne_fit_sphere_to_surf`, `mne_make_scalp_surfaces`, `mne_transform_points`
- **Conversion tools** (3 new): `mne_convert_ctf_markers`, `mne_epochs2mat`, `mne_mna_bids_converter`
- **Total CLI inventory: 82 tools** — full MNE-C feature parity

### CI/CD

- Upgraded `codecov/codecov-action` from v5 to v6 to resolve Node.js 20 deprecation warnings
- Added `MNE_REQUIRE_PYTHON: 'true'` environment variable to all platform test steps
- Added `scikit-learn` to Python test dependencies
- Updated lodash from 4.17.23 to 4.18.1 (Dependabot security fix)

### Bug Fixes

- Fixed potential silent test skips — Python-dependent tests now respect `MNE_REQUIRE_PYTHON` enforcement
- Fixed deprecated `qWarn` calls in `test_mne_source_analysis` (replaced with `qWarning()`)
- Fixed unused `[[nodiscard]]` return values in `test_tool_inverse_computation`

### Documentation

- Added 26 new command-line tool documentation pages
- Comprehensive MNA / MNX format manual page (`mna-format.mdx`, ~692 lines) — file types, BIDS-like data hierarchy, computational graph, execution modes, parameter system, op registry, verification & provenance, MNX byte layout, schema versioning, complete annotated JSON example
- Rewrote MNA API reference page (`api-mna.mdx`) with full class inventory (63 → 572 lines)
- Added ML library API reference page (`api-ml.mdx`)
- Added Statistics library API reference page (`api-sts.mdx`)
- Updated tools overview with all 26 new tools and new "Inspection and Information" section
- Updated overview and API pages with Mna, Ml, Sts library entries
- Expanded glossary with 14 new terms (CMNE, DICS, LCMV, MNA, MxNE, ONNX, etc.) and 3 file format entries (`.mna`, `.mnx`, `.onnx`)
- Fixed orphaned pages (`mna-format`, `api-mna`) by adding to sidebar navigation
- Added zero-tolerance warning policy to development conventions
- Added `mne_analyze` feature-parity gap analysis (Section 14 of `gap-analysis.md`); concrete plugin-level tasks moved to `optimization-requirements.md` § 15
- Added reusable `code-optimization` Copilot prompt (`.github/prompts/code-optimization.prompt.md`) and matching `optimization-requirements.md` backlog with explicit performance scope

## [2.1.0] - 2026-04-10

### New Features

- **MNE Browse overhaul**
  - QRhi-based 2D channel rendering for Metal/Vulkan/D3D12 performance
  - ICA browser with component visualization, rejection, and original-data reset
  - Epoch review workflow with evoked computation and histogram view
  - Annotation workflow with FIF interoperability and auto-merge
  - Virtual channel derivations with weighted reference workflows
  - Covariance matrix heatmap visualization and whitening controls (W key)
  - Projector dialog (J/Shift+J) and SSP toggle
  - 2D layout view with mouse-wheel zoom, auto-scale, and channel scaling spinboxes
  - Dark mode, drag-and-drop file open, recent files list
  - GPU clipping/z-score shaders, fullscreen (F11), zen mode (F)
  - Linear detrending (D key cycles None/DC/Linear)
  - GFP overlay on butterfly view
  - Interactive ruler tool with amplitude snap and semi-transparent overlay
  - Double-click bad-channel marking, click-to-toggle bad channels
  - Stimulus lane above time ruler with event markers
  - Per-description event/annotation filtering and visibility toggles
  - Channel sort by type, epoch grid lines (G), overview bar toggle (O), scroll speed control
  - Source-estimate export workflow
  - Session filtering workflow
  - Settings persistence across sessions via QSettings
  - Event file saving to FIF and ASCII formats
  - WASM loading screen and optimized WebAssembly build

- **MNE Analyze Studio**
  - Agent-based workflow engine with grounded LLM safety
  - Multi-provider planner profiles (OpenAI Responses API)
  - Reusable inspect 3D surface views
  - Extension-based view architecture with hosted view tooling
  - VS Code chat UX integration and `.mna` project format

- **BIDS library** (`mne_bids`): New library for reading and writing BIDS-formatted datasets with testframe

- **Beamformers**: Added beamformer classes to the `inv` library with testframes

- **DSP library enhancements**
  - `WelchPsd`: Welch power spectral density estimation
  - `MorletTfr`: Morlet wavelet time-frequency representation
  - `xDAWN`: xDAWN spatial filtering for ERP denoising
  - KMeans: Cosine and correlation distance normalization implemented
  - Real-time covariance: Channel picks applied (MEG/EEG only)

- **Logging**: Renamed `ApplicationLogger` to `MNELogger` with `MNE_LOG_LEVEL` and `MNE_LOG_MODE` environment variables

### CI/CD

- Qt 6.11 support with auto-detection in `init.sh`
- Build Qt Installer Framework 4.11.0 toolchains from source
- Offline self-contained installers (.run, .dmg, .exe)
- Nightly WASM builds deployed alongside stable releases
- Reusable Qt toolchain assets consumed by CI workflows
- Eigen downloaded at configure time (no longer bundled)

### Bug Fixes

- Fixed Metal/QRhi rendering on macOS: native window, infinite repaint loop
- Fixed butterfly whitening scale and circular channel arrangement
- Fixed 2D layout channel filtering, scale persistence, and item sizing
- Fixed crosshair repaint cascade, split MEG grad/mag scale labels
- Fixed DSP SSS and bad-channel regressions
- Fixed DSP pi constants on Windows, params defaults on Clang/GCC
- Fixed Windows linker error for nested `Params` struct exports
- Fixed macOS test flakiness and WASM build errors
- Fixed `mne_anonymize` unique_ptr usage
- Fixed ruler amplitude precision (1 → 3 decimal places)
- Fixed stim chip overlap, event line z-order, and resize resampling
- Fixed scroll pan boundary and smooth inertial scroll

### Documentation

- Updated website to reflect renamed/split libraries
- Updated build guide paths (`tools/` → `scripts/`)
- Fixed release download page for installers and WebAssembly
- Updated Zenodo DOI to 19139238

## [2.0.0] - 2026-03-20

### Versioning

- **MNE-CPP libraries**: Version 2.0.0
- **CLI tools** (51 ported MNE-C tools, `mne_dipole_fit`, `mne_inspect`): Derive version from the MNE-CPP project version (2.0.0) via CMake compile definition
- **MNE Scan**: Version 1.0.0 (first stable release as a standalone application)
- **MNE Analyze**: Version 1.0.0 (first stable release as a standalone application)
- **IConnector plugin interface**: Bumped to `mne_rt_server/2.0`

### Breaking Changes

- **Library renames**: `connectivity` → `conn`, `communication` → `com`, `rtprocessing` → `dsp`, `inverse` → `inv`, `disp3D_rhi` → `disp3D`
- **Class renames**: Inverse library classes now use `Inv` prefix (`InvRapMusic`, `InvMinimumNorm`, `InvSourceEstimate`, `InvHpiFit`, …)
- **Moved classes**: `InvInverseOperator`, `InvMeasData`, `InvMeasDataSet` moved from `inv` to `mne` library (as `MNEInverseOperator`, `MNEMeasData`, `MNEMeasDataSet`); backward-compatible forwarding headers provided
- **Moved classes**: `FwdForwardSolution` moved to `MNEForwardSolution` in the `mne` library; forwarding header in `fwd/` for backward compatibility
- **Removed**: `InvCorSourceEstimate` class (superseded by unified `InvSourceEstimate`)
- **Split**: `MNEMath` split into `Linalg` and `Numerics` in the `math` library
- **Moved**: `events` library moved into `mne_analyze` application (no longer a standalone library)
- **Qt requirement**: Qt 6.10.0 minimum (Qt 5 no longer supported)
- **Eigen upgrade**: Eigen 5.0.1

### New Features

- **51 CLI tools**: Ported all MNE-C command-line tools to modern C++ with `QCommandLineParser` and `ApplicationLogger`
  - Conversion: `mne_edf2fiff`, `mne_ctf2fiff`, `mne_kit2fiff`, `mne_brain_vision2fiff`, and more
  - Forward modeling: `mne_forward_solution`, `mne_setup_forward_model`, `mne_prepare_bem_model`, `mne_make_source_space`, `mne_make_sphere_bem`, `mne_average_forward_solutions`
  - Inverse: `mne_compute_raw_inverse`, `mne_compute_mne`, `mne_inverse_operator`
  - Preprocessing: `mne_anonymize`, `mne_process_raw`, `mne_compensate_data`
  - Surface: `mne_surf2bem`, `mne_check_surface`, `mne_watershed_bem`, `mne_flash_bem`
  - Info: `mne_show_fiff`, `mne_compare_fif_files`, `mne_list_source_space`, `mne_list_bem`
  - Server: `mne_rt_server`
  - Simulation: `mne_simulate_data`
- **Unified InvSourceEstimate**: Single class with composition layers for grid data, focal dipoles, couplings, connectivity, positions, and metadata; includes tokenization support for streaming
- **RHI-based 3D rendering**: Migrated from Qt3D to QRhi pipeline with holographic rendering mode, Fresnel effects, dipole overlay, and async STC loading
- **Flexible clustering**: Variable cluster sizes in source space computation
- **MNE Scan improvements**: Plugin loading progress on splash screen, macOS app bundle icons
- **LSL library modernized**: Added `lsl_` prefix, comprehensive tests, updated to LSL 1.17.5

### Refactoring & Modernization

- **Forward library (fwd)**: Removed ~700 lines of dead code; replaced `printf` → `qInfo`/`qWarning`; replaced `goto` → early return/lambdas; replaced C macros (`ARSINH`, `EPS`) → `constexpr`/`inline`; modernized `FwdBemModel` members from raw pointers to Eigen types and `std::shared_ptr`
- **FIFF I/O consolidation**: Moved duplicated FIFF reading functions to library classes (`FiffStream::read_bad_channels`, `MNECovMatrix::read`, `MNERawInfo::find_meas_info`)
- **Thread safety**: Fixed thread-unsafe `static float*` in `MNEProjOp::project_vector` using `thread_local`
- **Network input validation**: Added bounds checking in BabyMEG client byte conversion functions
- **Sparse matrix crash fix**: Guarded `MNEHemisphere::operator==` against segfault on default-constructed sparse matrices
- **Copyright**: Updated to 2026 across all library headers
- **Qt6 serialization**: Updated `QDataStream` version from `Qt_5_1` to `Qt_6_0` in rt_server
- **Centralized CLI versioning**: All 51 CLI tools now derive their `PROGRAM_VERSION` from the CMake project version (`MNE_CPP_VERSION`) instead of hardcoded strings
- **Format specifier fix**: Fixed `%d` → `%lld` for `qsizetype` in BabyMEG client warnings

### CI/CD

- Restructured to Main-Staging-Feature branch model
- Build matrix: Ubuntu 24.04, macOS 26, Windows 2025 with Qt 6.10.0 and Qt 6.10.2
- CodeQL and Coverity static analysis (weekly)
- Code coverage tracking with 35% library threshold
- CPackIFW-based installer generation (.run, .dmg, .exe)
- WebAssembly build support

### Documentation

- Docusaurus-based documentation site
- Organized CLI tools by domain category in sidebar
- Added MNE handbook reference pages (cookbook, FIFF format, sample dataset, data conversion)
- Completed Doxygen documentation for `InvRapMusic`, `InvMinimumNorm`, `InvDipole`

### Bug Fixes

- Fixed crash in `make_inverse_operator` with non-surface-oriented forward solutions
- Fixed segfault in forward computation
- Fixed FIFF double I/O, `make_dir` EOF, `write_cov` packing, and `source_cov` kind
- Fixed `MNECTFCompDataSet` copy constructor member initialization
- Fixed coordinate transformation handling in 3D visualization
- Fixed inflated brain alignment and surface projection tolerances
- Fixed floating-point comparison tolerances in forward solution tests
- Fixed sphere simplex test convergence
