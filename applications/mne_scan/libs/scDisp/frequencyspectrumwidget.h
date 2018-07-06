//=============================================================================================================
/**
* @file     frequencyspectrumwidget.h
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
* @brief    Declaration of the FrequencySpectrumWidget Class.
*
*/

#ifndef FREQUENCYSPECTRUMWIDGET_H
#define FREQUENCYSPECTRUMWIDGET_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "scdisp_global.h"
#include "measurementwidget.h"
#include "helpers/frequencyspectrummodel.h"
#include "helpers/frequencyspectrumdelegate.h"
#include "helpers/frequencyspectrumsettingswidget.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>
#include <QAction>
#include <QSpinBox>
#include <QDoubleSpinBox>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QTime;

namespace SCMEASLIB
{
class FrequencySpectrum;
}


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE SCDISPLIB
//=============================================================================================================

namespace SCDISPLIB
{

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCMEASLIB;


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


//=============================================================================================================
/**
* DECLARE CLASS FrequencySpectrumWidget
*
* @brief The FrequencySpectrumWidget class provides a equalizer display
*/
class SCDISPSHARED_EXPORT FrequencySpectrumWidget : public MeasurementWidget
{
    Q_OBJECT

    friend class FrequencySpectrumSettingsWidget;
public:
    //=========================================================================================================
    /**
    * Constructs a FrequencySpectrumWidget which is a child of parent.
    *
    * @param [in] pNE           pointer to noise estimation measurement.
    * @param [in] pTime         pointer to application time.
    * @param [in] parent        pointer to parent widget; If parent is 0, the new NumericWidget becomes a window. If parent is another widget, NumericWidget becomes a child window inside parent. NumericWidget is deleted when its parent is deleted.
    */
    FrequencySpectrumWidget(QSharedPointer<FrequencySpectrum> pNE, QSharedPointer<QTime> &pTime, QWidget* parent = 0);

    //=========================================================================================================
    /**
    * Destroys the FrequencySpectrumWidget.
    */
    ~FrequencySpectrumWidget();

    //=========================================================================================================
    /**
    * Is called when new data are available.
    *
    * @param [in] pMeasurement  pointer to measurement -> not used because its direct attached to the measurement.
    */
    virtual void update(SCMEASLIB::Measurement::SPtr pMeasurement);

    //=========================================================================================================
    /**
    * Is called when new data are available.
    */
    virtual void getData();

    //=========================================================================================================
    /**
    * Initialise the FrequencySpectrumWidget.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Initialise the SettingsWidget.
    */
    void initSettingsWidget();

    virtual bool eventFilter(QObject * watched, QEvent * event);

private:

    //=========================================================================================================
    /**
    * Broadcast settings of frequency spectrum settings widget
    */
    void broadcastSettings();

    //=========================================================================================================
    /**
    * Show the frequency spectrum settings widget
    */
    void showFrequencySpectrumSettingsWidget();

    QAction* m_pActionFrequencySettings;        /**< Frequency spectrum settings action */

    FrequencySpectrumModel*      m_pFSModel;    /**< FS model */
    FrequencySpectrumDelegate*   m_pFSDelegate; /**< FS delegate */
    QTableView* m_pTableView;                   /**< the QTableView being part of the model/view framework of Qt */


    QSharedPointer<FrequencySpectrumSettingsWidget> m_pFrequencySpectrumSettingsWidget;   /**< Frequency spectrum settings modality widget. */


    QSharedPointer<FrequencySpectrum> m_pFS;    /**< The frequency spectrum measurement. */

    float m_fLowerFrqBound;                    /**< Lower frequency bound */
    float m_fUpperFrqBound;                    /**< Upper frequency bound */

    bool m_bInitialized;                        /**< Is Initialized */

signals:
    //=========================================================================================================
    /**
    * Signals for sending the mouse location to the delegate
    */
    void sendMouseLoc(int row, int x, int y, QRect visRect);
};

} // NAMESPACE

#endif // FREQUENCYSPECTRUMWIDGET_H
