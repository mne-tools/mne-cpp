//=============================================================================================================
/**
 * @file     inv_cmne.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    InvCmne class definition (Contextual MNE, Dinh et al. 2021).
 *
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

//=============================================================================================================
// MNE-CPP INCLUDES
//=============================================================================================================

#include <ml/ml_trainer.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvCmneResult InvCmne::compute(
    const MatrixXd& matEvoked,
    const MatrixXd& matGain,
    const MatrixXd& matNoiseCov,
    const MatrixXd& matSrcCov,
    const InvCmneSettings& settings)
{
    InvCmneResult result;

    // Step 1: Compute dSPM kernel
    MatrixXd matKernelDspm = computeDspmKernel(matGain, matNoiseCov, matSrcCov, settings.lambda2);
    result.matKernelDspm = matKernelDspm;

    // Step 2: Apply kernel to evoked data -> dSPM source estimate
    MatrixXd matDspmData = matKernelDspm * matEvoked;  // n_sources x n_times

    // Build dSPM source estimate
    VectorXi vertices = VectorXi::LinSpaced(matDspmData.rows(), 0, matDspmData.rows() - 1);
    result.stcDspm = InvSourceEstimate(matDspmData, vertices, 0.0f, 1.0f);

    // Step 3: Z-score rectify
    MatrixXd matZScored = zScoreRectify(matDspmData);

    // Step 4: Apply LSTM correction if model available and enough time points
    int nTimes = matDspmData.cols();
    MatrixXd matCmneData;

    if (!settings.onnxModelPath.isEmpty() && nTimes >= settings.lookBack) {
        matCmneData = applyLstmCorrection(matZScored, settings.onnxModelPath, settings.lookBack);

        // Store raw LSTM prediction for diagnostics
        result.stcLstmPredict = InvSourceEstimate(matCmneData, vertices, 0.0f, 1.0f);
    } else {
        // No correction possible — CMNE falls back to dSPM
        matCmneData = matDspmData;

        if (settings.onnxModelPath.isEmpty()) {
            qDebug() << "[InvCmne::compute] No ONNX model path provided, returning uncorrected dSPM.";
        } else {
            qDebug() << "[InvCmne::compute] Not enough time points for lookBack window"
                     << "(need" << settings.lookBack << ", have" << nTimes << ").";
        }
    }

    // Build CMNE source estimate
    result.stcCmne = InvSourceEstimate(matCmneData, vertices, 0.0f, 1.0f);

    return result;
}

//=============================================================================================================

MatrixXd InvCmne::computeDspmKernel(
    const MatrixXd& matGain,
    const MatrixXd& matNoiseCov,
    const MatrixXd& matSrcCov,
    double lambda2)
{
    int nChannels = matGain.rows();
    int nSources = matGain.cols();

    // Step 1: Whiten noise covariance via eigendecomposition
    // C_n = V * D * V^T  ->  C_n^{-1/2} = V * D^{-1/2} * V^T
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
    MatrixXd matGainWhitened = matWhitener * matGain;  // n_channels x n_sources

    // Step 3: MNE kernel
    // K = C_R * G_tilde^T * (G_tilde * C_R * G_tilde^T + lambda2 * I)^{-1}
    MatrixXd matGCR = matGainWhitened * matSrcCov;                         // n_channels x n_sources
    MatrixXd matA = matGCR * matGainWhitened.transpose();                  // n_channels x n_channels
    matA.diagonal().array() += lambda2;

    // Solve: K = C_R * G_tilde^T * A^{-1}
    // Equivalent to: K^T = A^{-1} * G_tilde * C_R, then transpose
    MatrixXd matKT = matA.ldlt().solve(matGCR);                            // n_channels x n_sources
    MatrixXd matK = matSrcCov * matGainWhitened.transpose() * matA.ldlt().solve(MatrixXd::Identity(nChannels, nChannels));
    // Simplification: K = (C_R * G_tilde^T) * A^{-1}
    matK = (matSrcCov * matGainWhitened.transpose()) * matA.ldlt().solve(MatrixXd::Identity(nChannels, nChannels));

    // Step 4: dSPM normalization
    // noise_norm_i = sqrt((K * C_n * K^T)(i,i))
    // K_dSPM(i,:) = K(i,:) / noise_norm_i
    MatrixXd matKCn = matK * matNoiseCov;  // n_sources x n_channels
    for (int i = 0; i < nSources; ++i) {
        double noiseNorm = std::sqrt(matKCn.row(i).dot(matK.row(i)));
        if (noiseNorm > 1e-10) {
            matK.row(i) /= noiseNorm;
        }
    }

    return matK;  // n_sources x n_channels (dSPM kernel)
}

//=============================================================================================================

MatrixXd InvCmne::zScoreRectify(const MatrixXd& matStcData)
{
    int nSources = matStcData.rows();
    int nTimes = matStcData.cols();

    MatrixXd matResult(nSources, nTimes);

    for (int i = 0; i < nSources; ++i) {
        // Absolute value
        VectorXd absRow = matStcData.row(i).cwiseAbs();

        // Mean and standard deviation across time
        double mu = absRow.mean();
        double variance = (absRow.array() - mu).square().mean();
        double sigma = std::sqrt(variance);

        // Z-score (guard against zero std)
        double denom = std::max(sigma, 1e-10);
        matResult.row(i) = (absRow.array() - mu) / denom;
    }

    return matResult;
}

//=============================================================================================================

MatrixXd InvCmne::applyLstmCorrection(
    const MatrixXd& matDspmData,
    const QString& onnxModelPath,
    int lookBack)
{
    Q_UNUSED(onnxModelPath)

    int nSources = matDspmData.rows();
    int nTimes = matDspmData.cols();

    MatrixXd result = matDspmData;  // copy — for t < lookBack: identity (no correction)

    // For t >= lookBack: apply temporal correction
    for (int t = lookBack; t < nTimes; ++t) {
        // Extract window: last lookBack columns before t
        MatrixXd window = result.middleCols(t - lookBack, lookBack);

        // TODO: Replace with actual ONNX inference when ml library is ready
        // For now, use moving average as control estimate (from paper)
        VectorXd prediction = window.rowwise().mean();

        // Normalize prediction (Eq. 12)
        double maxVal = prediction.cwiseAbs().maxCoeff();
        if (maxVal > 1e-10) {
            prediction = prediction.cwiseAbs() / maxVal;
        }

        // CMNE correction: element-wise product (Eq. 13)
        result.col(t) = prediction.cwiseProduct(matDspmData.col(t));
    }

    return result;
}

//=============================================================================================================

UTILSLIB::PythonRunnerResult InvCmne::trainLstm(
    const QString& fwdPath,
    const QString& covPath,
    const QString& epochsPath,
    const QString& outOnnxPath,
    const InvCmneSettings& settings,
    const QString& gtStcPrefix,
    int hiddenSize,
    int numLayers,
    int trainEpochs,
    double learningRate,
    int batchSize,
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
        qWarning() << "[InvCmne::trainLstm]" << result.stdErr;
        return result;
    }

    qDebug() << "[InvCmne::trainLstm] Script:" << scriptPath;
    qDebug() << "[InvCmne::trainLstm] Package dir:" << cmneDir;

    // Map method integer to string
    QString methodStr;
    switch (settings.method) {
        case 0: methodStr = QStringLiteral("MNE");     break;
        case 1: methodStr = QStringLiteral("dSPM");    break;
        case 2: methodStr = QStringLiteral("sLORETA"); break;
        case 3: methodStr = QStringLiteral("eLORETA"); break;
        default: methodStr = QStringLiteral("dSPM");   break;
    }

    double snr = 1.0 / std::sqrt(settings.lambda2);

    // Build argument list matching train_cmne_lstm.py CLI
    QStringList args;
    args << QStringLiteral("--fwd")          << fwdPath
         << QStringLiteral("--cov")          << covPath
         << QStringLiteral("--epochs")       << epochsPath
         << QStringLiteral("--out")          << outOnnxPath
         << QStringLiteral("--look-back")    << QString::number(settings.lookBack)
         << QStringLiteral("--method")       << methodStr
         << QStringLiteral("--snr")          << QString::number(snr, 'g', 6)
         << QStringLiteral("--hidden")       << QString::number(hiddenSize)
         << QStringLiteral("--layers")       << QString::number(numLayers)
         << QStringLiteral("--train-epochs") << QString::number(trainEpochs)
         << QStringLiteral("--lr")           << QString::number(learningRate, 'g', 6)
         << QStringLiteral("--batch")        << QString::number(batchSize);

    if (!gtStcPrefix.isEmpty()) {
        args << QStringLiteral("--gt-stc") << gtStcPrefix;
    }

    // Configure PythonRunner with venv + pyproject.toml
    // Venv lives inside the cmne package directory as .venv/
    UTILSLIB::PythonRunnerConfig config;
    config.pythonExe  = pythonExe;
    config.venvDir    = QDir(cmneDir).absoluteFilePath(QStringLiteral(".venv"));
    config.packageDir = cmneDir;

    MLLIB::MlTrainer trainer(config);

    return trainer.run(scriptPath, args);
}
