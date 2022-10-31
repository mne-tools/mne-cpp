//=============================================================================================================
/**
 * @file     view3d.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.6
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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

#ifndef MNEANALYZE_VIEW3D_H
#define MNEANALYZE_VIEW3D_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "view3d_global.h"

#include <anShared/Plugins/abstractplugin.h>

#include <rtprocessing/helpers/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <Qt3DRender>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
    class BemDataModel;
    class AbstractModel;
    class DipoleFitModel;
}

namespace DISP3DLIB {
    class View3D;
    class Data3DTreeModel;
    class BemTreeItem;
    class DigitizerSetTreeItem;
    class EcdDataTreeItem;
}

namespace DISPLIB {
    class Control3DView;
}

namespace FIFFLIB {
    class FiffCoordTrans;
    class FiffDigPointSet;
}

namespace INVERSELIB {
    class ECDSet;
}
//=============================================================================================================
// DEFINE NAMESPACE VIEW3DPLUGIN
//=============================================================================================================

namespace VIEW3DPLUGIN
{

//=============================================================================================================
// View3DPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * View3D Plugin
 *
 * @brief The View3D class provides a plugin for visualizing information in 3D.
 */
class VIEW3DSHARED_EXPORT View3D : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "view3d.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs an View3D object.
     */
    View3D();

    //=========================================================================================================
    /**
     * Destroys the View3D object.
     */
    ~View3D() override;

    // AbstractPlugin functions
    virtual QSharedPointer<AbstractPlugin> clone() const override;
    virtual void init() override;
    virtual void unload() override;
    virtual QString getName() const override;

    virtual QMenu* getMenu() override;
    virtual QDockWidget* getControl() override;
    virtual QWidget* getView() override;
    virtual QString getBuildInfo() override;

    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private:
    //=========================================================================================================
    /**
     * Updates the available Bem Models for the coregistration.
     */
    void updateCoregBem(QSharedPointer<ANSHAREDLIB::BemDataModel> pNewModel);

    //=========================================================================================================
    /**
     * Updates the digitizer set for the coregistration.
     */
    void updateCoregDigitizer(FIFFLIB::FiffDigPointSet digSet);

    //=========================================================================================================
    /**
     * Updates the mri fiducials stored as digitizer set for the coregistration.
     */
    void updateCoregMriFid(FIFFLIB::FiffDigPointSet digSetFid);

    //=========================================================================================================
    /**
     * Transforms the digitizers to mri space with the new transformation.
     */
    void updateCoregTrans(FIFFLIB::FiffCoordTrans headMriTrans);

    //=========================================================================================================
    /**
     * Activate/deactivate fiducial picking.
     */
    void fiducialPicking(const bool bActivatePicking);

    //=========================================================================================================
    /**
     * Activate/deactivate fiducial picking.
     */
    void onFiducialChanged(const int iFiducial);

    //=========================================================================================================
    /**
     * Handle incoming picking event from DISP3DLIB::3DView.
     */
    void newPickingEvent(Qt3DRender::QPickEvent *qPickEvent);

    //=========================================================================================================
    /**
     * Emits new model as an event
     *
     * @param[in] pModel   new 3D model to be emitted.
     */
    void new3DModel(QSharedPointer<DISP3DLIB::Data3DTreeModel> pModel);

    //=========================================================================================================
    /**
     * Update view parameters based on input viewParameters
     *
     * @param[in] viewParameters   parameters with changes to bw appied to view.
     */
    void settingsChanged(ANSHAREDLIB::View3DParameters viewParameters);

    //=========================================================================================================
    /**
     * @brief newDipoleFit
     * @param ecdSet.
     */
    void newDipoleFit(const INVERSELIB::ECDSet& ecdSet);

    //=========================================================================================================
    /**
     * Loads new  model when current loaded model is changed
     *
     * @param[in, out] pNewModel    pointer to currently loaded FiffRawView Model.
     */
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * Handles clearing view if currently used model is being removed
     *
     * @param[in] pRemovedModel    Pointer to model being removed.
     */
    void onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel);

    QPointer<ANSHAREDLIB::Communicator>                     m_pCommu;               /**< To broadcst signals. */

    int                                                     m_iFiducial;            /**< Currently selected fiducial. */

    QSharedPointer<DISP3DLIB::Data3DTreeModel>              m_p3DModel;             /**< The 3D model data. */
    DISP3DLIB::BemTreeItem*                                 m_pBemTreeCoreg;        /**< TThe BEM head model of the coregistration plugin. */
    DISP3DLIB::DigitizerSetTreeItem*                        m_pDigitizerCoreg;      /**< The 3D item pointing to the tracked digitizers. */
    DISP3DLIB::DigitizerSetTreeItem*                        m_pMriFidCoreg;         /**< The 3D item pointing to the mri fiducials. */
    DISP3DLIB::EcdDataTreeItem*                             m_pDipoleFit;           /**< The 3D item pointing to the dipole fit. */

    DISP3DLIB::View3D*                                      m_pView3D;              /**< The Disp3D view. */
    DISPLIB::Control3DView*                                 m_pControl3DView;       /**< The 3D Control view. */

    bool                                                    m_bPickingActivated;    /**< If Picking is activated*/

signals:
    //=========================================================================================================
    /**
     * Emit new scene color
     *
     * @param[in] colSceneColor    new scene color.
     */
    void sceneColorChanged(const QColor& colSceneColor);

    //=========================================================================================================
    /**
     * Emit toggle rotation
     *
     * @param[in] bRotationChanged     rotation toggle.
     */
    void rotationChanged(bool bRotationChanged);

    //=========================================================================================================
    /**
     * Emit toggle coord axis
     *
     * @param[in] bShowCoordAxis   coord axis toggle.
     */
    void showCoordAxis(bool bShowCoordAxis);

    //=========================================================================================================
    /**
     * Emit toggle fullscreen
     *
     * @param[in] bShowFullScreen      fullscreen toggle.
     */
    void showFullScreen(bool bShowFullScreen);

    //=========================================================================================================
    /**
     * Emit new light color
     *
     * @param[in] color    new light color.
     */
    void lightColorChanged(const QColor& color);

    //=========================================================================================================
    /**
     * Emit new light intensity value
     *
     * @param[in] value    new light intenisty.
     */
    void lightIntensityChanged(double value);

    //=========================================================================================================
    /**
     * Emit trigger for taking screenshot
     */
    void takeScreenshotChanged();
};

} // NAMESPACE

#endif // MNEANALYZE_VIEW3D_H

