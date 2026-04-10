//=============================================================================================================
/**
 * @file     inv_cmne.h
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
 * @brief    InvCmne class declaration (Contextual MNE, Dinh et al. 2021).
 *
 */

#ifndef INV_CMNE_H
#define INV_CMNE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "../inv_source_estimate.h"
#include "inv_cmne_settings.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace UTILSLIB { struct PythonRunnerResult; }

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Result container for the CMNE inverse solver.
 *
 * @brief CMNE result
 */
struct INVSHARED_EXPORT InvCmneResult
{
    InvSourceEstimate stcDspm;       /**< Uncorrected dSPM estimate. */
    InvSourceEstimate stcCmne;       /**< CMNE-corrected estimate. */
    InvSourceEstimate stcLstmPredict;/**< Raw LSTM prediction (diagnostics). */
    Eigen::MatrixXd matKernelDspm;   /**< Static dSPM kernel (n_sources x n_channels). */
};

//=============================================================================================================
/**
 * Contextual Minimum Norm Estimate (CMNE) inverse solver.
 *
 * Implements the algorithm from:
 *   Dinh et al. "Contextual Minimum-Norm Estimates (CMNE): A Deep Learning Method
 *   for Source Estimation in Neuroimaging", 2021.
 *
 * @brief CMNE inverse solver
 */
class INVSHARED_EXPORT InvCmne
{
public:
    //=========================================================================================================
    /**
     * Compute CMNE inverse solution.
     *
     * @param[in] matEvoked      Evoked data (n_channels x n_times).
     * @param[in] matGain        Forward gain matrix (n_channels x n_sources).
     * @param[in] matNoiseCov    Noise covariance (n_channels x n_channels).
     * @param[in] matSrcCov      Source covariance (n_sources x n_sources, diagonal).
     * @param[in] settings       CMNE settings.
     *
     * @return InvCmneResult containing dSPM and CMNE source estimates.
     */
    static InvCmneResult compute(
        const Eigen::MatrixXd& matEvoked,
        const Eigen::MatrixXd& matGain,
        const Eigen::MatrixXd& matNoiseCov,
        const Eigen::MatrixXd& matSrcCov,
        const InvCmneSettings& settings);

    //=========================================================================================================
    /**
     * Apply LSTM-based temporal correction to z-scored rectified dSPM data.
     *
     * @param[in] matDspmData    Z-scored rectified dSPM data (n_sources x n_times).
     * @param[in] onnxModelPath  Path to ONNX model file.
     * @param[in] lookBack       Number of past time steps (k).
     *
     * @return Corrected source data (n_sources x n_times).
     */
    static Eigen::MatrixXd applyLstmCorrection(
        const Eigen::MatrixXd& matDspmData,
        const QString& onnxModelPath,
        int lookBack);

    //=========================================================================================================
    /**
     * Train the CMNE LSTM model by invoking the Python training script.
     *
     * This is a convenience wrapper that calls
     * ``scripts/ml/training/train_cmne_lstm.py`` via UTILSLIB::PythonRunner.
     * The heavy lifting (PyTorch LSTM training + ONNX export) happens in
     * Python; C++ only launches the process and streams its output.
     *
     * @param[in] fwdPath        Path to forward solution FIFF file.
     * @param[in] covPath        Path to noise covariance FIFF file.
     * @param[in] epochsPath     Path to epochs FIFF file.
     * @param[in] outOnnxPath    Desired output path for the ONNX model.
     * @param[in] settings       CMNE settings (look-back, method, SNR are forwarded).
     * @param[in] gtStcPrefix    Ground-truth STC prefix (optional; empty = simulation mode).
     * @param[in] hiddenSize     LSTM hidden dimension (default 256).
     * @param[in] numLayers      LSTM layers (default 1).
     * @param[in] trainEpochs    Number of training epochs (default 50).
     * @param[in] learningRate   Learning rate (default 1e-3).
     * @param[in] batchSize      Batch size (default 64).
     * @param[in] finetuneOnnxPath  Existing ONNX model to fine-tune from (optional).
     * @param[in] pythonExe      Python interpreter (default "python3").
     *
     * @return PythonRunnerResult with exit code, captured output and progress.
     */
    static UTILSLIB::PythonRunnerResult trainLstm(
        const QString& fwdPath,
        const QString& covPath,
        const QString& epochsPath,
        const QString& outOnnxPath,
        const InvCmneSettings& settings,
        const QString& gtStcPrefix = {},
        int hiddenSize = 256,
        int numLayers = 1,
        int trainEpochs = 50,
        double learningRate = 1e-3,
        int batchSize = 64,
        const QString& finetuneOnnxPath = {},
        const QString& pythonExe = QStringLiteral("python3"));

private:
    //=========================================================================================================
    /**
     * Compute dSPM kernel.
     *
     * @param[in] matGain        Forward gain matrix (n_channels x n_sources).
     * @param[in] matNoiseCov    Noise covariance (n_channels x n_channels).
     * @param[in] matSrcCov      Source covariance (n_sources x n_sources).
     * @param[in] lambda2        Tikhonov regularisation parameter.
     *
     * @return dSPM kernel (n_sources x n_channels).
     */
    static Eigen::MatrixXd computeDspmKernel(
        const Eigen::MatrixXd& matGain,
        const Eigen::MatrixXd& matNoiseCov,
        const Eigen::MatrixXd& matSrcCov,
        double lambda2);

    //=========================================================================================================
    /**
     * Z-score rectify source data (absolute value, then z-score per source).
     *
     * @param[in] matStcData     Source data (n_sources x n_times).
     *
     * @return Z-scored rectified data.
     */
    static Eigen::MatrixXd zScoreRectify(
        const Eigen::MatrixXd& matStcData);
};

} // namespace INVLIB

#endif // INV_CMNE_H
