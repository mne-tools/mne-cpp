//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     view3d.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.6
 * @date     August, 2020
 * @brief    View3D class declaration.
 */

#ifndef MNEANALYZE_VIEW3D_H
#define MNEANALYZE_VIEW3D_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "view3d_global.h"

#include <anShared/Plugins/abstractplugin.h>

#include <mne/dsp/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::analyze
{
class Communicator;
class BemDataModel;
class AbstractModel;
class DipoleFitModel;
}

namespace mne::viz::qt
{
class BrainView;
class BrainTreeModel;
class BemTreeItem;
class DigitizerSetTreeItem;
class DipoleTreeItem;
}

namespace mne::viz::qt
{
class Control3DView;
}

namespace mne
{
class FiffCoordTrans;
class FiffDigPointSet;
}

namespace mne
{
class InvEcdSet;
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
class VIEW3DSHARED_EXPORT View3D : public mne::analyze::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "view3d.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::analyze::AbstractPlugin)

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

    virtual void handleEvent(QSharedPointer<mne::analyze::Event> e) override;
    virtual QVector<mne::analyze::EVENT_TYPE> getEventSubscriptions() const override;

private:
    //=========================================================================================================
    /**
     * Updates the available Bem Models for the coregistration.
     */
    void updateCoregBem(QSharedPointer<mne::analyze::BemDataModel> pNewModel);

    //=========================================================================================================
    /**
     * Updates the digitizer set for the coregistration.
     */
    void updateCoregDigitizer(const mne::FiffDigPointSet& digSet);

    //=========================================================================================================
    /**
     * Updates the mri fiducials stored as digitizer set for the coregistration.
     */
    void updateCoregMriFid(const mne::FiffDigPointSet& digSetFid);

    //=========================================================================================================
    /**
     * Transforms the digitizers to mri space with the new transformation.
     */
    void updateCoregTrans(const mne::FiffCoordTrans& headMriTrans);

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
     * Handle incoming picking event from BrainView.
     */
    void newPickingEvent(const QVector3D& worldIntersection);

    //=========================================================================================================
    /**
     * Emits new model as an event
     *
     * @param[in] pModel   new 3D model to be emitted.
     */
    void new3DModel(QSharedPointer<mne::viz::qt::BrainTreeModel> pModel);

    //=========================================================================================================
    /**
     * Update view parameters based on input viewParameters
     *
     * @param[in] viewParameters   parameters with changes to bw appied to view.
     */
    void settingsChanged(mne::analyze::View3DParameters viewParameters);

    //=========================================================================================================
    /**
     * @brief newDipoleFit
     * @param ecdSet.
     */
    void newDipoleFit(const mne::InvEcdSet& ecdSet);

    //=========================================================================================================
    /**
     * Loads new  model when current loaded model is changed
     *
     * @param[in, out] pNewModel    pointer to currently loaded FiffRawView Model.
     */
    void onModelChanged(QSharedPointer<mne::analyze::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * Handles clearing view if currently used model is being removed
     *
     * @param[in] pRemovedModel    Pointer to model being removed.
     */
    void onModelRemoved(QSharedPointer<mne::analyze::AbstractModel> pRemovedModel);

    QPointer<mne::analyze::Communicator> m_pCommu; /**< To broadcst signals. */

    int m_iFiducial; /**< Currently selected fiducial. */

    QSharedPointer<mne::viz::qt::BrainTreeModel> m_p3DModel;                        /**< The 3D model data. */
    QHash<const mne::analyze::AbstractModel*, QPersistentModelIndex> m_dipoleItems; /**< Scene item of each dipole fit model. */
    mne::viz::qt::BemTreeItem* m_pBemTreeCoreg;                                     /**< TThe BEM head model of the coregistration plugin. */

    QPointer<mne::viz::qt::BrainView> m_pView3D;   /**< The Disp3D view; the main window owns it once getView() handed it over. */
    mne::viz::qt::Control3DView* m_pControl3DView; /**< The 3D Control view. */

    bool m_bPickingActivated; /**< If Picking is activated*/

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
