//=============================================================================================================
/**
 * @file     realtimeevokedsetwidget.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the RealTimeEvokedSetWidget Class.
 *
 */

#ifndef REALTIMEEVOKEDSETWIDGET_H
#define REALTIMEEVOKEDSETWIDGET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"

#include "measurementwidget.h"

#include <disp/viewers/butterflyview.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QPointer>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace SCMEASLIB {
    class RealTimeEvokedSet;
    class RealTimeSampleArrayChInfo;
}

namespace FIFFLIB {
    class FiffInfo;
}

namespace DISPLIB {
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
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{

//=============================================================================================================
// SCDISPLIB FORWARD DECLARATIONS
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
    typedef QSharedPointer<RealTimeEvokedSetWidget> SPtr;              /**< Shared pointer type for RealTimeEvokedSetWidget. */
    typedef QSharedPointer<const RealTimeEvokedSetWidget> ConstSPtr;   /**< Const shared pointer type for RealTimeEvokedSetWidget. */

    //=========================================================================================================
    /**
     * Constructs a RealTimeEvokedSetWidget which is a child of parent.
     *
     * @param[in] pTime         pointer to application time.
     * @param[in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
     */
    RealTimeEvokedSetWidget(QSharedPointer<QTime> &pTime,
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
    virtual void init(){}

    //=========================================================================================================
    /**
     * Is called when new data are available.
     *
     * @param[in] pMeasurement  pointer to measurement.
     */
    virtual void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
     * Update the OpenGL viewport. This, e.g., necessary if this widget was set to a QDockWidget which changes
     * its floating state.
     */
    void updateOpenGLViewport();

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
    bool virtual eventFilter(QObject *object, QEvent *event);

    QSharedPointer<DISPLIB::EvokedSetModel>             m_pEvokedSetModel;          /**< RTE data model. */
    QSharedPointer<SCMEASLIB::RealTimeEvokedSet>        m_pRTESet;                  /**< The real-time evoked measurement. */
    QSharedPointer<DISPLIB::ChannelSelectionView>       m_pChannelSelectionView;    /**< ChannelSelectionView. */
    QSharedPointer<DISPLIB::ChannelInfoModel>           m_pChannelInfoModel;        /**< Channel info model. */
    QSharedPointer<FIFFLIB::FiffInfo>                   m_pFiffInfo;                /**< FiffInfo, which is used instead of ListChInfo*/
    QPointer<DISPLIB::AverageLayoutView>                m_pAverageLayoutView;       /**< 2D layout view for plotting averages*/
    QPointer<DISPLIB::ButterflyView>                    m_pButterflyView;           /**< Butterfly plot. */

    QList<qint32>                       m_qListCurrentSelection;    /**< Current selection list -> hack around C++11 lambda . */

    bool                                m_bHideBadChannels;         /**< Hide bad channels flag. */
    qint32                              m_iMaxFilterTapSize;        /**< Maximum number of allowed filter taps. This number depends on the size of the receiving blocks. */

    QPointer<QVBoxLayout>               m_pRTESetLayout;            /**< RTE Widget layout. */
    QPointer<QLabel>                    m_pLabelInit;               /**< Initialization Label. */
    QPointer<QToolBox>                  m_pToolBox;                 /**< The toolbox which holds the butterfly and 2D layout plot. */

signals:
    void windowSizeChanged(int iWindowSize);
};
} // NAMESPACE SCDISPLIB

#endif // REALTIMEEVOKEDSETWIDGET_H
