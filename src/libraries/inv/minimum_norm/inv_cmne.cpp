//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_cmne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of the CMNE solver (dSPM kernel, z-score rectification, LSTM inference, optional training driver).
 *
 * Implements the dSPM kernel assembly, the per-source z-score
 * rectification, the sliding-window LSTM inference via ONNX Runtime
 * and (outside WebAssembly builds) the @ref mne::PythonRunner-based
 * training driver that invokes the upstream
 * @c train_cmne_lstm.py script. The training driver is a thin process
 * wrapper — the actual PyTorch model + ONNX export lives in Python and
 * this C++ side only forwards arguments and captures progress.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_cmne.h"

#include <base/log.h>
#include <base/json.h>
#include <base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigenvalues>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================


#include <algorithm>
#include <filesystem>
#include <format>
#include <limits>

//=============================================================================================================
// MNE-CPP INCLUDES
//=============================================================================================================

#include <ml/ml_onnx_model.h>
#include <ml/ml_tensor.h>

#ifndef WASMBUILD
#include <ml/ml_trainer.h>
#endif

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvCMNEResult InvCMNE::compute(
    const MatrixXd& matEvoked,
    const MatrixXd& matGain,
    const MatrixXd& matNoiseCov,
    const MatrixXd& matSrcCov,
    const InvCMNESettings& settings)
{
    InvCMNEResult result;
    result.matKernelDspm = computeDspmKernel(matGain, matNoiseCov, matSrcCov, settings.lambda2);
    const MatrixXd matDspm = result.matKernelDspm * matEvoked;
    const VectorXi vertices = VectorXi::LinSpaced(matDspm.rows(), 0, static_cast<int>(matDspm.rows()) - 1);
    result.stcDspm = InvSourceEstimate(matDspm, vertices, 0.0f, 1.0f);

    MatrixXd sensing, prediction, cmne;
    if (settings.onnxModelPath.empty()) {
        mne::Log::info("[InvCMNE] No model: control estimate with look-back {}", settings.lookBack);
        cmne = controlEstimate(matDspm, settings.lookBack);
        sensing = zScoreRectify(matDspm);
        prediction = sensing;
    } else if (!applyCmne(matDspm, settings.onnxModelPath, sensing, prediction, cmne)) {
        mne::Log::warning("[InvCMNE] CMNE could not be applied; returning the dSPM estimate only.");
        return result;
    }
    result.stcSensing = InvSourceEstimate(sensing, vertices, 0.0f, 1.0f);
    result.stcLstmPredict = InvSourceEstimate(prediction, vertices, 0.0f, 1.0f);
    result.stcCmne = InvSourceEstimate(cmne, vertices, 0.0f, 1.0f);
    return result;
}

//=============================================================================================================

