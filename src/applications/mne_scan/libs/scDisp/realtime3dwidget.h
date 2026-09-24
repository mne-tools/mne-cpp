//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     realtime3dwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     October, 2016
 * @brief    Declaration of the RealTime3DWidget Class.
 */

#ifndef REALTIME3DWIDGET_H
#define REALTIME3DWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"

#include "measurementwidget.h"

#include <fiff/fiff_coord_trans.h>

#include <fs/fs_surfaceset.h>
#include <fs/fs_annotationset.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QPointer>
#include <QMatrix4x4>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class BrainView;
class BrainTreeModel;
class NetworkTreeItem;
class BemTreeItem;
class DigitizerSetTreeItem;

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
     * Adds digitizer points to view
     *
     * @param[in] digSet    set of digitizer points
     */
    void addDigSetToView(const FIFFLIB::FiffDigPointSet& digSet);

    //=========================================================================================================
    /**
     * Calculates matrix based on input digitizer data
     *
     * @param[in] pDigData      source digitizer data
     * @param[in] scale         scaling factor
     *
     * @return
     */
    QMatrix4x4 calculateInverseMatrix(const QSharedPointer<FIFFLIB::FiffDigitizerData> pDigData,
                                      float scale) const;

    //=========================================================================================================
    /**
     * Alligns 3D head model based on input matrix
     *
     * @param[in] invMat    matrix used to alligned 3d head
     */
    void applyAlignmentTransform(QMatrix4x4& invMat);

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

    QString                                                     m_sFilePathDigitizers;  /**< Path to loaded fiff file with digitizer data. */
    QSharedPointer<FIFFLIB::FiffDigitizerData>                  m_pFiffDigitizerData;   /**< Fiff digitizer data for current measurement. */

    int                                                         m_iNumberBadChannels;   /**< The last received number of bad channels. */

    FSLIB::FsAnnotationSet                                        m_annotationSet;        /**< The current annotation set. */
    FSLIB::FsSurfaceSet                                           m_surfSet;              /**< The current surface set. */

    QMatrix4x4                                                  m_tAlignment;           /**< Transformation matrix alignment fiducials/tracked in head space. */
    FIFFLIB::FiffCoordTrans                                     m_mriHeadTrans;         /**< The mri to head transformation. */

    QSharedPointer<BrainTreeModel>                              m_pData3DModel;         /**< The Disp3D model. */

    DigitizerSetTreeItem*                                    m_pTrackedDigitizer;    /**< The 3D item pointing to the tracked digitizers. */
    QPointer<BrainView>                                         m_p3DView;              /**< The Disp3D view. */
    NetworkTreeItem*                                            m_pRtConnectivityItem;  /**< The Disp3D real time item. */
    bool                                                        m_bRtSourceActive;      /**< Whether realtime source is active. */
    BemTreeItem*                                                m_pBemHeadAvr;          /**< The fsaverage BEM head model. */
    QPointer<QAction>                                           m_pActionQuickControl;  /**< Show quick control widget. */
};
} // NAMESPACE

#endif // REALTIME3DWIDGET_H
