//=============================================================================================================
/**
* @file     fiffsimulator.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hämäläinen <msh@nmr.mgh.harvard.edu>
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
* @brief    Contains the implementation of the FiffSimulator Class.
*
*/

#ifndef FIFFSIMULATOR_H
#define FIFFSIMULATOR_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator_global.h"
#include "../../core/IConnector.h"

//#include "circularbuffer.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE FiffConnectorPlugin
//=============================================================================================================

namespace FiffSimulatorPlugin
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SourceConnector;


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
class FIFFCONNECTORSHARED_EXPORT FiffSimulator : public IConnector
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "source_connector/1.0" FILE "fiffsimulator.json") //NEw Qt5 Plugin system replaces Q_EXPORT_PLUGIN2 macro
    // Use the Q_INTERFACES() macro to tell Qt's meta-object system about the interfaces
    Q_INTERFACES(SourceConnector::IConnector)


    friend class FiffProducer;

public:

    //=========================================================================================================
    /**
    * Constructs a FiffSimulator.
    */
    FiffSimulator();


    //=========================================================================================================
    /**
    * Destroys the ECGSimulator.
    */
    virtual ~FiffSimulator();

    virtual bool start();
    virtual bool stop();

    virtual ConnectorID getConnectorID() const;

    virtual const char* getName() const;

protected:
    virtual void run();

private:
    //=========================================================================================================
    /**
    * Initialise the FiffSimulator.
    */
    void init();


    FiffProducer*   m_pFiffProducer;    /**< Holds the DataProducer.*/

    float           m_fSamplingRate;    /**< Holds the sampling rate.*/


//    QString     m_qStringResourcePath;  /**< Holds the path to the Fiff resource directory.*/
    QString     m_sResourceDataPath;  /**< Holds the path to the Fiff resource simulation file directory.*/

};

} // NAMESPACE

#endif // FIFFSIMULATOR_H
