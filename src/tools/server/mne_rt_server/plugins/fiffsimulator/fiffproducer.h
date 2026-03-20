//=============================================================================================================
/**
 * @file     fiffproducer.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief     Declaration of the FiffProducer class.
 *
 */

#ifndef FIFFPRODUCER_H
#define FIFFPRODUCER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiffsimulator_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QThread>

//=============================================================================================================
// DEFINE NAMESPACE FIFFSIMULATORRTSERVERPLUGIN
//=============================================================================================================

namespace FIFFSIMULATORRTSERVERPLUGIN
{

//=============================================================================================================
// FIFFSIMULATORRTSERVERPLUGIN FORWARD DECLARATIONS
//=============================================================================================================

class FiffSimulator;

//=============================================================================================================
/**
 * DECLARE CLASS FiffProducer
 *
 * @brief The FiffProducer class provides a data producer for a given sampling rate.
 */
class FIFFSIMULATORSHARED_EXPORT FiffProducer : public QThread
{
public:

    //=========================================================================================================
    /**
     * Constructs a FiffProducer.
     */
    FiffProducer(FiffSimulator* simulator = Q_NULLPTR);

    //=========================================================================================================
    /**
     * Destroys the FiffProducer.
     */
    ~FiffProducer();

    //=========================================================================================================
    /**
     * Stops the FiffProducer by stopping the producer's thread.
     */
    virtual bool stop();

protected:
    //=========================================================================================================
    /**
     * The starting point for the thread. After calling start(), the newly created thread calls this function.
     * Returning from this method will end the execution of the thread.
     * Pure virtual method inherited by QThread.
     */
    virtual void run();

private:
    FiffSimulator*  m_pFiffSimulator;   /**< Holds a pointer to corresponding FiffSimulator.*/
    bool            m_bIsRunning;       /**< Holds whether ECGProducer is running.*/
};
} // NAMESPACE

#endif // FIFFPRODUCER_H
