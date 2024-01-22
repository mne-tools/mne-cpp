//=============================================================================================================
/**
 * @file     averaging.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.3
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
 * @brief    Averaging class declaration.
 *
 */

#ifndef AVERAGING_H
#define AVERAGING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging_global.h"

#include <anShared/Plugins/abstractplugin.h>

#include <rtprocessing/helpers/filterkernel.h>

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
    class AveragingDataModel;
    class Communicator;
}

namespace DISPLIB {
    class AveragingSettingsView;
    class ChannelSelectionView;
    class AverageLayoutView;
    class ChannelInfoModel;
    class EvokedSetModel;
    class ButterflyView;
    class SelectionItem;
}

namespace FIFFLIB {
    class FiffEvokedSet;
    class FiffEvoked;
    class FiffInfo;
    class FiffRawData;
}

//=============================================================================================================
// DEFINE NAMESPACE AVERAGINGPLUGIN
//=============================================================================================================

namespace AVERAGINGPLUGIN
{

//=============================================================================================================
// AVERAGINGPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * Averaging Plugin
 *
 * @brief The averaging class provides a plugin for computing averages.
 */
class AVERAGINGSHARED_EXPORT Averaging : public ANSHAREDLIB::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "averaging.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(ANSHAREDLIB::AbstractPlugin)

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

signals:
    //=========================================================================================================
    void showSelectedChannels(const QList<int> selectedChannelsIndexes);
    void showAllChannels();
    void channelSelectionManagerChanged(const QVariant &data);
    void layoutChanged(const QMap<QString,QPointF> &layoutMap);
private:
    //=========================================================================================================
    /**
     * Loads new Fiff model whan current loaded model is changed
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

    //=========================================================================================================
    /**
     * @brief onNewAveragingModel
     * @param pAveragingModel.
     */
    void onNewAveragingModel(QSharedPointer<ANSHAREDLIB::AveragingDataModel> pAveragingModel);

    //=========================================================================================================
    /**
     * Change the number of averages
     *
     * @param[in] numAve     new number of averages.
     */
    void onChangeNumAverages(qint32 numAve);

    //=========================================================================================================
    /**
     * Change the baseline from value
     *
     * @param[in] fromMS     the new baseline from value in milliseconds.
     */
    void onChangeBaselineFrom(qint32 fromMS);

    //=========================================================================================================
    /**
     * Change the baseline to value
     *
     * @param[in] fromMS     the new baseline to value in milliseconds.
     */
    void onChangeBaselineTo(qint32 toMS);

    //=========================================================================================================
    /**
     * Change the pre stim stim
     *
     * @param[in] mseconds     the new pres stim in milliseconds.
     */
    void onChangePreStim(qint32 mseconds);

    //=========================================================================================================
    /**
     * Change the post stim stim
     *
     * @param[in] mseconds     the new post stim in milliseconds.
     */
    void onChangePostStim(qint32 mseconds);

    //=========================================================================================================
    /**
     * Change the baseline active state
     *
     * @param[in] state     the new state.
     */
    void onChangeBaselineActive(bool state);

    //=========================================================================================================
    /**
     * Reset the averaging plugin and delete all currently stored data
     *
     * @param[in] state     the new state.
     */
    void onResetAverage(bool state);

    //=========================================================================================================
    /**
     * Gets called when compute button on GUI is clicked
     *
     * @param[in] bChecked     UNUSED - state of the button.
     */
    void onComputeButtonClicked(bool bChecked);

    //=========================================================================================================
    /**
     * Triggers averageCalculations to be run with QFuture.
     */
    void computeAverage();

    //=========================================================================================================
    /**
     * Calculates average and returns FiffEvoked Set. (Run in separate thread with QFuture)
     *
     * @return Retruns FiffEvoked setwith averaged data.
     */
    QSharedPointer<FIFFLIB::FiffEvokedSet> averageCalculation(FIFFLIB::FiffRawData pFiffRaw,
                                                              MatrixXi matEvents,
                                                              RTPROCESSINGLIB::FilterKernel filterKernel,
                                                              FIFFLIB::FiffInfo fiffInfo);

    //=========================================================================================================
    /**
     * Receives FiffEvoked set from QFuture and created new averaging model
     */
    void createNewAverage();

    //=========================================================================================================
    /**
     * Toggles dropping rejected when computing average
     */
    void onRejectionChecked(bool bState);

    //=========================================================================================================
    /**
     * Connected to GUI dropdown to select group based on group name input.
     *
     * @param[in] text  name of group selected in the GUI.
     */
    void onChangeGroupSelect(int iId);

