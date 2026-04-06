//=============================================================================================================
/**
 * @file     mne_ch_selection.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Christoph Dinh, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief    MNEChSelection class declaration.
 *
 */

#ifndef MNE_CH_SELECTION_H
#define MNE_CH_SELECTION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <Eigen/Core>

#include <QString>
#include <QStringList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Channel selection.
 */
class MNESHARED_EXPORT MNEChSelection
{
public:
    MNEChSelection() = default;
    ~MNEChSelection() = default;

    QString     name;               /**< Name of this selection. */
    QStringList chdef;              /**< Channel definitions (may contain regular expressions). */
    int         ndef = 0;           /**< How many of them. */
    QStringList chspick;            /**< Translated into channel names using the present data. */
    QStringList chspick_nospace;    /**< The same without spaces. */
    Eigen::VectorXi pick;           /**< Corresponding channels in raw data (< 0 indicates missing). */
    Eigen::VectorXi pick_deriv;     /**< Corresponding derivations in raw data. */
    int  nderiv = 0;                /**< How many derivations in the above. */
    Eigen::VectorXi ch_kind;        /**< Kinds of the channels corresponding to picks. */
    int  nchan = 0;                 /**< How many picked channels? */
    int  kind = 0;                  /**< Loaded from file or created here? */
};

/** Backward-compatible typedef aliases. */
typedef MNEChSelection  mneChSelectionRec;
typedef MNEChSelection* mneChSelection;

} // namespace MNELIB

#endif // MNE_CH_SELECTION_H
