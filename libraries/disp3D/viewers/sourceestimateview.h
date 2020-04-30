//=============================================================================================================
/**
 * @file     sourceestimateview.h
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
 * @brief    SourceEstimateView class declaration.
 *
 */

#ifndef DISP3DLIB_SOURCEESTIMATEVIEW_H
#define DISP3DLIB_SOURCEESTIMATEVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp3D_global.h"
#include "abstractview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace MNELIB {
    class MNESourceEstimate;
    class MNEForwardSolution;
}

namespace FSLIB {
    class SurfaceSet;
    class AnnotationSet;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class MneDataTreeItem;

//=============================================================================================================
/**
 * Adapter which provides visualization for MNE source estimate data and a control widget.
 *
 * @brief Visualizes ECD data.
 */
class DISP3DSHARED_EXPORT SourceEstimateView : public AbstractView
{
    Q_OBJECT

public:
    typedef QSharedPointer<SourceEstimateView> SPtr;             /**< Shared pointer type for SourceEstimateView class. */
    typedef QSharedPointer<const SourceEstimateView> ConstSPtr;  /**< Const shared pointer type for SourceEstimateView class. */

    //=========================================================================================================
    /**
     * Default constructor
     *
     */
    explicit SourceEstimateView(QWidget *parent = 0,
                                Qt::WindowFlags f = Qt::Widget);

    //=========================================================================================================
    /**
     * Default destructor
     */
    ~SourceEstimateView();

    //=========================================================================================================
    /**
     * Add data to the view
     *
     * @param[in] sSubject               The name of the subject.
     * @param[in] sMeasurementSetName    The name of the measurement set to which the data is to be added. If it does not exist yet, it will be created.
     * @param[in] tSourceEstimate        The MNESourceEstimate.
     * @param[in] tForwardSolution       The MNEForwardSolution.
     * @param[in] tSurfSet               The surface set holding the left and right hemisphere surfaces.
     * @param[in] tAnnotSet              The annotation set holding the left and right hemisphere annotations.
     *
     * @return                           Returns a pointer to the added tree item. Default is a NULL pointer if no item was added.
     */
    MneDataTreeItem* addData(const QString& sSubject,
                             const QString& sMeasurementSetName,
                             const MNELIB::MNESourceEstimate& tSourceEstimate,
                             const MNELIB::MNEForwardSolution& tForwardSolution,
                             const FSLIB::SurfaceSet& tSurfSet,
                             const FSLIB::AnnotationSet& tAnnotSet);

protected:
};
} // NAMESPACE

#endif // DISP3DLIB_SOURCEESTIMATEVIEW_H
