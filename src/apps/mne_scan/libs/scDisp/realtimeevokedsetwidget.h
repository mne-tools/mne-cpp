//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     realtimeevokedsetwidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     July, 2018
 * @brief    Declaration of the RealTimeEvokedSetWidget Class.
 */

#ifndef REALTIMEEVOKEDSETWIDGET_H
#define REALTIMEEVOKEDSETWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"
#include "measurementwidget.h"
#include <mne/viz/backends/qt/viewers/butterflyview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne::scan
{
class RealTimeEvokedSet;
class RealTimeSampleArrayChInfo;
}

namespace mne
{
class FiffInfo;
}

namespace mne::viz::qt
{
class EvokedSetModel;
class ButterflyView;
class ChannelSelectionView;
class ChannelInfoModel;
class AverageLayoutView;
}

class QVBoxLayout;
class QLabel;
class QToolBox;

//=============================================================================================================
// DEFINE NAMESPACE mne::scan
//=============================================================================================================

namespace mne::scan
{

//=============================================================================================================
// mne::scan FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS RealTimeEvokedSetWidget
 *
 * @brief The RealTimeEvokedSetWidget class provides a real-time display for multiple averages.
 */
class SCDISPSHARED_EXPORT RealTimeEvokedSetWidget : public MeasurementWidget
{
    Q_OBJECT

public:
    typedef QSharedPointer<RealTimeEvokedSetWidget> SPtr;            /**< Shared pointer type for RealTimeEvokedSetWidget. */
    typedef QSharedPointer<const RealTimeEvokedSetWidget> ConstSPtr; /**< Const shared pointer type for RealTimeEvokedSetWidget. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeEvokedSetWidget which is a child of parent.
     *
     * @param[in] pTime         pointer to application time.
     * @param[in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
     */
    RealTimeEvokedSetWidget(QSharedPointer<QTime>& pTime,
                            QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeEvokedSetWidget.
     */
    ~RealTimeEvokedSetWidget();

    //=========================================================================================================
    /**
     * Initialise the MeasurementWidget.
     */
    virtual void init()
    {
    }

    //=========================================================================================================
    /**
     * Is called when new data are available.
     *
     * @param[in] pMeasurement  pointer to measurement.
     */
    virtual void update(mne::scan::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Update the viewport. This, e.g., necessary if this widget was set to a QDockWidget which changes
     * its floating state.
     */
    void updateViewport();

private slots:
    //=========================================================================================================
    /**
     * Shows sensor selection widget
     */
    void showSensorSelectionWidget();

    //=========================================================================================================
    /**
     * Call this slot whenever you want to make a screenshot of the butterfly or layout view.
     *
     * @param[out] imageType     The current iamge type: png, svg.
     */
    void onMakeScreenshot(const QString& imageType);

private:
    //=========================================================================================================
    /**
     * Initialise the display control widgets to be shown in the QuickControlView.
     */
    void initDisplayControllWidgets();

    //=========================================================================================================
    /**
     * Reimplemented eventFilter
     */
    bool virtual eventFilter(QObject* object, QEvent* event);

    std::shared_ptr<mne::viz::qt::EvokedSetModel> m_pEvokedSetModel;             /**< RTE data model. */
    QSharedPointer<mne::scan::RealTimeEvokedSet> m_pRTESet;                      /**< The real-time evoked measurement. */
    std::shared_ptr<mne::viz::qt::ChannelSelectionView> m_pChannelSelectionView; /**< ChannelSelectionView. */
    std::shared_ptr<mne::viz::qt::ChannelInfoModel> m_pChannelInfoModel;         /**< Channel info model. */
    std::shared_ptr<mne::FiffInfo> m_pFiffInfo;                                  /**< FiffInfo, which is used instead of ListChInfo*/
    QPointer<mne::viz::qt::AverageLayoutView> m_pAverageLayoutView;              /**< 2D layout view for plotting averages*/
    QPointer<mne::viz::qt::ButterflyView> m_pButterflyView;                      /**< Butterfly plot. */

    QList<qint32> m_qListCurrentSelection; /**< Current selection list -> hack around C++11 lambda . */

    bool m_bHideBadChannels;    /**< Hide bad channels flag. */
    qint32 m_iMaxFilterTapSize; /**< Maximum number of allowed filter taps. This number depends on the size of the receiving blocks. */

    QPointer<QVBoxLayout> m_pRTESetLayout; /**< RTE Widget layout. */
    QPointer<QLabel> m_pLabelInit;         /**< Initialization FsLabel. */
    QPointer<QToolBox> m_pToolBox;         /**< The toolbox which holds the butterfly and 2D layout plot. */

signals:
    void windowSizeChanged(int iWindowSize);
};
} // NAMESPACE mne::scan

#endif // REALTIMEEVOKEDSETWIDGET_H
