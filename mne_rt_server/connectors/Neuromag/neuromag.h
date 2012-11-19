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
* @brief    Contains the implementation of the Neuromag Class.
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

#include "../../../MNE/fiff/fiff_raw_data.h"

//#include "circularbuffer.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QMutex>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE NeuromagPlugin
//=============================================================================================================

namespace NeuromagPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MSERVER;


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* DECLARE CLASS Neuromag
*
* @brief The Neuromag class provides a Fiff data simulator.
*/
class NEUROMAGSHARED_EXPORT Neuromag : public IConnector
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_rt_server/1.0" FILE "neuromag.json") //New Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(MSERVER::IConnector)

public:

    //=========================================================================================================
    /**
    * Constructs a Neuromag Connector.
    */
    Neuromag();


    //=========================================================================================================
    /**
    * Destroys the Neuromag Connector.
    */
    virtual ~Neuromag();

    virtual QByteArray availableCommands() const;

    virtual ConnectorID getConnectorID() const;

    virtual const char* getName() const;


    virtual bool parseCommand(QStringList& p_sListCommand, QByteArray& p_blockOutputInfo);


    virtual bool start();

    virtual bool stop();


//public slots: --> in Qt 5 not anymore declared as slot

    virtual void requestMeasInfo(qint32 ID);

    virtual void requestMeas();

    virtual void requestMeasStop();

    virtual void requestSetBufferSize(quint32 p_uiBuffSize);

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialise the FiffSimulator.
    */
    void init();

    bool readRawInfo();

    QMutex mutex;

    FiffRawData*    m_pRawInfo;         /**< Holds the fiff raw measurement information. */

    bool            m_bIsRunning;
};

} // NAMESPACE

#endif // NEUROMAG_H
