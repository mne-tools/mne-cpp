//=============================================================================================================
/**
 * @file     sourceestimateview.cpp
 * @author   Juan Garcia-Prieto <juangpc@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Juan Garcia-Prieto, Lorenz Esch. All rights reserved.
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
 * @brief    SourceEstimateView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sourceestimateview.h"

#include "../engine/model/data3Dtreemodel.h"
#include "../engine/model/items/sourcedata/mnedatatreeitem.h"

#include <fs/surfaceset.h>
#include <fs/annotationset.h>

#include <mne/mne_forwardsolution.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace MNELIB;
using namespace FSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SourceEstimateView::SourceEstimateView(QWidget* parent,
                                       Qt::WindowFlags f)
: AbstractView(parent, f)
{
}

//=============================================================================================================

SourceEstimateView::~SourceEstimateView()
{
}

//=============================================================================================================

MneDataTreeItem* SourceEstimateView::addData(const QString& sSubject,
                                             const QString& sMeasurementSetName,
                                             const MNESourceEstimate& tSourceEstimate,
                                             const MNEForwardSolution& tForwardSolution,
                                             const SurfaceSet& tSurfSet,
                                             const AnnotationSet& tAnnotSet)
{
    //Add network data
    return m_pData3DModel->addSourceData(sSubject,
                                         sMeasurementSetName,
                                         tSourceEstimate,
                                         tForwardSolution,
                                         tSurfSet,
                                         tAnnotSet);
}
