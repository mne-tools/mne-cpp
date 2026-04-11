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
 * @brief    InvCMNE class definition (Contextual MNE, Dinh et al. 2021).
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

    int nChannels = matGain.rows();
    int nSources  = matGain.cols();
    int nTimes    = matEvoked.cols();

    // Step 1: Compute dSPM kernel
    qInfo() << "[InvCMNE] Step 1/4: Computing dSPM kernel"
            << "(" << nChannels << "ch x" << nSources << "src, lambda2="
            << settings.lambda2 << ") …";
    MatrixXd matKernelDspm = computeDspmKernel(matGain, matNoiseCov, matSrcCov, settings.lambda2);
    result.matKernelDspm = matKernelDspm;
    qInfo() << "[InvCMNE] Step 1/4: dSPM kernel done"
            << "(" << matKernelDspm.rows() << "x" << matKernelDspm.cols() << ").";

    // Step 2: Apply kernel to evoked data -> dSPM source estimate
    qInfo() << "[InvCMNE] Step 2/4: Projecting evoked data to source space"
            << "(" << nTimes << "time points) …";
    MatrixXd matDspmData = matKernelDspm * matEvoked;  // n_sources x n_times

    // Build dSPM source estimate
    VectorXi vertices = VectorXi::LinSpaced(matDspmData.rows(), 0, matDspmData.rows() - 1);
    result.stcDspm = InvSourceEstimate(matDspmData, vertices, 0.0f, 1.0f);
    qInfo() << "[InvCMNE] Step 2/4: dSPM source estimate done"
            << "(" << matDspmData.rows() << "sources x" << matDspmData.cols() << "samples).";

    // Step 3: Z-score rectify
    qInfo() << "[InvCMNE] Step 3/4: Z-score rectifying source data …";
    MatrixXd matZScored = zScoreRectify(matDspmData);
    qInfo() << "[InvCMNE] Step 3/4: Z-score rectification done.";

    // Step 4: Apply LSTM correction if model available and enough time points
    MatrixXd matCmneData;

    if (!settings.onnxModelPath.isEmpty() && nTimes >= settings.lookBack) {
        qInfo() << "[InvCMNE] Step 4/4: Applying LSTM temporal correction"
               << "(look-back=" << settings.lookBack << ","
               << (nTimes - settings.lookBack) << "correctable time points) …";
        matCmneData = applyLstmCorrection(matZScored, settings.onnxModelPath, settings.lookBack);

        // Store raw LSTM prediction for diagnostics
        result.stcLstmPredict = InvSourceEstimate(matCmneData, vertices, 0.0f, 1.0f);
        qInfo() << "[InvCMNE] Step 4/4: LSTM correction done.";
    } else {
        // No correction possible — CMNE falls back to dSPM
        matCmneData = matDspmData;

        if (settings.onnxModelPath.isEmpty()) {
            qInfo() << "[InvCMNE] Step 4/4: No ONNX model — using moving-average correction.";
            matCmneData = applyLstmCorrection(matZScored, QString(), settings.lookBack);
            qInfo() << "[InvCMNE] Step 4/4: Moving-average correction done.";
        } else {
            qInfo() << "[InvCMNE] Step 4/4: Not enough time points for lookBack window"
                    << "(need" << settings.lookBack << ", have" << nTimes << ").";
        }
    }

    // Build CMNE source estimate
    result.stcCmne = InvSourceEstimate(matCmneData, vertices, 0.0f, 1.0f);

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
    MatrixXd matGainWhitened = matWhitener * matGain;  // n_channels x n_sources

    // Step 3: MNE kernel
    qInfo() << "  [dSPM kernel] Computing MNE kernel (LDLT solve," << nChannels << "x" << nChannels << ") …";
    // K = C_R * G_tilde^T * (G_tilde * C_R * G_tilde^T + lambda2 * I)^{-1}
    MatrixXd matGCR = matGainWhitened * matSrcCov;                         // n_channels x n_sources
    MatrixXd matA = matGCR * matGainWhitened.transpose();                  // n_channels x n_channels
    matA.diagonal().array() += lambda2;

    // Solve once: A^{-1} via LDLT, then K = (C_R * G_tilde^T) * A^{-1}
    auto ldlt = matA.ldlt();
    MatrixXd matK = (matSrcCov * matGainWhitened.transpose()) * ldlt.solve(MatrixXd::Identity(nChannels, nChannels));

    // Step 4: dSPM normalization
    // noise_norm_i = sqrt((K * C_n * K^T)(i,i))
    // K_dSPM(i,:) = K(i,:) / noise_norm_i
    qInfo() << "  [dSPM kernel] Normalizing" << nSources << "source rows …";
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

