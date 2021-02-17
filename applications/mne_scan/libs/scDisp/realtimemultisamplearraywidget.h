//=============================================================================================================
/**
 * @file     realtimemultisamplearraywidget.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the RealTimeMultiSampleArrayWidget Class.
 *
 */

#ifndef REALTIMEMULTISAMPLEARRAYWIDGET_H
#define REALTIMEMULTISAMPLEARRAYWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"
#include "measurementwidget.h"

#include <events/eventmanager.h>

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
     * Update the OpenGL viewport. This, e.g., necessary if this widget was set to a QDockWidget which changes
     * its floating state.
     */
    void updateOpenGLViewport();

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
