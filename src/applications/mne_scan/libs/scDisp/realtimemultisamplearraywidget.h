//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *
 * @file     realtimemultisamplearraywidget.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     February, 2013
 * @brief    Declaration of the RealTimeMultiSampleArrayWidget Class.
 */

#ifndef REALTIMEMULTISAMPLEARRAYWIDGET_H
#define REALTIMEMULTISAMPLEARRAYWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"
#include "measurementwidget.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>
#include <QMap>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace DISPLIB {
    class ChannelSelectionView;
    class ChannelInfoModel;
    class RtFiffRawView;
    class ChannelDataViewNew;
}

namespace FIFFLIB {
    class FiffInfo;
}

namespace SCMEASLIB{
    class RealTimeMultiSampleArray;
}

//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{

//=============================================================================================================
// SCDISPLIB FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * DECLARE CLASS RealTimeMultiSampleArrayWidget
 *
 * @brief The RealTimeMultiSampleArrayWidget class provides a real-time curve display.
 */
class SCDISPSHARED_EXPORT RealTimeMultiSampleArrayWidget : public MeasurementWidget
{
    Q_OBJECT

public:
    //=========================================================================================================
    /**
     * Constructs a RealTimeMultiSampleArrayWidget which is a child of parent.
     *
     * @param[in] pTime         pointer to application time.
     * @param[in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
     */
    RealTimeMultiSampleArrayWidget(QSharedPointer<QTime> &pTime,
                                   QWidget* parent = 0);

    //=========================================================================================================
    /**
     * Destroys the RealTimeMultiSampleArrayWidget.
     */
    ~RealTimeMultiSampleArrayWidget();

    //=========================================================================================================
    /**
     * Initialise the MeasurementWidget.
     */
    virtual void init(){}

    //=========================================================================================================
    /**
     * Is called when new data are available.
     *
     * @param[in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
     */
    virtual void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Update the viewport. This, e.g., necessary if this widget was set to a QDockWidget which changes
     * its floating state.
     */
    void updateViewport();

private:
    //=========================================================================================================
    /**
     * Initialise the display control widgets to be shown in the QuickControlView.
     */
    void initDisplayControllWidgets();

    //=========================================================================================================
    /**
     * Shows sensor selection widget
     */
    void showSensorSelectionWidget();

    //=========================================================================================================
    /**
     * Call this slot whenever you want to make a screenshot current view.
     *
     * @param[in] imageType  The current iamge type: png, svg.
     */
    void onMakeScreenshot(const QString& imageType);

    //=========================================================================================================
    /**
     * Toggle bad channel visibility
     */
    void onHideBadChannels();

private:
    QSharedPointer<SCMEASLIB::RealTimeMultiSampleArray>     m_pRTMSA;                       /**< The real-time sample array measurement. */

    QSharedPointer<DISPLIB::QuickControlView>               m_pQuickControlView;            /**< quick control widget. */
    QSharedPointer<DISPLIB::ChannelInfoModel>               m_pChannelInfoModel;            /**< channel info model. */
    QSharedPointer<DISPLIB::ChannelSelectionView>           m_pChannelSelectionView;        /**< ChannelSelectionView. */
    QPointer<DISPLIB::RtFiffRawView>                        m_pChannelDataView;             /**< the QTableView being part of the model/view framework of Qt. */

    QSharedPointer<FIFFLIB::FiffInfo>                       m_pFiffInfo;                    /**< FiffInfo, which is used insteadd of ListChInfo*/

    QPointer<QAction>                                       m_pActionHideBad;               /**< Hide bad channels. */

    qint32                                                  m_iMaxFilterTapSize;            /**< Maximum number of allowed filter taps. This number depends on the size of the receiving blocks. */
};
} // NAMESPACE SCDISPLIB

#endif // REALTIMEMULTISAMPLEARRAYWIDGET_H
