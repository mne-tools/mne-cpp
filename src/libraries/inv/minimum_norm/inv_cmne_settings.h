//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_cmne_settings.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Settings record for the @ref INVLIB::InvCMNE solver (model path, sliding-window length, regularisation, base method).
 *
 * @ref INVLIB::InvCMNESettings collects every knob that
 * @ref InvCMNE::compute and @ref InvCMNE::trainLstm need: the path to
 * the trained ONNX LSTM, the look-back window length @c k that defines
 * the temporal context fed into the network, the source-space size
 * (which must match the LSTM's input dimension), the Tikhonov
 * regulariser @f$ \lambda^{2} @f$, the underlying linear method (0 =
 * MNE, 1 = dSPM, 2 = sLORETA) and the loose-orientation constraint.
 */

#ifndef INV_CMNE_SETTINGS_H
#define INV_CMNE_SETTINGS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Settings for the Contextual MNE (CMNE) inverse solver (Dinh et al. 2021).
 *
 * @brief CMNE settings
 */
struct INVSHARED_EXPORT InvCMNESettings
{
    QString onnxModelPath;          /**< Path to trained LSTM model (.onnx). */
    int lookBack = 80;              /**< k: number of past time steps. */
    int numSources = 5124;          /**< n_s: must match model input dim. */
    double lambda2 = 1.0 / 9.0;    /**< Tikhonov regularisation (SNR=3 -> lambda^2=1/9). */
    int method = 1;                 /**< 0=MNE, 1=dSPM (default), 2=sLORETA. */
    double looseOriConstraint = 0.2;/**< Orientation constraint. */
};

} // namespace INVLIB

#endif // INV_CMNE_SETTINGS_H
