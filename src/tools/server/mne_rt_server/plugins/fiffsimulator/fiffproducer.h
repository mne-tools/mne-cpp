//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiffproducer.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 * @brief     Declaration of the FiffProducer class.
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
