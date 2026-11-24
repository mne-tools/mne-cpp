//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Implements mne_compute_cmne application.
 *
 *           Computes Contextual Minimum-Norm Estimate (CMNE) source time
 *           courses from evoked data, or trains / fine-tunes the LSTM model.
 *
 *           Three modes of operation:
 *             compute   – Apply dSPM + LSTM correction → write STC files
 *             train     – Train CMNE LSTM from FIFF files → export ONNX
 *             finetune  – Continue training from an existing model
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_evoked.h>
#include <mne/fiff/fiff_cov.h>

#include <mne/mne/mne_forward_solution.h>

#include <mne/inv/inv_source_estimate.h>
#include <mne/inv/minimum_norm/inv_cmne.h>
#include <mne/inv/minimum_norm/inv_cmne_settings.h>

#include <mne/ml/ml_trainer.h>
#include <mne/pybridge/python_runner.h>
#include <mne/base/command_line.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdio>
#include <filesystem>
#include <format>
#include <print>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace mne;

//=============================================================================================================
// HELPERS
//=============================================================================================================

static int methodStringToInt(const std::string& method)
{
    if (iequals(method, "MNE"))
        return 0;
    if (iequals(method, "dSPM"))
        return 1;
    if (iequals(method, "sLORETA"))
        return 2;
    if (iequals(method, "eLORETA"))
        return 3;
    return 1; // default dSPM
}

//=============================================================================================================

static bool fileExists(const std::filesystem::path& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec);
}

//=============================================================================================================

static std::string resolveScriptDir()
{
    const std::filesystem::path relPath = "scripts/ml/training/cmne";
    const std::filesystem::path marker = "pyproject.toml";

    // 1) Binary lives in <root>/out/<config>/bin/ → go up 4 levels to repo root
    std::filesystem::path dir = executable_dir();
    for (int i = 0; i < 5 && !dir.empty(); ++i) {
        const std::filesystem::path candidate = dir / relPath;
        if (fileExists(candidate / marker))
            return utf8_from_path(candidate);
        const std::filesystem::path parent = dir.parent_path();
        if (parent == dir)
            break;
        dir = parent;
    }

    // 2) Relative to working directory (works when cwd is repo root)
    if (fileExists(relPath / marker)) {
        std::error_code ec;
        return utf8_from_path(std::filesystem::absolute(relPath, ec));
    }

    return {};
}

//=============================================================================================================
// COMPUTE MODE
//=============================================================================================================

