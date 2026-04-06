//=============================================================================================================
/**
 * @file     mne_layout.h
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
 * @brief    MNELayout class declaration.
 *
 */

#ifndef MNE_LAYOUT_H
#define MNE_LAYOUT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_layout_port.h"

#include <QString>
#include <vector>

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Full plotter layout with viewports.
 *
 * Describes the visual arrangement of signal channels in a
 * plotter display, loaded from a layout file.
 */
class MNESHARED_EXPORT MNELayout
{
public:
    MNELayout() = default;
    ~MNELayout() = default;

    QString   name;              /**< Source file name this layout was loaded from. */
    float     xmin = 0;          /**< VDC left limit. */
    float     xmax = 0;          /**< VDC right limit. */
    float     ymin = 0;          /**< VDC bottom limit. */
    float     ymax = 0;          /**< VDC top limit. */
    float     cxmin = 0;         /**< Confined VDC left limit. */
    float     cxmax = 0;         /**< Confined VDC right limit. */
    float     cymin = 0;         /**< Confined VDC bottom limit. */
    float     cymax = 0;         /**< Confined VDC top limit. */
    std::vector<MNELayoutPort> ports; /**< Viewports. */
    Eigen::MatrixXi match;       /**< Channel-to-port matching matrix (nchan x nport). */

    /**
     * @brief Returns the number of channels (rows in match matrix).
     */
    int nmatch() const { return match.rows(); }
};

} // namespace MNELIB

#endif // MNE_LAYOUT_H