MatrixXd InvCMNE::zScoreRectify(const MatrixXd& matStcData)
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

MatrixXd InvCMNE::applyLstmCorrection(
    const MatrixXd& matDspmData,
    const QString& onnxModelPath,
    int lookBack)
{
    int nSources = matDspmData.rows();
    int nTimes = matDspmData.cols();

    MatrixXd result = matDspmData;  // copy — for t < lookBack: identity (no correction)

    int nCorrectableSteps = nTimes - lookBack;
    int reportInterval = qMax(1, nCorrectableSteps / 10);  // report ~10 times

    // Try to load ONNX model for LSTM inference
    MLLIB::MlOnnxModel lstmModel;
    bool useOrt = false;

    if (!onnxModelPath.isEmpty()) {
        if (lstmModel.load(onnxModelPath)) {
            useOrt = true;
            qInfo() << "  [LSTM correction] ONNX model loaded — using LSTM inference.";
        } else {
            qWarning() << "  [LSTM correction] Failed to load ONNX model — falling back to moving average.";
        }
    } else {
        qInfo() << "  [LSTM correction] No ONNX model path — using moving average.";
    }

    // Pre-allocate input buffer for ORT: shape [1, lookBack, nSources] (batch, seq, features)
    // Row-major layout: [seq][features]
    std::vector<float> inputBuf;
    if (useOrt) {
        inputBuf.resize(static_cast<size_t>(lookBack) * static_cast<size_t>(nSources));
    }

    // For t >= lookBack: apply temporal correction
    for (int t = lookBack; t < nTimes; ++t) {
        int step = t - lookBack;
        if (step % reportInterval == 0 || t == nTimes - 1) {
            double pct = 100.0 * (step + 1) / nCorrectableSteps;
            qInfo().noquote() << QString("  [LSTM correction] %1% (%2/%3 time steps)")
                .arg(pct, 0, 'f', 0).arg(step + 1).arg(nCorrectableSteps);
        }

        VectorXd prediction;

        if (useOrt) {
            // Fill input buffer: double→float, column-major→row-major
            // Layout: inputBuf[k * nSources + s] = matDspmData(s, t - lookBack + k)
            for (int k = 0; k < lookBack; ++k) {
                int col = t - lookBack + k;
                for (int s = 0; s < nSources; ++s) {
                    inputBuf[static_cast<size_t>(k) * static_cast<size_t>(nSources)
                             + static_cast<size_t>(s)] = static_cast<float>(result(s, col));
                }
            }

            // Create MlTensor view over the pre-allocated buffer — zero-copy
            std::vector<int64_t> inputShape = {1, static_cast<int64_t>(lookBack),
                                               static_cast<int64_t>(nSources)};
            MLLIB::MlTensor inputTensor = MLLIB::MlTensor::view(inputBuf.data(), inputShape);

            // Run LSTM inference
            MLLIB::MlTensor outputTensor = lstmModel.predict(inputTensor);

            // Convert output to Eigen VectorXd
            // Expected output shape: [1, nSources] or [nSources]
            prediction.resize(nSources);
            const float* outPtr = outputTensor.data();
            for (int s = 0; s < nSources; ++s) {
                prediction(s) = static_cast<double>(outPtr[s]);
            }
        } else {
            // Moving average fallback (control estimate from paper)
            MatrixXd window = result.middleCols(t - lookBack, lookBack);
            prediction = window.rowwise().mean();
        }

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

    if (!finetuneOnnxPath.isEmpty()) {
        args << QStringLiteral("--finetune") << finetuneOnnxPath;
    }

    // Configure PythonRunner with venv + pyproject.toml
    // Venv lives inside the cmne package directory as .venv/
    UTILSLIB::PythonRunnerConfig config;
    config.pythonExe  = pythonExe;
    config.venvDir    = QDir(cmneDir).absoluteFilePath(QStringLiteral(".venv"));
    config.packageDir = cmneDir;

    MLLIB::MLTrainer trainer(config);

    return trainer.run(scriptPath, args);
}

#endif // !WASMBUILD
