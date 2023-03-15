//=============================================================================================================
/**
 * @file     view3D.h
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lars Debor, Lorenz Esch. All rights reserved.
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
 * @brief    View3D class declaration.
 *
 */

#ifndef DISP3DLIB_VIEW3D_H
#define DISP3DLIB_VIEW3D_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../../disp3D_global.h"
#include "orbitalcameracontroller.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DExtras/Qt3DWindow>
#include <QVector3D>
#include <QPointer>
#include <QObjectPicker>
#include <Qt3DCore>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QPropertyAnimation;

namespace Qt3DRender {
    class QPointLight;
    class QRenderCaptureReply;
    class QPickEvent;
    class QRenderSurfaceSelector;
    class QViewport;
}

//=============================================================================================================
// DEFINE NAMESPACE DISP3DLIB
//=============================================================================================================

namespace DISP3DLIB
{

//=============================================================================================================
// DISP3DLIB FORWARD DECLARATIONS
//=============================================================================================================

class Data3DTreeModel;
class CustomFrameGraph;

//=============================================================================================================
/**
 * Visualizes 3D/2D objects in a 3D space such as brain, DTI, MRI, sensor, helmet data.
 *
 * @brief Visualizes 3D data
 */
class DISP3DSHARED_EXPORT View3D : public Qt3DExtras::Qt3DWindow
{
    Q_OBJECT

public:
    typedef QSharedPointer<View3D> SPtr;             /**< Shared pointer type for View3D class. */
    typedef QSharedPointer<const View3D> ConstSPtr;  /**< Const shared pointer type for View3D class. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    explicit View3D(/*QWidget *parent = 0*/);

    //=========================================================================================================
    /**
     * Default destructor
     */
    ~View3D();

    //=========================================================================================================
    /**
     * Return the tree model which holds the subject information.
     *
     * @param[in] pModel     The tree model holding the 3d models.
     */
    void setModel(QSharedPointer<DISP3DLIB::Data3DTreeModel> pModel);

    //=========================================================================================================
    /**
     * Set the background color of the scene.
     *
     * @param[in] colSceneColor          The new background color of the view.
     */
    void setSceneColor(const QColor& colSceneColor);

    //=========================================================================================================
    /**
     * Starts or stops to rotate camera around 3D models.
     */
    void startStopCameraRotation(bool bChecked);

    //=========================================================================================================
    /**
     * Rotate camera arround given angle.
     *
     * @param[in] iAngle          The to rotate the camera.
     *
     */
    void setCameraRotation(float fAngle);

    //=========================================================================================================
    /**
     * Returns the camera's position via transform.
     *
     * @return The transform.
     *
     */
    Qt3DCore::QTransform getCameraTransform();

    //=========================================================================================================
    /**
     * Toggle the coord axis visibility.
     */
    void toggleCoordAxis(bool bChecked);

    //=========================================================================================================
    /**
     * Show fullscreen.
     */
    void showFullScreen(bool bChecked);

    //=========================================================================================================
    /**
     * Change light color.
     */
    void setLightColor(const QColor &color);

    //=========================================================================================================
    /**
     * Set light intensity.
     */
    void setLightIntensity(double value);

    //=========================================================================================================
    /**
     * Renders a screenshot of the view and saves it to the passed path. SVG and PNG supported.
     *
     * @param[in] fileName     The file name and path where to store the screenshot.
     */
    void takeScreenshot();

    //=========================================================================================================
    /**
     * Initilize the object picking.
     *
     * @param[in] bActivatePicker     Wheater to activate the object picker.
     */
    void activatePicker(const bool bActivatePicker);

    //=========================================================================================================
    /**
     * Toggles view to display single view.
     */
    void showSingleView();

    //=========================================================================================================
    /**
     * Toggles view to display multi-view.
     */
    void showMultiView();

protected:

    void saveScreenshot();

    //=========================================================================================================
    /**
     * Init the light for the 3D view
     */
    void initLight();

    //=========================================================================================================
    /**
     * Initilize the object picking.
     */
    void initObjectPicking();

