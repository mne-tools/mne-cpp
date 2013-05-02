//=============================================================================================================
/**
* @file     babymeg.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief     implementation of the BabyMEG Class.
*
*/

#ifndef BABYMEG_H
#define BABYMEG_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg_global.h"

#include "babymegclient.h"


#include "../../mne_rt_server/IConnector.h"



//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <generics/circularmatrixbuffer.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QMutex>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE BabyeMEGPlugin
//=============================================================================================================

namespace BabyMEGPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;
using namespace IOBuffer;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS BabyMEG
*
* @brief The BabyMEG class provides a Fiff data simulator.
*/
class BABYMEGSHARED_EXPORT BabyMEG : public IConnector
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_rt_server/1.0" FILE "babymeg.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(RTSERVER::IConnector)

public:

    //=========================================================================================================
    /**
    * Constructs a BabyMEG.
    */
    BabyMEG();

    //=========================================================================================================
    /**
    * Destroys the BabyMEG.
    */
    virtual ~BabyMEG();

    virtual void connectCommandManager();

    virtual ConnectorID getConnectorID() const;

    virtual const char* getName() const;

    virtual void info(qint32 ID);

    virtual bool start();

    virtual bool stop();


    void setFiffInfo(FIFFLIB::FiffInfo);
    void setFiffData(QByteArray DATA);

protected:
    virtual void run();

private:

    //Slots
    //=========================================================================================================
    /**
    * Sets the buffer sample size
    *
    * @param[in] p_command  The buffer sample size command.
    */
    void comBufsize(Command p_command);

    //=========================================================================================================
    /**
    * Returns the buffer sample size
    *
    * @param[in] p_command  The buffer sample size command.
    */
    void comGetBufsize(Command p_command);


    //////////

    //=========================================================================================================
    /**
    * Initialise the BabyMEG.
    */
    void init();

    QMutex mutex;


    BabyMEGClient *myClient;
    BabyMEGClient *myClientComm;
    BabyMEGInfo   *pInfo;
    bool DataStartFlag;


    FiffInfo    m_FiffInfoBabyMEG;      /**< Holds the fiff information. */

    bool    m_bIsRunning;

    // OLD Simulation stuff
    FiffRawData         m_RawInfo;              /**< Holds the fiff raw measurement information. */
    quint32             m_uiBufferSampleSize;   /**< Sample size of the buffer */

    CircularMatrixBuffer<float>::SPtr m_pRawMatrixBuffer;    /**< The Circular Raw Matrix Buffer. */

};

} // NAMESPACE

#endif // BABYMEG_H
