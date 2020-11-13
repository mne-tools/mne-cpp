//=============================================================================================================
/**
 * @file     dipolefit.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
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

#ifndef DIPOLEFITMANAGER_H
#define DIPOLEFITMANAGER_H

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
    class NoiseModel;
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
     * @param [in] bEEG     whether to use EEG (true = yes, false = no)
     * @param [in] bMEG     whether to use MEG (true = yes, false = no)
     */
    void onModalityChanged(bool bEEG, bool bMEG);

    //=========================================================================================================
    /**
     * Set new time parameters in dipole settings
     *
     * @param [in] iMin     new minimum in milliseconds
     * @param [in] iMax     new maximum in milliseconds
     * @param [in] iStep    new step value in milliseconds
     * @param [in] iInt     new integration value in milliseconds
     */
    void onTimeChanged(int iMin, int iMax, int iStep, int iInt);

    //=========================================================================================================
    /**
     * Set new fitting values in dipole settings
     *
     * @param [in] iMinDistance     distance to inner skull in millimeters
     * @param [in] iSize        radius guess in millimeters
     */
    void onFittingChanged(float fMinDistance, float fSize);

    //=========================================================================================================
    /**
     * Loads new model whan current model is changed
     *
     * @param [in,out] pNewModel    pointer to newly selected model
     */
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * @brief onBaselineChanged
     * @param iBMin
     * @param iBMax
     */
    void onBaselineChanged(int iBMin, int iBMax);

    //=========================================================================================================
    /**
     * @brief onNoiseChanged
     *
     * @param dGrad
     * @param dMag
     * @param dEeg
     */
    void onNoiseChanged(double dGrad,
                        double dMag,
                        double dEeg);

    //=========================================================================================================
    /**
     * @brief onRegChanged
     *
     * @param iReg
     * @param iRegGrad
     * @param iRegMag
     * @param iRegEeg
     */
    void onRegChanged(double dRegGrad,
                      double dRegMag,
                      double dRegEeg);

    //=========================================================================================================
    /**
     * @brief onSetChanged
     *
     * @param iSet
     */
    void onSetChanged(int iSet);

    //=========================================================================================================
    /**
     * @brief onSphereChanged
     *
     * @param dX
     * @param dY
     * @param dZ
     * @param dRadius
     */
    void onSphereChanged(double dX,
                         double dY,
                         double dZ,
                         double dRadius);

    //=========================================================================================================
    /**
     * Sends new edc set to be added to 3dView
     *
     * @param [in] set      new EDCSet to be sent
     */
    void newDipoleFit(INVERSELIB::ECDSet set, const QString& sFitName);

    //=========================================================================================================
    /**
     * @brief onNewBemSelected
     *
     * @param sName
     */
    void onNewBemSelected(const QString& sName);

    //=========================================================================================================
    /**
     * @brief onNewMriSelected
     *
     * @param sName
     */
    void onNewMriSelected(const QString& sName);

    //=========================================================================================================
    /**
     * @brief onNewNoiseSelected
     *
     * @param sName
     */
    void onNewNoiseSelected(const QString& sName);

    //=========================================================================================================
    /**
     * @brief onNewMeasSelected
     *
     * @param sName
     */
    void onNewMeasSelected(const QString& sName);

    QList<QSharedPointer<ANSHAREDLIB::AbstractModel>>       m_ModelList;

    QPointer<ANSHAREDLIB::Communicator>                     m_pCommu;
    INVERSELIB::DipoleFitSettings                           m_DipoleSettings;

    QMutex                                                  m_FitMutex;

signals:

    //=========================================================================================================
    /**
     * @brief newBemModel
     * @param sModelName
     */
    void newBemModel(const QString& sModelName);

    //=========================================================================================================
    /**
     * @brief newNoiseModel
     * @param sModelName
     */
    void newNoiseModel(const QString& sModelName);

    //=========================================================================================================
    /**
     * @brief newMriModel
     * @param sModelName
     */
    void newMriModel(const QString& sModelName);

    //=========================================================================================================
    void newMeasurment(const QString& sModelName);

    //=========================================================================================================
    void getUpdate();
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

} // NAMESPACE

#endif // DIPOLEFIT_H
