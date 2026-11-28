//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2020-2026 MNE-CPP Authors
 *
 * @file     averaging.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.3
 * @date     May, 2020
 * @brief    Averaging class declaration.
 */

#ifndef AVERAGING_H
#define AVERAGING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging_global.h"
#include <anShared/Plugins/abstractplugin.h>
#include <mne/dsp/filterkernel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>

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
class FiffRawViewModel;
class AbstractModel;
class AveragingDataModel;
class Communicator;
}

namespace mne::viz
{
class AverageLayoutPanel;
}

namespace mne::viz::qt
{
class AveragingSettingsView;
class AverageSelectionView;
class FiffRawViewSettings;
class ChannelSelectionView;
class PanelWidget;
class ChannelInfoModel;
class EvokedSetModel;
class ButterflyView;
class SelectionItem;
}

namespace mne
{
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
class AVERAGINGSHARED_EXPORT Averaging : public mne::analyze::AbstractPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "ansharedlib/1.0" FILE "averaging.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(mne::analyze::AbstractPlugin)

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

    virtual void handleEvent(QSharedPointer<mne::analyze::Event> e) override;
    virtual QVector<mne::analyze::EVENT_TYPE> getEventSubscriptions() const override;

signals:
    //=========================================================================================================
    void showSelectedChannels(const std::vector<int>& selectedChannelsIndexes);
    void showAllChannels();
    void channelSelectionManagerChanged(const QVariant& data);
    void layoutChanged(const QMap<QString, QPointF>& layoutMap);

private:
    //=========================================================================================================
    /**
     * Loads new Fiff model whan current loaded model is changed
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

    //=========================================================================================================
    /**
     * @brief onNewAveragingModel
     * @param pAveragingModel.
     */
    void onNewAveragingModel(QSharedPointer<mne::analyze::AveragingDataModel> pAveragingModel);

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
     * Triggers averageCalculations to be run on a worker thread.
     */
    void computeAverage();

    //=========================================================================================================
    /**
     * Calculates average and returns FiffEvoked Set. (Run in separate thread)
     *
     * @return Retruns FiffEvoked setwith averaged data.
     */
    std::shared_ptr<mne::FiffEvokedSet> averageCalculation(const mne::FiffRawData& pFiffRaw,
                                                           MatrixXi matEvents,
                                                           mne::FilterKernel filterKernel,
                                                           mne::FiffInfo fiffInfo);

    //=========================================================================================================
    /**
     * Receives FiffEvoked set from the worker thread and created new averaging model
     */
    void createNewAverage();

    //=========================================================================================================
    /**
     * Toggles dropping rejected when computing average
     */
    void onRejectionChecked(bool bState);

    //=========================================================================================================
    /**
     *  Loads averging GUI components that are dependent on FiffRawModel to be initialized
     */
    void loadFullGui(std::shared_ptr<mne::FiffInfo> pInfo);

    //=========================================================================================================
    /**
     * Sets channel selection for butterfly and 2D layout view based on QVariant with a SelectionItem object
     *
     * @param[in] data     QVariant with a SelectionItem object with channel selection information.
     */
    void setChannelSelection(const QVariant& data);

    //=========================================================================================================
    /**
     * Sets scaling map for averaging views
     *
     * @param[in] data     QVariant with a ScalingParameters object with relevant scaling data.
     */
    void setScalingMap(const QVariant& data);

    //=========================================================================================================
    /**
     * Sets the view settings for the averaging views
     *
     * @param pViewParams.
     */
    void setViewSettings(mne::analyze::ViewParameters viewParams);

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

    QSharedPointer<mne::analyze::FiffRawViewModel> m_pFiffRawModel;      /**< Pointer to currently loaded FiffRawView Model. */
    QSharedPointer<QList<QPair<int, double>>> m_pTriggerList;            /**< Pointer to list of stim triggers. */
    std::shared_ptr<mne::viz::qt::EvokedSetModel> m_pEvokedModel;        /**< Pointer to model used to display averaging data from m_pFiffEvokedSet and m_pFiffEvoked. */
    std::shared_ptr<mne::viz::qt::ChannelInfoModel> m_pChannelInfoModel; /**< Pointer to model that holds channel info data. */
    std::shared_ptr<mne::FiffInfo> m_pFiffInfo;                          /**< Pointer to info about loaded fiff data. */

    QPointer<mne::analyze::Communicator> m_pCommu;                  /**< To broadcst signals. */
    QPointer<mne::viz::qt::ButterflyView> m_pButterflyView;         /**< The butterfly plot view. */
    std::shared_ptr<mne::viz::AverageLayoutPanel> m_pAverageLayout; /**< The average layout plot. */
    QPointer<mne::viz::qt::PanelWidget> m_pAverageLayoutView;       /**< The average layout plot view. */

    mne::viz::qt::AveragingSettingsView* m_pAveragingSettingsView; /**< Pointer to averaging settings GUI. */

    std::unique_ptr<mne::viz::qt::AverageSelectionView> m_pAverageSelectionView;   /**< Holds the average selection; not shown. */
    std::unique_ptr<mne::viz::qt::FiffRawViewSettings> m_pChannelDataSettingsView; /**< Holds the view settings; not shown. */

    float m_fBaselineFromS;    /**< Baseline start - in seconds relative to stim(0) - can be negative*/
    float m_fBaselineToS;      /**< Baseline end - in seconds relative to stim(0) - can be negative*/
    float m_fPreStim;          /**< Time before stim - in seconds - stored as positive number (>0). */
    float m_fPostStim;         /**< Time after stim - in seconds - stored as positive number (>0). */
    float m_fTriggerThreshold; /**< Threshold to count stim channel events. */

    QVBoxLayout* m_pLayout; /**< Pointer to layout that holds parameter GUI tab elements. */
    QTabWidget* m_pTabView; /**< Pointer to object that stores multiple tabs of GUI items. */

    bool m_bBaseline;         /**< Whether to apply baseline correction. */
    bool m_bRejection;        /**< Whether to drop data points marked fro rejection when calculating average. */
    bool m_bLoaded;           /**< Whether the full GUI has already been laoaded. */
    bool m_bPerformFiltering; /**< Flag whether to activate/deactivate filtering. */
    bool m_bAutoRecompute;    /**< Whether to auto recompute averages */
    bool m_bSavingAverage;    /**< Whether to save average */

    mne::FilterKernel m_filterKernel; /**< List of currently active filters. */

    std::future<std::shared_ptr<mne::FiffEvokedSet>> m_Future; /**< Future for performing average calculations of separate thread; valid until the result is consumed. */

    QMutex m_ParameterMutex; /**< Mutex for thread-safing. */
};

} // NAMESPACE

#endif // AVERAGING_H