    //=========================================================================================================
    /**
     * Window function
     */
    void keyPressEvent(QKeyEvent* e) override;

    //=========================================================================================================
    /**
     * Creates a coordiante system (x/Green, y/Red, z/Blue).
     *
     * @param[in] parent         The parent identity which will "hold" the coordinate system.
     */
    void createCoordSystem(Qt3DCore::QEntity* parent);

    //=========================================================================================================
    /**
     * Handle Picking events.
     *
     * @param[in] qPickEvent         The picking event that occured.
     */
    void handlePickerPress(Qt3DRender::QPickEvent *qPickEvent);

    //=========================================================================================================
    /**
     * Initilaize single view camera parameters.
     */
    void initSingleCam();

    //=========================================================================================================
    /**
     * Initialize multi-view camera parameters.
     */
    void initMultiCams();

    //=========================================================================================================
    /**
     * Initialize single view viewport/framegraph.
     */
    void initSingleView();

    //=========================================================================================================
    /**
     * Initilize multiview viewports.
     */
    void initMultiView();

    //=========================================================================================================
    /**
     * Update multiview camera parameters based on aspect ratio.
     */
    void updateMultiViewAspectRatio();

    //=========================================================================================================
    /**
     * Sets multiview views to vertical layout.
     */
    void setMultiViewVertical();

    //=========================================================================================================
    /**
     * Sets multiview views to horizontal layout.
     */
    void setMultiViewHorizontal();

    //=========================================================================================================
    /**
     * Handles resize event for non-default multiview cameras.
     */
    void resizeEvent(QResizeEvent *) override;

    enum MultiViewOrientation{
        Horizontal,
        Veritical
    };

    QPointer<Qt3DCore::QEntity>                 m_pRootEntity;                  /**< The root/most top level entity buffer. */
    QPointer<Qt3DCore::QEntity>                 m_p3DObjectsEntity;             /**< The root/most top level entity buffer. */
    QPointer<Qt3DCore::QEntity>                 m_pLightEntity;                 /**< The root/most top level entity buffer. */
    QSharedPointer<Qt3DCore::QEntity>           m_pCoordSysEntity;              /**< The entity representing the x/y/z coord system. */

    QPointer<CustomFrameGraph>                  m_pFrameGraph;                  /**< The frameGraph entity. */
    QPointer<Qt3DRender::QRenderSurfaceSelector>m_pMultiFrame;

    QPointer<Qt3DRender::QCamera>               m_pCamera;                      /**< The camera entity. */
    QPointer<Qt3DRender::QCamera>               m_pMultiCam1;                   /**< First multiview camera entity. */
    QPointer<Qt3DRender::QCamera>               m_pMultiCam2;                   /**< Second multiview camera entity. */
    QPointer<Qt3DRender::QCamera>               m_pMultiCam3;                   /**< Third multiview camera entity. */

    QPointer<Qt3DRender::QViewport>             m_pMultiViewport1;              /**< First multiview viewport. */
    QPointer<Qt3DRender::QViewport>             m_pMultiViewport2;              /**< Second multiview viewport. */
    QPointer<Qt3DRender::QViewport>             m_pMultiViewport3;              /**< Third multiview viewport. */

    QPointer<Qt3DRender::QRenderCaptureReply>   m_pScreenCaptureReply;          /**< The capture reply object to save screenshots. */
    QPointer<Qt3DRender::QObjectPicker>         m_pPicker;                      /**< The Picker entity. */

    QPointer<OrbitalCameraController>           m_pCamController;               /**< The controller for camera position. */
    QPointer<QPropertyAnimation>                m_pCameraAnimation;             /**< The animations to rotate the camera. */

    QList<QPointer<Qt3DRender::QPointLight> >   m_lLightSources;                /**< The light sources. */

    MultiViewOrientation                        m_MultiViewOrientation;         /**< The current orientation of the multiview viewports. */

signals:
    /*
     * Send whenever a pick event occured
     */
    void pickEventOccured(Qt3DRender::QPickEvent *qPickEvent);

};
} // NAMESPACE

#endif // DISP3DLIB_VIEW3D_H
