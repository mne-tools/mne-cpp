//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_cmne.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Contextual Minimum-Norm Estimate (CMNE) inverse solver — deep-learning-corrected dSPM (Dinh et al. 2021).
 *
 * @ref INVLIB::InvCMNE implements the CMNE algorithm of Dinh et al.,
 * @em Contextual Minimum-Norm Estimates: A Deep Learning Method for
 * Source Estimation in Neuroimaging, 2021, and matches the reference
 * implementation, the @c cmne Python package (0.2.1). The static @c compute
 * method combines a closed-form dSPM kernel with the contextual re-weighting
 * of Eqs. 9-13: the dSPM estimate is rectified and z-scored (q), and for
 * t >= k an LSTM, run in ONNX Runtime, predicts the next estimate from the
 * previous k contextual estimates b; q_t is weighted by the max-normalised
 * |prediction|. Without a model the paper's control estimate is returned. The
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
namespace UTILSLIB
{
struct PythonRunnerResult;
}
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
    InvSourceEstimate stcDspm;        /**< Uncorrected dSPM estimate. */
    InvSourceEstimate stcSensing;     /**< Rectified, z-scored dSPM q_t (Eq. 9). */
    InvSourceEstimate stcCmne;        /**< Contextual estimate b_t (Eqs. 10-13), or the control estimate without a model. */
    InvSourceEstimate stcLstmPredict; /**< Raw LSTM prediction (q_t for the first k samples). */
    Eigen::MatrixXd matKernelDspm;    /**< Static dSPM kernel (n_sources x n_channels). */
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
     * Contextual estimate of Eqs. 9-13, as cmne.apply_cmne with normalised weights.
     *
     * The model must be exported by cmne.export_onnx: look-back, source count and
     * rectification are read from its @c cmne_config metadata.
     *
     * @param[in] matDspmData    Signed dSPM estimate (n_sources x n_times).
     * @param[in] onnxModelPath  Path to the ONNX model.
     * @param[out] sensing       q_t (Eq. 9).
     * @param[out] prediction    LSTM prediction (q_t for the first k samples).
     * @param[out] cmne          Contextual estimate b_t.
     *
     * @return False if the model cannot be loaded, does not fit the data, or there are no more than k samples.
     */
    static bool applyCmne(const Eigen::MatrixXd& matDspmData,
                          const QString& onnxModelPath,
                          Eigen::MatrixXd& sensing,
                          Eigen::MatrixXd& prediction,
                          Eigen::MatrixXd& cmne);

    //=========================================================================================================
    /**
     * Control estimate of the paper (cmne.control_estimate): q_t times the mean of
     * q over the previous @p lookBack samples, without an LSTM.
     *
     * @param[in] matDspmData    Signed dSPM estimate (n_sources x n_times).
     * @param[in] lookBack       Window length k.
     *
     * @return Control estimate (n_sources x n_times).
     */
    static Eigen::MatrixXd controlEstimate(const Eigen::MatrixXd& matDspmData, int lookBack);

    //=========================================================================================================
    /**
     * Rectify and z-score each source over time (Eq. 9).
     *
     * @param[in] matStcData     Source data (n_sources x n_times).
     *
     * @return |x| standardised per row; constant rows are centred only.
     */
    static Eigen::MatrixXd zScoreRectify(const Eigen::MatrixXd& matStcData);

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

    static Eigen::MatrixXd standardize(const Eigen::MatrixXd& matStcData);
};

} // namespace INVLIB

#endif // INV_CMNE_H
