//=============================================================================================================
/**
 * @file     coregistration.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.6
 * @date     August, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    Contains the declaration of the CoRegistration class.
 *
 */

#ifndef COREGISTRATION_H
#define COREGISTRATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coregistration_global.h"
#include <anShared/Plugins/abstractplugin.h>

#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>

#include <utils/buildtime.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class Communicator;
    class AbstractModel;
}

namespace DISPLIB {
    class CoregSettingsView;
}

namespace MNELIB {
    class MNEBem;
}

namespace FIFFLIB {
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
class COREGISTRATIONSHARED_EXPORT CoRegistration : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "coregistration.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

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
    virtual QString getBuildDateTime() override;
    virtual void handleEvent(QSharedPointer<ANSHAREDLIB::Event> e) override;
    virtual QVector<ANSHAREDLIB::EVENT_TYPE> getEventSubscriptions() const override;

private:
    //=========================================================================================================
    /**
     * Updates the dropdown display for selecting the bem model.
     */
    void updateBemList(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * Removes BEM model from stored list and GUI drop down list
     *
     * @param[in] pRemovedModel    model being removed.
     *
     * @return  whether model is removed (true if removed, false if not removed).
     */
    bool removeFromBemList(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel);

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
    void onChangeSelectedBem(const QString &sText);

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
     * Triggers computeICP to be run with QFuture.
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
    FIFFLIB::FiffCoordTrans computeICP(FIFFLIB::FiffCoordTrans transInit,
                                       FIFFLIB::FiffDigPointSet digSetHSP,
                                       MNELIB::MNEBem bemHead);

    //=========================================================================================================
    /**
     * Receives FiffCoordTrans set from QFuture and create respectively update transformation
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
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * Handles clearing view if currently used model is being removed
     *
     * @param[in] pRemovedModel    Pointer to model being removed.
     */
    void onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel);

    QVector<QSharedPointer<ANSHAREDLIB::AbstractModel>>     m_vecBemDataModels;     /**< Vector with all available Bem Models. */
    QSharedPointer<MNELIB::MNEBem>                          m_pBem;                 /**< The currently selected Bem model. */
    QString                                                 m_sCurrentSelectedBem;  /**< The name of the currently selected Bem. */
    FIFFLIB::FiffDigPointSet                                m_digSetHead;           /**< The currently selected digitizer set. */
    FIFFLIB::FiffDigPointSet                                m_digFidMri;            /**< The currently selected mri fiducials. */
    FIFFLIB::FiffCoordTrans                                 m_transHeadMri;         /**< The resulting head-mri transformation. */

    int m_iFiducial;                                        /**< Currently selected fiducial. */
    bool m_bScale;                                            /**< Wether to apply automatic scaling. */
    float m_fWeightLPA;                                       /**< Weight LPA. */
    float m_fWeightNAS;                                       /**< Weight NAS. */
    float m_fWeightRPA;                                       /**< Weight RPA. */
    float m_fWeightHPI;                                       /**< Weight HPI. */
    float m_fWeightHSP;                                       /**< Weight HSP (Head Shape Point). */
    float m_fWeightEEG;                                       /**< Weight EEG . */
    float m_fMaxDist;                                         /**< Maximum distace - electrode surface. */
    float m_fTol;                                             /**< Divergence Tolerance . */
    int m_iMaxIter;                                           /**< Maximum ammount of iterations. */

    QPointer<ANSHAREDLIB::Communicator>                     m_pCommu;

    DISPLIB::CoregSettingsView*                             m_pCoregSettingsView;   /**< Pointer to coreg GUI. */

    QFutureWatcher<FIFFLIB::FiffCoordTrans>                 m_FutureWatcher;
    QFuture<FIFFLIB::FiffCoordTrans>                        m_Future;
    QMutex                                                  m_ParameterMutex;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // COREGISTRATION_H