static int doCompute(const CommandLineParser& parser)
{
    // ── Validate required arguments ────────────────────────────────────
    const std::string fwdPath = parser.value("fwd");
    const std::string covPath = parser.value("cov");
    const std::string evokedPath = parser.value("evoked");
    const std::string outPrefix = parser.value("out");

    if (fwdPath.empty() || covPath.empty() || evokedPath.empty()) {
        std::print(stderr, "Error: --fwd, --cov, and --evoked are required for compute mode.\n");
        return 1;
    }
    if (outPrefix.empty()) {
        std::print(stderr, "Error: --out is required for compute mode.\n");
        return 1;
    }

    // ── Read forward solution ──────────────────────────────────────────
    std::print("Reading forward solution: {}\n", fwdPath);
    std::fflush(stdout);
    auto fwdRead = MNEForwardSolution::read(path_from_utf8(fwdPath), {.force_fixed = true, .surf_ori = true});
    if (!fwdRead) {
        std::print(stderr, "Error: {}\n", fwdRead.error().what());
        return 1;
    }
    const MNEForwardSolution& fwd = *fwdRead;

    const MatrixXd matGain = fwd.sol.data;
    const int nChannels = static_cast<int>(matGain.rows());
    const int nSources = static_cast<int>(matGain.cols());
    std::print("  {} channels x {} sources\n", nChannels, nSources);

    // ── Read noise covariance ──────────────────────────────────────────
    std::print("Reading noise covariance: {}\n", covPath);
    std::fflush(stdout);
    FiffCov noiseCov(path_from_utf8(covPath));
    if (noiseCov.empty()) {
        std::print(stderr, "Error: Failed to read noise covariance from {}\n", covPath);
        return 1;
    }

    const MatrixXd matNoiseCov = noiseCov.data;
    std::print("  {} x {}\n", matNoiseCov.rows(), matNoiseCov.cols());

    // ── Read evoked data ───────────────────────────────────────────────
    const int setno = to_int(parser.value("setno")).value_or(0);
    std::print("Reading evoked data: {} (set {})\n", evokedPath, setno);
    std::fflush(stdout);
    FiffEvoked evoked(path_from_utf8(evokedPath), setno);
    if (evoked.empty()) {
        std::print(stderr, "Error: Failed to read evoked data from {}\n", evokedPath);
        return 1;
    }

    const float tmin = evoked.times(0);
    const float tstep = (evoked.times.size() > 1)
        ? (evoked.times(1) - evoked.times(0))
        : 1.0f;

    // ── Pick channels to match forward solution ────────────────────────
    // The evoked file may contain extra channels (STIM, EOG, ECG, …)
    // that are not in the forward solution.  Subset to forward channels.
    const StringList fwdChNames = fwd.sol.row_names;
    FiffEvoked evokedPicked = evoked.pick_channels(fwdChNames);
    FiffCov noiseCovPicked = noiseCov.pick_channels(fwdChNames);

    const MatrixXd matEvoked = evokedPicked.data;
    const MatrixXd matNoiseCovPicked = noiseCovPicked.data;

    std::print("  {} channels in file, {} channels after picking ({} samples, tmin={:g} s, tstep={:g} ms)\n",
               evoked.data.rows(), matEvoked.rows(), matEvoked.cols(), tmin, tstep * 1000.0f);

    // ── Source covariance (identity) ───────────────────────────────────
    MatrixXd matSrcCov = MatrixXd::Identity(nSources, nSources);

    // ── Configure CMNE settings ────────────────────────────────────────
    const double snr = to_double(parser.value("snr")).value_or(0.0);
    const std::string methodStr = parser.value("method");
    const int lookBack = to_int(parser.value("look-back")).value_or(0);
    const std::string onnxPath = parser.value("onnx");

    InvCMNESettings settings;
    settings.lambda2 = 1.0 / (snr * snr);
    settings.method = methodStringToInt(methodStr);
    settings.lookBack = lookBack;
    settings.numSources = nSources;
    settings.onnxModelPath = onnxPath;

    std::print("Settings: method={}, SNR={:g}, lambda2={:g}, look-back={}\n", methodStr, snr, settings.lambda2, lookBack);

    if (!onnxPath.empty()) {
        if (!fileExists(path_from_utf8(onnxPath))) {
            std::print(stderr, "Error: ONNX model not found: {}\n", onnxPath);
            return 1;
        }
        std::print("ONNX model: {}\n", onnxPath);
    } else {
        std::print("\n"
                   "NOTE: No ONNX model specified (--onnx).\n"
                   "  Writing the paper's control estimate (no LSTM) instead of CMNE.\n"
                   "  For CMNE, train a model first:\n"
                   "\n"
                   "    mne_compute_cmne --mode train --fwd <fwd.fif> --cov <cov.fif> \\\n"
                   "                     --epochs <epo.fif> --onnx-out cmne_lstm.onnx\n"
                   "\n"
                   "  Then re-run compute with:\n"
                   "\n"
                   "    mne_compute_cmne --mode compute --fwd <fwd.fif> --cov <cov.fif> \\\n"
                   "                     --evoked <ave.fif> --onnx cmne_lstm.onnx --out <prefix>\n"
                   "\n");
    }

    // ── Compute CMNE ───────────────────────────────────────────────────
    std::print("Computing CMNE inverse solution …\n");
    std::fflush(stdout);

    InvCMNEResult result = InvCMNE::compute(
        matEvoked, matGain, matNoiseCovPicked, matSrcCov, settings);
    if (result.stcCmne.empty()) {
        std::print(stderr, "Error: the model cannot be applied to these data (see warnings above).\n");
        return 1;
    }

    // Propagate the source space and timing
    const std::vector<VectorXi> vertno = fwd.src.get_vertno();
    VectorXi vertices(nSources);
    vertices << vertno[0], vertno[1];
    for (InvSourceEstimate* stc : {&result.stcDspm, &result.stcCmne}) {
        stc->vertices = vertices;
        stc->nVerticesLh = static_cast<int>(vertno[0].size());
        stc->tmin = tmin;
        stc->tstep = tstep;
    }

    std::print("  dSPM: {} sources x {} samples\n", result.stcDspm.data.rows(), result.stcDspm.data.cols());
    std::print("  CMNE: {} sources x {} samples\n", result.stcCmne.data.rows(), result.stcCmne.data.cols());

    // ── Write STC files ────────────────────────────────────────────────
    if (!result.stcDspm.write_hemisphere_pair(path_from_utf8(outPrefix + "-dspm"))) {
        std::print(stderr, "Error: Failed to write dSPM STC.\n");
        return 1;
    }
    std::print("Wrote dSPM STC: {}-dspm-{{lh,rh}}.stc\n", outPrefix);

    if (!result.stcCmne.write_hemisphere_pair(path_from_utf8(outPrefix + "-cmne"))) {
        std::print(stderr, "Error: Failed to write CMNE STC.\n");
        return 1;
    }
    std::print("Wrote CMNE STC: {}-cmne-{{lh,rh}}.stc\n", outPrefix);

    std::print("Done.\n");
    return 0;
}