MatrixXd InvCMNE::computeDspmKernel(
    const MatrixXd& matGain,
    const MatrixXd& matNoiseCov,
    const MatrixXd& matSrcCov,
    double lambda2)
{
    int nChannels = matGain.rows();
    int nSources = matGain.cols();

    // Step 1: Whiten noise covariance via eigendecomposition
    // C_n = V * D * V^T  ->  C_n^{-1/2} = V * D^{-1/2} * V^T
    mne::Log::info("  [dSPM kernel] Eigendecomposition of noise covariance ( {} x {} ) …", nChannels, nChannels);
    // Decompose the correlation matrix D^-1/2 C D^-1/2: MEG (T^2) and EEG (V^2) variances differ by 1e13,
    // so a cutoff relative to the largest eigenvalue of C would drop every MEG component.
    const VectorXd scale = matNoiseCov.diagonal().cwiseMax(std::numeric_limits<double>::min()).cwiseSqrt();
    const MatrixXd correlation = scale.cwiseInverse().asDiagonal() * matNoiseCov * scale.cwiseInverse().asDiagonal();
    SelfAdjointEigenSolver<MatrixXd> eigSolver(correlation);
    VectorXd eigVals = eigSolver.eigenvalues();
    MatrixXd eigVecs = eigSolver.eigenvectors();

    // Regularize: clamp small eigenvalues
    double maxEig = eigVals.maxCoeff();
    double threshold = maxEig * 1e-10;
    VectorXd eigValsInvSqrt(nChannels);
    for (int i = 0; i < nChannels; ++i) {
        eigValsInvSqrt(i) = (eigVals(i) > threshold) ? 1.0 / std::sqrt(eigVals(i)) : 0.0;
    }

    // W = (R^+)^1/2 D^-1/2 satisfies W C W^T = I on the retained subspace.
    MatrixXd matWhitener = eigVecs * eigValsInvSqrt.asDiagonal() * eigVecs.transpose() * scale.cwiseInverse().asDiagonal();

    // Step 2: Whiten gain matrix
    mne::Log::info("  [dSPM kernel] Whitening gain matrix …");
    MatrixXd matGainWhitened = matWhitener * matGain; // n_channels x n_sources

    // Step 3: MNE kernel
    mne::Log::info("  [dSPM kernel] Computing MNE kernel (LDLT solve, {} x {} ) …", nChannels, nChannels);
    // K = C_R * G_tilde^T * (G_tilde * C_R * G_tilde^T + lambda2 * I)^{-1}
    MatrixXd matGCR = matGainWhitened * matSrcCov;        // n_channels x n_sources
    MatrixXd matA = matGCR * matGainWhitened.transpose(); // n_channels x n_channels
    matA.diagonal().array() += lambda2;

    // Solve once: A^{-1} via LDLT, then K = (C_R * G_tilde^T) * A^{-1}
    auto ldlt = matA.ldlt();
    MatrixXd matK = (matSrcCov * matGainWhitened.transpose()) * ldlt.solve(matWhitener);

    // Step 4: dSPM normalization
    // noise_norm_i = sqrt((K * C_n * K^T)(i,i))
    // K_dSPM(i,:) = K(i,:) / noise_norm_i
    mne::Log::info("  [dSPM kernel] Normalizing {} source rows …", nSources);
    MatrixXd matKCn = matK * matNoiseCov; // n_sources x n_channels
    for (int i = 0; i < nSources; ++i) {
        double noiseNorm = std::sqrt(matKCn.row(i).dot(matK.row(i)));
        if (noiseNorm > 1e-10) {
            matK.row(i) /= noiseNorm;
        }
    }

    return matK; // n_sources x n_channels (dSPM kernel)
}

//=============================================================================================================

MatrixXd InvCMNE::standardize(const MatrixXd& matStcData)
{
    // Constant rows are centred but not scaled, as in cmne.standardize.
    const VectorXd mean = matStcData.rowwise().mean();
    MatrixXd result = matStcData.colwise() - mean;
    const VectorXd std = (result.array().square().rowwise().sum() / static_cast<double>(result.cols())).sqrt();
    for (int i = 0; i < result.rows(); ++i) {
        if (std(i) > 0.0)
            result.row(i) /= std(i);
    }
    return result;
}

//=============================================================================================================

MatrixXd InvCMNE::zScoreRectify(const MatrixXd& matStcData)
{
    return standardize(matStcData.cwiseAbs()); // Eq. 9
}

//=============================================================================================================

MatrixXd InvCMNE::controlEstimate(const MatrixXd& matDspmData, int lookBack)
{
    // Paper's control: q_t times the mean of the previous k sensing estimates (no LSTM, not recursive).
    const MatrixXd q = zScoreRectify(matDspmData);
    MatrixXd result = q;
    for (int t = lookBack; t < q.cols(); ++t)
        result.col(t) = q.col(t).cwiseProduct(q.middleCols(t - lookBack, lookBack).rowwise().mean());
    return result;
}

//=============================================================================================================

bool InvCMNE::applyCmne(const MatrixXd& matDspmData,
                        const std::string& onnxModelPath,
                        MatrixXd& sensing,
                        MatrixXd& prediction,
                        MatrixXd& cmne)
{
    mne::MlOnnxModel model;
    if (!model.load(onnxModelPath)) {
        mne::Log::warning("[InvCMNE] Cannot load CMNE model {}", onnxModelPath);
        return false;
    }
    const mne::JsonDocument metadata = mne::JsonDocument::from_json(model.metadata("cmne_config"));
    const mne::JsonObject& config = metadata.object();
    const int k = config.value("look_back").to_int();
    const int nSources = config.value("n_sources").to_int();
    if (k <= 0 || nSources != matDspmData.rows()) {
        mne::Log::warning("[InvCMNE] Model {} has no cmne_config for {} sources (look_back {} , n_sources {} ).", onnxModelPath, matDspmData.rows(), k, nSources);
        return false;
    }
    if (matDspmData.cols() <= k) {
        mne::Log::warning("[InvCMNE] Need more than look_back = {} samples, got {}", k, matDspmData.cols());
        return false;
    }
    sensing = config.value("rectify").to_bool(true) ? zScoreRectify(matDspmData) : standardize(matDspmData);

    // The network sees float32 time-major windows of the contextual estimate b (Eq. 13).
    const MatrixXf q = sensing.cast<float>();
    MatrixXf b = q;
    MatrixXf pred = q;
    std::vector<float> window(static_cast<size_t>(k) * static_cast<size_t>(nSources));
    for (int t = k; t < q.cols(); ++t) {
        Map<MatrixXf>(window.data(), nSources, k) = b.middleCols(t - k, k);
        const mne::MlTensor out = model.predict(mne::MlTensor::view(window.data(), {1, k, nSources}));
        const Map<const VectorXf> p(out.data(), nSources);
        pred.col(t) = p;
        const VectorXf w = p.cwiseAbs();
        const float maxW = std::max(w.maxCoeff(), std::numeric_limits<float>::min());
        b.col(t) = (w / maxW).cwiseProduct(q.col(t)); // Eqs. 10-11
    }
    prediction = pred.cast<double>();
    cmne = b.cast<double>();
    return true;
}

