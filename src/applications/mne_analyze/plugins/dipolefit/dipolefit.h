//=============================================================================================================
/**
 * @file     dipolefit.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.7
 * @date     October, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Gabriel Motta. All rights reserved.
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
 * @brief    Contains the declaration of the DipoleFit class.
 *
 */

#ifndef MNEANALYZE_DIPOLEFIT_H
#define MNEANALYZE_DIPOLEFIT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dipolefit_global.h"
#include <anShared/Plugins/abstractplugin.h>

#include <inverse/dipoleFit/dipole_fit_settings.h>

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
    class AbstractModel;
    class FiffRawViewModel;
    class BemDataModel;
    class MriCoordModel;
    class CovarianceModel;
    class AveragingDataModel;
    class Communicator;
}

//=============================================================================================================
// DEFINE NAMESPACE SURFERPLUGIN
//=============================================================================================================

namespace DIPOLEFITPLUGIN
{

//=============================================================================================================
/**
 * DipoleFit Plugin
 *
 * @brief The DipoleFit class provides a view with all currently loaded models.
 */
class DIPOLEFITSHARED_EXPORT DipoleFit : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "dipolefit.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs a DipoleFit.
     */
    DipoleFit();

    //=========================================================================================================
    /**
     * Destroys the DipoleFit.
     */
    virtual ~DipoleFit() override;

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
     * Performs dipole fit calculations
     */
    void onPerformDipoleFit(const QString& sFitName);

    //=========================================================================================================
    /**
     * Updates modalities in dipole settings
     *
     * @param[in] bEEG     whether to use EEG (true = yes, false = no).
     * @param[in] bMEG     whether to use MEG (true = yes, false = no).
     */
    void onModalityChanged(bool bEEG, bool bMEG);

    //=========================================================================================================
    /**
     * Set new time parameters in dipole settings
     *
     * @param[in] iMin     new minimum in milliseconds.
     * @param[in] iMax     new maximum in milliseconds.
     * @param[in] iStep    new step value in milliseconds.
     * @param[in] iInt     new integration value in milliseconds.
     */
    void onTimeChanged(int iMin, int iMax, int iStep, int iInt);

    //=========================================================================================================
    /**
     * Set new fitting values in dipole settings
     *
     * @param[in] iMinDistance     distance to inner skull in millimeters.
     * @param[in] iSize        radius guess in millimeters.
     */
    void onFittingChanged(float fMinDistance, float fSize);

    //=========================================================================================================
    /**
     * Loads new model whan current model is changed
     *
     * @param[in, out] pNewModel    pointer to newly selected model.
     */
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * Handles clearing view if currently used model is being removed
     *
     * @param[in] pRemovedModel    Pointer to model being removed.
     */
    void onModelRemoved(QSharedPointer<ANSHAREDLIB::AbstractModel> pRemovedModel);

    //=========================================================================================================
    /**
     * Set new baseline parameters
     *
     * @param[in] iBMin    baseline start time in milliseconds.
     * @param[in] iBMax    baseline end time in milliseconds.
     */
    void onBaselineChanged(int iBMin,
                           int iBMax);

    //=========================================================================================================
    /**
     * Set new manual noise parameters
     *
     * @param[in] dGrad    gradiometer value.
     * @param[in] dMag     magnetometer value.
     * @param[in] dEeg     eeg value.
     */
    void onNoiseChanged(double dGrad,
                        double dMag,
                        double dEeg);

    //=========================================================================================================
    /**
     * Set new regularization parameters
     *
     * @param[in] iReg         overall regularization parameter.
     * @param[in] iRegGrad     gradiatometer regularizaation parameter.
     * @param[in] iRegMag      magnetometer regularization parameter.
     * @param[in] iRegEeg      eeg regularization parameter.
     */
    void onRegChanged(double dRegGrad,
                      double dRegMag,
                      double dRegEeg);

    //=========================================================================================================
    /**
     * Select set from measurement to use
     *
     * @param[in] iSet.
     */
    void onSetChanged(int iSet);

    //=========================================================================================================
    /**
     * Set new spherer model parameters
     *
     * @param[in] dX           x position in millimeters.
     * @param[in] dY           y position in millimeters.
     * @param[in] dZ           z position in millimeters.
     * @param[in] dRadius      radius in millimeters.
     */
    void onSphereChanged(double dX,
                         double dY,
                         double dZ,
                         double dRadius);

    //=========================================================================================================
    /**
     * Sends new edc set to be added to 3dView
     *
     * @param[in] set      new EDCSet to be sent.
     */
    void newDipoleFit(INVERSELIB::ECDSet set, const QString& sFitName);

    //=========================================================================================================
    /**
     * Set new Bem model
     *
     * @param[in] sName    file name.
     */
    void onNewBemSelected(const QString& sName);

    //=========================================================================================================
    /**
     * Set new Mri model
     *
     * @param[in] sName    file name.
     */
    void onNewMriSelected(const QString& sName);

    //=========================================================================================================
    /**
     * Set new Noise model
     *
     * @param[in] sName    file name.
     */
    void onNewNoiseSelected(const QString& sName);

    //=========================================================================================================
    /**
     * Set new measurement model
     *
     * @param[in] sName    file name.
     */
    void onNewMeasSelected(const QString& sName);

    //=========================================================================================================
    /**
     * Performs dipole fit calculation. Meant to be run on separate thread.
     *
     * @return returs ECD set resulting from dipole fit.
     */
    INVERSELIB::ECDSet dipoleFitCalculation();

    //=========================================================================================================
    /**
     * @brief dipoleFitResults
     */
    void dipoleFitResults();

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

    QList<QSharedPointer<ANSHAREDLIB::AbstractModel>>       m_ModelList;            /**< List of models used in dipole fitting. Usded for storing for later selection. */
    INVERSELIB::DipoleFitSettings                           m_DipoleSettings;       /**< Settings for dipole fit. */
    QString                                                 m_sFitName;             /**< Fit name for dipole fit. */

    QPointer<ANSHAREDLIB::Communicator>                     m_pCommu;               /**< Communicator for sending events. */

    QMutex                                                  m_FitMutex;             /**< Mutex for thread-safing. */

    QFutureWatcher<INVERSELIB::ECDSet>                      m_FutureWatcher;        /**< Future watcher for notifing of completed fit calculations. */
    QFuture<INVERSELIB::ECDSet>                             m_Future;               /**< Future for performing fit calculations of separate thread. */

signals:

    //=========================================================================================================
    /**
     * New BEM file available (add to gui)
     *
     * @param[in] sModelName.
     */
    void newBemModel(const QString& sModelName);

    //=========================================================================================================
    /**
     * New noise file available (add to gui)
     *
     * @param[in] sModelName.
     */
    void newCovarianceModel(const QString& sModelName);

    //=========================================================================================================
    /**
     * New MRI file available (add to gui)
     *
     * @param[in] sModelName.
     */
    void newMriModel(const QString& sModelName);

    //=========================================================================================================
    /**
     * New measurement file available (add to gui)
     *
     * @param[in] sModelName.
     */
    void newMeasurment(const QString& sModelName);

    //=========================================================================================================
    /**
     * Removes model from view
     *
     * @param[in] sModelName   name of model to be removed.
     * @param[in] iType        type of model (1-measurement, 2-BEM, 3-MRI, 4-Cov).
     */
    void removeModel(const QString& sModelName, int iType);

    //=========================================================================================================
    /**
     * Request update of view parameter settings
     */
    void getUpdate();
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // DIPOLEFIT_H
