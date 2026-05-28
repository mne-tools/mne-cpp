//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_signal_model.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Sinusoidal HPI signal model — builds and inverts the regressor matrix that extracts coil amplitudes from MEG data.
 *
 * @ref INVLIB::InvSignalModel builds the model matrix that pairs the
 * HPI coil drive frequencies (and, optionally, the line-frequency
 * harmonics) with the sample window length defined by the input data.
 * Inverting the model with a pseudo-inverse projects the MEG buffer
 * onto the per-coil amplitudes that @ref InvHpiFit feeds into the
 * dipole search. The model matrix is cached and re-derived only when
 * the @ref InvHpiModelParameters change or the number of samples in
 * the buffer changes.
 */

#ifndef INV_SIGNAL_MODEL_H
#define INV_SIGNAL_MODEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"
#include "inv_hpi_model_parameters.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace INVLIB
{
//=============================================================================================================
// Declare all structures to be used
//=============================================================================================================

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Generates the forward sinusoidal model matrix for HPI coil signals at known drive frequencies
 */
class INVSHARED_EXPORT InvSignalModel
{

public:
    typedef QSharedPointer<InvSignalModel> SPtr;            /**< Shared pointer type for InvSignalModel. */
    typedef QSharedPointer<const InvSignalModel> ConstSPtr; /**< Const shared pointer type for InvSignalModel. */

    //=========================================================================================================
    /**
     * Constructs a InvSignalModel object.
     */
    explicit InvSignalModel() = default;

    //=========================================================================================================
    /**
     * Fit the data to the model constructed from given model parameters.
     *
     * @param[in] hpiModelParameters   The InvHpiModelParameters.
     * @param[in] matData           The data matrix.
     *
     * @return the fitted data
     *
     */
    Eigen::MatrixXd fitData(const InvHpiModelParameters& hpiModelParameters,
                            const Eigen::MatrixXd& matData);

private:
    //=========================================================================================================
    /**
     * Selects the model to compute and calls coresponding compute function.
     *
     */
    void selectModelAndCompute();

    //=========================================================================================================
    /**
     * Computes the model.
     *
     */
    void computeInverseBasicModel();
    void computeInverseAdvancedModel();

    //=========================================================================================================
    /**
     * Check if dimensions of input data match the model.
     *
     * @param[in] iCols     The number of Clumns to compare.
     * @return true if changed
     *
     */
    bool checkDataDimensions(const int iCols);

    //=========================================================================================================
    /**
     * Check if the InvHpiModelParameters changed.
     *
     * @param[in] hpiModelParameters     The model parameters.
     * @return true if changed
     */
    bool checkModelParameters(const InvHpiModelParameters& hpiModelParameters);

    //=========================================================================================================
    /**
     * Check if the InvHpiModelParameters are empty. HPI and sampling frequencies need to be set.
     *
     * @param[in] hpiModelParameters     The model parameters.
     * @return true if empty
     */
    bool checkEmpty(const InvHpiModelParameters& hpiModelParameters);

    Eigen::MatrixXd m_matInverseSignalModel{Eigen::MatrixXd(0,0)};
    int m_iCurrentModelCols{0};
    InvHpiModelParameters m_modelParameters{InvHpiModelParameters()};
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // namespace INVLIB

#endif // SignalModel_H

