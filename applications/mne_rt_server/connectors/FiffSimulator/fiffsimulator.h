//=============================================================================================================
/**
* @file     fiffsimulator.h
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
* @brief     implementation of the FiffSimulator Class.
*
*/

#ifndef FIFFSIMULATOR_H
#define FIFFSIMULATOR_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator_global.h"
#include "../../mne_rt_server/IConnector.h"


//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <utils/generics/circularmatrixbuffer.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QMutex>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FiffConnectorPlugin
//=============================================================================================================

namespace FIFFSIMULATORPLUGIN
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTSERVER;
using namespace IOBUFFER;

//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffProducer;


//=============================================================================================================
/**
* DECLARE CLASS FiffSimulator
*
* @brief The FiffSimulator class provides a Fiff data simulator.
*/
class FIFFSIMULATORSHARED_EXPORT FiffSimulator : public IConnector
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "mne_rt_server/1.0" FILE "fiffsimulator.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(RTSERVER::IConnector)

    friend class FiffProducer;

public:
    struct Commands
    {
        static const QString BUFSIZE;
        static const QString GETBUFSIZE;
        static const QString ACCEL;
        static const QString GETACCEL;
        static const QString SIMFILE;
    };

    //=========================================================================================================
    /**
    * Constructs a FiffSimulator.
    */
    FiffSimulator();

    //=========================================================================================================
    /**
    * Destroys the FiffSimulator.
    */
    virtual ~FiffSimulator();

    virtual void connectCommandManager();

    virtual ConnectorID getConnectorID() const;

    virtual const char* getName() const;

    virtual void info(qint32 ID);

    virtual bool start();

    virtual bool stop();

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

    //=========================================================================================================
    /**
    * Sets the acceleration factor
    *
    * @param[in] p_command  The acceleration factor command.
    */
    void comAccel(Command p_command);

    //=========================================================================================================
    /**
    * Returns the acceleration factor
    *
    * @param[in] p_command  The acceleration factor command.
    */
    void comGetAccel(Command p_command);

    //=========================================================================================================
    /**
    * Sets the fiff simulation file
    *
    * @param[in] p_command  The fiff simulation file command.
    */
    void comSimfile(Command p_command);

    //////////

    //=========================================================================================================
    /**
    * Initialise the FiffSimulator.
    */
    void init();

    bool readRawInfo();

    QMutex mutex;

    FiffProducer*   m_pFiffProducer;        /**< Holds the DataProducer.*/
    FiffRawData     m_RawInfo;              /**< Holds the fiff raw measurement information. */
    QString         m_sResourceDataPath;    /**< Holds the path to the Fiff resource simulation file directory.*/
    quint32         m_uiBufferSampleSize;   /**< Sample size of the buffer */
    float           m_AccelerationFactor;   /**< Acceleration factor to simulate different sampling rates. */
    float           m_TrueSamplingRate;     /**< The true sampling rate of the fif file. */

    RawMatrixBuffer* m_pRawMatrixBuffer;    /**< The Circular Raw Matrix Buffer. */

    bool            m_bIsRunning;
};

} // NAMESPACE

#endif // FIFFSIMULATOR_H
