//=============================================================================================================
/**
 * @file     inv_cmne_settings.h
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
 * @brief    InvCMNESettings struct declaration.
 *
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
