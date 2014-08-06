//=============================================================================================================
/**
* @file     averaging.h
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
* @brief    Contains the declaration of the Averaging class.
*
*/

#ifndef AVERAGING_H
#define AVERAGING_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging_global.h"

#include <mne_x/Interfaces/IAlgorithm.h>
#include <generics/circularmatrixbuffer.h>
#include <xMeas/newrealtimemultisamplearray.h>
#include <xMeas/realtimeevoked.h>
#include <rtInv/rtave.h>


//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_info.h>
#include <fiff/fiff_evoked.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtWidgets>
#include <QSpinBox>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE AveragingPlugin
//=============================================================================================================

namespace AveragingPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEX;
using namespace XMEASLIB;
using namespace IOBuffer;
using namespace FIFFLIB;
using namespace RTINVLIB;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class AveragingSettingsWidget;


//=============================================================================================================
/**
* DECLARE CLASS Averaging
*
* @brief The Averaging class provides a Averaging algorithm structure.
*/
class AVERAGINGSHARED_EXPORT Averaging : public IAlgorithm
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_x/1.0" FILE "averaging.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MNEX::IAlgorithm)

    friend class AveragingSettingsWidget;

public:
    //=========================================================================================================
    /**
    * Constructs a Averaging.
    */
    Averaging();

    //=========================================================================================================
    /**
    * Destroys the Averaging.
    */
    ~Averaging();

    //=========================================================================================================
    /**
    * Initialise input and output connectors.
    */
    void init();

    void changeNumAverages(qint32 numAve);

    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    virtual bool start();
    virtual bool stop();

    virtual IPlugin::PluginType getType() const;
    virtual QString getName() const;


    void changeStimChannel(qint32 index);

    void changePreStim(qint32 samples);

    void changePostStim(qint32 samples);

    void appendEvoked(FiffEvoked::SPtr p_pEvoked);

    virtual QWidget* setupWidget();

    void update(XMEASLIB::NewMeasurement::SPtr pMeasurement);

    void showAveragingWidget();

signals:
    //=========================================================================================================
    /**
    * Emitted when fiffInfo is available
    */
    void fiffInfoAvailable();

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialises the output connector.
    */
    void initConnector();


    QMutex mutex;

    PluginInputData<NewRealTimeMultiSampleArray>::SPtr   m_pAveragingInput;     /**< The RealTimeSampleArray of the Averaging input.*/
    PluginOutputData<RealTimeEvoked>::SPtr  m_pAveragingOutput;                 /**< The RealTimeEvoked of the Averaging output.*/

    FiffInfo::SPtr  m_pFiffInfo;        /**< Fiff measurement info.*/
    QList<qint32> m_qListStimChs;       /**< Stimulus channels.*/

    CircularMatrixBuffer<double>::SPtr   m_pAveragingBuffer;      /**< Holds incoming data.*/

    bool m_bIsRunning;      /**< If source lab is running */
    bool m_bProcessData;    /**< If data should be received for processing */

    RtAve::SPtr m_pRtAve;   /**< Real-time average. */

    qint32 m_iPreStimSamples;
    qint32 m_iPostStimSamples;

    qint32 m_iNumAverages;

    qint32 m_iStimChan;

    QVector<FiffEvoked::SPtr>   m_qVecEvokedData;   /**< Evoked data set */

    QSharedPointer<AveragingSettingsWidget> m_pAveragingWidget;

    QAction* m_pActionShowAdjustment;

};

} // NAMESPACE

#endif // AVERAGING_H
