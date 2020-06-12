//=============================================================================================================
/**
 * @file     averaging.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.2
 * @date     May, 2020
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
 * @brief    Contains the declaration of the averaging class.
 *
 */

#ifndef AVERAGING_H
#define AVERAGING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging_global.h"
#include "ave.h"

#include <anShared/Interfaces/IPlugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace ANSHAREDLIB {
    class FiffRawViewModel;
    class AbstractModel;
    class Communicator;
}

namespace DISPLIB {
    class AveragingSettingsView;
    class AverageLayoutView;
    class EvokedSetModel;
    class ButterflyView;
}

namespace FIFFLIB {
    class FiffEvoked;
}

//=============================================================================================================
// DEFINE NAMESPACE averagingPLUGIN
//=============================================================================================================

namespace AVERAGINGPLUGIN
{

//=============================================================================================================
// AVERAGINGPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * averaging Plugin
 *
 * @brief The averaging class provides input and output capabilities for the fiff file format.
 */
class AVERAGINGSHARED_EXPORT Averaging : public ANSHAREDLIB::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "averaging.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::IPlugin)

public:
    //=========================================================================================================
    /**
     * Constructs an Averaging object.
     */
    Averaging();

    //=========================================================================================================
    /**
     * Destroys the Averaging object.
     */
    ~Averaging() override;

    // IPlugin functions
    virtual QSharedPointer<IPlugin> clone() const override;
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
     * Loads new Fiff model whan current loaded model is changed
     *
     * @param [in,out] pNewModel    pointer to currently loaded FiffRawView Model
     */
    void onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel);

    //=========================================================================================================
    /**
     * Change the number of averages
     *
     * @param[in] numAve     new number of averages
     */
    void onChangeNumAverages(qint32 numAve);

    //=========================================================================================================
    /**
     * Change the baseline from value
     *
     * @param[in] fromMSeconds     the new baseline from value in seconds
     */
    void onChangeBaselineFrom(qint32 fromMSeconds);

    //=========================================================================================================
    /**
     * Change the baseline to value
     *
     * @param[in] fromMSeconds     the new baseline to value in seconds
     */
    void onChangeBaselineTo(qint32 toMSeconds);

    //=========================================================================================================
    /**
     * Change the pre stim stim
     *
     * @param[in] mseconds     the new pres stim in seconds
     */
    void onChangePreStim(qint32 mseconds);

    //=========================================================================================================
    /**
     * Change the post stim stim
     *
     * @param[in] mseconds     the new post stim in seconds
     */
    void onChangePostStim(qint32 mseconds);

    //=========================================================================================================
    /**
     * Change the baseline active state
     *
     * @param[in] state     the new state
     */
    void onChangeBaselineActive(bool state);

    //=========================================================================================================
    /**
     * Reset the averaging plugin and delete all currently stored data
     *
     * @param[in] state     the new state
     */
    void onResetAverage(bool state);

    //=========================================================================================================
    void onComputeButtonClicked(bool bChecked);

    void onCheckBoxStateChanged();

    QPointer<ANSHAREDLIB::Communicator>                     m_pCommu;                   /**< To broadcst signals */

    QSharedPointer<ANSHAREDLIB::FiffRawViewModel>           m_pFiffRawModel;            /**< Pointer to currently loaded FiffRawView Model */
    QSharedPointer<QList<QPair<int,double>>>                m_pTriggerList;
    QSharedPointer<FIFFLIB::FiffEvoked>                     m_pFiffEvoked;
    QSharedPointer<FIFFLIB::FiffEvokedSet>                  m_pFiffEvokedSet;
    QSharedPointer<Ave>                                     m_pAve;                     /**< Averaging Object */
    QSharedPointer<DISPLIB::EvokedSetModel>                 m_pEvokedModel;

    DISPLIB::AveragingSettingsView*                         m_pAveragingSettingsView;

    QPointer<DISPLIB::ButterflyView>                        m_pButterflyView;       /**< The butterfly plot view. */
    QPointer<DISPLIB::AverageLayoutView>                    m_pAverageLayoutView;   /**< The average layout plot view */


    FIFFLIB::FiffInfo::SPtr                                 m_pFiffInfo;

    int                                                     m_iNumAve;
    int                                                     m_iBaselineFrom;
    int                                                     m_iBaselineTo;

    float                                                   m_fPreStim;
    float                                                   m_fPostStim;

    bool m_bUseAnn;

    QRadioButton* m_pAnnCheck;
    QRadioButton* m_pStimCheck;


};

} // NAMESPACE

#endif // AVERAGING_H
