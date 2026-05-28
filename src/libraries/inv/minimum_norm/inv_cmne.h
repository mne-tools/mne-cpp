//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_cmne.h
 * @since April 2026
 * @brief Contextual Minimum-Norm Estimate (CMNE) inverse solver — deep-learning-corrected dSPM (Dinh et al. 2021).
 *
 * @ref INVLIB::InvCMNE implements the CMNE algorithm of Dinh et al.,
 * @em Contextual Minimum-Norm Estimates: A Deep Learning Method for
 * Source Estimation in Neuroimaging, 2021. The static @c compute method
 * combines a closed-form dSPM kernel with an LSTM correction step that
 * runs in ONNX Runtime: dSPM is computed first, the time-courses are
 * z-scored and rectified, an LSTM consumes a sliding @c lookBack window
 * of past time samples and outputs the contextual correction, and the
 * final CMNE estimate is the LSTM-modulated dSPM. The
 * @c trainLstm helper drives the Python training pipeline
 * (@c scripts/ml/training/train_cmne_lstm.py) through
 * @ref UTILSLIB::PythonRunner so the full train-and-deploy cycle is
 * reachable from C++ without leaving the mne-cpp process.
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

#ifndef WASMBUILD
namespace UTILSLIB { struct PythonRunnerResult; }
#endif

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
struct INVSHARED_EXPORT InvCMNEResult
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
class INVSHARED_EXPORT InvCMNE
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
     * @return InvCMNEResult containing dSPM and CMNE source estimates.
     */
    static InvCMNEResult compute(
        const Eigen::MatrixXd& matEvoked,
        const Eigen::MatrixXd& matGain,
        const Eigen::MatrixXd& matNoiseCov,
        const Eigen::MatrixXd& matSrcCov,
        const InvCMNESettings& settings);

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
#ifndef WASMBUILD
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
        const InvCMNESettings& settings,
        const QString& gtStcPrefix = {},
        int hiddenSize = 256,
        int numLayers = 1,
        int trainEpochs = 50,
        double learningRate = 1e-3,
        int batchSize = 64,
        const QString& finetuneOnnxPath = {},
        const QString& pythonExe = QStringLiteral("python3"));
#endif

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
