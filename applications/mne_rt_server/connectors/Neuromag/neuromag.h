//=============================================================================================================
/**
* @file     neuromag.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
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
* @brief     Declaration of the Neuromag Class.
*
*/

#ifndef NEUROMAG_H
#define NEUROMAG_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag_global.h"
#include "../../mne_rt_server/IConnector.h"

#include <utils/generics/circularmatrixbuffer.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMutex>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NEUROMAGRTSERVERPLUGIN
//=============================================================================================================

namespace NEUROMAGRTSERVERPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// NEUROMAGRTSERVERPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class DacqServer;


//=============================================================================================================
/**
* DECLARE CLASS Neuromag
*
* @brief The Neuromag class provides an Elekta Neuromag connector.
*/
class NEUROMAGSHARED_EXPORT Neuromag : public RTSERVER::IConnector
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_rt_server/1.0" FILE "neuromag.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(RTSERVER::IConnector)

    friend class DacqServer;

public:

    //=========================================================================================================
    /**
    * Constructs a Neuromag Connector.
    */
    Neuromag();

    //=========================================================================================================
    /**
    * Destroys the Neuromag Connector.
    *
    */
    virtual ~Neuromag();

    virtual void connectCommandManager();

    virtual RTSERVER::ConnectorID getConnectorID() const;

    virtual const char* getName() const;

    virtual void info(qint32 ID);

    virtual bool start();

    virtual bool stop();

    void releaseMeasInfo();

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Sets the buffer sample size
    *
    * @param[in] p_command  The buffer sample size command.
    */
    void comBufsize(RTSERVER::Command p_command);

    //=========================================================================================================
    /**
    * Returns the buffer sample size
    *
    * @param[in] p_command  The buffer sample size command.
    */
    void comGetBufsize(RTSERVER::Command p_command);

    //=========================================================================================================
    /**
    * Initialise the FiffSimulator.
    */
    void init();

    QMutex                      mutex;

    QSharedPointer<DacqServer>  m_pDacqServer;
    IOBUFFER::RawMatrixBuffer*  m_pRawMatrixBuffer;    /**< The Circular Raw Matrix Buffer. */

    FIFFLIB::FiffInfo           m_info;

    int             m_iID;
    quint32         m_uiBufferSampleSize;       /**< Sample size of the buffer */
    bool            m_bIsRunning;

};

} // NAMESPACE

#endif // NEUROMAG_H
