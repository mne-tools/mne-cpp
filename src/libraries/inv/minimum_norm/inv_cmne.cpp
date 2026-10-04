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
 * and (outside WebAssembly builds) the @ref UTILSLIB::PythonRunner-based
 * training driver that invokes the upstream
 * @c train_cmne_lstm.py script. The training driver is a thin process
 * wrapper — the actual PyTorch model + ONNX export lives in Python and
 * this C++ side only forwards arguments and captures progress.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_cmne.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Eigenvalues>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
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

using namespace INVLIB;
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
    if (settings.onnxModelPath.isEmpty()) {
        qInfo() << "[InvCMNE] No model: control estimate with look-back" << settings.lookBack;
        cmne = controlEstimate(matDspm, settings.lookBack);
        sensing = zScoreRectify(matDspm);
        prediction = sensing;
    } else if (!applyCmne(matDspm, settings.onnxModelPath, sensing, prediction, cmne)) {
        qWarning() << "[InvCMNE] CMNE could not be applied; returning the dSPM estimate only.";
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
    qInfo() << "  [dSPM kernel] Eigendecomposition of noise covariance"
            << "(" << nChannels << "x" << nChannels << ") …";
    SelfAdjointEigenSolver<MatrixXd> eigSolver(matNoiseCov);
    VectorXd eigVals = eigSolver.eigenvalues();
    MatrixXd eigVecs = eigSolver.eigenvectors();

    // Regularize: clamp small eigenvalues
    double maxEig = eigVals.maxCoeff();
    double threshold = maxEig * 1e-10;
    VectorXd eigValsInvSqrt(nChannels);
    for (int i = 0; i < nChannels; ++i) {
        eigValsInvSqrt(i) = (eigVals(i) > threshold) ? 1.0 / std::sqrt(eigVals(i)) : 0.0;
    }

    MatrixXd matWhitener = eigVecs * eigValsInvSqrt.asDiagonal() * eigVecs.transpose();

    // Step 2: Whiten gain matrix
    qInfo() << "  [dSPM kernel] Whitening gain matrix …";
    MatrixXd matGainWhitened = matWhitener * matGain; // n_channels x n_sources

    // Step 3: MNE kernel
    qInfo() << "  [dSPM kernel] Computing MNE kernel (LDLT solve," << nChannels << "x" << nChannels << ") …";
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
    qInfo() << "  [dSPM kernel] Normalizing" << nSources << "source rows …";
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
                        const QString& onnxModelPath,
                        MatrixXd& sensing,
                        MatrixXd& prediction,
                        MatrixXd& cmne)
{
    MLLIB::MlOnnxModel model;
    if (!model.load(onnxModelPath)) {
        qWarning() << "[InvCMNE] Cannot load CMNE model" << onnxModelPath;
        return false;
    }
    const QJsonObject config = QJsonDocument::fromJson(model.metadata(QStringLiteral("cmne_config")).toUtf8()).object();
    const int k = config.value(QStringLiteral("look_back")).toInt();
    const int nSources = config.value(QStringLiteral("n_sources")).toInt();
    if (k <= 0 || nSources != matDspmData.rows()) {
        qWarning() << "[InvCMNE] Model" << onnxModelPath << "has no cmne_config for" << matDspmData.rows()
                   << "sources (look_back" << k << ", n_sources" << nSources << ").";
        return false;
    }
    if (matDspmData.cols() <= k) {
        qWarning() << "[InvCMNE] Need more than look_back =" << k << "samples, got" << matDspmData.cols();
        return false;
    }
    sensing = config.value(QStringLiteral("rectify")).toBool(true) ? zScoreRectify(matDspmData) : standardize(matDspmData);

    // The network sees float32 time-major windows of the contextual estimate b (Eq. 13).
    const MatrixXf q = sensing.cast<float>();
    MatrixXf b = q;
    MatrixXf pred = q;
    std::vector<float> window(static_cast<size_t>(k) * static_cast<size_t>(nSources));
    for (int t = k; t < q.cols(); ++t) {
        Map<MatrixXf>(window.data(), nSources, k) = b.middleCols(t - k, k);
        const MLLIB::MlTensor out = model.predict(MLLIB::MlTensor::view(window.data(), {1, k, nSources}));
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

UTILSLIB::PythonRunnerResult InvCMNE::trainLstm(
    const QString& fwdPath,
    const QString& covPath,
    const QString& epochsPath,
    const QString& outOnnxPath,
    const InvCMNESettings& settings,
    const QString& gtStcPrefix,
    int hiddenSize,
    int numLayers,
    int trainEpochs,
    double learningRate,
    int batchSize,
    const QString& finetuneOnnxPath,
    const QString& pythonExe)
{
    // Resolve training package directory (contains pyproject.toml + script)
    // Expected layout: <app_dir>/../scripts/ml/training/cmne/
    QString appDir = QCoreApplication::applicationDirPath();
    QString cmneDir = QDir(appDir).absoluteFilePath(
        QStringLiteral("../scripts/ml/training/cmne"));

    // Fallback: source tree relative to working directory
    if (!QFile::exists(QDir(cmneDir).absoluteFilePath(QStringLiteral("pyproject.toml")))) {
        cmneDir = QStringLiteral("scripts/ml/training/cmne");
    }

    QString scriptPath = QDir(cmneDir).absoluteFilePath(QStringLiteral("train_cmne_lstm.py"));

    if (!QFile::exists(scriptPath)) {
        UTILSLIB::PythonRunnerResult result;
        result.stdErr = QStringLiteral("Training script not found: ") + scriptPath;
        qWarning() << "[InvCMNE::trainLstm]" << result.stdErr;
        return result;
    }

    qDebug() << "[InvCMNE::trainLstm] Script:" << scriptPath;
    qDebug() << "[InvCMNE::trainLstm] Package dir:" << cmneDir;

    // Map method integer to string
    QString methodStr;
    switch (settings.method) {
        case 0:
            methodStr = QStringLiteral("MNE");
            break;
        case 1:
            methodStr = QStringLiteral("dSPM");
            break;
        case 2:
            methodStr = QStringLiteral("sLORETA");
            break;
        case 3:
            methodStr = QStringLiteral("eLORETA");
            break;
        default:
            methodStr = QStringLiteral("dSPM");
            break;
    }

    double snr = 1.0 / std::sqrt(settings.lambda2);

    // Build argument list matching train_cmne_lstm.py CLI
    QStringList args;
    args << QStringLiteral("--fwd") << fwdPath
         << QStringLiteral("--cov") << covPath
         << QStringLiteral("--epochs") << epochsPath
         << QStringLiteral("--out") << outOnnxPath
         << QStringLiteral("--look-back") << QString::number(settings.lookBack)
         << QStringLiteral("--method") << methodStr
         << QStringLiteral("--snr") << QString::number(snr, 'g', 6)
         << QStringLiteral("--hidden") << QString::number(hiddenSize)
         << QStringLiteral("--layers") << QString::number(numLayers)
         << QStringLiteral("--train-epochs") << QString::number(trainEpochs)
         << QStringLiteral("--lr") << QString::number(learningRate, 'g', 6)
         << QStringLiteral("--batch") << QString::number(batchSize);

    if (!gtStcPrefix.isEmpty()) {
        args << QStringLiteral("--gt-stc") << gtStcPrefix;
    }

    if (!finetuneOnnxPath.isEmpty()) {
        args << QStringLiteral("--finetune") << finetuneOnnxPath;
    }

    // Configure PythonRunner with venv + pyproject.toml
    // Venv lives inside the cmne package directory as .venv/
    UTILSLIB::PythonRunnerConfig config;
    config.pythonExe = pythonExe;
    config.venvDir = QDir(cmneDir).absoluteFilePath(QStringLiteral(".venv"));
    config.packageDir = cmneDir;

    MLLIB::MLTrainer trainer(config);

    return trainer.run(scriptPath, args);
}

#endif // !WASMBUILD