//=============================================================================================================
// TRAIN / FINETUNE MODE
//=============================================================================================================

static int doTrain(const CommandLineParser& parser, bool finetune)
{
    // ── Validate required arguments ────────────────────────────────────
    const std::string fwdPath = parser.value("fwd");
    const std::string covPath = parser.value("cov");
    const std::string epochsPath = parser.value("epochs");
    const std::string onnxOutPath = parser.value("onnx-out");

    if (fwdPath.empty() || covPath.empty() || epochsPath.empty()) {
        std::print(stderr, "Error: --fwd, --cov, and --epochs are required for {} mode.\n", finetune ? "finetune" : "train");
        return 1;
    }
    if (onnxOutPath.empty()) {
        std::print(stderr, "Error: --onnx-out is required for {} mode.\n", finetune ? "finetune" : "train");
        return 1;
    }

    const std::string finetuneOnnxPath = finetune
        ? parser.value("finetune")
        : std::string();

    if (finetune && finetuneOnnxPath.empty()) {
        std::print(stderr, "Error: --finetune <onnx_path> is required in finetune mode.\n");
        return 1;
    }

    if (finetune && !fileExists(path_from_utf8(finetuneOnnxPath))) {
        std::print(stderr, "Error: Fine-tune model not found: {}\n", finetuneOnnxPath);
        return 1;
    }

    // ── Resolve training script ────────────────────────────────────────
    const std::string cmneDir = resolveScriptDir();
    if (cmneDir.empty()) {
        std::print(stderr, "Error: Cannot find CMNE training package (scripts/ml/training/cmne/).\n");
        return 1;
    }

    const std::filesystem::path scriptPath = path_from_utf8(cmneDir) / "train_cmne_lstm.py";
    if (!fileExists(scriptPath)) {
        std::print(stderr, "Error: Training script not found: {}\n", utf8_from_path(scriptPath));
        return 1;
    }

    // ── Read settings from CLI ─────────────────────────────────────────
    const double snr = to_double(parser.value("snr")).value_or(0.0);
    const std::string methodStr = parser.value("method");
    const int lookBack = to_int(parser.value("look-back")).value_or(0);
    const int hiddenSize = to_int(parser.value("hidden")).value_or(0);
    const int numLayers = to_int(parser.value("layers")).value_or(0);
    const int trainEpochs = to_int(parser.value("train-epochs")).value_or(0);
    const double lr = to_double(parser.value("lr")).value_or(0.0);
    const int batchSize = to_int(parser.value("batch")).value_or(0);
    const std::string gtStcPfx = parser.value("gt-stc");
    const std::string pythonExe = parser.value("python");

    // ── Build Python script arguments ──────────────────────────────────
    mne::StringList args = {
        "--fwd", fwdPath,
        "--cov", covPath,
        "--epochs", epochsPath,
        "--out", onnxOutPath,
        "--look-back", std::to_string(lookBack),
        "--method", methodStr,
        "--snr", std::format("{:.6g}", snr),
        "--hidden", std::to_string(hiddenSize),
        "--layers", std::to_string(numLayers),
        "--train-epochs", std::to_string(trainEpochs),
        "--lr", std::format("{:.6g}", lr),
        "--batch", std::to_string(batchSize)};

    if (!gtStcPfx.empty()) {
        args.insert(args.end(), {"--gt-stc", gtStcPfx});
    }

    if (finetune) {
        args.insert(args.end(), {"--finetune", finetuneOnnxPath});
    }

    // ── Configure PythonRunner with venv ───────────────────────────────
    PythonRunnerConfig config;
    config.python_exe = pythonExe;
    config.venv_dir = path_from_utf8(cmneDir) / ".venv";
    config.package_dir = path_from_utf8(cmneDir);

    MLTrainer trainer(config);

    // ── Set up live progress reporting ─────────────────────────────────
    trainer.runner().set_line_callback([](PythonChannel channel, std::string_view line) {
        if (channel == PythonChannel::StdOut) {
            std::print("{}\n", line);
            std::fflush(stdout);
        } else {
            std::print(stderr, "{}\n", line);
            std::fflush(stderr);
        }
    });

    trainer.runner().set_progress_callback([](float pct, std::string_view msg) {
        std::print("\r[{:.1f}%] {}", static_cast<double>(pct), msg);
        std::fflush(stdout);
    });

    // ── Print summary ──────────────────────────────────────────────────
    std::print("=== CMNE LSTM {} ===\n", finetune ? "Fine-tuning" : "Training");
    std::print("  Forward   : {}\n", fwdPath);
    std::print("  Covariance: {}\n", covPath);
    std::print("  Epochs    : {}\n", epochsPath);
    std::print("  Output    : {}\n", onnxOutPath);
    std::print("  Method    : {}  SNR={:g}\n", methodStr, snr);
    std::print("  LSTM      : hidden={}  layers={}  look-back={}\n", hiddenSize, numLayers, lookBack);
    std::print("  Training  : epochs={}  lr={:g}  batch={}\n", trainEpochs, lr, batchSize);
    if (finetune) {
        std::print("  Fine-tune : {}\n", finetuneOnnxPath);
    }
    if (!gtStcPfx.empty()) {
        std::print("  Ground truth: {}\n", gtStcPfx);
    }
    std::print("  Script    : {}\n", utf8_from_path(scriptPath));
    std::print("  Venv      : {}\n", utf8_from_path(config.venv_dir));
    std::print("\n");
    std::fflush(stdout);

    // ── Run training ───────────────────────────────────────────────────
    PythonRunnerResult result = trainer.run(scriptPath, args);

    std::print("\n");

    if (result.success) {
        std::print("Training completed successfully.\n");
        const std::filesystem::path onnxOut = path_from_utf8(onnxOutPath);
        std::error_code ec;
        if (std::filesystem::exists(onnxOut, ec)) {
            const std::uintmax_t size = std::filesystem::file_size(onnxOut, ec);
            std::print("ONNX model: {} ({} KB)\n",
                       utf8_from_path(std::filesystem::absolute(onnxOut, ec).lexically_normal()),
                       ec ? 0 : size / 1024);
        }
    } else {
        std::print(stderr, "Training failed");
        if (result.timed_out) {
            std::print(stderr, " (timed out)");
        }
        std::print(stderr, " with exit code {}.\n", result.exit_code);

        if (!result.std_err.empty()) {
            std::print(stderr, "--- stderr ---\n{}\n", result.std_err);
        }
    }

    return result.success ? 0 : 1;
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    // ── Command-line parser ────────────────────────────────────────────
    CommandLineParser parser("Contextual MNE (CMNE) — compute source estimates or train the LSTM model.\n\n"
                             "Modes:\n"
                             "  compute   Compute dSPM + CMNE source estimates from evoked data (default)\n"
                             "  train     Train the CMNE LSTM from FIFF files and export to ONNX\n"
                             "  finetune  Continue training from an existing model");
    parser.add_help_option();
    parser.add_version_option();

    // -- Mode --
    parser.add_option({{"mode"}, "Operation mode: compute, train, or finetune.", "mode", "compute"});

    // -- Common data inputs --
    parser.add_option({{"fwd"}, "Forward solution FIFF file.", "file", {}});
    parser.add_option({{"cov"}, "Noise covariance FIFF file.", "file", {}});
    parser.add_option({{"snr"}, "Signal-to-noise ratio (default: 3.0).", "value", "3.0"});
    parser.add_option({{"method"}, "Inverse method: MNE, dSPM, sLORETA, eLORETA (default: dSPM).", "name", "dSPM"});
    parser.add_option({{"look-back"},
                       "Number of past time steps k for training and for the control estimate without --onnx; "
                       "a model carries its own k (default: 80).",
                       "k",
                       "80"});

    // -- Compute-mode options --
    parser.add_option({{"evoked"}, "Evoked data FIFF file (compute mode).", "file", {}});
    parser.add_option({{"onnx"},
                       "ONNX model with cmne_config metadata (compute mode); without it the control estimate is written.",
                       "file",
                       {}});
    parser.add_option({{"out"},
                       "Output STC prefix; writes <prefix>-dspm-{lh,rh}.stc and <prefix>-cmne-{lh,rh}.stc.",
                       "prefix",
                       {}});
    parser.add_option({{"setno"}, "Evoked data set number (default: 0).", "n", "0"});

    // -- Train / finetune options --
    parser.add_option({{"epochs"}, "MNE Epochs FIFF file (train/finetune mode).", "file", {}});
    parser.add_option({{"gt-stc"}, "Ground-truth STC prefix (optional; omit for pseudo-GT mode).", "prefix", {}});
    parser.add_option({{"onnx-out"}, "Output ONNX model path (train/finetune mode).", "file", "cmne_lstm.onnx"});
    parser.add_option({{"hidden"}, "LSTM hidden dimension (default: 256).", "n", "256"});
    parser.add_option({{"layers"}, "Number of LSTM layers (default: 1).", "n", "1"});
    parser.add_option({{"train-epochs"}, "Number of training epochs (default: 50).", "n", "50"});
    parser.add_option({{"lr"}, "Learning rate (default: 0.001).", "value", "0.001"});
    parser.add_option({{"batch"}, "Batch size (default: 64).", "n", "64"});
    parser.add_option({{"finetune"}, "Existing ONNX model to fine-tune from.", "file", {}});
    parser.add_option({{"python"}, "Python interpreter (default: python3).", "exe", "python3"});

    parser.process(argc, argv, "mne_compute_cmne", MNE_CPP_VERSION);

    // ── Dispatch ───────────────────────────────────────────────────────
    const std::string mode = to_lower(parser.value("mode"));

    if (mode == "compute") {
        return doCompute(parser);
    } else if (mode == "train") {
        return doTrain(parser, false);
    } else if (mode == "finetune") {
        return doTrain(parser, true);
    } else {
        std::print(stderr, "Error: Unknown mode '{}'. Use compute, train, or finetune.\n", mode);
        return 1;
    }
}
