/**
* @file     ftbuffer.h
* @author   Gabriel B Motta <gbmotta@mgh.harvard.edu>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2020
*
* @section  LICENSE
*
* Copyright (C) 2020, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the declaration of the FtBuffer class.
*
*/

#ifndef FTBUFFER_H
#define FTBUFFER_H


//*************************************************************************************************************
//=============================================================================================================
// QT Includes
//=============================================================================================================

#include <QtWidgets>
#include <QtCore/QtPlugin>
#include <QDebug>
#include <QSharedPointer>
#include <QThread>

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ftbuffer_global.h"

#include "FormFiles/ftbufferaboutwidget.h"
#include "FormFiles/ftbuffersetupwidget.h"
#include "FormFiles/ftbufferyourwidget.h"

#include <ftsrc/ftbuffclient.h>

#include <scShared/Interfaces/ISensor.h>
#include <scShared/Interfaces/IAlgorithm.h>

#include <scMeas/realtimemultisamplearray.h>
//#include <utils/generics/circularmatrixbuffer.h>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FTBUFFERPLUGIN
//=============================================================================================================

namespace FTBUFFERPLUGIN {

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATION
//=============================================================================================================

class FtBuffProducer;

class FTBUFFER_EXPORT FtBuffer : public SCSHAREDLIB::ISensor
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "scsharedlib/1.0" FILE "ftbuffer.json")

    Q_INTERFACES(SCSHAREDLIB::ISensor)

    friend class FtBufferSetupWidget;
    friend class FtBuffProducer;
    friend class FtBuffClient;

public:

    FtBuffer();

    ~FtBuffer();

    //ISENSOR Functions
    //=========================================================================================================
    /**
    * Clone the plugin
    */
    virtual QSharedPointer<IPlugin> clone() const;

    //=========================================================================================================
    /**
    * Initializes the plugin.
    */
    virtual void init();

    //=========================================================================================================
    /**
    * Is called when plugin is detached of the stage. Can be used to safe settings.
    */
    virtual void unload();

    //=========================================================================================================
    /**
    * Starts the ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool start();

    //=========================================================================================================
    /**
    * Stops the ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return true if success, false otherwise
    */
    virtual bool stop();

    //=========================================================================================================
    /**
    * Returns the plugin type.
    * Pure virtual method inherited by IModule.
    *
    * @return type of the ISensor
    */
    virtual PluginType getType() const;

    //=========================================================================================================
    /**
    * Returns the plugin name.
    * Pure virtual method inherited by IModule.
    *
    * @return the name of the ISensor.
    */
    virtual QString getName() const;


    //=========================================================================================================
    /**
    * Returns the set up widget for configuration of ISensor.
    * Pure virtual method inherited by IModule.
    *
    * @return the setup widget.
    */
    virtual QWidget* setupWidget();

    //=========================================================================================================
    /**
    * Changes stored address and connects the member FtBuffClient to that address
    *
    * @brief connects buffer client to provided address
    */
    bool connectToBuffer(QString addr);

    //=========================================================================================================
    /**
    * Changes stored address and connects the member FtBuffClient to that address
    *
    * @brief connects buffer client to provided address
    */
    bool disconnectFromBuffer();

    //=========================================================================================================
    /**
    * runs idleCall() on a loop to continuously request buffer data
    *
    * @brief requests buffer data on a loop
    */
    Eigen::MatrixXd getData();

    //void updateBufferAddress(QString address);

    void setUpFiffInfo();

    bool isRunning();



protected:

    virtual void run();

    void showYourWidget();

    void onNewDataAvailable(const Eigen::MatrixXd &matData);

private:
    bool                                            m_bIsRunning;                   /**< Whether ftbuffer is running. */

    QThread                                         m_pProducerThread;              /**< Producer object that manages the data */
    QMutex                                          m_mutex;                        /**< Guards shared data from beng accessed at the same time */

    QSharedPointer<QList<Eigen::MatrixXd>>          m_pListReceivedSamples;         /**< List of samples recived from the buffer */

    QSharedPointer<FtBuffProducer>                  m_pFtBuffProducer;
    QSharedPointer<FtBufferYourWidget>              m_pYourWidget;
    QAction*                                        m_pActionShowYourWidget;

    char*                                           m_pTempAddress;

    FtBuffClient*                                   m_pFtBuffClient;

    QSharedPointer<FIFFLIB::FiffInfo>               m_pFiffInfo;
    QSharedPointer<SCSHAREDLIB::PluginOutputData<SCMEASLIB::RealTimeMultiSampleArray>> m_pRTMSA_BufferOutput;

};

}//namespace end brace

#endif // FTBUFFER_H