    //=========================================================================================================
    /**
     *  Loads averging GUI components that are dependent on FiffRawModel to be initialized
     */
    void loadFullGui(QSharedPointer<FIFFLIB::FiffInfo> pInfo);

    //=========================================================================================================
    /**
     * Sets channel selection for butterfly and 2D layout view based on QVariant with a SelectionItem object
     *
     * @param[in] data     QVariant with a SelectionItem object with channel selection information.
     */
    void setChannelSelection(const QVariant &data);

    //=========================================================================================================
    /**
     * Sets scaling map for averaging views
     *
     * @param[in] data     QVariant with a ScalingParameters object with relevant scaling data.
     */
    void setScalingMap(const QVariant &data);

    //=========================================================================================================
    /**
     * Sets the view settings for the averaging views
     *
     * @param pViewParams.
     */
    void setViewSettings(ANSHAREDLIB::ViewParameters viewParams);

    //=========================================================================================================
    /**
     * Call this slot whenever you want to make a screenshot of the butterfly or layout view.
     *
     * @param[out] imageType     The current iamge type: png, svg.
     */
    void onMakeScreenshot(const QString& imageType);

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

    void setAutoCompute(bool bShouldAutoCompute);

    void updateEvokedSetModel();

    QSharedPointer<ANSHAREDLIB::FiffRawViewModel>           m_pFiffRawModel;            /**< Pointer to currently loaded FiffRawView Model. */
    QSharedPointer<QList<QPair<int,double>>>                m_pTriggerList;             /**< Pointer to list of stim triggers. */
    QSharedPointer<DISPLIB::EvokedSetModel>                 m_pEvokedModel;             /**< Pointer to model used to display averaging data from m_pFiffEvokedSet and m_pFiffEvoked. */
    QSharedPointer<DISPLIB::ChannelInfoModel>               m_pChannelInfoModel;        /**< Pointer to model that holds channel info data. */
    QSharedPointer<FIFFLIB::FiffInfo>                       m_pFiffInfo;                /**< Pointer to info about loaded fiff data. */

    QPointer<ANSHAREDLIB::Communicator>                     m_pCommu;                   /**< To broadcst signals. */
    QPointer<DISPLIB::ButterflyView>                        m_pButterflyView;           /**< The butterfly plot view. */
    QPointer<DISPLIB::AverageLayoutView>                    m_pAverageLayoutView;       /**< The average layout plot view. */

    DISPLIB::AveragingSettingsView*                         m_pAveragingSettingsView;   /**< Pointer to averaging settings GUI. */

    float                                                   m_fBaselineFromS;            /**< Baseline start - in seconds relative to stim(0) - can be negative*/
    float                                                   m_fBaselineToS;              /**< Baseline end - in seconds relative to stim(0) - can be negative*/
    float                                                   m_fPreStim;                 /**< Time before stim - in seconds - stored as positive number (>0). */
    float                                                   m_fPostStim;                /**< Time after stim - in seconds - stored as positive number (>0). */
    float                                                   m_fTriggerThreshold;        /**< Threshold to count stim channel events. */

    QVBoxLayout*                                            m_pLayout;                  /**< Pointer to layout that holds parameter GUI tab elements. */
    QTabWidget*                                             m_pTabView;                 /**< Pointer to object that stores multiple tabs of GUI items. */

    bool                                                    m_bBaseline;                /**< Whether to apply baseline correction. */
    bool                                                    m_bRejection;               /**< Whether to drop data points marked fro rejection when calculating average. */
    bool                                                    m_bLoaded;                  /**< Whether the full GUI has already been laoaded. */
    bool                                                    m_bPerformFiltering;        /**< Flag whether to activate/deactivate filtering. */
    bool                                                    m_bAutoRecompute;           /**< Whether to auto recompute averages */
    bool                                                    m_bSavingAverage;           /**< Whether to save average */

    RTPROCESSINGLIB::FilterKernel                           m_filterKernel;             /**< List of currently active filters. */

    QFutureWatcher<QSharedPointer<FIFFLIB::FiffEvokedSet>>  m_FutureWatcher;            /**< Future watcher for notifing of completed average calculations. */
    QFuture<QSharedPointer<FIFFLIB::FiffEvokedSet>>         m_Future;                   /**< Future for performing average calculations of separate thread. */

    QMutex                                                  m_ParameterMutex;           /**< Mutex for thread-safing. */

};

} // NAMESPACE

#endif // AVERAGING_H
