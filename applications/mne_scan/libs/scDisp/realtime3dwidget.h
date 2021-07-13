//=============================================================================================================
/**
 * @file     realtime3dwidget.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Ruben Dörfel. All rights reserved.
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
 * @brief    Declaration of the RealTime3DWidget Class.
 *
 */

#ifndef REALTIME3DWIDGET_H
#define REALTIME3DWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"

#include "measurementwidget.h"

#include <fiff/fiff_coord_trans.h>

#include <fs/surfaceset.h>
#include <fs/annotationset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <Qt3DCore/QTransform>
#include <QPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISP3DLIB {
    class NetworkTreeItem;
    class View3D;
    class Data3DTreeModel;
    class MneDataTreeItem;
    class BemTreeItem;
    class DigitizerSetTreeItem;
}

namespace SCMEASLIB {
    class RealTimeConnectivityEstimate;
}

namespace DISPLIB {
    class QuickControlView;
    class Control3DView;
}

namespace FIFFLIB {
    class FiffDigPointSet;
}

//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS RealTime3DWidget
 *
 * @brief The RealTime3DWidget class provides a real-time network display.
 */

class SCDISPSHARED_EXPORT RealTime3DWidget : public MeasurementWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTime3DWidget> SPtr;             /**< Shared pointer type for RealTime3DWidget class. */
    typedef QSharedPointer<const RealTime3DWidget> ConstSPtr;  /**< Const shared pointer type for RealTime3DWidget class. */

    //=========================================================================================================
    /**
     * Constructs a RealTime3DWidget which is a child of parent.
     *
     * @param[in] parent    pointer to parent widget; If parent is 0, the new NumericWidget becomes a window.
     */
    RealTime3DWidget(QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTime3DWidget.
     */
    ~RealTime3DWidget();

    //=========================================================================================================
    /**
     * Initialise the MeasurementWidget.
     */
    virtual void init(){}

    //=========================================================================================================
    /**
     * Is called when new data are available.
     *
     * @param[in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
     */
    virtual void update(SCMEASLIB::Measurement::SPtr pMeasurement);

protected:

    //=========================================================================================================
    /**
     * Call this function whenever the digitizer changed and you want to align fiducials.
     *
     * @param[in] sFilePath    The file path to the new digitzers.
     */
    void alignFiducials(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Allign fiducials based on input digitizer data
     *
     * @param[in] pDigData      New digitizer data
     */
    void alignFiducials(QSharedPointer<FIFFLIB::FiffDigitizerData> pDigData);

    //=========================================================================================================
    /**
     * Allign fiducials based on input digitizer data
     *
     * @param[in] pDigData      New digitizer data
     */
    void alignFiducials(FIFFLIB::FiffDigitizerData* pDigData);

    //=========================================================================================================
    /**
     * Adds digitizer points to view
     *
     * @param[in] digSet    set of digitizer points
     */
    void addDigSetToView(const FIFFLIB::FiffDigPointSet& digSet);

    //=========================================================================================================

    QMatrix4x4 calculateInverseMatrix(FIFFLIB::FiffDigitizerData *pDigData,
                                      float scale);

    //=========================================================================================================

    void applyAlignmentTransform(QMatrix4x4 invMat);

    //=========================================================================================================
    /**
     * Initialise the display control widgets to be shown in the QuickControlView.
     */
    void initDisplayControllWidgets();

    //=========================================================================================================
    /**
     * Creates the GUI.
     */
    void createGUI();

    QString                                                     m_sFilePathDigitizers;
    QSharedPointer<FIFFLIB::FiffDigitizerData>                  m_pFiffDigitizerData;

    int                                                         m_iNumberBadChannels;   /**< The last received number of bad channels. */

    FSLIB::AnnotationSet                                        m_annotationSet;        /**< The current annotation set. */
    FSLIB::SurfaceSet                                           m_surfSet;              /**< The current surface set. */

    Qt3DCore::QTransform                                        m_tAlignment;           /**< Transformation matrix alignment fiducials/tracked in head space. */
    FIFFLIB::FiffCoordTrans                                     m_mriHeadTrans;         /**< The mri to head transformation. */

    QSharedPointer<DISP3DLIB::Data3DTreeModel>                  m_pData3DModel;         /**< The Disp3D model. */

    QPointer<DISP3DLIB::DigitizerSetTreeItem>                   m_pTrackedDigitizer;    /**< The 3D item pointing to the tracked digitizers. */
    QPointer<DISP3DLIB::View3D>                                 m_p3DView;              /**< The Disp3D view. */
    QPointer<DISP3DLIB::NetworkTreeItem>                        m_pRtConnectivityItem;  /**< The Disp3D real time item. */
    QPointer<DISP3DLIB::MneDataTreeItem>                        m_pRtMNEItem;           /**< The Disp3D real time items. */
    QPointer<DISP3DLIB::BemTreeItem>                            m_pBemHeadAvr;          /**< TThe fsaverage BEM head model. */
    QPointer<QAction>                                           m_pActionQuickControl;  /**< Show quick control widget. */
};
} // NAMESPACE

#endif // REALTIME3DWIDGET_H