//=============================================================================================================

#ifndef WASMBUILD

mne::PythonRunnerResult InvCMNE::train_lstm(
    const std::filesystem::path& fwdPath,
    const std::filesystem::path& covPath,
    const std::filesystem::path& epochsPath,
    const std::filesystem::path& outOnnxPath,
    const InvCMNESettings& settings,
    const std::string& gtStcPrefix,
    int hiddenSize,
    int numLayers,
    int trainEpochs,
    double learningRate,
    int batchSize,
    const std::filesystem::path& finetuneOnnxPath,
    const std::string& pythonExe)
{
    namespace fs = std::filesystem;
    using mne::utf8FromPath;

    // Training package directory (pyproject.toml + script): <app_dir>/../scripts/ml/training/cmne/, with the source
    // tree relative to the working directory as fallback
    fs::path cmneDir = mne::executable_dir().parent_path() / "scripts" / "ml" / "training" / "cmne";
    std::error_code ec;
    if (!fs::exists(cmneDir / "pyproject.toml", ec)) {
        cmneDir = fs::path("scripts") / "ml" / "training" / "cmne";
    }
    cmneDir = fs::absolute(cmneDir, ec).lexically_normal();
    const fs::path scriptPath = cmneDir / "train_cmne_lstm.py";

    if (!fs::exists(scriptPath, ec)) {
        mne::PythonRunnerResult result;
        result.std_err = "Training script not found: " + utf8FromPath(scriptPath);
        mne::Log::warning("[InvCMNE::train_lstm] {}", result.std_err);
        return result;
    }

    mne::Log::debug("[InvCMNE::train_lstm] Script: {}", utf8FromPath(scriptPath));
    mne::Log::debug("[InvCMNE::train_lstm] Package dir: {}", utf8FromPath(cmneDir));

    const char* method = "dSPM";
    switch (settings.method) {
        case 0:
            method = "MNE";
            break;
        case 2:
            method = "sLORETA";
            break;
        case 3:
            method = "eLORETA";
            break;
        default:
            break;
    }

    const double snr = 1.0 / std::sqrt(settings.lambda2);

    // Argument list of the train_cmne_lstm.py command line
    mne::StringList args = {
        "--fwd", utf8FromPath(fwdPath),
        "--cov", utf8FromPath(covPath),
        "--epochs", utf8FromPath(epochsPath),
        "--out", utf8FromPath(outOnnxPath),
        "--look-back", std::to_string(settings.lookBack),
        "--method", method,
        "--snr", std::format("{:.6g}", snr),
        "--hidden", std::to_string(hiddenSize),
        "--layers", std::to_string(numLayers),
        "--train-epochs", std::to_string(trainEpochs),
        "--lr", std::format("{:.6g}", learningRate),
        "--batch", std::to_string(batchSize)};
    if (!gtStcPrefix.empty()) {
        args.insert(args.end(), {"--gt-stc", gtStcPrefix});
    }
    if (!finetuneOnnxPath.empty()) {
        args.insert(args.end(), {"--finetune", utf8FromPath(finetuneOnnxPath)});
    }

    // The venv lives inside the cmne package directory as .venv/
    mne::PythonRunnerConfig config;
    config.python_exe = pythonExe;
    config.venv_dir = cmneDir / ".venv";
    config.package_dir = cmneDir;

    mne::MLTrainer trainer(config);
    return trainer.run(scriptPath, args);
}

#endif // !WASMBUILD
