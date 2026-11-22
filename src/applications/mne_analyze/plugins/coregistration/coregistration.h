//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     coregistration.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     August, 2020
 * @brief    Contains the declaration of the CoRegistration class.
 */

#ifndef COREGISTRATION_H
#define COREGISTRATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coregistration_global.h"
#include <anShared/Plugins/abstractplugin.h>
#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_dig_point.h>
#include <mne/fiff/fiff_dig_point_set.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <future>
#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::analyze
{
class Communicator;
class AbstractModel;
}

namespace mne::disp
{
class CoregSettingsView;
}

namespace mne
{
class MNEBem;
}

namespace mne
{
class FiffDigPoint;
class FiffDigPointSet;
class FiffCoordTrans;
}
//=============================================================================================================
// DEFINE NAMESPACE SURFERPLUGIN
//=============================================================================================================

namespace COREGISTRATIONPLUGIN
{

//=============================================================================================================
/**
 * CoRegistration Plugin
 *
 * @brief The CoRegistration class provides a view with all currently loaded models.
 */
class COREGISTRATIONSHARED_EXPORT CoRegistration : public mne::analyze::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "coregistration.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::analyze::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs a CoRegistration.
     */
    CoRegistration();

    //=========================================================================================================
    /**
     * Destroys the CoRegistration.
     */
    virtual ~CoRegistration() override;

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
     * Updates the dropdown display for selecting the bem model.
     */
    void updateBemList(QSharedPointer<mne::analyze::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * Removes BEM model from stored list and GUI drop down list
     *
     * @param[in] pRemovedModel    model being removed.
     *
     * @return  whether model is removed (true if removed, false if not removed).
     */
    bool removeFromBemList(QSharedPointer<mne::analyze::AbstractModel> pRemovedModel);

    //=========================================================================================================
    /**
     * Handle the deletion of models.
     */
    void deleteModels();

    //=========================================================================================================
    /**
     * Connected to GUI dropdown to select bem based on bem name input.
     *
     * @param[in] text  name of bem selected in the GUI.
     */
    void onChangeSelectedBem(const QString& sText);

    //=========================================================================================================
    /**
     * Activate/Deactivate Fiducial Picking.
     */
    void onPickFiducials(const bool bActivatePicking);

    //=========================================================================================================
    /**
     * Create new fiducial from ObjectPicking input.
     */
    void onSetFiducial(const QVector3D vecResult);

    //=========================================================================================================
    /**
     * Updates selected fiducial.
     */
    void onPickedFiducialChanged(const int iFiducial);

    //=========================================================================================================
    /**
     * Call this funciton whenever new digitzers were loaded.
     *
     * @param[in] sFilePath     The file path to the new digitzers.
     */
    void onDigitizersChanged(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Call this funciton whenever new fiducials were loaded.
     *
     * @param[in] sFilePath     The file path to the new digitzers.
     */
    void onFiducialsChanged(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Load existing transformation from file.
     *
     * @param[in] sFilePath     The file path to the transformation.
     */
    void onLoadTrans(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Store the transformation to the given file path.
     *
     * @param[in] sFilePath     The file path to store the transformation.
     */
    void onStoreTrans(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Triggers computeICP to be run on a worker thread.
     */
    void onFitICP();

    //=========================================================================================================
    /**
     * Perform the actual Coregistration with the ICP algorithm.
     *
     * @param[in] transInit     The finitial coordinate transformation matrix.
     * @param[in] digSetHSP     The digitizer set containing the Head Shap Points, HPI coils, etc..
     * @param[in] bemHead       The head surface.
     *
     * @return The resulting coordinate transformation from head to mri space.
     */
    mne::FiffCoordTrans computeICP(mne::FiffCoordTrans transInit,
                                   mne::FiffDigPointSet digSetHSP,
                                   mne::MNEBem bemHead);

    //=========================================================================================================
    /**
     * Receives FiffCoordTrans set from the worker thread and create respectively update transformation
     */
    void createNewTrans();

    //=========================================================================================================
    /**
     * Fit fiducials as initial alignment for further coregistration with ICP algorithm.
     */
    void onFitFiducials();

    //=========================================================================================================
    /**
     * Update the transformation with new rotation, translation and scaling parameters from widget.
     */
    void onUpdateTrans();

    //=========================================================================================================
    /**
     * Get the transformation parameters from the matrix (rotation euler angle, translation, scale)
     *
     * Following https://math.stackexchange.com/a/1463487
     *
     * @param[in] matTrans      The transformation matrix to obtain the parametrs from.
     * @param[out] vecTrans     The translation vector (x,y,z).
     * @param[out] vecRot       The rotation angle vector in rad (due to euler transformation: z,y,x) .
     * @param[out] vecScale     The vector with the scaling parameters (x,y,z).
     */
    void getParamFromTrans(const Matrix4f& matTrans,
                           Vector3f& vecRot,
                           Vector3f& vecTrans,
                           Vector3f& vecScale);

    //=========================================================================================================
    /**
     * Get the transformation matrix from the parameters (rotation euler angle, translation, scale)
     *
     * @param[out] matTrans    The transformation matrix to store the parameters in.
     * @param[in] vecTrans     The translation vector (x,y,z).
     * @param[in] vecRot       The rotation angle vector in rad (due to euler transformation: z,y,x).
     * @param[in] vecScale     The vector with the scaling parameters (x,y,z).
     */
    void getTransFromParam(Matrix4f& matTrans,
                           const Vector3f& vecRot,
                           const Vector3f& vecTrans,
                           const Vector3f& vecScale);

    //=========================================================================================================
    /**
     * Store the fiducials to the given file path
     *
     * @param[in] sFilePath     The file path to store the fiducials.
     */
    void onStoreFiducials(const QString& sFilePath);

    //=========================================================================================================
    /**
     * Get ICP properties from cpregsettingsview and store to member variables
     */
    void setIcpProperties();

    //=============================================================================================================
    /**
     * Sends event to trigger loading bar to appear and sMessage to show
     *
     * @param[in] sMessage     loading bar message.
     */
    void triggerLoadingStart(QString sMessage);

    //=============================================================================================================
    /**
     * Sends event to hide loading bar
     */
    void triggerLoadingEnd(QString sMessage);

    //=============================================================================================================
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

    QVector<QSharedPointer<mne::analyze::AbstractModel>> m_vecBemDataModels; /**< Vector with all available Bem Models. */
    std::shared_ptr<mne::MNEBem> m_pBem;                                     /**< The currently selected Bem model. */
    QString m_sCurrentSelectedBem;                                           /**< The name of the currently selected Bem. */
    mne::FiffDigPointSet m_digSetHead;                                       /**< The currently selected digitizer set. */
    mne::FiffDigPointSet m_digFidMri;                                        /**< The currently selected mri fiducials. */
    mne::FiffCoordTrans m_transHeadMri;                                      /**< The resulting head-mri transformation. */

    int m_iFiducial;             /**< Currently selected fiducial. */
    bool m_bScale;               /**< Wether to apply automatic scaling. */
    float m_fWeightLPA;          /**< Weight LPA. */
    float m_fWeightNAS;          /**< Weight NAS. */
    float m_fWeightRPA;          /**< Weight RPA. */
    float m_fWeightHPI;          /**< Weight HPI. */
    float m_fWeightHSP;          /**< Weight HSP (Head Shape Point). */
    float m_fWeightEEG;          /**< Weight EEG . */
    float m_fMaxDist;            /**< Maximum distace - electrode surface. */
    float m_fTol;                /**< Divergence Tolerance . */
    int m_iMaxIter;              /**< Maximum ammount of iterations. */
    std::vector<int> m_lPickHSP; /**< Digitizer kinds used by the ICP fit. */
    float m_fRMSE;               /**< RMSE of the last ICP fit, in m. */
    int m_iNDiscarded;           /**< Number of digitizers the last ICP fit discarded as outliers. */

    QPointer<mne::analyze::Communicator> m_pCommu;

    mne::disp::CoregSettingsView* m_pCoregSettingsView; /**< Pointer to coreg GUI. */

    std::future<mne::FiffCoordTrans> m_Future; /**< Pending ICP fit; valid until its result is consumed. */
    QMutex m_ParameterMutex;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // COREGISTRATION_H
