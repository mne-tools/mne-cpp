//=============================================================================================================
/**
* @file     realtimeevokedwidget.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Declaration of the RealTimeEvokedWidget Class.
*
*/

#ifndef REALTIMEEVOKEDWIDGET_H
#define REALTIMEEVOKEDWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "xdisp_global.h"
#include "newmeasurementwidget.h"
#include "helpers/realtimeevokedmodel.h"
#include "helpers/realtimebutterflyplot.h"

#include "helpers/evokedmodalitywidget.h"
#include "helpers/sensorwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>
#include <QAction>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVBoxLayout>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace XMEASLIB
{
class RealTimeEvoked;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE XDISPLIB
//=============================================================================================================

namespace XDISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// ENUMERATIONS
//=============================================================================================================

////=============================================================================================================
///**
//* Tool enumeration.
//*/
//enum Tool
//{
//    Freeze     = 0,       /**< Freezing tool. */
//    Annotation = 1        /**< Annotation tool. */
//};

struct Modality {
    QString m_sName;
    bool m_bActive;
    float m_fNorm;

    Modality(QString name, bool active, double norm)
    : m_sName(name), m_bActive(active), m_fNorm(norm)
    {}
};


//=============================================================================================================
/**
* DECLARE CLASS RealTimeMultiSampleArrayNewWidget
*
* @brief The RealTimeMultiSampleArrayNewWidget class provides a real-time curve display.
*/
class XDISPSHARED_EXPORT RealTimeEvokedWidget : public NewMeasurementWidget
{
    Q_OBJECT

    friend class EvokedModalityWidget;
public:
    //=========================================================================================================
    /**
    * Constructs a RealTimeEvokedWidget which is a child of parent.
    *
    * @param [in] pRTE          pointer to real-time evoked measurement.
    * @param [in] pTime         pointer to application time.
    * @param [in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
    */
    RealTimeEvokedWidget(QSharedPointer<RealTimeEvoked> pRTE, QSharedPointer<QTime> &pTime, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the RealTimeEvokedWidget.
    */
    ~RealTimeEvokedWidget();

    //=========================================================================================================
    /**
    * Broadcast settings to attached widgets
    */
    void broadcastSettings();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    *
    * @param [in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
    */
    virtual void update(XMEASLIB::NewMeasurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Is called when new data are available.
    */
    virtual void getData();

    //=========================================================================================================
    /**
    * Initialise the RealTimeEvokedWidget.
    */
    virtual void init();

private:
    //=========================================================================================================
    /**
    * Shows sensor selection widget
    */
    void showSensorSelectionWidget();

    //=========================================================================================================
    /**
    * Show the modality selection widget
    */
    void showModalitySelectionWidget();


    QVBoxLayout *m_pRteLayout;  /**< RTE Widget layout */
    QLabel *m_pLabelInit;       /**< Initialization LAbel */

    RealTimeEvokedModel*        m_pRTEModel;            /**< RTE data model */
    RealTimeButterflyPlot*      m_pButterflyPlot;       /**< Butterfly plot */

    QAction* m_pActionSelectModality;           /**< Modality selection action */

    QSharedPointer<RealTimeEvoked> m_pRTE;                  /**< The real-time evoked measurement. */

    bool m_bInitialized;                                    /**< Is Initialized */

    QList<RealTimeSampleArrayChInfo>    m_qListChInfo;      /**< Channel info list. ToDo: check if this is obsolete later on*/

    QAction*    m_pActionSelectSensors;                     /**< show roi select widget */

    SensorModel* m_pSensorModel;                            /**< Sensor model for channel selection */
    QSharedPointer<SensorWidget> m_pSensorSelectionWidget;  /**< Sensor selection widget. */

    QSharedPointer<EvokedModalityWidget> m_pEvokedModalityWidget;   /**< Evoked modality widget. */
    QList< Modality > m_qListModalities;


    QList<qint32> m_qListCurrentSelection;  /**< Current selection list -> hack around C++11 lambda  */
    void applySelection();                  /**< apply the in m_qListCurrentSelection stored selection -> hack around C++11 lambda */
    void resetSelection();                  /**< reset the in m_qListCurrentSelection stored selection -> hack around C++11 lambda */
};

} // NAMESPACE

#endif // REALTIMEEVOKEDWIDGET_H
